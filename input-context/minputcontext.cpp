/* * This file is part of meego-im-framework *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

#include "minputcontext.h"

#include "mdbusglibinputcontextadaptor.h"
#include "glibdbusimserverproxy.h"

#ifdef HAVE_MEEGOTOUCH
#include "mpreeditstyle.h"
#else
class MPreeditStyle
{};

class MPreeditStyleContainer
{};
#endif

#include <QX11Info>
#include <QInputContext>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QTextFormat>
#include <QApplication>
#include <QClipboard>
#include <QDebug>

#ifdef HAVE_MEEGOTOUCH
#include <MApplication>
#include <MTheme>
#include <MComponentData>
#include <MLibrary>
#include <MInputMethodState>
#include <MWindow>
#include <mtimestamp.h>
#else
#include <QApplication>
#include <QGraphicsView>
#endif

#ifdef HAVE_MEEGOTOUCH
#include <MPreeditInjectionEvent>
#endif

#include <X11/XKBlib.h>

#ifdef HAVE_MEEGOTOUCH
M_LIBRARY
#endif

namespace
{
    const int SoftwareInputPanelHideTimer = 500;
    const QString DBusCallbackPath("/com/meego/inputmethod/inputcontext");
    const char * const ToolbarTarget("/toolbar");

    enum {
        XKeyPress = KeyPress,
        XKeyRelease = KeyRelease
    };
}

#undef KeyPress
#undef KeyRelease

int MInputContext::connectionCount = -1;

// Note: this class can also be used on plain Qt applications.
// This means that the functionality _can_ _not_ rely on
// MApplication or classes relying on it being initialized.


MInputContext::MInputContext(QObject *parent)
    : QInputContext(parent),
      active(false),
      inputPanelState(InputPanelHidden),
      imServer(0),
      correctionEnabled(false),
      styleContainer(0),
#ifdef HAVE_MEEGOTOUCH
      currOrientation(M::Angle0),
#endif
      connectedObject(0),
      pasteAvailable(false),
      copyAvailable(false),
      copyAllowed(true),
      redirectKeys(false),
      objectPath(QString("%1%2").arg(DBusCallbackPath).arg(++connectionCount)),
      currentKeyEventTime(0)
{
    int opcode = -1;
    int xkbEventBase = -1;
    int xkbErrorBase = -1;
    int xkblibMajor = XkbMajorVersion;
    int xkblibMinor = XkbMinorVersion;
    if (!XkbLibraryVersion(&xkblibMajor, &xkblibMinor)) {
        qCritical("%s xkb query version error!", __PRETTY_FUNCTION__);
        return;
    }

    Display* display = QX11Info::display();
    if (!XkbQueryExtension(display, &opcode, &xkbEventBase, &xkbErrorBase, &xkblibMajor, &xkblibMinor)) {
        qCritical("%s xkb query extension error!", __PRETTY_FUNCTION__);
        return;
    }

    sipHideTimer.setSingleShot(true);
    sipHideTimer.setInterval(SoftwareInputPanelHideTimer);
    // TODO: sipHideTimer is used not only when focus is lost but also with
    // CloseSoftwareInputPanel event, so hideOnFocusOut is misleading.
    connect(&sipHideTimer, SIGNAL(timeout()), SLOT(hideOnFocusOut()));

#ifdef HAVE_MEEGOTOUCH
    // using theming only when there is MComponentData instance, should be
    // there for meegotouch apps (naturally) and for plain qt with meego style plugin
    if (MComponentData::instance() != 0) {
        styleContainer = new MPreeditStyleContainer;
        styleContainer->initialize("DefaultStyle", "MPreeditStyle", 0);
    }
#endif

    connectToDBus();

#ifdef HAVE_MEEGOTOUCH
    connect(MInputMethodState::instance(),
            SIGNAL(activeWindowOrientationAngleAboutToChange(M::OrientationAngle)),
            this, SLOT(notifyOrientationAboutToChange(M::OrientationAngle)));

    connect(MInputMethodState::instance(),
            SIGNAL(activeWindowOrientationAngleAboutToChange(M::OrientationAngle)),
            this, SLOT(notifyOrientationAboutToChange(M::OrientationAngle)));

    connect(MInputMethodState::instance(),
            SIGNAL(activeWindowOrientationAngleChanged(M::OrientationAngle)),
            this, SLOT(notifyOrientationChanged(M::OrientationAngle)));

    connect(MInputMethodState::instance(),
            SIGNAL(attributeExtensionRegistered(int, QString)),
            this, SLOT(notifyAttributeExtensionRegistered(int, QString)));

    connect(MInputMethodState::instance(),
            SIGNAL(attributeExtensionUnregistered(int)),
            this, SLOT(notifyAttributeExtensionUnregistered(int)));

    connect(MInputMethodState::instance(),
            SIGNAL(toolbarItemAttributeChanged(int, QString, QString, QVariant)),
            this, SLOT(notifyToolbarItemAttributeChanged(int, QString, QString, QVariant)));

    connect(MInputMethodState::instance(),
            SIGNAL(extendedAttributeChanged(int, QString, QString, QString, QVariant)),
            this, SLOT(notifyExtendedAttributeChanged(int, QString, QString, QString, QVariant)));
#endif
}


MInputContext::~MInputContext()
{
    delete imServer;

    delete styleContainer;
}


void MInputContext::connectToDBus()
{
    qDebug() << __PRETTY_FUNCTION__;
    g_type_init();
    MDBusGlibInputContextAdaptor *inputContextAdaptor
        = M_DBUS_GLIB_INPUT_CONTEXT_ADAPTOR(
            g_object_new(M_TYPE_DBUS_GLIB_INPUT_CONTEXT_ADAPTOR, NULL));

    inputContextAdaptor->inputContext = this;
    imServer = new GlibDBusIMServerProxy(G_OBJECT(inputContextAdaptor), DBusCallbackPath);

    connect(imServer, SIGNAL(dbusConnected()), this, SLOT(onDBusConnection()));
    connect(imServer, SIGNAL(dbusDisconnected()), this, SLOT(onDBusDisconnection()));
}

// Hack: do this here to avoid touching all the lines of code in what
// it a tiny patch.  Note it's position-critical: can't have usage of
// "imServer" *above* this line modified.
#define imServer server()

bool MInputContext::event(QEvent *event)
{
#ifdef HAVE_MEEGOTOUCH
    if (event->type() == MPreeditInjectionEvent::eventNumber()) {
        if (correctionEnabled) {
            MPreeditInjectionEvent *injectionEvent
                = dynamic_cast<MPreeditInjectionEvent *>(event);

            if (injectionEvent == 0) {
                return false;
            }

            qDebug() << "MInputContext" << __PRETTY_FUNCTION__
                     << "MInputContext got preedit injection:"
                     << injectionEvent->preedit()
                     << ", event cursor pos:" << injectionEvent->eventCursorPosition();
            // send the injected preedit to input method server and back to the widget with proper
            // styling
            // Note: plugin could change the preedit style in imServer->setPreedit().
            // The cursor is hidden for preedit by default. The input method server can decide
            // whether it needs the cursor.
            QList<MInputMethod::PreeditTextFormat> preeditFormats;
            MInputMethod::PreeditTextFormat preeditFormat(0, injectionEvent->preedit().length(),
                                                          MInputMethod::PreeditDefault);
            preeditFormats << preeditFormat;
            updatePreeditInternally(injectionEvent->preedit(), preeditFormats);
            imServer->setPreedit(injectionEvent->preedit(), injectionEvent->eventCursorPosition());

            event->accept();
            return true;
        } else {
            qDebug() << "MInputContext" << __PRETTY_FUNCTION__
                     << "MInputContext ignored preedit injection because correction is disabled";
            return false;
        }
    }
#endif

    return QInputContext::event(event);
}


QString MInputContext::identifierName()
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;
    return "MInputContext";
}


bool MInputContext::isComposing() const
{
    return !preedit.isEmpty();
}


QString MInputContext::language()
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;
    return "EN"; // FIXME
}

QString MInputContext::dbusObjectPath() const
{
    return objectPath;
}

void MInputContext::reset()
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;

    // send existing preedit to widget, documentation unclear whether this is
    // allowed, but trolls gave permission to use it. Most of qt's internal
    // input methods do the same thing.
    const bool hadPreedit = !preedit.isEmpty();
    if (hadPreedit) {
        QInputMethodEvent event;
        event.setCommitString(preedit);
        sendEvent(event);
        preedit.clear();
    }

    // reset input method server, preedit requires synchronization.
    // rationale: input method might be autocommitting existing preedit without 
    // user interaction.
    imServer->reset(hadPreedit);
}


void MInputContext::update()
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;

    const QWidget *const focused = focusWidget();

    if (focused == 0) {
        return;
    }

    // get the state information of currently focused widget, and pass it to input method server
    QMap<QString, QVariant> stateInformation = getStateInformation();

    imServer->updateWidgetInformation(stateInformation, false);
}


void MInputContext::mouseHandler(int x, QMouseEvent *event)
{
    Q_UNUSED(x);

    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;
    qDebug() << "MInputContext" << " event pos: " << event->globalPos() << " cursor pos:" << x;

    if (event->type() == QEvent::MouseButtonPress && (x < 0 || x > preedit.length())) {
        reset();
        return;
    }

    // input method server needs to be informed about clicks
    if (event->type() == QEvent::MouseButtonRelease && x >= 0) {

        // Query preedit rectangle and pass it to im server if found.
        // In case of plain QT application, null rectangle will be passed.
        QWidget *focused = focusWidget();
        QRect preeditRect;

#ifdef HAVE_MEEGOTOUCH
        if (focused) {
            // TODO: Move M::PreeditRectangleQuery to MInputMethod
            Qt::InputMethodQuery query
                = static_cast<Qt::InputMethodQuery>(M::PreeditRectangleQuery);
            preeditRect = focused->inputMethodQuery(query).toRect();
        }
#else
        Q_UNUSED(focused);
#endif

        imServer->mouseClickedOnPreedit(event->globalPos(), preeditRect);
    }
}


void MInputContext::setFocusWidget(QWidget *focused)
{
    QObject *focusedObject = focused;
    QGraphicsItem *focusItem = 0;
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__ << focused;
    QInputContext::setFocusWidget(focused);

    // get detailed focus information from inside qgraphicsview
    QGraphicsView *graphicsView = qobject_cast<QGraphicsView *>(focusWidget());

    if (graphicsView && graphicsView->scene()) {
        focusItem = graphicsView->scene()->focusItem();

        if (focusItem) {
            focusedObject = dynamic_cast<QObject *>(focusItem);
        }
    }

    const QMap<QString, QVariant> stateInformation = getStateInformation();
    if (focused) {
        // for non-null focus widgets, we'll have this context activated
        if (!active) {
            imServer->activateContext();
            active = true;

#ifdef HAVE_MEEGOTOUCH
            // Notify whatever application's orientation is currently.
            notifyOrientationChanged(currOrientation);
#endif
        }

        imServer->updateWidgetInformation(stateInformation, true);

        // check if copyable text is selected
        Qt::InputMethodQuery query = Qt::ImCurrentSelection;
        QVariant queryResult = focused->inputMethodQuery(query);
        if (queryResult.isValid()) {
            copyAvailable = !queryResult.toString().isEmpty();
        }

        if (focusItem) {
            copyAllowed = !(focusItem->inputMethodHints() & Qt::ImhHiddenText);
        } else {
            copyAllowed = !(focused->inputMethodHints() & Qt::ImhHiddenText);
        }

        pasteAvailable = !QApplication::clipboard()->text().isEmpty();

        connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(handleClipboardDataChange()), Qt::UniqueConnection);
    } else {
        copyAvailable = false;
        copyAllowed = false;
        imServer->updateWidgetInformation(stateInformation, true);

        disconnect(QApplication::clipboard(), SIGNAL(dataChanged()), this, 0);
    }

    // show or hide Copy/Paste button on input method server
    notifyCopyPasteState();

    if (inputPanelState == InputPanelShowPending && focused) {
        imServer->showInputMethod();
        inputPanelState = InputPanelShown;
    }

    if (connectedObject) {
        connectedObject->disconnect(this);
        connectedObject = 0;
    }

    if (focusedObject && focusedObject->metaObject()) {
        if (focusedObject->metaObject()->indexOfSignal("copyAvailable(bool)") != -1) {
            // for MTextEdit
            connect(focusedObject, SIGNAL(copyAvailable(bool)),
                    this, SLOT(handleCopyAvailabilityChange(bool)));
            connectedObject = focusedObject;
        } else if (focusedObject->metaObject()->indexOfSignal("selectedTextChanged()") != -1) {
            // for QML::TextInput
            connect(focusedObject, SIGNAL(selectedTextChanged()),
                    this, SLOT(handleSelectedTextChange()));
            connectedObject = focusedObject;
        } else if (focusedObject->metaObject()->indexOfSignal("selectionChanged()") != -1) {
            // for QLineEdit
            connect(focusedObject, SIGNAL(selectionChanged()),
                    this, SLOT(handleSelectedTextChange()));
            connectedObject = focusedObject;
        }
    }
}

bool MInputContext::x11FilterEvent(QWidget */*widget*/, XEvent *event)
{
    if ((event->type == XKeyPress || (event->type == XKeyRelease))) {
        currentKeyEventTime = event->xkey.time;
    }

    return false;               // let filterEvent() to really handle the event
}

