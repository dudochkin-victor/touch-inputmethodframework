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

#define _BSD_SOURCE             // for mkdtemp

#include "minputcontextglibdbusconnection.h"
#include "mattributeextensionmanager.h"
#include "mattributeextensionid.h"

#include <QDataStream>
#include <QDebug>
#include <QRegion>
#include <QKeyEvent>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QFile>
#include <QDir>

#include "mabstractinputmethod.h"
#include "mimapplication.h"

namespace
{
    const char * const SocketDirectory = "/tmp/meego-im-uiserver";
    const char * const SocketName = "imserver_dbus";
    const char * const DBusPath = "/com/meego/inputmethod/uiserver1";

    const char * const DBusClientPath = "/com/meego/inputmethod/inputcontext";
    const char * const DBusClientInterface = "com.meego.inputmethod.inputcontext1";

    // attribute names for updateWidgetInformation() map
    const char * const FocusStateAttribute = "focusState";
    const char * const ContentTypeAttribute = "contentType";
    const char * const CorrectionAttribute = "correctionEnabled";
    const char * const PredictionAttribute = "predictionEnabled";
    const char * const AutoCapitalizationAttribute = "autocapitalizationEnabled";
    const char * const SurroundingTextAttribute = "surroundingText";
    const char * const AnchorPositionAttribute = "anchorPosition";
    const char * const CursorPositionAttribute = "cursorPosition";
    const char * const HasSelectionAttribute = "hasSelection";
    const char * const InputMethodModeAttribute = "inputMethodMode";
    const char * const VisualizationAttribute = "visualizationPriority";
    const char * const ToolbarIdAttribute = "toolbarId";
    const char * const ToolbarAttribute = "toolbar";
    const char * const WinId = "winId";
    const char * const CursorRectAttribute = "cursorRectangle";
    const char * const HiddenTextAttribute = "hiddenText";
}

//! \internal
//! \brief GObject-based input context connection class
//!
//! There is one of these for each client connection
struct MDBusGlibICConnection
{
    GObject parent;

    //! This is the glib-level connection established when the input context calls us
    DBusGConnection* dbusConnection;
    //! This is a proxy object used to call input context methods
    DBusGProxy *inputContextProxy;
    //! The actual C++-level connection that acts as a proxy for the currently active input context
    MInputContextGlibDBusConnection *icConnection;
    //! Running number used to identify the connection, used for toolbar ids
    unsigned int connectionNumber;
};

struct MDBusGlibICConnectionClass
{
    GObjectClass parent;
};
//! \internal_end

#define M_TYPE_DBUS_GLIB_IC_CONNECTION              (m_dbus_glib_ic_connection_get_type())
#define M_DBUS_GLIB_IC_CONNECTION(object)           (G_TYPE_CHECK_INSTANCE_CAST((object), M_TYPE_DBUS_GLIB_IC_CONNECTION, MDBusGlibICConnection))
#define M_DBUS_GLIB_IC_CONNECTION_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), M_TYPE_DBUS_GLIB_IC_CONNECTION, MDBusGlibICConnectionClass))
#define M_IS_M_DBUS_GLIB_IC_CONNECTION(object)      (G_TYPE_CHECK_INSTANCE_TYPE((object), M_TYPE_DBUS_GLIB_IC_CONNECTION))
#define M_IS_M_DBUS_GLIB_IC_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), M_TYPE_DBUS_GLIB_IC_CONNECTION))
#define M_DBUS_GLIB_IC_CONNECTION_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), M_TYPE_DBUS_GLIB_IC_CONNECTION, MDBusGlibICConnectionClass))

G_DEFINE_TYPE(MDBusGlibICConnection, m_dbus_glib_ic_connection, G_TYPE_OBJECT)

