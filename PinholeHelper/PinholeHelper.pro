CONFIG(debug, debug|release) {
    ConfigurationName = Debug
}
CONFIG(release, debug|release) {
    ConfigurationName = Release
}

TEMPLATE = app
TARGET = PinholeHelper
DESTDIR = ../$${ConfigurationName}
QT += core network gui widgets
CONFIG += 
DEFINES += QT_NETWORK_LIB QT_WIDGETS_LIB
INCLUDEPATH += ./GeneratedFiles/$${ConfigurationName} \
    .
LIBS += $${DESTDIR}/libqmsgpack.a $${DESTDIR}/libUGlobalHotkey.a -lcrypto -ldl
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$${ConfigurationName}
OBJECTS_DIR += $${ConfigurationName}
UI_DIR += ./GeneratedFiles/$${ConfigurationName}
RCC_DIR += ./GeneratedFiles/$${ConfigurationName}
include(PinholeHelper.pri)

linux {
LIBS += -lxcb -lxcb-keysyms -lX11
DEFINES += _GLIBCXX_USE_CXX11_ABI=0
}

macx {
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L"/usr/local/opt/openssl/lib" -framework Carbon
QMAKE_INFO_PLIST = ../run/Info.plist.app.nodock
}
