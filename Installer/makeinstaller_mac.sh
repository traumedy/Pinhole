#!/bin/bash

QT_VER=$(cat ../QtVer.txt)
QT_INSTALLER_VER=3.2
eval QT_PATH="~/Qt/$QT_VER/clang_64"

echo Qt version: ${QT_VER}

./updatexml.py

VERSION=`cat Version`

rm -rf packages/com.obscura.root/data
rm -rf packages/com.obscura.root.console/data
rm -rf packages/com.obscura.root.server/data
mkdir packages/com.obscura.root/data
#mkdir packages/com.obscura.root/data/platforms
#mkdir packages/com.obscura.root/data/imageformats
mkdir packages/com.obscura.root.console/data
mkdir packages/com.obscura.root.console/data/doc
mkdir packages/com.obscura.root.server/data
#declare -a LIBLIST=("libicudata.so.56" "libicui18n.so.56" "libicuuc.so.56" "libQt5Core.so.5" "libQt5DBus.so.5" "libQt5Gui.so.5" "libQt5Network.so.5" "libQt5Widgets.so.5" "libQt5XcbQpa.so.5")
#for LIB in "${LIBLIST[@]}"
#do
#	cp -L $QT_PATH/lib/$LIB packages/com.obscura.root/data
#done
#cp $QT_PATH/plugins/platforms/*.dylib packages/com.obscura.root/data/platforms
#rm packages/com.obscura.root/data/platforms/*_debug.dylib
#cp $QT_PATH/plugins/imageformats/*.dylib packages/com.obscura.root/data/imageformats/
#rm packages/com.obscura.root/data/imageformats/*_debug.dylib
cp ../run/Pinhole.Server.plist_autostart packages/com.obscura.root.server/data
cp -r ../doc/* packages/com.obscura.root.console/data/doc
cp -r ../Release/PinholeConsole.app packages/com.obscura.root.console/data
$QT_PATH/bin/macdeployqt packages/com.obscura.root.console/data/PinholeConsole.app
sudo codesign --force --deep --sign - packages/com.obscura.root.console/data/PinholeConsole.app
cp -r ../Release/PinholeClient.app packages/com.obscura.root.console/data
$QT_PATH/bin/macdeployqt packages/com.obscura.root.console/data/PinholeClient.app
sudo codesign --force --deep --sign - packages/com.obscura.root.console/data/PinholeClient.app
cp -r ../Release/PinholeServer.app packages/com.obscura.root.server/data
$QT_PATH/bin/macdeployqt packages/com.obscura.root.server/data/PinholeServer.app
sudo codesign --force --deep --sign - packages/com.obscura.root.server/data/PinholeServer.app
cp -r ../Release/PinholeHelper.app packages/com.obscura.root.server/data
$QT_PATH/bin/macdeployqt packages/com.obscura.root.server/data/PinholeHelper.app
sudo codesign --force --deep --sign - packages/com.obscura.root.server/data/PinholeHelper.app
$QT_PATH/../../Tools/QtInstallerFramework/$QT_INSTALLER_VER/bin/binarycreator --offline-only -c config/config.xml -p packages PinholeInstaller_${VERSION}_macos
$QT_PATH/bin/macdeployqt PinholeInstaller_${VERSION}_macos.app -dmg -always-overwrite
#hdiutil create -volname PinholeConsole -srcfolder packages/com.obscura.root.console/data -ov -format UDZO PinholeConsole_${VERSION}.dmg
#hdiutil create -volname PinholeServer -srcfolder packages/com.obscura.root.server/data -ov -format UDZO PinholeServer_${VERSION}.dmg
