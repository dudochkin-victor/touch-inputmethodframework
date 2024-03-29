include(../common_top.pri)

LIBS += ../../src/libmeegoimframework.so -lXfixes

# Input
HEADERS += \
    ut_passthroughserver.h \
    $$PASSTHROUGH_DIR/mpassthruwindow.h \

SOURCES += \
    ut_passthroughserver.cpp \


CONFIG += meegotouch
QT += testlib

isEqual(code_coverage_option, off) {
    SOURCES += \
        $$PASSTHROUGH_DIR/mpassthruwindow.cpp
}else {
    QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs -fno-elide-constructors
    # requires to run make (for $$PASSTHROUGH_DIR) before make check:
    LIBS += $$PASSTHROUGH_DIR/mpassthruwindow.o
}


include(../common_check.pri)
