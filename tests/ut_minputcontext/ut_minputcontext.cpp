#include "ut_minputcontext.h"

#include <QStringList>
#include <QDebug>
#include <QPointer>
#include <QClipboard>
#include <QGraphicsView>

#ifdef HAVE_MEEGOTOUCH
#include <MWindow>
#include <mpreeditinjectionevent.h>
#endif

#include "glibdbusimserverproxy.h"


namespace
{
    const QString DBUS_SERVICE_NAME("com.meego.inputmethod.uiserver1");
    const QString DBUS_PATH("/com/meego/inputmethod/uiserver1");
    const QString DBUS_INTERFACE("com.meego.inputmethod.uiserver1");

    const int WidgetStubCursorPosition(37);
    const QString WidgetStubSurroundingText("surrounding");
    const Qt::KeyboardModifier FnLevelModifier = Qt::GroupSwitchModifier;
    const Qt::Key FnLevelKey = Qt::Key_AltGr;
}

namespace
{
    QPointer<QWidget> gFocusedWidget = 0;
}

QWidget *QApplication::focusWidget()
{
    return gFocusedWidget;
}

InputMethodServerDBusStub* DBusStub;

// Glib DBus stubbing........................................................

void GlibDBusIMServerProxy::connectToDBus()
{
}

void GlibDBusIMServerProxy::setContextObject(const QString &inputContextIdentifier)
{
    DBusStub->setContextObject(inputContextIdentifier);
}

void GlibDBusIMServerProxy::activateContext()
{
    DBusStub->activateContext();
}

void GlibDBusIMServerProxy::showInputMethod()
{
    DBusStub->showInputMethod();
}

void GlibDBusIMServerProxy::hideInputMethod()
{
    DBusStub->hideInputMethod();
}

void GlibDBusIMServerProxy::mouseClickedOnPreedit(const QPoint &pos, const QRect &preeditRect)
{
    DBusStub->mouseClickedOnPreedit(pos, preeditRect);
}

void GlibDBusIMServerProxy::setPreedit(const QString &text, int cursorPos)
{
    DBusStub->setPreedit(text, cursorPos);
}

void GlibDBusIMServerProxy::updateWidgetInformation(const QMap<QString, QVariant> &/*stateInformation*/,
                                                    bool /*focusChanged*/)
{
}

void GlibDBusIMServerProxy::reset(bool)
{
    DBusStub->reset();
}

void GlibDBusIMServerProxy::appOrientationChanged(int angle)
{
    DBusStub->appOrientationChanged(angle);
}

void GlibDBusIMServerProxy::setCopyPasteState(bool copyAvailable, bool pasteAvailable)
{
    DBusStub->setCopyPasteState(copyAvailable, pasteAvailable);
}

void GlibDBusIMServerProxy::processKeyEvent(QEvent::Type keyType, Qt::Key keyCode,
                                            Qt::KeyboardModifiers modifiers,
                                            const QString &text, bool autoRepeat, int count,
                                            quint32 nativeScanCode, quint32 nativeModifiers,
                                            unsigned long time)
{
    DBusStub->processKeyEvent(keyType, keyCode, modifiers, text, autoRepeat, count, nativeScanCode,
                              nativeModifiers, time);
}

void GlibDBusIMServerProxy::registerAttributeExtension(int /*id*/, const QString &/*fileName*/)
{
}

void GlibDBusIMServerProxy::unregisterAttributeExtension(int /*id*/)
{
}

void GlibDBusIMServerProxy::setExtendedAttribute(int /*id*/, const QString &/*target*/, const QString &/*item*/,
                                                 const QString &/*attribute*/, const QVariant &/*value*/)
{
}


// Qt DBus stub class implementation..........................................

void InputMethodServerDBusStub::RedirectedKeyParamsStruct::clear()
{
    keyType = 0;
    keyCode = 0;
    text.clear();
}

InputMethodServerDBusStub::InputMethodServerDBusStub(QObject *object)
{
    Q_UNUSED(object);
    resetCallCounts();
}


InputMethodServerDBusStub::~InputMethodServerDBusStub()
{
}