bool MInputContext::filterEvent(const QEvent *event)
{
    bool eaten = false;

    switch (event->type()) {
    case QEvent::RequestSoftwareInputPanel:
        qDebug() << "MInputContext" << "got event" << event->type();
        sipHideTimer.stop();

        if (!active || focusWidget() == 0) {
            // in case SIP request comes without a properly focused widget, we
            // don't ask input method server to be shown. It's done when the next widget
            // is focused, so the widget state information can be set.
            inputPanelState = InputPanelShowPending;

        } else {
            // note: could do this also if panel was hidden
            imServer->showInputMethod();
            inputPanelState = InputPanelShown;
        }

        eaten = true;
        break;

    case QEvent::CloseSoftwareInputPanel:
        qDebug() << "MInputContext" << "got event" << event->type();
        sipHideTimer.start();
        eaten = true;
        break;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        if (focusWidget() == 0) {
            break;  // Don't emit signals without focused widget
        }

#ifdef HAVE_MEEGOTOUCH
        // TODO: Move MInputMethodState to IM FW and let LMT include an IM-specific LMT extension?
        if (event->type() == QEvent::KeyPress) {
            MInputMethodState::instance()->emitKeyPress(*(static_cast<const QKeyEvent*>(event)));
        } else {
            MInputMethodState::instance()->emitKeyRelease(*(static_cast<const QKeyEvent*>(event)));
        }
#endif

        if (redirectKeys) {
            const QKeyEvent *key = static_cast<const QKeyEvent *>(event);
            imServer->processKeyEvent(key->type(), static_cast<Qt::Key>(key->key()),
                                      key->modifiers(), key->text(), key->isAutoRepeat(),
                                      key->count(), key->nativeScanCode(),
                                      key->nativeModifiers(), currentKeyEventTime);
            eaten = true;
        }
        break;

    default:
#ifdef HAVE_MEEGOTOUCH
        if (event->type() == MPreeditInjectionEvent::eventNumber()) {
            if (correctionEnabled) {
                const MPreeditInjectionEvent *injectionEvent
                    = dynamic_cast<const MPreeditInjectionEvent *>(event);

                if (injectionEvent == 0) {
                    break;
                }

                qDebug() << "MInputContext" << "MInputContext got preedit injection:"
                                            << injectionEvent->preedit()
                                            << ", event cursor pos:" << injectionEvent->eventCursorPosition();
                // send the injected preedit to input method server and back to the widget with proper
                // styling
                // Note: plugin could change the preedit style in imServer->setPreedit().
                // The cursor is hidden for preedit by default. The input method server can decide
                // whether it needs the cursor.
                QList<MInputMethod::PreeditTextFormat> preeditFormats;
                const MInputMethod::PreeditTextFormat preeditFormat(
                    0, injectionEvent->preedit().length(), MInputMethod::PreeditDefault);
                preeditFormats << preeditFormat;
                // TODO: updatePreeditInternally() below causes update() to be called which
                // communicates new cursor position (among other things) to the active
                // input method.  The cursor is at the beginning of the pre-edit but the
                // input method does not yet know that there is an active pre-edit,
                // because that is communicated separately by the imServer->setPreedit()
                // call below.  This means, for example, that if the cursor position is
                // such that autocapitalization applies, it will have an effect, and once
                // the setPreedit() below has been called, the effect will be undone.
                // This causes flickering in vkb/sbox but not on the device, so for now
                // we'll leave this to be fixed later.  Please refer to NB#226907.
                updatePreeditInternally(injectionEvent->preedit(), preeditFormats);
                imServer->setPreedit(injectionEvent->preedit(), injectionEvent->eventCursorPosition());

                eaten = true;

            } else {
                qDebug() << "MInputContext"
                         << "MInputContext ignored preedit injection because correction is disabled";
            }
        }
#endif
        break;
    }

    return eaten;
}


