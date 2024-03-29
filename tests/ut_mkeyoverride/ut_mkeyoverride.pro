include(../common_top.pri)

INCLUDEPATH += ../stubs \

# Input
HEADERS += \
    ut_mkeyoverride.h \
    ../stubs/mimsettings_stub.h \
    ../stubs/fakegconf.h \

SOURCES += \
    ut_mkeyoverride.cpp \
    ../stubs/fakegconf.cpp \

isEqual(code_coverage_option, off){
HEADERS += \
    $$SRC_DIR/mkeyoverride.h \
    $$SRC_DIR/minputmethodnamespace.h \

SOURCES += \
    $$SRC_DIR/mkeyoverride.cpp \
}

CONFIG += plugin meegotouch qdbus

LIBS += \
    ../../src/libmeegoimframework.so.0 \

include(../common_check.pri)
