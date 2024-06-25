# Pinhole  

## Author: Josh Buchbinder  

Cross Platform Application Monitoring system built on Qt5.

## About Pinhole  

Pinhole is a system for launching applications and ensuring
those applications remain running.  It is a client/server
system allowing remote configuration and control over the 
network using a graphical console (PinholeConsole) which 
has a large help system built in using tooltips, WhatsThis
popups and links to external HTML.  

There is also a minimal console client (PinholeClient) 
that can be used for scripting, and a proxy server that can
be used for managing groups of remote servers (PinholeBackend).  

Pinhole has many additional features such as scheduling, 
system resource monitoring, remote alert reporting, system
diagnostics, network interfaces and more.  

## Abstract  

This repository contains a Visual Studio 2017 solution for 
building x64 Windows target.  It also contains a Qt 
`.pro` file to build for Linux and Mac OS targets.  
There are instructions below for updating the `.pri` files
if additional source files are added or removed from the 
Visual Studio project files.  The `.pro` file was hand 
generated and should be edited with care.  

- In this documentation where you see a Qt version number 
substitute the appropriate version number that is being
used.  Make sure the file QtVer.txt matches the installed
version of Qt.  
- In this documentation where you see -j9 substitute the
number of CPU cores + 1 to build faster.  

## Windows Build  

Install Qt Visual Studio Tools 2017:  
https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools-19123

Install Qt5 OpenSorce:  
http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe
- MSVC 2017 64-bit  
- (Optional) Sources  
- (Optional) Qt Debug Information Files (Debuging symbols)  
- Tools -> Qt Installer Framework 3.2  

Create a "Qt Version" in Visual Studio named `Qt5_x64` pointing to 
the installed `msvc2017_64\bin` Qt directory.
(Qt VS Tools->Qt Options->Qt Versions tab)

Install vcpkg and install openssl-windows:x64-windows.

Use sln solution file to build all projects in Visual Studio 2017 IDE.  

### To run Pinhole in place from the build directory  

From `%VCPKG_ROOT%\Installed\x64-windows\bin` copy 
`libcrypto-1_1-x64.dll` and `libssl-1_1-x64.dll` to 
`x64\release`, vcpkg does not always copy them automatically.
Pinhole will run without these DLLs and seem to operate but 
networking will fail to connect.  

You will also need to copy the Qt DLLs into the `x64\Release` directory:
`Qt5Core.dll`, `Qt5Gui.dll`, `Qt5Widgets.dll` and `Qt5Network.dll`.  

You will also need to create a subdirectory named `platform` and copy
the `Qt\...\plugins\platform\` directory.  

For Running from the `x64\debug` directory use the appropriate 
debug versions of the DLLs from 
`%VCPKG_ROOT%\Installed\x64-windows\debug\bin` and `Qt5Cored.dll`,
`Qt5Guid.dll`, `Qt5Widgetsd.dll` and `Qt5Networkd.dll`.  

### For building installer  

## !! CRITICAL STEP  

Download and rename the `Visual C 2017 x64 Runtime Installer` to 
`vcredist_x64_2017.exe` and copy to `x64\Release` build directory.  
This is installed by the installer executable.  

The installer requires Python 3 to update the package xml.  

!! NOTE:  
The installer script assumes Qt is installed in `\Qt` on the same drive
as the build.  If not, modify the installer build script and modify the
`QTROOT` variable at the top of the script.

To build the installer enter the `Installer` directory and run:  
```
makeinstaller_windows.cmd
```

## To export the pri files needed to build on Mac/Linux if files are added/removed from the Windows project  

From 'Qt VS Tools' menu select 'Create Basic .pro File...', uncheck 
'Open Created Files', click OK.  When it asks if you want to overwrite 
.pro files, say No.  When it asks to save .pri files accept their 
default location and say Yes to overwrite.

or

To export the .pri from a single project, right click on the project 
and select "Export Project to .pri File..."

## Ubuntu build  

Install Qt5 OpenSorce:  
http://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run
- Desktop gcc 64-bit   
- (Optional) Sources  
- (Optional) Qt Debug Information Files (Debuging symbols)  
- Tools -> Qt Installer Framework 3.2  

Install necessary dependencies:  
```
sudo apt-get install build-essential libfontconfig1 mesa-common-dev libglu1-mesa-dev libssl1.0-dev libxcb-keysyms1-dev -y
```

From project root:
```
export PATH=$PATH:~/Qt/5.12.1/gcc_64/bin
qmake 
make -j9
```
Files will be output to Release/  

The installer requires python 3 to update the package xml.  

To build the installer enter the `Installer` directory and run:  
```
./makeinstaller_linux.sh
```
(The installer script assumes Qt is installed in `~/Qt`, modify script if not.)

## Mac build  

Install XCode and XCode command line tools from app store.  
Install homebrew and dependencies:
```
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
brew install openssl
```
Install Qt5 OpenSorce:  
http://download.qt.io/official_releases/online_installers/qt-unified-mac-x64-online.dmg  
- macOS  
- Sources  
- Tools -> Qt Installer Framework 3.2  

Rebuild Qt with openssll and replace QtNetwork:  
```
cd Qt/5.12.1
mv clang_64/lib/QtNetwork.framework QtNetwork.famework.old
cd Src
./configure -opensource -nomake examples -nomake tests -openssl-linked -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtmacextras -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtsvg -skip qttools -skip qttranslations -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin -skip qtwebsockets -skip qtwebview -skip qtwinextras -skip qtx11extras -skip qtxmlpatterns
# (When prompted accept license agreement)
make -j9
cd qtbase/lib
cp -R QtNetwork.framework ../../../clang_64/lib
```

From project root:
```
export PATH=$PATH:~/Qt/5.12.1/clang_64/bin
qmake 
make -j9
```
Files will be output to `Release/`

The installer requires python to update the package xml.  

To build the installer enter the `Installer` directory and run:  
```
./makeinstaller_mac.sh
```
(The installer script assumes Qt is installed in `~/Qt`, modify script if not.)