void MInputContext::hideOnFocusOut()
{
    imServer->hideInputMethod();

    inputPanelState = InputPanelHidden;
}

void MInputContext::activationLostEvent()
{
    // This method is called when activation was gracefully lost.
    // There is similar cleaning up done in onDBusDisconnection.
    active = false;
    inputPanelState = InputPanelHidden;
}


void MInputContext::imInitiatedHide()
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;
    inputPanelState = InputPanelHidden;

    // need to remove focus from the current text entry
    if (focusWidget()) {
        QGraphicsView *graphicsView = qobject_cast<QGraphicsView *>(focusWidget());

        if (graphicsView && graphicsView->scene()) {
            // inside qgraphics we remove the focus from item focused there
            QGraphicsScene *scene = graphicsView->scene();
            QGraphicsItem *item = scene->focusItem();
            if (item) {
                item->clearFocus();
            }

        } else {
            focusWidget()->clearFocus();
        }
    }
}


void MInputContext::commitString(const QString &string, int replacementStart,
                                 int replacementLength, int cursorPos)
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;

#ifdef HAVE_MEEGOTOUCH
    mTimestamp("MInputContext", string);
#endif

    if (imServer->pendingResets()) {
        return;
    }

    preedit.clear();

    int start = -1;
    const QWidget *focused = focusWidget();
    if (cursorPos >= 0 && focused) {
        // obtain the cursor absolute position
        QVariant queryResult = focused->inputMethodQuery(Qt::ImCursorPosition);
        if (queryResult.isValid()) {
            start = queryResult.toInt() + cursorPos;
        }
    }

    if (start >= 0) {
        QList<QInputMethodEvent::Attribute> attributes;
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, start, 0, QVariant());
        QInputMethodEvent event("", attributes);
        event.setCommitString(string, replacementStart, replacementLength);
        sendEvent(event);
    } else {
        QInputMethodEvent event;
        event.setCommitString(string, replacementStart, replacementLength);
        sendEvent(event);
    }
}