void InputMethodServerDBusStub::resetCallCounts()
{
    showInputMethodCallCount = 0;
    hideInputMethodCallCount = 0;
    mouseClickedOnPreeditCallCount = 0;
    setPreeditCallCount = 0;
    resetCallCount = 0;

    setContextObjectCallCount = 0;
    activateContextCallCount = 0;

    keyEventCallCount = 0;
    appOrientationChangedCount = 0;
    setCopyPasteStateCallCount = 0;
    redirectKeyCallCount = 0;
}


int InputMethodServerDBusStub::showInputMethodCount()
{
    return showInputMethodCallCount;
}


int InputMethodServerDBusStub::hideInputMethodCount()
{
    return hideInputMethodCallCount;
}


int InputMethodServerDBusStub::mouseClickedOnPreeditCount()
{
    return mouseClickedOnPreeditCallCount;
}



int InputMethodServerDBusStub::setPreeditCount()
{
    return setPreeditCallCount;
}


int InputMethodServerDBusStub::resetCount()
{
    return resetCallCount;
}


int InputMethodServerDBusStub::setContextObjectCount()
{
    return setContextObjectCallCount;
}

int InputMethodServerDBusStub::activateContextCount()
{
    return activateContextCallCount;
}

int InputMethodServerDBusStub::keyEventCount()
{
    return keyEventCallCount;
}

int InputMethodServerDBusStub::orientationChangedCount()
{
    return appOrientationChangedCount;
}

int InputMethodServerDBusStub::setCopyPasteStateCount()
{
    return setCopyPasteStateCallCount;
}

QList<bool>& InputMethodServerDBusStub::setCopyPasteStateParams()
{
    return setCopyPasteStateCallParams;
}

int InputMethodServerDBusStub::redirectKeyCount()
{
    return redirectKeyCallCount;
}

InputMethodServerDBusStub::RedirectedKeyParamsStruct &InputMethodServerDBusStub::redirectKeyParams()
{
    return redirectKeyCallParams;
}

///////
void InputMethodServerDBusStub::showInputMethod()
{
    showInputMethodCallCount++;
}


void InputMethodServerDBusStub::hideInputMethod()
{
    hideInputMethodCallCount++;
}


void InputMethodServerDBusStub::mouseClickedOnPreedit(const QPoint &pos, const QRect &preeditRect)
{
    Q_UNUSED(pos);
    Q_UNUSED(preeditRect);

    mouseClickedOnPreeditCallCount++;
}


void InputMethodServerDBusStub::setPreedit(const QString &text, int cursorPos)
{
    Q_UNUSED(text);
    Q_UNUSED(cursorPos);

    setPreeditCallCount++;
}


void InputMethodServerDBusStub::reset()
{
    resetCallCount++;
}


void InputMethodServerDBusStub::setContextObject(const QString &callbackObject)
{
    Q_UNUSED(callbackObject)

    setContextObjectCallCount++;
}


void InputMethodServerDBusStub::activateContext()
{
    activateContextCallCount++;
}

void InputMethodServerDBusStub::sendKeyEvent(const QKeyEvent &keyEvent,
                                             MInputMethod::EventRequestType requestType)
{
    Q_UNUSED(keyEvent);
    Q_UNUSED(requestType)
    keyEventCallCount++;
}

void InputMethodServerDBusStub::appOrientationChanged(int angle)
{
    Q_UNUSED(angle);
    appOrientationChangedCount++;
}

void InputMethodServerDBusStub::setCopyPasteState(bool copyAvailable, bool pasteAvailable)
{
    ++setCopyPasteStateCallCount;
    setCopyPasteStateCallParams.append(copyAvailable);
    setCopyPasteStateCallParams.append(pasteAvailable);
}

void InputMethodServerDBusStub::processKeyEvent(int keyType, int keyCode, int /* modifiers */,
                                                const QString &text, bool /* autoRepeat */,
                                                int /* count */, unsigned int /* nativeScanCode */,
                                                unsigned int /* nativeModifiers */, unsigned long /* time */)
{
    ++redirectKeyCallCount;
    redirectKeyCallParams.keyType = keyType;
    redirectKeyCallParams.keyCode = keyCode;
    redirectKeyCallParams.text = text;
}

//////////////
// widget stub

WidgetStub::WidgetStub(QObject *, bool enableVisualizationPriority)
    : m_lastKeyEvent(QKeyEvent::KeyPress, 0, 0),
      visualizationPriority(enableVisualizationPriority)
{
    setAttribute(Qt::WA_InputMethodEnabled);
    resetCounters();
}


