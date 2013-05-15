TEMPLATE = lib
TARGET = ../plugins/$$qtLibraryTarget(dummyplugin)
DEPENDPATH += .
INCLUDEPATH += . ../../src
LIBS += -L../../src -lmeegoimframework

OBJECTS_DIR = .obj
MOC_DIR = .moc

CONFIG += debug plugin 

HEADERS += dummyplugin.h
SOURCES += dummyplugin.cpp

target.path += /usr/lib/meego-im-framework-tests/plugins

INSTALLS += target

QMAKE_CLEAN += ../plugins/libdummyplugin.so

QMAKE_EXTRA_TARGETS += check
check.target = check
check.command = $$system(true)

QMAKE_EXTRA_TARGETS += check-xml
check-xml.target = check-xml
check-xml.command = $$system(true)

QMAKE_EXTRA_TARGETS += memcheck
memcheck.target = memcheck
memcheck.command = $$system(true)