void MInputContext::updatePreedit(const QString &string,
                                  const QList<MInputMethod::PreeditTextFormat> &preeditFormats,
                                  int replacementStart, int replacementLength, int cursorPos)
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__ << "preedit:" << string
                                << ", replacementStart:" << replacementStart
                                << ", replacementLength:" << replacementLength
                                << ", cursorPos:" << cursorPos;

    if (imServer->pendingResets()) {
        return;
    }

    updatePreeditInternally(string, preeditFormats, replacementStart, replacementLength, cursorPos);
}

void MInputContext::updatePreeditInternally(const QString &string,
                                            const QList<MInputMethod::PreeditTextFormat> &preeditFormats,
                                            int replacementStart, int replacementLength, int cursorPos)
{
#ifdef HAVE_MEEGOTOUCH
    mTimestamp("MInputContext", "start text=" + string);
#endif

    preedit = string;

    QList<QInputMethodEvent::Attribute> attributes;
    foreach (const MInputMethod::PreeditTextFormat &preeditFormat, preeditFormats) {

        // set proper formatting
        QTextCharFormat format;
        format.merge(standardFormat(QInputContext::PreeditFormat));

        // update style mode
        if (styleContainer) {
#ifdef HAVE_MEEGOTOUCH
            switch (preeditFormat.preeditFace) {
            case MInputMethod::PreeditNoCandidates:
                styleContainer->setModeNoCandidates();
                break;

            case MInputMethod::PreeditKeyPress:
                styleContainer->setModeKeyPress();
                break;

            case MInputMethod::PreeditDefault:
            default:
                styleContainer->setModeDefault();
            }

            format.setUnderlineStyle((*styleContainer)->underline());
            format.setUnderlineColor((*styleContainer)->underlineColor());
            QColor color = (*styleContainer)->backgroundColor();

            if (color.isValid()) {
                format.setBackground(color);
            }

            color = (*styleContainer)->fontColor();

            if (color.isValid()) {
                format.setForeground(color);
            }
#endif // HAVE_MEEGOTOUCH
        } else {
            // hard coded styling:
            format.setUnderlineStyle(QTextCharFormat::SingleUnderline);

            QColor underlineColor;
            if (preeditFormat.preeditFace == MInputMethod::PreeditNoCandidates) {
                underlineColor.setRgb(255, 0, 0);
            } else {
                underlineColor.setRgb(0, 0, 0);
            }

            format.setUnderlineColor(QColor());
        }

        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,
                                                   preeditFormat.start,
                                                   preeditFormat.length, format);
    }

    if (cursorPos >= 0) {
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursorPos, 1, QVariant());
    }

    QInputMethodEvent event(string, attributes);
    if (replacementLength) {
        event.setCommitString("", replacementStart, replacementLength);
    }

    sendEvent(event);

