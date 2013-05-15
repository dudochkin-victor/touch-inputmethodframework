include(../mconfig.pri)

VERSION = 0.1.0
TEMPLATE = lib
TARGET = meegoimframework
INCLUDEPATH += ../passthroughserver

# Input
HEADERSINSTALL = \
        minputmethodplugin.h \
        mabstractinputmethod.h \
        mabstractinputmethodhost.h \
        mimpluginmanager.h \
        mtoolbaritem.h \
        mtoolbardata.h \
        mkeyoverride.h \
        mkeyoverridedata.h \
        mattributeextension.h \
        minputmethodnamespace.h \
        mabstractinputmethodsettings.h \
        mtoolbarlayout.h \
        mimextensionevent.h \
        mimgraphicsview.h \
        mimwidget.h \
        mimplugindescription.h \

HEADERS += $$HEADERSINSTALL \
        mimpluginmanager_p.h \
        mimpluginmanageradaptor.h \
        mindicatorserviceclient.h \
        mimapplication.h \
        minputcontextconnection.h \
        minputmethodhost.h \
        mtoolbardata_p.h \
        mtoolbaritem_p.h \
        mkeyoverride_p.h \
        mattributeextensionmanager.h \
        mattributeextensionid.h \
        mtoolbarlayout_p.h \
        minputcontextglibdbusconnection.h \
        mimremotewindow.h \
        mimxerrortrap.h \
        mimxextension.h \
        mimsettings.h \
        mimhwkeyboardtracker.h \
        mimgraphicsview_p.h \
        mimwidget_p.h \
        ../passthroughserver/mpassthruwindow.h \
        mimrotationanimation.h \

SOURCES += mimpluginmanager.cpp \
        mimpluginmanageradaptor.cpp \
        mindicatorserviceclient.cpp \
        mabstractinputmethod.cpp \
        mabstractinputmethodhost.cpp \
        minputmethodhost.cpp \
        minputcontextconnection.cpp \
        mimapplication.cpp \
        mtoolbaritem.cpp \
        mtoolbardata.cpp \
        mkeyoverride.cpp \
        mkeyoverridedata.cpp \
        mattributeextensionmanager.cpp \
        mattributeextensionid.cpp \
        mattributeextension.cpp \
        mtoolbarlayout.cpp \
        minputcontextglibdbusconnection.cpp \
        mimremotewindow.cpp \
        mimxerrortrap.cpp \
        mimxextension.cpp \
        mimextensionevent.cpp \
        mimsettings.cpp \
        mimhwkeyboardtracker.cpp \
        mimgraphicsview.cpp \
        mimwidget.cpp \
        ../passthroughserver/mpassthruwindow.cpp \
        mimrotationanimation.cpp \
        mimplugindescription.cpp \

CONFIG += qdbus link_pkgconfig
QT = core gui xml

PKGCONFIG += dbus-glib-1 dbus-1 gconf-2.0

contains(CONFIG, nomeegotouch) {
} else {
    CONFIG  += meegotouchcore
    DEFINES += HAVE_MEEGOTOUCH
}

# coverage flags are off per default, but can be turned on via qmake COV_OPTION=on
for(OPTION,$$list($$lower($$COV_OPTION))){
    isEqual(OPTION, on){
        QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs -fno-elide-constructors
        LIBS += -lgcov
    }
}

QMAKE_CLEAN += *.gcno *.gcda

target.path += $$M_INSTALL_LIBS

OBJECTS_DIR = .obj
MOC_DIR = .moc

headers.path += $$M_INSTALL_HEADERS/meegoimframework
headers.files += $$HEADERSINSTALL

install_pkgconfig.path = $$[QT_INSTALL_LIBS]/pkgconfig
install_pkgconfig.files = MeegoImFramework.pc

install_prf.path = $$[QT_INSTALL_DATA]/mkspecs/features
install_prf.files = meegoimframework.prf

INSTALLS += target \
    headers \
    install_prf \
    install_pkgconfig \

QMAKE_EXTRA_TARGETS += check-xml
check-xml.target = check-xml
check-xml.depends += lib$${TARGET}.so.$${VERSION}

QMAKE_EXTRA_TARGETS += check
check.target = check
check.depends += lib$${TARGET}.so.$${VERSION}

contains(DEFINES, M_IM_DISABLE_TRANSLUCENCY) {
    M_IM_FRAMEWORK_FEATURE += M_IM_DISABLE_TRANSLUCENCY
} else {
    M_IM_FRAMEWORK_FEATURE -= M_IM_DISABLE_TRANSLUCENCY
}

LIBS += -lXcomposite -lXdamage -lX11 -lXfixes

PRF_FILE = meegoimframework.prf.in

prffilegenerator.output = meegoimframework.prf
prffilegenerator.input = PRF_FILE
prffilegenerator.commands += sed -e \"s:M_IM_FRAMEWORK_FEATURE:$$M_IM_FRAMEWORK_FEATURE:g\" ${QMAKE_FILE_NAME} > ${QMAKE_FILE_OUT}
prffilegenerator.CONFIG = target_predeps no_link
prffilegenerator.depends += Makefile
QMAKE_EXTRA_COMPILERS += prffilegenerator

QMAKE_EXTRA_TARGETS += mdbusglibicconnectionserviceglue.h
mdbusglibicconnectionserviceglue.h.commands = \
    dbus-binding-tool --prefix=m_dbus_glib_ic_connection --mode=glib-server \
        --output=mdbusglibicconnectionserviceglue.h minputmethodserver1interface.xml
mdbusglibicconnectionserviceglue.h.depends = minputmethodserver1interface.xml