static gboolean
m_dbus_glib_ic_connection_activate_context(MDBusGlibICConnection *obj, GError **/*error*/)
{
    obj->icConnection->activateContext(obj);
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_show_input_method(MDBusGlibICConnection *obj, GError **/*error*/)
{
    obj->icConnection->showInputMethod(obj);
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_hide_input_method(MDBusGlibICConnection *obj, GError **/*error*/)
{
    obj->icConnection->hideInputMethod(obj);
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_mouse_clicked_on_preedit(MDBusGlibICConnection *obj,
                                                   gint32 posX, gint32 posY,
                                                   gint32 preeditX, gint32 preeditY,
                                                   gint32 preeditWidth, gint32 preeditHeight,
                                                   GError **/*error*/)
{
    obj->icConnection->mouseClickedOnPreedit(obj, QPoint(posX, posY), QRect(preeditX, preeditY,
                                                                            preeditWidth, preeditHeight));
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_set_preedit(MDBusGlibICConnection *obj, const char *text,
                                      gint32 cursorPos, GError **/*error*/)
{
    obj->icConnection->setPreedit(obj, QString::fromUtf8(text), cursorPos);
    return TRUE;
}

static bool deserializeVariant(QVariant& target, const GArray *data, const char *methodName)
{
    const QByteArray storageWrapper(QByteArray::fromRawData(data->data, data->len));
    QDataStream dataStream(storageWrapper);
    dataStream >> target;
    if (dataStream.status() != QDataStream::Ok || !target.isValid()) {
        qWarning("m_dbus_glib_ic_connection: Invalid parameter to %s.", methodName);
        return false;
    }
    return true;
}

template <typename T>
static bool deserializeValue(T& target, const GArray *data, const char *methodName)
{
    QVariant variant;
    if (!deserializeVariant(variant, data, methodName)) {
        return false;
    }
    if (!variant.canConvert<T>()) {
        qWarning("m_dbus_glib_ic_connection: Invalid parameter to %s.", methodName);
        return false;
    }
    target = variant.value<T>();
    return true;
}

static gboolean
m_dbus_glib_ic_connection_update_widget_information(MDBusGlibICConnection *obj,
                                                    GArray *stateInformation,
                                                    gboolean focusChanged, GError **/*error*/)
{
    QMap<QString, QVariant> stateMap;
    if (deserializeValue(stateMap, stateInformation, "updateWidgetInformation")) {
        obj->icConnection->updateWidgetInformation(obj, stateMap, focusChanged == TRUE);
    }
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_reset(MDBusGlibICConnection *obj, GError **/*error*/)
{
    obj->icConnection->reset(obj);
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_app_orientation_about_to_change(MDBusGlibICConnection *obj, gint32 angle,
                                                         GError **/*error*/)
{
    obj->icConnection->receivedAppOrientationAboutToChange(obj, static_cast<int>(angle));
    return TRUE;
}


static gboolean
m_dbus_glib_ic_connection_app_orientation_changed(MDBusGlibICConnection *obj, gint32 angle,
                                                  GError **/*error*/)
{
    obj->icConnection->receivedAppOrientationChanged(obj, static_cast<int>(angle));
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_set_copy_paste_state(MDBusGlibICConnection *obj, gboolean copyAvailable,
                                               gboolean pasteAvailable, GError **/*error*/)
{
    obj->icConnection->setCopyPasteState(obj, copyAvailable == TRUE, pasteAvailable == TRUE);
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_process_key_event(MDBusGlibICConnection *obj, gint32 keyType,
                                            gint32 keyCode, gint32 modifiers,
                                            const char *text, gboolean autoRepeat, gint32 count,
                                            guint32 nativeScanCode, guint32 nativeModifiers,
                                            unsigned long time,
                                            GError **/*error*/)
{
    obj->icConnection->processKeyEvent(obj, static_cast<QEvent::Type>(keyType),
                                       static_cast<Qt::Key>(keyCode),
                                       static_cast<Qt::KeyboardModifiers>(modifiers),
                                       QString::fromUtf8(text), autoRepeat == TRUE,
                                       static_cast<int>(count), static_cast<quint32>(nativeScanCode),
                                       static_cast<quint32>(nativeModifiers), time);
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_register_attribute_extension(MDBusGlibICConnection *obj, gint32 id,
                                                       const char *fileName, GError **/*error*/)
{
    obj->icConnection->registerAttributeExtension(obj, static_cast<int>(id), QString::fromUtf8(fileName));
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_unregister_attribute_extension(MDBusGlibICConnection *obj, gint32 id,
                                                         GError **/*error*/)
{
    obj->icConnection->unregisterAttributeExtension(obj, static_cast<int>(id));
    return TRUE;
}

static gboolean
m_dbus_glib_ic_connection_set_extended_attribute(MDBusGlibICConnection *obj, gint32 id,
                                                 const char *target, const char *targetItem,
                                                 const char *attribute, GArray *value, GError **/*error*/)
{
    qDebug() << __PRETTY_FUNCTION__;
    QVariant deserializedValue;
    if (deserializeVariant(deserializedValue, value, "setExtendedAttribute")) {
        obj->icConnection->setExtendedAttribute(obj, static_cast<int>(id), QString::fromUtf8(target),
                                                QString::fromUtf8(targetItem),
                                                QString::fromUtf8(attribute),
                                                deserializedValue);
    }
    return TRUE;
}


#include "mdbusglibicconnectionserviceglue.h"

static void
m_dbus_glib_ic_connection_dispose(GObject *object)
{
    qDebug() << __PRETTY_FUNCTION__;

    MDBusGlibICConnection *self(M_DBUS_GLIB_IC_CONNECTION(object));

    if (self->dbusConnection) {
        dbus_g_connection_unref(self->dbusConnection);
        self->dbusConnection = 0;
    }

    G_OBJECT_CLASS(m_dbus_glib_ic_connection_parent_class)->finalize(object);
}


static void
m_dbus_glib_ic_connection_finalize(GObject */*object*/)
{
    qDebug() << __PRETTY_FUNCTION__;
}


static void
m_dbus_glib_ic_connection_init(MDBusGlibICConnection */*obj*/)
{
}

static void
m_dbus_glib_ic_connection_class_init(MDBusGlibICConnectionClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = m_dbus_glib_ic_connection_dispose;
    gobject_class->finalize = m_dbus_glib_ic_connection_finalize;

    dbus_g_object_type_install_info(M_TYPE_DBUS_GLIB_IC_CONNECTION,
                                    &dbus_glib_m_dbus_glib_ic_connection_object_info);
}


// MInputContextGlibDBusConnection...........................................

static void handleDisconnectionTrampoline(DBusGProxy */*proxy*/, gpointer userData)
{
    qDebug() << __PRETTY_FUNCTION__;

    MDBusGlibICConnection *connection(M_DBUS_GLIB_IC_CONNECTION(userData));
    connection->icConnection->handleDBusDisconnection(connection);
}

void MInputContextGlibDBusConnection::handleDBusDisconnection(MDBusGlibICConnection *connection)
{
    // unregister toolbars registered by the lost connection
    const QString service(QString::number(connection->connectionNumber));
    QSet<MAttributeExtensionId>::iterator i(attributeExtensionIds.begin());
    while (i != attributeExtensionIds.end()) {
        if ((*i).service() == service) {
            MAttributeExtensionManager::instance().unregisterAttributeExtension(*i);
            i = attributeExtensionIds.erase(i);
        } else {
            ++i;
        }
    }

    g_object_unref(G_OBJECT(connection));

    if (activeContext != connection) {
        return;
    }

    activeContext = 0;

    // notify plugins
    foreach (MAbstractInputMethod *target, targets()) {
        target->handleClientChange();
    }
}

static void handleNewConnection(DBusServer */*server*/, DBusConnection *connection, gpointer userData)
{
    qDebug() << __PRETTY_FUNCTION__;
    dbus_connection_ref(connection);
    dbus_connection_setup_with_g_main(connection, NULL);

    MDBusGlibICConnection *obj = M_DBUS_GLIB_IC_CONNECTION(
        g_object_new(M_TYPE_DBUS_GLIB_IC_CONNECTION, NULL));

    obj->dbusConnection = dbus_connection_get_g_connection(connection);
    obj->icConnection = static_cast<MInputContextGlibDBusConnection *>(userData);

    // Proxy for calling input context methods
    DBusGProxy *inputContextProxy = dbus_g_proxy_new_for_peer(
        obj->dbusConnection, DBusClientPath, DBusClientInterface);
    if (!inputContextProxy) {
        qFatal("Unable to find the service.");
    }
    obj->inputContextProxy = inputContextProxy;

    g_signal_connect(G_OBJECT(inputContextProxy), "destroy",
                     G_CALLBACK(handleDisconnectionTrampoline), obj);

    static unsigned int connectionCounter = 0;
    obj->connectionNumber = connectionCounter++;

    dbus_g_connection_register_g_object(obj->dbusConnection, DBusPath, G_OBJECT(obj));
}


MInputContextGlibDBusConnection::MInputContextGlibDBusConnection()
    : activeContext(NULL),
      globalCorrectionEnabled(false),
      redirectionEnabled(false),
      detectableAutoRepeat(false),
      lastOrientation(0),
      server(NULL)
{
    dbus_g_thread_init();
    g_type_init();

    if (!QDir().mkpath(SocketDirectory)) {
        qFatal("IMServer: couldn't create directory for D-Bus socket.");
    }
    socketAddress = SocketDirectory;
    socketAddress.append("/");
    socketAddress.append(SocketName);
    QFile::remove(socketAddress);
    socketAddress.prepend("unix:path=");

    DBusError error;
    dbus_error_init(&error);

    server = dbus_server_listen(socketAddress, &error);
    if (!server) {
        qFatal("Couldn't create D-Bus server: %s", error.message);
    }

    dbus_server_setup_with_g_main(server, NULL);
    dbus_server_set_new_connection_function(server, handleNewConnection, this, NULL);
    connect(&MAttributeExtensionManager::instance(), SIGNAL(keyOverrideCreated()),
            this,                                    SIGNAL(keyOverrideCreated()));
}


MInputContextGlibDBusConnection::~MInputContextGlibDBusConnection()
{
    dbus_server_disconnect(server);
    dbus_server_unref(server);
}


// Server -> input context...................................................

QDataStream &operator<<(QDataStream &s, const MInputMethod::PreeditTextFormat &t)
{
    s << (qint32)t.start << (qint32)t.length
      << (qint32)t.preeditFace;
    return s;
}

void MInputContextGlibDBusConnection::sendPreeditString(const QString &string,
                                                        const QList<MInputMethod::PreeditTextFormat> &preeditFormats,
                                                        int replaceStart, int replaceLength,
                                                        int cursorPos)
{
    if (activeContext) {
        preedit = string;

        QByteArray temporaryStorage;
        QDataStream valueStream(&temporaryStorage, QIODevice::WriteOnly);
        valueStream << preeditFormats;
        GArray * const gdata(g_array_sized_new(FALSE, FALSE, 1, temporaryStorage.size()));
        g_array_append_vals(gdata, temporaryStorage.constData(),
                            temporaryStorage.size());
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "updatePreedit",
                                   G_TYPE_STRING, string.toUtf8().data(),
                                   DBUS_TYPE_G_UCHAR_ARRAY, gdata,
                                   G_TYPE_INT, replaceStart,
                                   G_TYPE_INT, replaceLength,
                                   G_TYPE_INT, cursorPos,
                                   G_TYPE_INVALID);
        g_array_unref(gdata);
    }
}


void MInputContextGlibDBusConnection::sendCommitString(const QString &string, int replaceStart,
                                                       int replaceLength, int cursorPos)
{
    if (activeContext) {
        const int cursorPosition(widgetState[CursorPositionAttribute].toInt());
        bool validAnchor(false);

        preedit.clear();

        if (replaceLength == 0  // we don't support replacement
            // we don't support selections
            && anchorPosition(validAnchor) == cursorPosition
            && validAnchor) {
            const int insertPosition(cursorPosition + replaceStart);
            if (insertPosition >= 0) {
                widgetState[SurroundingTextAttribute]
                    = widgetState[SurroundingTextAttribute].toString().insert(insertPosition, string);
                widgetState[CursorPositionAttribute] = cursorPos < 0 ? (insertPosition + string.length()) : cursorPos;
                widgetState[AnchorPositionAttribute] = widgetState[CursorPositionAttribute];
            }
        }
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "commitString",
                                   G_TYPE_STRING, string.toUtf8().data(),
                                   G_TYPE_INT, replaceStart,
                                   G_TYPE_INT, replaceLength,
                                   G_TYPE_INT, cursorPos,
                                   G_TYPE_INVALID);
    }
}


void MInputContextGlibDBusConnection::sendKeyEvent(const QKeyEvent &keyEvent,
                                                   MInputMethod::EventRequestType requestType)
{
    if (activeContext) {
        if (requestType != MInputMethod::EventRequestSignalOnly
            && preedit.isEmpty()
            && keyEvent.key() == Qt::Key_Backspace
            && keyEvent.type() == QEvent::KeyPress) {
            QString surrString(widgetState[SurroundingTextAttribute].toString());
            const int cursorPosition(widgetState[CursorPositionAttribute].toInt());
            bool validAnchor(false);

            if (!surrString.isEmpty()
                && cursorPosition > 0
                // we don't support selections
                && anchorPosition(validAnchor) == cursorPosition
                && validAnchor) {
                widgetState[SurroundingTextAttribute] = surrString.remove(cursorPosition - 1, 1);
                widgetState[CursorPositionAttribute] = cursorPosition - 1;
                widgetState[AnchorPositionAttribute] = cursorPosition - 1;
            }
        }

        int type = static_cast<int>(keyEvent.type());
        int key = static_cast<int>(keyEvent.key());
        int modifiers = static_cast<int>(keyEvent.modifiers());

        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "keyEvent",
                                   G_TYPE_INT, type,
                                   G_TYPE_INT, key,
                                   G_TYPE_INT, modifiers,
                                   G_TYPE_STRING, keyEvent.text().toUtf8().data(),
                                   G_TYPE_BOOLEAN, keyEvent.isAutoRepeat(),
                                   G_TYPE_INT, keyEvent.count(),
                                   G_TYPE_UCHAR, static_cast<uchar>(requestType),
                                   G_TYPE_INVALID);
    }
}


void MInputContextGlibDBusConnection::notifyImInitiatedHiding()
{
    if (activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "imInitiatedHide",
                                   G_TYPE_INVALID);
    }
}

int MInputContextGlibDBusConnection::contentType(bool &valid)
{
    QVariant contentTypeVariant = widgetState[ContentTypeAttribute];
    return contentTypeVariant.toInt(&valid);
}

bool MInputContextGlibDBusConnection::correctionEnabled(bool &valid)
{
    QVariant correctionVariant = widgetState[CorrectionAttribute];
    valid = correctionVariant.isValid();
    return correctionVariant.toBool();
}


bool MInputContextGlibDBusConnection::predictionEnabled(bool &valid)
{
    QVariant predictionVariant = widgetState[PredictionAttribute];
    valid = predictionVariant.isValid();
    return predictionVariant.toBool();
}

bool MInputContextGlibDBusConnection::autoCapitalizationEnabled(bool &valid)
{
    QVariant capitalizationVariant = widgetState[AutoCapitalizationAttribute];
    valid = capitalizationVariant.isValid();
    return capitalizationVariant.toBool();
}

QRect MInputContextGlibDBusConnection::cursorRectangle(bool &valid)
{
    QVariant cursorRectVariant = widgetState[CursorRectAttribute];
    valid = cursorRectVariant.isValid();
    return cursorRectVariant.toRect();
}

bool MInputContextGlibDBusConnection::hiddenText(bool &valid)
{
    QVariant hiddenTextVariant = widgetState[HiddenTextAttribute];
    valid = hiddenTextVariant.isValid();
    return hiddenTextVariant.toBool();
}

void MInputContextGlibDBusConnection::setGlobalCorrectionEnabled(bool enabled)
{
    if ((enabled != globalCorrectionEnabled) && activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "setGlobalCorrectionEnabled",
                                   G_TYPE_BOOLEAN, enabled,
                                   G_TYPE_INVALID);
    }

    globalCorrectionEnabled = enabled;
}

bool MInputContextGlibDBusConnection::surroundingText(QString &text, int &cursorPosition)
{
    QVariant textVariant = widgetState[SurroundingTextAttribute];
    QVariant posVariant = widgetState[CursorPositionAttribute];

    if (textVariant.isValid() && posVariant.isValid()) {
        text = textVariant.toString();
        cursorPosition = posVariant.toInt();
        return true;
    }

    return false;
}

bool MInputContextGlibDBusConnection::hasSelection(bool &valid)
{
    QVariant selectionVariant = widgetState[HasSelectionAttribute];
    valid = selectionVariant.isValid();
    return selectionVariant.toBool();
}


QString MInputContextGlibDBusConnection::selection(bool &valid)
{
    GError *error = NULL;

    QString selectionText;
    gboolean gvalidity = FALSE;
    gchar *gdata = NULL;
    if (activeContext && dbus_g_proxy_call(activeContext->inputContextProxy, "selection", &error,
                                           G_TYPE_INVALID,
                                           G_TYPE_BOOLEAN, &gvalidity,
                                           G_TYPE_STRING, &gdata,
                                           G_TYPE_INVALID)) {
        if (gdata) {
            selectionText = QString::fromUtf8(gdata);
            g_free(gdata);
        }
        valid = gvalidity == TRUE;
    } else {
        if (error) { // dbus_g_proxy_call may return FALSE and not set error despite what the doc says
            g_error_free(error);
        }
        valid = false;
        return QString();        
    }

    return selectionText;
}

void
MInputContextGlibDBusConnection::addTarget(MAbstractInputMethod *target)
{
    MInputContextConnection::addTarget(target);
    target->handleAppOrientationChanged(lastOrientation);
}

int MInputContextGlibDBusConnection::inputMethodMode(bool &valid)
{
    QVariant modeVariant = widgetState[InputMethodModeAttribute];
    return modeVariant.toInt(&valid);
}

QRect MInputContextGlibDBusConnection::preeditRectangle(bool &valid)
{
    GError *error = NULL;

    gboolean gvalidity;
    gint32 x, y, width, height;

    if (activeContext &&
        dbus_g_proxy_call(activeContext->inputContextProxy, "preeditRectangle", &error, G_TYPE_INVALID,
                          G_TYPE_BOOLEAN, &gvalidity, G_TYPE_INT, &x, G_TYPE_INT, &y,
                          G_TYPE_INT, &width, G_TYPE_INT, &height, G_TYPE_INVALID)) {
        valid = gvalidity == TRUE;
    } else {
        if (error) { // dbus_g_proxy_call may return FALSE and not set error despite what the doc says
            g_error_free(error);
        }
        valid = false;
        return QRect();
    }

    return QRect(x, y, width, height);
}

void MInputContextGlibDBusConnection::setRedirectKeys(bool enabled)
{
    if ((redirectionEnabled != enabled) && activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "setRedirectKeys",
                                   G_TYPE_BOOLEAN, enabled ? TRUE : FALSE,
                                   G_TYPE_INVALID);
    }
    redirectionEnabled = enabled;
}

void MInputContextGlibDBusConnection::setDetectableAutoRepeat(bool enabled)
{
    if ((detectableAutoRepeat != enabled) && activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "setDetectableAutoRepeat",
                                   G_TYPE_BOOLEAN, enabled,
                                   G_TYPE_INVALID);
    }
    detectableAutoRepeat = enabled;
}


void MInputContextGlibDBusConnection::copy()
{
    if (activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "copy",
                                   G_TYPE_INVALID);
    }
}


void MInputContextGlibDBusConnection::paste()
{
    if (activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "paste",
                                   G_TYPE_INVALID);
    }
}


void MInputContextGlibDBusConnection::setSelection(int start, int length)
{
    if (activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "setSelection",
                                   G_TYPE_INT, start,
                                   G_TYPE_INT, length,
                                   G_TYPE_INVALID);
    }
}