#ifdef HAVE_MEEGOTOUCH
    mTimestamp("MInputContext", "end");
#endif
}


void MInputContext::keyEvent(int type, int key, int modifiers, const QString &text,
                             bool autoRepeat, int count,
                             MInputMethod::EventRequestType requestType)
{
    qDebug() << "MInputContext" << "in" << __PRETTY_FUNCTION__;

    // Construct an event instance out of the parameters.
    QEvent::Type eventType = static_cast<QEvent::Type>(type);
    QKeyEvent event(eventType, key,
                    static_cast<Qt::KeyboardModifiers>(modifiers),
                    text, autoRepeat, count);

#ifdef HAVE_MEEGOTOUCH
    if (requestType != MInputMethod::EventRequestEventOnly) {
        if (eventType == QEvent::KeyPress) {
            MInputMethodState::instance()->emitKeyPress(event);
        } else if (eventType == QEvent::KeyRelease) {
            MInputMethodState::instance()->emitKeyRelease(event);
        }
    }
#endif

    if (focusWidget() != 0 && requestType != MInputMethod::EventRequestSignalOnly) {
        QCoreApplication::sendEvent(focusWidget(), &event);
    }
}


void MInputContext::updateInputMethodArea(const QList<QVariant> &data)
{
    QRect rect;
    if (!data.isEmpty())
        rect = data.at(0).toRect();

#ifdef HAVE_MEEGOTOUCH
    MInputMethodState::instance()->setInputMethodArea(rect);
#endif

    emit inputMethodAreaChanged(rect);
}