WidgetStub::~WidgetStub()
{
}


QVariant WidgetStub::inputMethodQuery(Qt::InputMethodQuery query) const
{
    m_inputMethodQueryCount++;

    if (query == Qt::ImSurroundingText) {
        return QVariant(WidgetStubSurroundingText);
#ifdef HAVE_MEEGOTOUCH
    } else if (static_cast<int>(query) == M::VisualizationPriorityQuery) {
        return QVariant(visualizationPriority);
#endif
    } else if (query == Qt::ImCursorPosition) {
        return QVariant(WidgetStubCursorPosition);
    } else if (query == Qt::ImCurrentSelection) {
        return QVariant(selectedText);
    }

    return QVariant();
}


void WidgetStub::inputMethodEvent(QInputMethodEvent *event)
{
    m_inputMethodEventCount++;
    m_lastImEvent = *event;
}

bool WidgetStub::event(QEvent *event)
{
    ++m_eventCount;

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *kevent = dynamic_cast<QKeyEvent *>(event);
        if (kevent)
            m_lastKeyEvent = *kevent;
    }

    return QWidget::event(event);
}


void WidgetStub::resetCounters()
{
    m_inputMethodEventCount = 0;
    m_inputMethodQueryCount = 0;
    m_eventCount = 0;
}


int WidgetStub::inputMethodQueryCount()
{
    return m_inputMethodQueryCount;
}


int WidgetStub::inputMethodEventCount()
{
    return m_inputMethodEventCount;
}

int WidgetStub::eventCount()
{
    return m_eventCount;
}

QInputMethodEvent WidgetStub::lastInputMethodEvent()
{
    return m_lastImEvent;
}
QKeyEvent WidgetStub::lastKeyEvent()
{
    return m_lastKeyEvent;
}

void WidgetStub::sendCopyAvailable(bool yes)
{
    emit copyAvailable(yes);
}


///////////////
// actual test


void Ut_MInputContext::initTestCase()
{
    static char *argv[2] = { (char *) "ut_minputcontext",
                             (char *) "-software"};
    static int argc = 2;

    // QInputContext::setFocusWidget seems to require a valid X11 window.
    // However, the test crashes with MeeGo GS. That's why we set the graphics
    // system manually. Should be removed once NB#220339 [1] gets fixed.
    // [1] "QWidget::create crashes in window-less application"
    QApplication::setGraphicsSystem("raster");

    QCoreApplication::setLibraryPaths(QStringList("./inputmethods"));
    app.reset(new QApplication(argc, argv));

    m_stub = new InputMethodServerDBusStub(this);

    DBusStub = m_stub;          // for glib dbus use

    m_subject = new MInputContext;
    QVERIFY(m_subject != 0);

    // Overly cautious sanity check, but we do use native C API in
    // MInputContext and friends:
    QCOMPARE(TRUE, true);
    QCOMPARE(FALSE, false);
}


void Ut_MInputContext::cleanupTestCase()
{
    delete m_subject;
    delete m_stub;
}


void Ut_MInputContext::init()
{
    m_stub->resetCallCounts();
}

void Ut_MInputContext::cleanup()
{
}

void Ut_MInputContext::testAddCoverage()
{
    // Visit the less useful API here
    qDebug() << m_subject->identifierName();
    qDebug() << m_subject->isComposing();
    qDebug() << m_subject->language();
    m_subject->updateInputMethodArea(QList<QVariant>());
}

void Ut_MInputContext::testEvent()
{
#ifdef HAVE_MEEGOTOUCH
    WidgetStub widget(0);

    gFocusedWidget = &widget;
    // test that input context accepts
    m_subject->setGlobalCorrectionEnabled(true);
    MPreeditInjectionEvent *injectionEvent = new MPreeditInjectionEvent("preedit");
    bool accepted = QCoreApplication::sendEvent(m_subject, injectionEvent);
    QVERIFY(accepted == true);

    waitAndProcessEvents(500);

    // This event claims to be pre-edit injection event but it's not.
    QEvent fakeInjectionEvent((QEvent::Type)MPreeditInjectionEvent::eventNumber());
    accepted = QCoreApplication::sendEvent(m_subject, &fakeInjectionEvent);
    QVERIFY(accepted == false);

    waitAndProcessEvents(500);

    QCOMPARE(m_stub->setPreeditCount(), 1);

    m_stub->resetCallCounts();
    m_subject->setGlobalCorrectionEnabled(false);
    injectionEvent->setAccepted(false);
    accepted = QCoreApplication::sendEvent(m_subject, injectionEvent);
    QVERIFY(accepted == false);

    waitAndProcessEvents(500);

    //won't call setPreedit if global error correction is off
    QCOMPARE(m_stub->setPreeditCount(), 0);

    delete injectionEvent;
    gFocusedWidget = 0;
#endif
}