int MInputContextGlibDBusConnection::anchorPosition(bool &valid)
{
    QVariant posVariant = widgetState[AnchorPositionAttribute];
    valid = posVariant.isValid();
    return posVariant.toInt();
}

void MInputContextGlibDBusConnection::updateInputMethodArea(const QRegion &region)
{
    if (activeContext) {
        QList<QVariant> data;
        data.append(region.boundingRect());

        QByteArray temporaryStorage;
        QDataStream valueStream(&temporaryStorage, QIODevice::WriteOnly);
        valueStream << data;
        GArray * const gdata(g_array_sized_new(FALSE, FALSE, 1, temporaryStorage.size()));
        g_array_append_vals(gdata, temporaryStorage.constData(),
                            temporaryStorage.size());

        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "updateInputMethodArea",
                                   DBUS_TYPE_G_UCHAR_ARRAY, gdata,
                                   G_TYPE_INVALID);

        g_array_unref(gdata);
    }
}


// Input context -> server...................................................

void MInputContextGlibDBusConnection::activateContext(MDBusGlibICConnection *connection)
{
    MDBusGlibICConnection *previousActive = activeContext;

    activeContext = connection;

    if (activeContext) {
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "setGlobalCorrectionEnabled",
                                   G_TYPE_BOOLEAN, globalCorrectionEnabled,
                                   G_TYPE_INVALID);
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "setRedirectKeys",
                                   G_TYPE_BOOLEAN, redirectionEnabled,
                                   G_TYPE_INVALID);
        dbus_g_proxy_call_no_reply(activeContext->inputContextProxy, "detectableAutoRepeat",
                                   G_TYPE_BOOLEAN, detectableAutoRepeat,
                                   G_TYPE_INVALID);

        if ((previousActive != 0) && (previousActive != activeContext)) {
            // TODO: we can't use previousActive here like this
            dbus_g_proxy_call_no_reply(previousActive->inputContextProxy, "activationLostEvent",
                                       G_TYPE_INVALID);
        }
    }

    // notify plugins
    foreach (MAbstractInputMethod *target, targets()) {
        target->handleClientChange();
    }
}