void MInputContext::setGlobalCorrectionEnabled(bool enabled)
{
    correctionEnabled = enabled;
}


QRect MInputContext::preeditRectangle(bool &valid) const
{
    QRect rect;
    valid = false;

#ifdef HAVE_MEEGOTOUCH
    Qt::InputMethodQuery query = static_cast<Qt::InputMethodQuery>(M::PreeditRectangleQuery);

    if (focusWidget()) {
        QVariant queryResult = focusWidget()->inputMethodQuery(query);

        valid = queryResult.isValid();
        rect = queryResult.toRect();
    }
#endif

    return rect;
}

void MInputContext::handleClipboardDataChange()
{
    bool newPasteAvailable = !QApplication::clipboard()->text().isEmpty();

    if (newPasteAvailable != pasteAvailable) {
        pasteAvailable = newPasteAvailable;
        notifyCopyPasteState();
    }
}


void MInputContext::copy()
{
    bool ok = false;

    if (connectedObject) {
        ok = QMetaObject::invokeMethod(connectedObject, "copy", Qt::DirectConnection);
    }

    qDebug() << "MInputContext" << __PRETTY_FUNCTION__ << "result=" << ok;

    if (!ok) {
        // send Ctrl-Ckey event because suitable slot was not found
        keyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, "", false, 1);
        keyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier, "", false, 1);
    }
}


void MInputContext::paste()
{
    bool ok = false;

    if (connectedObject) {
        ok = QMetaObject::invokeMethod(connectedObject, "paste", Qt::DirectConnection);
    }

    qDebug() << "MInputContext" << __PRETTY_FUNCTION__ << "result=" << ok;

    if (!ok) {
        // send Ctrl-V key event because suitable slot was not found
        keyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier, "", false, 1);
        keyEvent(QEvent::KeyRelease, Qt::Key_V, Qt::ControlModifier, "", false, 1);
    }
}


void MInputContext::onDBusDisconnection()
{
    qDebug() << __PRETTY_FUNCTION__;
    active = false;
    redirectKeys = false;

#ifdef HAVE_MEEGOTOUCH
    MInputMethodState::instance()->setInputMethodArea(QRect());
#endif
}

void MInputContext::onDBusConnection()
{
    qDebug() << __PRETTY_FUNCTION__;

    registerExistingAttributeExtensions();

    // There could already be focused item when the connection to the uiserver is
    // established. Show keyboard immediately in that case.
    QWidget *widget = qApp->focusWidget();
    // TODO: Should not call testAttribute explicitly, QInputContext::setFocusWidget()
    // already does it. The right way should be calling QInputContext::setFocusWidget()
    // and then checking whether QInputContext has focusWidget(). But the QT bug NB#181094
    // prevents us.
    if (widget && widget->testAttribute(Qt::WA_InputMethodEnabled)) {
        // Force activation, since setFocusWidget may have been called after
        // onDBusDisconnection set active to false.
        active = false;
        setFocusWidget(widget);
        if (inputPanelState != InputPanelHidden) {
            imServer->showInputMethod();
            inputPanelState = InputPanelShown;
        }
    }
}

void MInputContext::handleSelectedTextChange()
{
    if (connectedObject) {
        bool hasSelectedText = (connectedObject->property("selectionStart").toInt()
                                != connectedObject->property("selectionEnd").toInt());
        handleCopyAvailabilityChange(hasSelectedText);
    }
}

void MInputContext::handleSelectionChanged()
{
    if (connectedObject) {
        bool hasSelectedText = connectedObject->property( "hasSelectedText" ).toBool();
        handleCopyAvailabilityChange(hasSelectedText);
    }
}

void MInputContext::handleCopyAvailabilityChange(bool copyAvailable)
{
    if (this->copyAvailable != copyAvailable) {
        this->copyAvailable = copyAvailable;
        notifyCopyPasteState();
    }
}

void MInputContext::notifyCopyPasteState()
{
    imServer->setCopyPasteState(copyAvailable && copyAllowed, pasteAvailable);
}

#ifdef HAVE_MEEGOTOUCH
void MInputContext::notifyOrientationAboutToChange(M::OrientationAngle orientation)
{
    // can get called from signal so cannot be sure we are really currently active
    if (active) {
        imServer->appOrientationAboutToChange(static_cast<int>(orientation));
    }
}

