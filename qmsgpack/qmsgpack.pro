CONFIG(debug, debug|release) {
    ConfigurationName = Debug
}
CONFIG(release, debug|release) {
    ConfigurationName = Release
}

QT -= gui
TEMPLATE = lib
TARGET = qmsgpack
DESTDIR = ../$${ConfigurationName}
CONFIG += staticlib
DEFINES += MSGPACK_MAKE_LIB
INCLUDEPATH += ./GeneratedFiles/$${ConfigurationName} \
    .
LIBS += 
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$${ConfigurationName}
OBJECTS_DIR += $${ConfigurationName}
UI_DIR += ./GeneratedFiles/$${ConfigurationName}
RCC_DIR += ./GeneratedFiles/$${ConfigurationName}
include(qmsgpack.pri)

linux {
DEFINES += _GLIBCXX_USE_CXX11_ABI=0
}
