#!/bin/bash

QT_VER=$(cat ../QtVer.txt)
QT_INSTALLER_VER=3.2
eval QT_PATH="~/Qt/$QT_VER/gcc_64"

echo Qt version: ${QT_VER}

./updatexml.py

VERSION=`cat Version`

rm -rf packages/com.obscura.root/data
rm -rf packages/com.obscura.root.console/data
rm -rf packages/com.obscura.root.server/data
mkdir packages/com.obscura.root/data
mkdir packages/com.obscura.root/data/platforms
mkdir packages/com.obscura.root/data/imageformats
mkdir packages/com.obscura.root.console/data
mkdir packages/com.obscura.root.console/data/doc
mkdir packages/com.obscura.root.server/data
cp ../run/qt.conf packages/com.obscura.root/data
declare -a LIBLIST=("libicudata.so.56" "libicui18n.so.56" "libicuuc.so.56" "libQt5Core.so.5" "libQt5DBus.so.5" "libQt5Gui.so.5" "libQt5Network.so.5" "libQt5Widgets.so.5" "libQt5XcbQpa.so.5")
for LIB in "${LIBLIST[@]}"
do
	cp -L $QT_PATH/lib/$LIB packages/com.obscura.root/data
done
cp $QT_PATH/plugins/platforms/*.so packages/com.obscura.root/data/platforms
cp $QT_PATH/plugins/imageformats/*.so packages/com.obscura.root/data/imageformats/
cp ../run/launch.sh packages/com.obscura.root/data
cp -r ../doc/* packages/com.obscura.root.console/data/doc
cp ../Release/PinholeConsole packages/com.obscura.root.console/data
cp ../Release/PinholeClient packages/com.obscura.root.console/data
cp ../Release/PinholeServer packages/com.obscura.root.server/data
cp ../Release/PinholeHelper packages/com.obscura.root.server/data
cp ../run/pinhole.service packages/com.obscura.root.server/data
cp ../run/pinhole.conf packages/com.obscura.root.server/data
$QT_PATH/../../Tools/QtInstallerFramework/$QT_INSTALLER_VER/bin/binarycreator --offline-only -c config/config.xml -p packages PinholeInstaller_${VERSION}_linux.run