void MInputContextGlibDBusConnection::showInputMethod(MDBusGlibICConnection *sourceConnection)
{
    if (activeContext != sourceConnection)
        return;

    emit showInputMethodRequest();
}


void MInputContextGlibDBusConnection::hideInputMethod(MDBusGlibICConnection *sourceConnection)
{
    // Only allow this call for current active connection.
    if (activeContext != sourceConnection)
        return;

    emit hideInputMethodRequest();
}


void MInputContextGlibDBusConnection::mouseClickedOnPreedit(MDBusGlibICConnection *sourceConnection,
                                                            const QPoint &pos, const QRect &preeditRect)
{
    if (activeContext != sourceConnection)
        return;

    foreach (MAbstractInputMethod *target, targets()) {
        target->handleMouseClickOnPreedit(pos, preeditRect);
    }
}


void MInputContextGlibDBusConnection::setPreedit(MDBusGlibICConnection *sourceConnection,
                                                 const QString &text, int cursorPos)
{
    if (activeContext != sourceConnection)
        return;

    preedit = text;

    foreach (MAbstractInputMethod *target, targets()) {
        target->setPreedit(text, cursorPos);
    }
}


void MInputContextGlibDBusConnection::reset(MDBusGlibICConnection *sourceConnection)
{
    if (activeContext != sourceConnection)
        return;

    preedit.clear();

    foreach (MAbstractInputMethod *target, targets()) {
        target->reset();
    }
}