void Ut_MInputContext::testReset()
{
    WidgetStub widget(0);
    QInputMethodEvent event;
    QString preeditString("");
    QList<MInputMethod::PreeditTextFormat> preeditFormats;
    MInputMethod::PreeditTextFormat preeditFormat;

    preeditFormat.start = 0;
    preeditFormat.preeditFace = MInputMethod::PreeditDefault;
    m_subject->setFocusWidget(&widget);
    gFocusedWidget = &widget;

    // Reset with empty pre-edit
    preeditFormat.length = preeditString.length();
    preeditFormats << preeditFormat;
    m_subject->updatePreedit(preeditString, preeditFormats, -1);
    widget.resetCounters();

    m_subject->reset();

    waitAndProcessEvents(500);

    QCOMPARE(m_stub->resetCount(), 1);
    QCOMPARE(widget.inputMethodEventCount(), 0);

    m_stub->resetCallCounts();
    widget.resetCounters();
    preeditFormats.clear();

    // Reset with non-empty pre-edit
    preeditString = "some string";
    preeditFormat.length = preeditString.length();
    preeditFormats << preeditFormat;
    m_subject->updatePreedit(preeditString, preeditFormats, -1);
    widget.resetCounters();

    m_subject->reset();

    waitAndProcessEvents(500);

    QCOMPARE(m_stub->resetCount(), 1);
    QCOMPARE(widget.inputMethodEventCount(), 1);
    event = widget.lastInputMethodEvent();
    QCOMPARE(event.preeditString(), QString(""));
    QCOMPARE(event.commitString(),  preeditString);
}


void Ut_MInputContext::testMouseHandler()
{
    // TODO: test mouse button press and maybe double click

    WidgetStub widget(0);
    int x = 4;
    // mouse left button release on (10, 10), no additional modifiers
    QPoint point(10, 10);
    QPoint globalPoint(20, 20);
    QMouseEvent event(QEvent::MouseButtonRelease, point, globalPoint, Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);

    m_subject->setFocusWidget(&widget);
    m_subject->mouseHandler(x, &event);

    waitAndProcessEvents(500);

    QCOMPARE(m_stub->mouseClickedOnPreeditCount(), 1);

    m_subject->setFocusWidget(0);
}


void Ut_MInputContext::testInputMethodHidden()
{
    // nothing yet
}


void Ut_MInputContext::testCommitString()
{
    WidgetStub widget(0);
    QString commitString("committed string");
    m_subject->setFocusWidget(&widget);
    gFocusedWidget = &widget;
    m_subject->commitString(commitString);

    waitAndProcessEvents(0);

    QCOMPARE(widget.inputMethodEventCount(), 1);
    QInputMethodEvent event = widget.lastInputMethodEvent();
    QVERIFY(event.preeditString() == ""); // check that event doesn't contain wrong information
    QCOMPARE(event.commitString(), commitString);

    gFocusedWidget = 0;
}


void Ut_MInputContext::testUpdatePreedit()
{
    WidgetStub widget(0);
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event;
    QString updateString("preedit string");

    //test preedit with traditional style
    m_subject->setFocusWidget(&widget);
    gFocusedWidget = &widget;
    QList<MInputMethod::PreeditTextFormat> preeditFormats;
    MInputMethod::PreeditTextFormat preeditFormat(0, updateString.length(),
                                                  MInputMethod::PreeditDefault);
    preeditFormats << preeditFormat;
    m_subject->updatePreedit(updateString, preeditFormats, -1);

    waitAndProcessEvents(0);

    QCOMPARE(widget.inputMethodEventCount(), 1);
    event = widget.lastInputMethodEvent();
    QVERIFY(event.preeditString() == updateString);
    QVERIFY(event.commitString() == "");
    attributes = event.attributes();
    QVERIFY(attributes.count() > 0);

    //test preedit with alternate style
    preeditFormats.clear();
    preeditFormat.start = 0;
    preeditFormat.length = updateString.length();
    preeditFormat.preeditFace = MInputMethod::PreeditNoCandidates;
    preeditFormats << preeditFormat;
    m_subject->updatePreedit(updateString, preeditFormats, -1);

    waitAndProcessEvents(50);

    QCOMPARE(widget.inputMethodEventCount(), 2);
    event = widget.lastInputMethodEvent();
    QVERIFY(event.preeditString() == updateString);
    QVERIFY(event.commitString() == "");
    attributes = event.attributes();
    QVERIFY(attributes.count() > 0);

    gFocusedWidget = 0;
}

