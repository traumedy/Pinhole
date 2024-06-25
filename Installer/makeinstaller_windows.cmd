@echo off
set /p QTVER=<..\QtVer.txt
set QT_ROOT=\qt\%QTVER%\msvc2017_64
set QT_INSTALLER_VER=3.2

echo Qt version: %QTVER%

python updatexml.py

set /P VERSION=<Version

rd /q /s packages\com.obscura.root\data
rd /q /s packages\com.obscura.root.console\data
rd /q /s packages\com.obscura.root.server\data
md packages\com.obscura.root\data\platforms
md packages\com.obscura.root\data\imageformats
md packages\com.obscura.root.console\data\doc
md packages\com.obscura.root.server\data
xcopy /Y ..\run\qt.conf packages\com.obscura.root\data > NUL
xcopy /Y ..\x64\Release\vcredist_x64_2017.exe packages\com.obscura.root\data > NUL
xcopy /Y ..\x64\Release\RunDetached.exe packages\com.obscura.root\data > NUL
xcopy /Y ..\x64\Release\*.dll packages\com.obscura.root\data > NUL
xcopy /Y "%VCPKG_ROOT%\installed\x64-windows\bin\libcrypto-1_1-x64.dll" packages\com.obscura.root\data > NUL
xcopy /Y "%VCPKG_ROOT%\installed\x64-windows\bin\libssl-1_1-x64.dll" packages\com.obscura.root\data > NUL
xcopy /Y %QT_ROOT%\bin\Qt5Core.dll packages\com.obscura.root\data > NUL
xcopy /Y %QT_ROOT%\bin\Qt5Gui.dll packages\com.obscura.root\data > NUL
xcopy /Y %QT_ROOT%\bin\Qt5Network.dll packages\com.obscura.root\data > NUL
xcopy /Y %QT_ROOT%\bin\Qt5Widgets.dll packages\com.obscura.root\data > NUL
xcopy /Y %QT_ROOT%\plugins\platforms\qwindows.dll packages\com.obscura.root\data\platforms > NUL
xcopy /Y %QT_ROOT%\plugins\platforms\qdirect2d.dll packages\com.obscura.root\data\platforms > NUL
xcopy /Y %QT_ROOT%\plugins\imageformats\*.dll packages\com.obscura.root\data\imageformats > NUL
del packages\com.obscura.root\data\imageformats\*d.dll
xcopy /Y /s ..\doc\* packages\com.obscura.root.console\data\doc > NUL
xcopy /Y ..\x64\Release\PinholeConsole.exe packages\com.obscura.root.console\data > NUL
xcopy /Y ..\x64\Release\PinholeClient.exe packages\com.obscura.root.console\data > NUL
xcopy /Y ..\x64\Release\PinholeServer.exe packages\com.obscura.root.server\data > NUL
xcopy /Y ..\x64\Release\PinholeHelper.exe packages\com.obscura.root.server\data > NUL
xcopy /y ..\x64\Release\PinholeServer.pdb packages\com.obscura.root.server\data > NUL

%QT_ROOT%\..\..\tools\QtInstallerFramework\%QT_INSTALLER_VER%\bin\binarycreator.exe --offline-only -c config\config.xml -p packages PinholeInstaller_%Version%_win64.exe
