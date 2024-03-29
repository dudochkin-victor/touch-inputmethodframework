include(../common_top.pri)

INCLUDEPATH += ../../passthroughserver \
               ../stubs \

# Input
HEADERS += \
    ut_mimapplication.h \
    ../stubs/mimsettings_stub.h \
    ../stubs/fakegconf.h \
    ../stubs/minputcontextconnection_stub.h \

SOURCES += \
    ut_mimapplication.cpp \
    ../stubs/fakegconf.cpp \
    ../stubs/minputcontextconnection_stub.cpp \

isEqual(code_coverage_option, off) {
    HEADERS += \
        $$PASSTHROUGH_DIR/mpassthruwindow.h
    SOURCES += \
        $$PASSTHROUGH_DIR/mpassthruwindow.cpp
} else {
    LIBS += \
        $$PASSTHROUGH_DIR/moc_mpassthruwindow.o \
        $$PASSTHROUGH_DIR/mpassthruwindow.o
}


CONFIG += plugin qdbus

LIBS += \
    ../../src/libmeegoimframework.so.0 -lXfixes

include(../common_check.pri)