void Ut_MInputContext::testAppOrientationChanged()
{
#ifdef HAVE_MEEGOTOUCH
    m_subject->notifyOrientationChanged(M::Angle90);

    // Make sure DBus call gets through
    waitAndProcessEvents(300);

    // TODO: can not recieve signal, should check it
    QCOMPARE(m_stub->orientationChangedCount(), 1);
#endif
}

void Ut_MInputContext::testNonTextEntryWidget()
{

    int count = m_stub->hideInputMethodCount();
    gFocusedWidget = 0;

    QEvent close(QEvent::CloseSoftwareInputPanel);
    m_subject->filterEvent(&close);

    waitAndProcessEvents(1500);

    QCOMPARE(m_stub->hideInputMethodCount(), count + 1);
}

void Ut_MInputContext::testSendKeyEvent()
{
    QKeyEvent keyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    m_stub->sendKeyEvent(keyEvent, MInputMethod::EventRequestBoth);
    QCOMPARE(m_stub->keyEventCount(), 1);

    WidgetStub widget(0);
    m_subject->setFocusWidget(&widget);
    m_stub->sendKeyEvent(keyEvent, MInputMethod::EventRequestBoth);
    QCOMPARE(m_stub->keyEventCount(), 2);

}

void Ut_MInputContext::testKeyEvent()
{
    QEvent::Type eventType = QEvent::KeyPress;
    int eventKey = Qt::Key_A;
    Qt::KeyboardModifiers eventModifiers = Qt::NoModifier;
    QString eventText('A');
    bool eventRepeat = false;
    unsigned short eventCount = 1;

    WidgetStub widget(0);
    gFocusedWidget = &widget;
    m_subject->setFocusWidget(&widget);
    widget.resetCounters();
    m_subject->keyEvent(eventType, eventKey, eventModifiers, eventText, eventRepeat, eventCount);
    QVERIFY(widget.eventCount() > 0);

    QKeyEvent expected(eventType, eventKey, eventModifiers, eventText, eventRepeat, eventCount);
    QKeyEvent result = widget.lastKeyEvent();

    QCOMPARE(result.type(), expected.type());
    QCOMPARE(result.key(), expected.key());
    QCOMPARE(result.modifiers(), expected.modifiers());
    QCOMPARE(result.text(), expected.text());
    QCOMPARE(result.count(), expected.count());

    m_subject->setFocusWidget(0);
    gFocusedWidget = 0;
}

