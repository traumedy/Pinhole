
#include "TrayManager.h"
#include "../common/DummyWindow.h"
#include "../common/Utilities.h"
#include "../common/Version.h"
#include "../common/PinholeCommon.h"

#include "../UGlobalHotkey/uglobalhotkeys.h"

#include <QtWidgets/QApplication>

#include <signal.h>


void signal_handler(int param)
{
	Q_UNUSED(param);

	QCoreApplication::exit(0);
}


int main(int argc, char *argv[])
{
	// Catch ^C and termination requests
	signal(SIGINT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGTERM, signal_handler);

	// Make sure we get actual resolution screenshots for high DPI displays 
	QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
	QApplication::setApplicationVersion(PINHOLE_VERSION);
	QApplication::setApplicationName(PINHOLE_HELPERAPPNAME);
	QApplication::setOrganizationName(PINHOLE_ORG_NAME);
	QApplication::setOrganizationDomain(PINHOLE_ORG_DOMAIN);

	QApplication application(argc, argv);

	// Prevent program from running more than once
	if (IsThisProgramAlreadyRunning())
	{
		return 0;
	}

	// Register Msgpack types
	RegisterMsgpackTypes();

	// Prevent application from closing when windows close
	application.setQuitOnLastWindowClosed(false);

	DummyWindow dummyWindow;
	TrayManager trayManager;

	UGlobalHotkeys hotkeyManager;
	hotkeyManager.registerHotkey("Ctrl+Shift+F2");
	//hotkeyManager.registerHotkey("Ctrl+Shift+L");
	QObject::connect(&hotkeyManager, &UGlobalHotkeys::activated,
		[&trayManager](size_t /*id*/) { trayManager.showLogDialog(); });

	return application.exec();
}