void MInputContext::notifyOrientationChanged(M::OrientationAngle orientation)
{
    // can get called from signal so cannot be sure we are really currently active
    currOrientation = orientation;
    if (active) {
        imServer->appOrientationChanged(static_cast<int>(orientation));
    }
}
#endif

void MInputContext::notifyAttributeExtensionRegistered(int id, const QString &fileName)
{
    imServer->registerAttributeExtension(id, fileName);
}

void MInputContext::notifyAttributeExtensionUnregistered(int id)
{
    imServer->unregisterAttributeExtension(id);
}

void MInputContext::notifyToolbarItemAttributeChanged(int id, const QString &item,
                                                      const QString &attribute,
                                                      const QVariant& value)
{
    imServer->setExtendedAttribute(id, ToolbarTarget, item,
                                   attribute, value);
}

void MInputContext::notifyExtendedAttributeChanged(int id, const QString &target, const QString &targetItem,
                                                   const QString &attribute, const QVariant& value)
{
    qDebug() << __PRETTY_FUNCTION__;
    imServer->setExtendedAttribute(id, target, targetItem,
                                   attribute, value);
}

MInputMethod::TextContentType MInputContext::contentType(Qt::InputMethodHints hints) const
{
    MInputMethod::TextContentType type = MInputMethod::FreeTextContentType;
    hints &= Qt::ImhExclusiveInputMask;

    if (hints == Qt::ImhFormattedNumbersOnly || hints == Qt::ImhDigitsOnly) {
        type = MInputMethod::NumberContentType;
    } else if (hints == Qt::ImhDialableCharactersOnly) {
        type = MInputMethod::PhoneNumberContentType;
    } else if (hints == Qt::ImhEmailCharactersOnly) {
        type = MInputMethod::EmailContentType;
    } else if (hints == Qt::ImhUrlCharactersOnly) {
        type = MInputMethod::UrlContentType;
    }

    return type;
}

void MInputContext::setRedirectKeys(bool enabled)
{
    redirectKeys = enabled;
}

void MInputContext::setDetectableAutoRepeat(bool enabled)
{
    Bool detectableAutoRepeatSupported(False);
    XkbSetDetectableAutoRepeat(QX11Info::display(), enabled ? True : False,
                               &detectableAutoRepeatSupported);
    if (detectableAutoRepeatSupported == False) {
        qWarning() << "Detectable autorepeat not supported.";
    }
}

