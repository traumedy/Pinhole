CONFIG(debug, debug|release) {
    ConfigurationName = Debug
}
CONFIG(release, debug|release) {
    ConfigurationName = Release
}

TEMPLATE = app
TARGET = PinholeServer
DESTDIR = ../$${ConfigurationName}
QT += core network concurrent
QT -= gui
CONFIG += console
DEFINES += CONSOLE QT_NETWORK_LIB QT_CONCURRENT_LIB 
INCLUDEPATH += ./GeneratedFiles/$${ConfigurationName} \
    .
LIBS += $${DESTDIR}/libqmsgpack.a $${DESTDIR}/libSigar.a -lcrypto -ldl 
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$${ConfigurationName}
OBJECTS_DIR += $${ConfigurationName}
UI_DIR += ./GeneratedFiles/$${ConfigurationName}
RCC_DIR += ./GeneratedFiles/$${ConfigurationName}
include(PinholeServer.pri)

macx {
INCLUDEPATH += /usr/local/opt/openssl/include
LIBS += -L"/usr/local/opt/openssl/lib" -framework CoreServices
QMAKE_INFO_PLIST = ../run/Info.plist.app.nodock
}

linux {
DEFINES += _GLIBCXX_USE_CXX11_ABI=0
}
