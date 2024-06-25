CONFIG(debug, debug|release) {
    ConfigurationName = Debug
}
CONFIG(release, debug|release) {
    ConfigurationName = Release
}

QT -= gui
TEMPLATE = lib
TARGET = Sigar
DESTDIR = ../$${ConfigurationName}
CONFIG += staticlib
DEFINES += LIBSIGAR_LIB BUILD_STATIC
INCLUDEPATH += ./GeneratedFiles/$${ConfigurationName} \
    .
LIBS += 
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$${ConfigurationName}
OBJECTS_DIR += $${ConfigurationName}
UI_DIR += ./GeneratedFiles/$${ConfigurationName}
RCC_DIR += ./GeneratedFiles/$${ConfigurationName}
include(libSigar.pri)
