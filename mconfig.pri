# Linker optimization for release build
QMAKE_LFLAGS_RELEASE+=-Wl,--as-needed
# Compiler warnings are error if the build type is debug
QMAKE_CXXFLAGS_DEBUG+=-Werror

unix {
     M_PREFIX = /usr
     M_INSTALL_BIN = $$M_PREFIX/bin
     M_INSTALL_LIBS = $$M_PREFIX/lib
     M_INSTALL_HEADERS = $$M_PREFIX/include
}
mac {
     # Do mac stuff here
    M_BUILD_FEATURES = debug
    M_PROFILE_PARTS =
    INCLUDEPATH += include/
    M_INSTALL_BIN = $$M_PREFIX/bin/
    M_INSTALL_LIBS = $$M_PREFIX/lib/
    M_INSTALL_HEADERS = $$M_PREFIX/include
}
win32 {
     # Do win32 stuff here
     M_PREFIX = /usr
     M_INSTALL_BIN = $$M_PREFIX/bin/
     M_INSTALL_LIBS = $$M_PREFIX/lib/
     M_INSTALL_HEADERS = $$M_PREFIX/include
}