void
MInputContextGlibDBusConnection::updateWidgetInformation(
    MDBusGlibICConnection *connection, const QMap<QString, QVariant> &stateInfo,
    bool focusChanged)
{
    // check visualization change
    bool oldVisualization = false;
    bool newVisualization = false;

    QVariant variant = widgetState[VisualizationAttribute];

    if (variant.isValid()) {
        oldVisualization = variant.toBool();
    }

    variant = stateInfo[VisualizationAttribute];
    if (variant.isValid()) {
        newVisualization = variant.toBool();
    }

    // toolbar change
    MAttributeExtensionId oldAttributeExtensionId;
    MAttributeExtensionId newAttributeExtensionId;
    oldAttributeExtensionId = attributeExtensionId;

    variant = stateInfo[ToolbarIdAttribute];
    if (variant.isValid()) {
        // map toolbar id from local to global
        newAttributeExtensionId = MAttributeExtensionId(variant.toInt(), QString::number(connection->connectionNumber));
    }
    if (!newAttributeExtensionId.isValid()) {
        newAttributeExtensionId = MAttributeExtensionId::standardAttributeExtensionId();
    }

    // update state
    widgetState = stateInfo;

    if (focusChanged) {
        foreach (MAbstractInputMethod *target, targets()) {
            target->handleFocusChange(stateInfo[FocusStateAttribute].toBool());
        }

        updateTransientHint();
    }

    // call notification methods if needed
    if (oldVisualization != newVisualization) {
        foreach (MAbstractInputMethod *target, targets()) {
            target->handleVisualizationPriorityChange(newVisualization);
        }
    }

    // compare the toolbar id (global)
    if (oldAttributeExtensionId != newAttributeExtensionId) {
        QString toolbarFile = stateInfo[ToolbarAttribute].toString();
        if (!MAttributeExtensionManager::instance().contains(newAttributeExtensionId) && !toolbarFile.isEmpty()) {
            // register toolbar if toolbar manager does not contain it but
            // toolbar file is not empty. This can reload the toolbar data
            // if im-uiserver crashes.
            variant = stateInfo[ToolbarIdAttribute];
            if (variant.isValid()) {
                const int toolbarLocalId = variant.toInt();
                registerAttributeExtension(connection, toolbarLocalId, toolbarFile);
            }
        }
        emit toolbarIdChanged(newAttributeExtensionId);
        // store the new used toolbar id(global).
        attributeExtensionId = newAttributeExtensionId;
    }

    // general notification last
    foreach (MAbstractInputMethod *target, targets()) {
        target->update();
    }
}

