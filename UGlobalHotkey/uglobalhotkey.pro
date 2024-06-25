CONFIG(debug, debug|release) {
    ConfigurationName = Debug
}
CONFIG(release, debug|release) {
    ConfigurationName = Release
}

QT = core gui
unix {
    QT += gui-private
}
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UGlobalHotkey
TEMPLATE = lib
CONFIG += c++11 staticlib
DESTDIR = ../$${ConfigurationName}
MOC_DIR += ./GeneratedFiles/$${ConfigurationName}
OBJECTS_DIR += $${ConfigurationName}

# Switch ABI to export (vs import, which is default)
DEFINES += UGLOBALHOTKEY_LIBRARY

include(uglobalhotkey-headers.pri)
include(uglobalhotkey-sources.pri)
include(uglobalhotkey-libs.pri)

linux {
DEFINES += _GLIBCXX_USE_CXX11_ABI=0
}
