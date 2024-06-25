/* main.cpp - PinholeBackend - Backend server for Pinhole Suite */
/* Receives connections from PinholeServer and acts as proxy */

#include "Settings.h"
#include "ProxyServer.h"
#include "UdpInterface.h"
#include "WebInterface.h"
#include "../common/Version.h"
#include "../common/PinholeCommon.h"

#include <QtCore/QCoreApplication>
#include <QStandardPaths>
#include <QDir>

int main(int argc, char *argv[])
{

	QCoreApplication::setApplicationVersion(PINHOLE_VERSION);
	QCoreApplication::setApplicationName(PINHOLE_BACKENDAPPNAME);
	QCoreApplication::setOrganizationName(PINHOLE_ORG_NAME);
	QCoreApplication::setOrganizationDomain(PINHOLE_ORG_DOMAIN);

	QCoreApplication application(argc, argv);

	Settings settings;
	QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
	settings.setDataDir(dataDir);

	if (dataDir.isEmpty())
	{
		qWarning() << "Data dir empty!";
	}

	if (!QDir(dataDir).exists())
	{
		//qWarning() << "Creating data directory: " << settings.dataDir;
		if (!QDir().mkpath(dataDir))
		{
			qWarning() << "Failed to create data directory";
		}
	}
	else
	{
		qInfo() << "Using data directory: " << dataDir;
	}

	ProxyServer proxyServer(&settings, &application);
	UdpInterface udpInterface(&settings, &proxyServer, &application);
	WebInterface webInterface(&settings, &proxyServer, &application);

	qInfo() << PINHOLE_BACKENDAPPNAME " startup " << PINHOLE_VERSION;

	return application.exec();
}