QMap<QString, QVariant> MInputContext::getStateInformation() const
{
    QMap<QString, QVariant> stateInformation;
    const QWidget *focused = focusWidget();

    stateInformation["focusState"] = focused != NULL;

    if (!focused) {
        return stateInformation;
    }

    const QGraphicsView * const graphicsView = qobject_cast<const QGraphicsView *>(focused);
    const QGraphicsItem *focusedQGraphicsItem = 0;

    if (graphicsView) {
        const QGraphicsScene * const scene = graphicsView->scene();
        focusedQGraphicsItem = scene ? scene->focusItem() : 0;
    }


     QVariant queryResult;
#ifdef HAVE_MEEGOTOUCH
    // TODO: Move M::VisualizationPriorityQuery to dedicated IM/LMT extension package.
    // visualization priority
    queryResult = focused->inputMethodQuery(
        static_cast<Qt::InputMethodQuery>(M::VisualizationPriorityQuery));

    if (queryResult.isValid()) {
        stateInformation["visualizationPriority"] = queryResult.toBool();
    }

    // TODO: Move M::InputMethodToolbar*Query to dedicated IM/LMT extension package.
    // toolbar id
    queryResult = focused->inputMethodQuery(
        static_cast<Qt::InputMethodQuery>(M::InputMethodToolbarIdQuery));

    if (!queryResult.isValid()) {
        // fallback using qgraphicsobject property. Used to bypass qml restrictions 
        // for qt components / meego. Use elsewhere discouraged and nothing guaranteed.
        const QGraphicsObject *qgraphicsObject
            = qgraphicsitem_cast<const QGraphicsObject*>(focusedQGraphicsItem);

        if (qgraphicsObject) {
            queryResult = qgraphicsObject->property("meego-inputmethod-attribute-extension-id");
        }
    }

    if (queryResult.isValid()) {
        stateInformation["toolbarId"] = queryResult.toInt();
    }

    // toolbar file
    queryResult = focused->inputMethodQuery(
        static_cast<Qt::InputMethodQuery>(M::InputMethodToolbarQuery));

    if (queryResult.isValid()) {
        stateInformation["toolbar"] = queryResult.toString();
    }
#endif

    // surrounding text
    queryResult = focused->inputMethodQuery(Qt::ImSurroundingText);

    if (queryResult.isValid()) {
        stateInformation["surroundingText"] = queryResult.toString();
    }

    // cursor pos
    queryResult = focused->inputMethodQuery(Qt::ImCursorPosition);

    if (queryResult.isValid()) {
        stateInformation["cursorPosition"] = queryResult.toInt();
    }

    // anchor pos
    queryResult = focused->inputMethodQuery(Qt::ImAnchorPosition);

    if (queryResult.isValid()) {
        stateInformation["anchorPosition"] = queryResult.toInt();
    }

    Qt::InputMethodHints hints = focused->inputMethodHints();

    if (focusedQGraphicsItem) {
        // focused->inputMethodHints() always returns 0
        // therefore we explicitly call inputMethodHints with focused item
        hints = focusedQGraphicsItem->inputMethodHints();
    }

    // content type value
    stateInformation["contentType"] = contentType(hints);

#ifdef HAVE_MEEGOTOUCH
    // TODO: Move M::ImCorrectionEnabledQuery to dedicated IM/LMT extension package.
    // error correction support
    queryResult = focused->inputMethodQuery(
        static_cast<Qt::InputMethodQuery>(M::ImCorrectionEnabledQuery));

    if (queryResult.isValid()) {
        stateInformation["correctionEnabled"] = queryResult.toBool();
    }
#endif

    stateInformation["predictionEnabled"] = !(hints & Qt::ImhNoPredictiveText);
    stateInformation["autocapitalizationEnabled"] = !(hints & Qt::ImhNoAutoUppercase);
    stateInformation["hiddenText"] = static_cast<bool>(hints & Qt::ImhHiddenText);


#ifdef HAVE_MEEGOTOUCH
    // TODO: Move M::InputMethodMode* to dedicated IM/LMT extension package.
    // input method mode
    queryResult = focused->inputMethodQuery(
        static_cast<Qt::InputMethodQuery>(M::ImModeQuery));

    M::InputMethodMode inputMethodMode = M::InputMethodModeNormal;
    if (queryResult.isValid()) {
        inputMethodMode = static_cast<M::InputMethodMode>(queryResult.toInt());
    }
    stateInformation["inputMethodMode"] = inputMethodMode;
#endif

    // is text selected
    queryResult = focused->inputMethodQuery(Qt::ImCurrentSelection);

    if (queryResult.isValid())
        stateInformation["hasSelection"] = !(queryResult.toString().isEmpty());

    // window id for transient VKB - it is enough to just query the focused widget here:
    stateInformation["winId"] = QVariant::fromValue(focused->window()->effectiveWinId());

    queryResult = focused->inputMethodQuery(Qt::ImMicroFocus);
    if (queryResult.isValid()) {
        // The replied rectangle is covering the area of the input cursor in qwidget coordinates.
        // Need to map the rectangle to global coordinate.
        QRect cursorRect = queryResult.toRect();
        cursorRect = QRect(focused->mapToGlobal(cursorRect.topLeft()),
                           focused->mapToGlobal(cursorRect.bottomRight()));
        stateInformation["cursorRectangle"] = cursorRect;
    }

    return stateInformation;
}

void MInputContext::registerExistingAttributeExtensions()
{
#ifdef HAVE_MEEGOTOUCH
    QList<int> ids = MInputMethodState::instance()->attributeExtensionIds();

    foreach (int id, ids) {
        QString fileName = MInputMethodState::instance()->attributeExtensionFile(id);
        imServer->registerAttributeExtension(id, fileName);

        MInputMethodState::ExtendedAttributeMap extendedAttributes
            = MInputMethodState::instance()->extendedAttributes(id);

        foreach (QString target, extendedAttributes.keys()) {
            MInputMethodState::ItemAttributeMap items = extendedAttributes.value(target);
            foreach (QString item, items.keys()) {
                MInputMethodState::AttributeMap attributes = items.value(item);
                foreach (QString attributeName, attributes.keys()) {
                    QVariant value = attributes.value(attributeName);
                    imServer->setExtendedAttribute(id, target, item, attributeName, value);
                }
            }
        }
    }
#endif
}

void MInputContext::setSelection(int start, int length)
{
    QList<QInputMethodEvent::Attribute> attributes;
    attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, start,
                                                length, QVariant());
    QInputMethodEvent event("", attributes);
    sendEvent(event);
}

QString MInputContext::selection(bool &valid) const
{
    QString selectionText;
    valid = false;
    if (focusWidget()) {
        QVariant queryResult = focusWidget()->inputMethodQuery(Qt::ImCurrentSelection);
        valid = queryResult.isValid();
        selectionText = queryResult.toString();
    }
    return selectionText;
}

