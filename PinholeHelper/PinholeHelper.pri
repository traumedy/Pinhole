# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

# This is a reminder that you are using a generated .pro file.
# Remove it when you are finished editing this file.
message("You are running qmake on a generated .pro file. This may not work!")


HEADERS += ../common/Version.h \
    ../common/PinholeCommon.h \
    ../common/Utilities.h \
    ./TrayManager.h \
    ../common/HostClient.h \
    ../common/DummyWindow.h \
    ./IdWindow.h \
    ./LogDialog.h \
    ./Utilities_X11.h
SOURCES += ../common/DummyWindow.cpp \
    ../common/HostClient.cpp \
    ../common/Utilities.cpp \
    ../common/Utilities_Mac.cpp \
    ../common/Utilities_Win.cpp \
    ../common/Utilities_Linux.cpp \
    ./Utilities_X11.cpp \
    ./IdWindow.cpp \
    ./LogDialog.cpp \
    ./main.cpp \
    ./TrayManager.cpp
RESOURCES += PinholeHelper.qrc
