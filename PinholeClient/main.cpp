
#include "PinholeClient.h"
#include "../common/HostClient.h"
#include "../common/PinholeCommon.h"
#include "../common/Version.h"

#include <QtCore/QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>


int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationVersion(PINHOLE_VERSION);
	QCoreApplication::setApplicationName(PINHOLE_CLIENTAPPNAME);
	QCoreApplication::setOrganizationName(PINHOLE_ORG_NAME);
	QCoreApplication::setOrganizationDomain(PINHOLE_ORG_DOMAIN);

	QCommandLineParser parser;
	parser.setApplicationDescription("Pinhole Console Client");
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("address", QObject::tr("Server IP address or host name."));
	parser.addPositionalArgument("verb", QObject::tr("Type of command te execute, 'command' 'getprop' 'setprop' or 'log'.  Run with -l or -c to list commands and properties."));
	parser.addPositionalArgument("name", QObject::tr("Name of command to execute or property to get/set or the message to log."));
	parser.addPositionalArgument("value", QObject::tr("Argument for command or value for for setprop.  Defaults to empty string, sometimes optional."));
	QCommandLineOption commandsOption({ "c", "commands" }, QObject::tr("List commands that can be executed using the 'command' verb."));
	parser.addOption(commandsOption);
	QCommandLineOption listOption({ "l", "list" }, QObject::tr("List properties that can be set or retrieved using the 'getprop' or 'setprop' verbs."));
	parser.addOption(listOption);
	QCommandLineOption silentOption({ "s", "silent" }, 
		QObject::tr("Only output the property value, 'success' or 'failure'.  Useful for redirecting the output of the application for scripting."));
	parser.addOption(silentOption);
	QCommandLineOption showIdOption("showid", QObject::tr("Display the server ID"));
	parser.addOption(showIdOption);
	QCommandLineOption itemOption({ "i", "item", }, QObject::tr("The item to get or set the property of."), QObject::tr("item-name"));
	parser.addOption(itemOption);
	QCommandLineOption passwordOption({ "p", "password" }, QObject::tr("Password for Pinhole server."), QObject::tr("password"));
	//QCommandLineOption passwordOption({ "p", "password" }, QObject::tr("Password for Pinhole server."));
	parser.addOption(passwordOption);
	QCommandLineOption portOption({ "r", "port" }, QObject::tr("Port to connect on if using a proxy"), QObject::tr("PortNumber"));
	parser.addOption(portOption);

	// Process command line
	parser.process(app);

	PinholeClient pinholeClient(nullptr);

	const QStringList args = parser.positionalArguments();

	if (parser.isSet(commandsOption))
	{
		pinholeClient.listCommands();
		return 0;
	}

	if (parser.isSet(listOption))
	{
		pinholeClient.listProperties();
		return 0;
	}

	if (args.length() < 3)
	{
		qWarning() << "Missing argument (use -h for help)";
		return 1;
	}

	// Set the silent flag
	pinholeClient.setSilent(parser.isSet(silentOption));
	// Set the show server id flag
	pinholeClient.setShowId(parser.isSet(showIdOption));

	QString address = args[0];
	QString verb = args[1];
	QString name = args[2];
	QString value;
	if (args.length() > 3)
		value = args[3];
	QString password = parser.value(passwordOption);
	QString item = parser.value(itemOption);
	int port = HOST_TCPPORT;
	QString portString = parser.value(portOption);
	if (!portString.isEmpty())
		port = portString.toInt();

	HostClient::addAddressPassword(address, password);

	if ("command" == verb)
	{
		if (!pinholeClient.executeCommand(address, port, name, value))
		{
			return 1;
		}
	}
	else if ("getprop" == verb)
	{
		if (!pinholeClient.getProperty(address, port, item, name))
		{
			return 1;
		}
	}
	else if ("setprop" == verb)
	{
		if (!pinholeClient.setProperty(address, port, item, name, value))
		{
			return 1;
		}
	}
	else if ("log" == verb)
	{
		if (!pinholeClient.logMessage(address, port, name))
		{
			return 1;
		}
	}
	else
	{
		qWarning() << "Unknown verb " << verb << " (use -h for help)";
		return 1;
	}

	return 0;
	//return app.exec();
}