void
MInputContextGlibDBusConnection::receivedAppOrientationAboutToChange(MDBusGlibICConnection *sourceConnection,
                                                                     int angle)
{
    if (activeContext != sourceConnection)
        return;

    // Needs to be passed to the MImRotationAnimation listening
    // to this signal first before the plugins. This ensures
    // that the rotation animation can be painted sufficiently early.
    emit appOrientationAboutToChange(angle);
    foreach (MAbstractInputMethod *target, targets()) {
        target->handleAppOrientationAboutToChange(angle);
    }
}


void MInputContextGlibDBusConnection::receivedAppOrientationChanged(MDBusGlibICConnection *sourceConnection,
                                                                    int angle)
{
    if (activeContext != sourceConnection)
        return;

    // Handle orientation changes through MImRotationAnimation with priority.
    // That's needed for getting the correct rotated pixmap buffers.
    emit appOrientationChanged(angle);
    foreach (MAbstractInputMethod *target, targets()) {
        target->handleAppOrientationChanged(angle);
    }
    lastOrientation = angle;
}


void MInputContextGlibDBusConnection::setCopyPasteState(MDBusGlibICConnection *sourceConnection,
                                                        bool copyAvailable, bool pasteAvailable)
{
    if (activeContext != sourceConnection)
        return;

    MAttributeExtensionManager::instance().setCopyPasteState(copyAvailable, pasteAvailable);
}


