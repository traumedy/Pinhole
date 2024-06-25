CONFIG(debug, debug|release) {
    ConfigurationName = Debug
}
CONFIG(release, debug|release) {
    ConfigurationName = Release
}

TEMPLATE = app
TARGET = PinholeConsole
DESTDIR = ../$${ConfigurationName}
QT += core network gui widgets
CONFIG +=
DEFINES += QT_NETWORK_LIB QT_WIDGETS_LIB
INCLUDEPATH += ./GeneratedFiles/$${ConfigurationName} \
    .
LIBS += $${DESTDIR}/libqmsgpack.a -lcrypto -ldl
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$${ConfigurationName}
OBJECTS_DIR += $${ConfigurationName}
UI_DIR += ./GeneratedFiles/$${ConfigurationName}
RCC_DIR += ./GeneratedFiles/$${ConfigurationName}
include(PinholeConsole.pri)

macx {
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L"/usr/local/opt/openssl/lib"
ICON = PinholeConsole.icns
}

linux {
DEFINES += _GLIBCXX_USE_CXX11_ABI=0
}