void Ut_MInputContext::testCopyPasteState()
{
    WidgetStub widget(0);
    QList<bool> &params = m_stub->setCopyPasteStateParams();

    QApplication::clipboard()->clear();

    waitAndProcessEvents(100);

    int count = m_stub->setCopyPasteStateCount();
    params.clear();
    qDebug() << "No focused widget";
    gFocusedWidget = 0;
    m_subject->setFocusWidget(0);
    waitAndProcessEvents(100);
    ++count;
    QCOMPARE(m_stub->setCopyPasteStateCount(), count);
    QCOMPARE(params.count(), 2);
    QCOMPARE(params.takeFirst(), false);
    QCOMPARE(params.takeFirst(), false);

    qDebug() << "Set widget without selection";
    gFocusedWidget = &widget;
    m_subject->setFocusWidget(&widget);
    waitAndProcessEvents(100);
    ++count;
    QCOMPARE(m_stub->setCopyPasteStateCount(), count);
    QCOMPARE(params.count(), 2);
    QCOMPARE(params.takeFirst(), false);
    QCOMPARE(params.takeFirst(), false);

    qDebug() << "Update button state with empty selection";
    widget.sendCopyAvailable(false);
    waitAndProcessEvents(100);
    QCOMPARE(m_stub->setCopyPasteStateCount(), count);

    qDebug() << "Update button state with something selected";
    widget.selectedText = "Some text";
    widget.sendCopyAvailable(true);
    qDebug() << "Text was selected";
    waitAndProcessEvents(100);
    ++count;
    QVERIFY(m_stub->setCopyPasteStateCount() == count);
    QVERIFY(params.count() == 2);
    QCOMPARE(params.takeFirst(), true);
    QCOMPARE(params.takeFirst(), false);

    qDebug() << "Set some text to clipboard (simulate paste)";
    QApplication::clipboard()->setText("Some text");
    waitAndProcessEvents(100);
    ++count;
    QCOMPARE(m_stub->setCopyPasteStateCount(), count);
    QCOMPARE(params.count(), 2);
    QCOMPARE(params.takeFirst(), true);
    QCOMPARE(params.takeFirst(), true);

    qDebug() << "Update button state with text unselected";
    widget.sendCopyAvailable(false);
    waitAndProcessEvents(100);
    ++count;
    QCOMPARE(m_stub->setCopyPasteStateCount(), count);
    QCOMPARE(params.count(), 2);
    QCOMPARE(params.takeFirst(), false);
    QCOMPARE(params.takeFirst(), true);

    qDebug() << "Clear clipboard";
    QApplication::clipboard()->clear();
    waitAndProcessEvents(100);
    ++count;
    QCOMPARE(m_stub->setCopyPasteStateCount(), count);
    QCOMPARE(params.count(), 2);
    QCOMPARE(params.takeFirst(), false);
    QCOMPARE(params.takeFirst(), false);

    qDebug() << "Restore initial state";
    gFocusedWidget = 0;
    m_subject->setFocusWidget(0);
    waitAndProcessEvents(100);
    ++count;
    QVERIFY(m_stub->setCopyPasteStateCount() == count);
    QCOMPARE(params.count(), 2);
    QCOMPARE(params.takeFirst(), false);
    QCOMPARE(params.takeFirst(), false);

    qDebug() << "Set some text to clipboard (simulate paste)";
    QApplication::clipboard()->setText("Some text");
    waitAndProcessEvents(100);
    QVERIFY(m_stub->setCopyPasteStateCount() == count);

    qDebug() << "Clear clipboard";
    QApplication::clipboard()->clear();
    waitAndProcessEvents(100);
    QVERIFY(m_stub->setCopyPasteStateCount() == count);
}

void Ut_MInputContext::testSetRedirectKeys()
{
    WidgetStub widget(0);
    m_subject->setFocusWidget(&widget);
    // no redirection should happen
    int count = m_stub->redirectKeyCount();
    QKeyEvent request(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier, "");
    bool ret = m_subject->filterEvent(&request);
    QVERIFY(!ret);
    QCOMPARE(m_stub->redirectKeyCount(), count);

    // redirection should happen
    m_subject->setRedirectKeys(true);
    InputMethodServerDBusStub::RedirectedKeyParamsStruct &params = m_stub->redirectKeyParams();
    params.clear();
    ret = m_subject->filterEvent(&request);
    QVERIFY(ret);
    QCOMPARE(m_stub->redirectKeyCount(), count + 1);
    QCOMPARE(params.keyType, static_cast<int>(QEvent::KeyPress));
    QCOMPARE(params.keyCode, static_cast<int>(Qt::Key_Shift));
    QCOMPARE(params.text, QString(""));

    // no redirection should happen
    m_subject->setRedirectKeys(false);
    ret = m_subject->filterEvent(&request);
    QVERIFY(!ret);
    QCOMPARE(m_stub->redirectKeyCount(), count + 1);
    m_subject->setFocusWidget(0);
}

void Ut_MInputContext::waitAndProcessEvents(int waitTime)
{
    QTest::qWait(waitTime);
    while (app->hasPendingEvents()) {
        app->processEvents();
    }
}

void Ut_MInputContext::testInvalidScene()
{
    QGraphicsView view;

    // Regression test:
    // NB#177259 - MInputContext::setFocusWidget crash
    // commit f76915c6 - Prevent a crash in MInputContext if the QGraphicsView has no scene yet.
    m_subject->setFocusWidget(&view);

    m_subject->imInitiatedHide();
}

QTEST_APPLESS_MAIN(Ut_MInputContext)