void MInputContextGlibDBusConnection::processKeyEvent(
    MDBusGlibICConnection *sourceConnection, QEvent::Type keyType, Qt::Key keyCode,
    Qt::KeyboardModifiers modifiers, const QString &text, bool autoRepeat, int count,
    quint32 nativeScanCode, quint32 nativeModifiers, unsigned long time)
{
    if (activeContext != sourceConnection)
        return;

    foreach (MAbstractInputMethod *target, targets()) {
        target->processKeyEvent(keyType, keyCode, modifiers, text, autoRepeat, count,
                                nativeScanCode, nativeModifiers, time);
    }
}

void MInputContextGlibDBusConnection::registerAttributeExtension(MDBusGlibICConnection *connection, int id,
                                                         const QString &attributeExtension)
{
    MAttributeExtensionId globalId(id, QString::number(connection->connectionNumber));
    if (globalId.isValid() && !attributeExtensionIds.contains(globalId)) {
        MAttributeExtensionManager::instance().registerAttributeExtension(globalId, attributeExtension);
        attributeExtensionIds.insert(globalId);
    }
}

void MInputContextGlibDBusConnection::unregisterAttributeExtension(MDBusGlibICConnection *connection, int id)
{
    MAttributeExtensionId globalId(id, QString::number(connection->connectionNumber));
    if (globalId.isValid() && attributeExtensionIds.contains(globalId)) {
        MAttributeExtensionManager::instance().unregisterAttributeExtension(globalId);
        attributeExtensionIds.remove(globalId);
    }
}

void MInputContextGlibDBusConnection::setExtendedAttribute(
    MDBusGlibICConnection *connection, int id, const QString &target, const QString &targetName,
    const QString &attribute, const QVariant &value)
{
    qDebug() << __PRETTY_FUNCTION__;
    MAttributeExtensionId globalId(id, QString::number(connection->connectionNumber));
    if (globalId.isValid() && attributeExtensionIds.contains(globalId)) {
        MAttributeExtensionManager::instance().setExtendedAttribute(globalId, target, targetName, attribute, value);
    }
}

void MInputContextGlibDBusConnection::updateTransientHint()
{
    bool ok = false;
    WId appWinId = winId(ok);

    if (ok) {
        MIMApplication *app = MIMApplication::instance();

        if (app) {
            app->setTransientHint(appWinId);
        }
    }
}

WId MInputContextGlibDBusConnection::winId(bool &valid)
{
    QVariant winIdVariant = widgetState[WinId];
    valid = winIdVariant.canConvert<WId>();
    return winIdVariant.value<WId>();
}
