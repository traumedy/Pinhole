#include "PinholeConsole.h"
#include "../common/PinholeCommon.h"
#include "../common/Utilities.h"
#include "../common/Version.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	// For high DPI displays
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setApplicationVersion(PINHOLE_VERSION);
	QApplication::setApplicationName(PINHOLE_CONSOLEAPPNAME);
	QApplication::setOrganizationName(PINHOLE_ORG_NAME);
	QApplication::setOrganizationDomain(PINHOLE_ORG_DOMAIN);

	QApplication application(argc, argv);

	// Register Msgpack types
	RegisterMsgpackTypes();

	PinholeConsole w;
	w.show();
	return application.exec();
}
