#include "CommandInterface.h"
#include "Settings.h"
#include "AppManager.h"
#include "AlertManager.h"
#include "GroupManager.h"
#include "GlobalManager.h"
#include "ScheduleManager.h"
#include "Logger.h"
#include "Values.h"
#include "WinUtil.h"
#include "../common/PinholeCommon.h"
#include "../common/Utilities.h"
#include "../qmsgpack/msgpack.h"

#include <QtEndian>
#include <QHostInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QSettings>
#include <QMetaObject>
#include <QCryptographicHash>

#if defined(Q_OS_UNIX)
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#endif

#define CRYPTOMETHOD			QCryptographicHash::Sha3_512

CommandInterface::CommandInterface(Settings* settings, AlertManager* alertManager,
	AppManager* appManager, GroupManager* groupManager, GlobalManager* globalManager,
	ScheduleManager* scheduleManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_alertManager(alertManager),
	m_appManager(appManager), m_groupManager(groupManager)
	, m_globalManager(globalManager), m_scheduleManager(scheduleManager)
{
	readServerSettings();

	// Create the shared password for local clients
	createSharedPassword();

	connect(m_alertManager, &AlertManager::valueChanged,
		this, &CommandInterface::valueChanged);
	connect(m_appManager, &AppManager::valueChanged,
		this, &CommandInterface::valueChanged);
	connect(m_groupManager, &GroupManager::valueChanged,
		this, &CommandInterface::valueChanged);
	connect(m_globalManager, &GlobalManager::valueChanged,
		this, &CommandInterface::valueChanged);
	connect(m_scheduleManager, &ScheduleManager::valueChanged,
		this, &CommandInterface::valueChanged);

}


CommandInterface::~CommandInterface()
{
	// Make sure the logger doesn't attempt to call back into this object
	Logger::setCommandInterface(nullptr);
}


void CommandInterface::addClient(const QString& clientId)
{
	if (m_clientMap.contains(clientId))
	{
		Logger(LOG_ERROR) << tr("INTERNAL ERROR: Client map already contains id '%1': addClient").arg(clientId);
		return;
	}

	auto client = QSharedPointer<ClientInfo>::create();
	client->host = sender();
	client->clientId = clientId;
	m_clientMap[clientId] = client;
}


void CommandInterface::removeClient(const QString& clientId)
{
	if (!m_clientMap.contains(clientId))
	{
		Logger(LOG_ERROR) << tr("INTERNAL ERROR: Client map does not contain id '%1': removeClient").arg(clientId);
		return;
	}

	m_clientMap.remove(clientId);
}


void CommandInterface::processData(const QString& clientId, const QByteArray& data, QByteArray& response, bool& disconnect)
{
	if (!m_clientMap.contains(clientId))
	{
		Logger(LOG_ERROR) << "";
	}

	auto client = m_clientMap[clientId];

	// Parse as msgpack data
	QVariant var = MsgPack::unpack(data);


	// Validate data
	if (!var.isValid())
	{
		Logger(LOG_WARNING) << tr("Data is invalid from client ") << clientId;
		disconnect = true;
		return;
	}

	if (var.isNull())
	{
		Logger(LOG_WARNING) << tr("Data is null from client ") << clientId;
		disconnect = true;
		return;
	}

	if (!var.canConvert<QVariantList>())
	{
		Logger(LOG_WARNING) << tr("Data is not QVariantList from client ") << clientId;
		disconnect = true;
		return;
	}

	QVariantList vlist = var.value<QVariantList>();

	if (vlist.length() < 1 || !vlist[0].canConvert(QVariant::String))
	{
		Logger(LOG_WARNING) << tr("First data item not string from client %1: %2")
			.arg(clientId)
			.arg(vlist[0].typeName());
		disconnect = true;
		return;
	}

	QString command = vlist[0].toString();

	QVariantList vlresp;

	if (CMD_NOOP == command)
	{
		// Noop, keep-alive ping
		disconnect = false;
		return;
	}

	if (!client->authenticated)
	{
		if (CMD_AUTH != command || vlist.length() < 5)
		{
			Logger(LOG_WARNING) << tr("Bad authentication packet from client %1 command '%2' size %3")
				.arg(clientId)
				.arg(command)
				.arg(vlist.size());
			vlresp << CMD_AUTH << 0 << tr("Error");
			response = variantListData(vlresp);
			disconnect = true;
			return;
		}
		else
		{
			QString version;
			QString hostName;
			QString password;
			bool helperClient;
			VariantParser parser(CMD_AUTH, clientId, 1, vlist);
			if (!parser.arg(version) || !parser.arg(hostName) || !parser.arg(password) || !parser.arg(helperClient))
			{
				Logger(LOG_ERROR) << parser.errorString();
				vlresp << CMD_AUTH << 0 << tr("Error");
				response = variantListData(vlresp);
				disconnect = true;
				return;
			}

			// Authenticate
			if (!m_passwordHash.isEmpty() && password != m_localPassword &&
				m_passwordHash != QCryptographicHash::hash((m_passwordSalt + password).toUtf8(), CRYPTOMETHOD))
			{
				vlresp << CMD_AUTH << 0 << tr("Password error");
				response = variantListData(vlresp);
				Logger(LOG_WARNING) << tr("Authentication failed from client ") << clientId;
			}
			else
			{
				client->authenticated = true;
				client->version = version;
				client->hostName = hostName;
				client->helperClient = helperClient; // This is a special local client, PinholeHelper
				client->specialClient = m_passwordHash == QCryptographicHash::hash((m_passwordSalt + password).toUtf8(), CRYPTOMETHOD);

				Logger(LOG_DEBUG) << tr("Client connection: ") << clientId << " v" << version << " hostname:" << hostName;

				vlresp << CMD_AUTH << 1 << QHostInfo::localHostName() << QCoreApplication::applicationVersion() << m_settings->serverId();
			}
		}
	}
	else if (CMD_TERMINATE == command)
	{
		// Termination requested
		// Requeste the helper to terminate
		QVariantList vlreq;
		vlreq << CMD_TERMINATE;
		sendToHelper(vlreq);

		Logger(LOG_ALWAYS) << tr("Termination requested by ") << clientId << " " << client->hostName;

		// Terminate this server
		QMetaObject::invokeMethod(qApp, &QCoreApplication::quit, Qt::QueuedConnection);
		vlresp << CMD_CMDRESPONSE << QString() << QString() << CMD_RESPONSE_SUCCESS;
	}
	else if (CMD_SUBSCRIBECMD == command)
	{
		// Subscribe to receive certain broadcast commands
		QString subCmd;
		VariantParser parser(CMD_SUBSCRIBECMD, clientId, 1, vlist);
		if (!parser.arg(subCmd))
		{
			Logger(LOG_ERROR) << parser.errorString();
			disconnect = true;
			return;
		}

		client->subscribedCommands.append(subCmd);

#ifdef QT_DEBUG
		qDebug() << command << subCmd;
#endif
	}
	else if (CMD_SUBSCRIBEGROUP == command)
	{
		// Subscribe to receive property changes for a specific group
		QString group;
		VariantParser parser(CMD_SUBSCRIBEGROUP, clientId, 1, vlist);
		if (!parser.arg(group))
		{
			Logger(LOG_ERROR) << parser.errorString();
			disconnect = true;
			return;
		}

		client->subscribedGroups.append(group);

#ifdef QT_DEBUG
		qDebug() << command << group;
#endif
	}
	else if (CMD_QUERY == command)
	{
		vlresp = handleClientQuery(vlist, clientId);
	}
	else if (CMD_VALUE == command)
	{
		vlresp = handleClientValue(vlist, clientId);
	}
	else if (CMD_COMMAND == command)
	{
		vlresp = handleClientCommand(vlist, clientId, client);
	}
	else if (CMD_SCREENSHOT == command)
	{
		QByteArray screenshot;
		VariantParser parser(CMD_SCREENSHOT, clientId, 1, vlist);
		if (!parser.arg(screenshot))
		{
			Logger(LOG_ERROR) << parser.errorString();
			disconnect = true;
			return;
		}

		// Screenshot from helper in PNG format
		Logger(LOG_DEBUG) << tr("Screen shot size ") << screenshot.size();
		QVariantList vlmsg;
		vlmsg << CMD_CMDRESPONSE << GROUP_NONE << CMD_NONE_GETSCREENSHOT << CMD_RESPONSE_DATA << screenshot;
		sendCmdResponseToWaitingClients(vlmsg);
	}
	else
	{
		vlresp << CMD_CMDUNKNOWN << command;
	}

	disconnect = false;

	if (!vlresp.isEmpty())
	{
		response = variantListData(vlresp);
	}
}


void CommandInterface::logMessage(int level, const QString & message)
{
	QVariantList vlist;
	vlist << CMD_LOG << level << message;
	// Emit this instead of calling SendToAllClients directly because this function may be called from other threads
	QMetaObject::invokeMethod(this, "sendToAllClients", Qt::QueuedConnection, Q_ARG(QVariantList, vlist));
}


void CommandInterface::valueChanged(const QString& group, const QString& item, const QString& property, const QVariant& value) const
{
	QVariantList vlist;
	vlist << CMD_VALUE << group << item << property << value;
	sendToAllClients(vlist);
}


QVariantList CommandInterface::handleClientQuery(const QVariantList& vlist, const QString& clientId) const
{
	// query a value
	QString group;
	QString name;
	QString property;
	VariantParser parser(CMD_QUERY, clientId, 1, vlist);
	if (!parser.arg(group) || !parser.arg(name) || !parser.arg(property))
	{
		Logger(LOG_ERROR) << parser.errorString();
		return QVariantList();
	}

#ifdef QT_DEBUG
	qDebug() << "Query" << group << name << property;
#endif

	QVariant value;

	if (GROUP_APP == group)
	{
		value = m_appManager->getAppVariant(name, property);
	}
	else if (GROUP_GROUP == group)
	{
		value = m_groupManager->getGroupVariant(name, property);
	}
	else if (GROUP_GLOBAL == group)
	{
		value = m_globalManager->getGlobalVariant(name, property);
	}
	else if (GROUP_SCHEDULE == group)
	{
		value = m_scheduleManager->getEventVariant(name, property);
	}
	else if (GROUP_ALERT == group)
	{
		value = m_alertManager->getAlertVariant(name, property);
	}

	QVariantList vlresp;

	if (value.isNull())
	{
		vlresp << CMD_MISSING << group << name << property;
#ifdef QT_DEBUG
		qDebug() << "Value missing" << group << name << property;
#endif
	}
	else
	{
		vlresp << CMD_VALUE << group << name << property << value;
#ifdef QT_DEBUG
		qDebug() << "Value returned:" << group << name << property << value;
#endif
	}

	return vlresp;
}


QVariantList CommandInterface::handleClientValue(const QVariantList& vlist, const QString& clientId) const
{
	// set a value
	QString group;
	QString name;
	QString property;
	QVariant value;
	VariantParser parser(CMD_VALUE, clientId, 1, vlist);
	if (!parser.arg(group) || !parser.arg(name) || !parser.arg(property) || !parser.arg(value))
	{
		Logger(LOG_ERROR) << parser.errorString();
		return QVariantList();
	}

#ifdef QT_DEBUG
	qDebug() << "Value" << group << name << property << value;
#endif

	bool success = false;

	if (GROUP_APP == group)
	{
		success = m_appManager->setAppVariant(name, property, value);
	}
	else if (GROUP_GROUP == group)
	{
		success = m_groupManager->setGroupVariant(name, property, value);
	}
	else if (GROUP_GLOBAL == group)
	{
		success = m_globalManager->setGlobalVariant(name, property, value);
	}
	else if (GROUP_SCHEDULE == group)
	{
		success = m_scheduleManager->setEventVariant(name, property, value);
	}
	else if (GROUP_ALERT == group)
	{
		success = m_alertManager->setAlertVariant(name, property, value);
	}

	// Send back a failure/success message
	QVariantList vlresp;
	vlresp << CMD_VALUESET << group << name << property << success;
	return vlresp;
}


QVariantList CommandInterface::handleClientCommand(const QVariantList& vlist, const QString& clientId, QSharedPointer<ClientInfo> client)
{
	QString group;
	QString subCommand;
	VariantParser parser(CMD_COMMAND, clientId, 1, vlist);
	if (!parser.arg(group) || !parser.arg(subCommand))
	{
		Logger(LOG_ERROR) << parser.errorString();
		return QVariantList();
	}

#ifdef QT_DEBUG
	qDebug() << "Command" << group << subCommand;
#endif

	bool commandUnfound = false;
	bool commandPostpone = false;
	bool commandSuccess = true;
	QVariant commandData;

	if (GROUP_NONE == group)
	{
		if (CMD_NONE_SETPASSWORD == subCommand)
		{
			// Set server password
			QString password;
			VariantParser parser(CMD_NONE_SETPASSWORD, clientId, 3, vlist);
			if (!parser.arg(password))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			if (password.isEmpty())
			{
				m_passwordSalt.clear();
				m_passwordHash.clear();
				Logger(LOG_ALWAYS) << tr("Password CLEARED by ") << clientId << " " << client->hostName;
			}
			else
			{
				m_passwordSalt = GenerateRandomString(16);
				m_passwordHash = QCryptographicHash::hash((m_passwordSalt + password).toUtf8(), CRYPTOMETHOD);
				Logger(LOG_ALWAYS) << tr("Password changed by ") << clientId << " " << client->hostName;
			}
		}
		else if (CMD_NONE_GETSCREENSHOT == subCommand)
		{
			QVariantList vlreq;
			vlreq << CMD_SCREENSHOT;

			// Send screenshot request to helper
			if (!sendToHelper(vlreq))
			{
				// Inform console of failure
				Logger(LOG_WARNING) << tr("Failed to send screenshot request to helper, helper may not be connected/running");
				commandSuccess = false;
			}
			else
			{
				// Mark client as waiting for screenshot response
				client->waitingForCommand = true;
				client->waitingCommandGroup = group;
				client->waitingSubCommand = subCommand;

				// Postpone the response
				commandPostpone = true;
			}
		}
		else if (CMD_NONE_IMPORTSETTINGS == subCommand)
		{
			// Import settings from JSON
			QByteArray importData;
			VariantParser parser(CMD_NONE_IMPORTSETTINGS, clientId, 3, vlist);
			if (!parser.arg(importData))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = importSettingsData(importData, clientId, client->hostName);
		}
		else if (CMD_NONE_EXPORTSETTINGS == subCommand)
		{
			commandData = generateSettingsData();
		}
		else if (CMD_NONE_SHOWSCREENIDS == subCommand)
		{
			QVariantList vlreq;
			vlreq << CMD_SHOWSCREENIDS;

			// Send request request to helper
			commandSuccess = sendToHelper(vlreq);
		}
		else if (CMD_NONE_RETRIEVELOG == subCommand)
		{
			// Retrieve log in date range
			QString startDate;
			QString endDate;
			VariantParser parser(CMD_NONE_RETRIEVELOG, clientId, 3, vlist);
			if (!parser.arg(startDate) || !parser.arg(endDate))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			QByteArray logData = Logger::getLogFileData(startDate, endDate);

			if (logData.isNull())
			{
				commandSuccess = false;
			}
			else
			{
				commandData = QVariant(qCompress(logData));
			}
		}
		else if (CMD_NONE_LOGMESSAGE == subCommand)
		{
			// Log packet from Helper or Console app
			int logLevel;
			QString message;
			VariantParser parser(CMD_NONE_LOGMESSAGE, clientId, 3, vlist);
			if (!parser.arg(logLevel) || !parser.arg(message))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			QString clientName;
			if (client->helperClient)
				clientName = "TrayIcon";
			else
				clientName = clientId + ":" + client->hostName;

			Logger(logLevel) << "[" << clientName << "] " << message;
		}
		else
		{
			commandUnfound = true;
		}
	}
	else if (GROUP_APP == group)
	{
		if (CMD_APP_ADDAPP == subCommand)
		{
			QString appName;
			VariantParser parser(CMD_APP_ADDAPP, clientId, 3, vlist);
			if (!parser.arg(appName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->addApplication(appName);
		}
		else if (CMD_APP_DELETEAPP == subCommand)
		{
			QString appName;
			VariantParser parser(CMD_APP_DELETEAPP, clientId, 3, vlist);
			if (!parser.arg(appName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->deleteApplication(appName);
		}
		else if (CMD_APP_RENAMEAPP == subCommand)
		{
			QString appName;
			QString newAppName;
			VariantParser parser(CMD_APP_RENAMEAPP, clientId, 3, vlist);
			if (!parser.arg(appName) || !parser.arg(newAppName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->renameApplication(appName, newAppName);
		}
		else if (CMD_APP_STARTAPPS == subCommand)
		{
			QStringList appNames;
			VariantParser parser(CMD_APP_STARTAPPS, clientId, 3, vlist);
			if (!parser.arg(appNames))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->startApps(appNames);
		}
		else if (CMD_APP_STARTSTARTUPAPPS == subCommand)
		{
			commandSuccess = m_appManager->startStartupApps();
		}
		else if (CMD_APP_RESTARTAPPS == subCommand)
		{
			QStringList appNames;
			VariantParser parser(CMD_APP_RESTARTAPPS, clientId, 3, vlist);
			if (!parser.arg(appNames))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->restartApps(appNames);
		}
		else if (CMD_APP_STOPAPPS == subCommand)
		{
			QStringList appNames;
			VariantParser parser(CMD_APP_STOPAPPS, clientId, 3, vlist);
			if (!parser.arg(appNames))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->stopApps(appNames);
		}
		else if (CMD_APP_STOPALLAPPS == subCommand)
		{
			commandSuccess = m_appManager->stopAllApps();
		}
		else if (CMD_APP_GETCONSOLE == subCommand)
		{
			// Retrieve console output file for an application
			QString appName;
			VariantParser parser(CMD_APP_GETCONSOLE, clientId, 3, vlist);
			if (!parser.arg(appName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandData = QVariant(qCompress(m_appManager->getConsoleOutputFile(appName)));
		}
		else if (CMD_APP_EXECUTE == subCommand)
		{
			// Execute a command on the server
			QByteArray file;
			QString command;
			QString args;
			QString directory;
			bool capture;
			bool elevated;
			VariantParser parser(CMD_APP_EXECUTE, clientId, 3, vlist);
			if (!parser.arg(file) || !parser.arg(command) || !parser.arg(args) ||
				!parser.arg(directory) || !parser.arg(capture) || !parser.arg(elevated))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->executeCommand(
				file,
				command,
				args,
				directory,
				capture,
				elevated,
				commandData);
		}
		else if (CMD_APP_STARTVARS == subCommand)
		{
			QString appName;
			QStringList vars;
			VariantParser parser(CMD_APP_STARTVARS, clientId, 3, vlist);
			if (!parser.arg(appName) || !parser.arg(vars))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_appManager->startAppVariables(appName, vars);
		}
		else
		{
			commandUnfound = true;
		}
	}
	else if (GROUP_GROUP == group)
	{
		if (CMD_GROUP_ADDGROUP == subCommand)
		{
			QString groupName;
			VariantParser parser(CMD_GROUP_ADDGROUP, clientId, 3, vlist);
			if (!parser.arg(groupName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_groupManager->addGroup(groupName);
		}
		else if (CMD_GROUP_DELETEGROUP == subCommand)
		{
			QString groupName;
			VariantParser parser(CMD_GROUP_DELETEGROUP, clientId, 3, vlist);
			if (!parser.arg(groupName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_groupManager->deleteGroup(groupName);
		}
		else if (CMD_GROUP_RENAMEGROUP == subCommand)
		{
			QString oldName;
			QString newName;
			VariantParser parser(CMD_GROUP_RENAMEGROUP, clientId, 3, vlist);
			if (!parser.arg(oldName) || !parser.arg(newName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_groupManager->renameGroup(oldName, newName);
		}
		else if (CMD_GROUP_STARTGROUP == subCommand)
		{
			QString groupName;
			VariantParser parser(CMD_GROUP_STARTGROUP, clientId, 3, vlist);
			if (!parser.arg(groupName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_groupManager->startGroup(groupName);
		}
		else if (CMD_GROUP_STOPGROUP == subCommand)
		{
			QString groupName;
			VariantParser parser(CMD_GROUP_STOPGROUP, clientId, 3, vlist);
			if (!parser.arg(groupName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_groupManager->stopGroup(groupName);
		}
		else
		{
			commandUnfound = true;
		}
	}
	else if (GROUP_SCHEDULE == group)
	{
		if (CMD_SCHED_ADDEVENT == subCommand)
		{
			QString eventName;
			VariantParser parser(CMD_SCHED_ADDEVENT, clientId, 3, vlist);
			if (!parser.arg(eventName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_scheduleManager->addEvent(eventName);
		}
		else if (CMD_SCHED_DELETEEVENT == subCommand)
		{
			QString eventName;
			VariantParser parser(CMD_SCHED_DELETEEVENT, clientId, 3, vlist);
			if (!parser.arg(eventName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_scheduleManager->deleteEvent(eventName);
		}
		else if (CMD_SCHED_RENAMEEVENT == subCommand)
		{
			QString oldName;
			QString newName;
			VariantParser parser(CMD_SCHED_RENAMEEVENT, clientId, 3, vlist);
			if (!parser.arg(oldName) || !parser.arg(newName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_scheduleManager->renameEvent(oldName, newName);
		}
		else if (CMD_SCHED_TRIGGEREVENTS == subCommand)
		{
			QStringList eventNames;
			VariantParser parser(CMD_SCHED_TRIGGEREVENTS, clientId, 3, vlist);
			if (!parser.arg(eventNames))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_scheduleManager->triggerEvents(eventNames);
		}
		else
		{
			commandUnfound = true;
		}
	}
	else if (GROUP_GLOBAL == group)
	{
		if (CMD_GLOBAL_SHUTDOWN == subCommand)
		{
			commandSuccess = m_globalManager->shutdown();
		}
		else if (CMD_GLOBAL_REBOOT == subCommand)
		{
			commandSuccess = m_globalManager->reboot();
		}
		else if (CMD_GLBOAL_SYSINFO == subCommand)
		{
			commandData = QVariant(qCompress(m_globalManager->getSysInfoData()));
		}
		else
		{
			commandUnfound = true;
		}
	}
	else if (GROUP_ALERT == group)
	{
		if (CMD_ALERT_ADDSLOT == subCommand)
		{
			QString slotName;
			VariantParser parser(CMD_ALERT_ADDSLOT, clientId, 3, vlist);
			if (!parser.arg(slotName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_alertManager->addAlertSlot(slotName);
		}
		else if (CMD_ALERT_DELETESLOT == subCommand)
		{
			QString slotName;
			VariantParser parser(CMD_ALERT_DELETESLOT, clientId, 3, vlist);
			if (!parser.arg(slotName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_alertManager->deleteAlertSlot(slotName);
		}
		else if (CMD_ALERT_RENAMESLOT == subCommand)
		{
			QString oldName;
			QString newName;
			VariantParser parser(CMD_ALERT_RENAMESLOT, clientId, 3, vlist);
			if (!parser.arg(oldName) || !parser.arg(newName))
			{
				Logger(LOG_ERROR) << parser.errorString();
				return QVariantList();
			}

			commandSuccess = m_alertManager->renameAlertSlot(oldName, newName);
		}
		else if (CMD_ALERT_RESETCOUNT == subCommand)
		{
			commandSuccess = m_alertManager->resetActiveAlertCount();
		}
		else if (CMD_ALERT_RETRIEVELIST == subCommand)
		{
			commandData = QVariant(qCompress(m_alertManager->retrieveAlertList()));
		}
		else
		{
			commandUnfound = true;
		}
	}
	else
	{
		commandUnfound = true;
	}

	QVariantList vlresp;

	// Send response
	if (commandUnfound)
	{
		vlresp << CMD_CMDRESPONSE << group << subCommand << CMD_RESPONSE_UNKNOWN;
#ifdef QT_DEBUG
		qDebug() << "Command unfound" << group << subCommand;
#endif
	}
	else if (!commandPostpone)
	{
		if (!commandSuccess)
		{
			vlresp << CMD_CMDRESPONSE << group << subCommand << CMD_RESPONSE_ERROR;
#ifdef QT_DEBUG
			qDebug() << "Command error" << group << subCommand;
#endif
		}
		else if (!commandData.isNull())
		{
			vlresp << CMD_CMDRESPONSE << group << subCommand << CMD_RESPONSE_DATA << commandData;
#ifdef QT_DEBUG
			qDebug() << "Command data" << group << subCommand << commandData.typeName();
#endif
		}
		else
		{
			vlresp << CMD_CMDRESPONSE << group << subCommand << CMD_RESPONSE_SUCCESS;
#ifdef QT_DEBUG
			qDebug() << "Command success" << group << subCommand;
#endif
		}
	}

	return vlresp;
}


bool CommandInterface::importSettingsData(const QByteArray& data, const QString& clientAddr, const QString& clientHostName) const
{
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
	if (jsonDoc.isNull())
	{
		Logger(LOG_WARNING) << tr("Error parsing import JSON size %1 from client %2 %3")
			.arg(data.size())
			.arg(clientAddr)
			.arg(clientHostName);
		return false;
	}
	else if (jsonDoc[JSONTAG_GLOBALSETTINGS].isUndefined() &&
		jsonDoc[JSONTAG_APPSETTINGS].isUndefined() &&
		jsonDoc[JSONTAG_GROUPSETTINGS].isUndefined() &&
		jsonDoc[JSONTAG_SCHEDETTINGS].isUndefined() &&
		jsonDoc[JSONTAG_ALERTSETTINGS].isUndefined())
	{
		Logger(LOG_WARNING) << tr("Missing tags in JSON size %1 from client %2 %3")
			.arg(data.size())
			.arg(clientAddr)
			.arg(clientHostName);
		return false;
	}

	if (!jsonDoc[JSONTAG_GLOBALSETTINGS].isUndefined())
	{
		Logger() << tr("Importing global settings");
		m_globalManager->importSettings(jsonDoc[JSONTAG_GLOBALSETTINGS].toObject());
	}

	if (!jsonDoc[JSONTAG_APPSETTINGS].isUndefined())
	{
		Logger() << tr("Importing application settings");
		m_appManager->importSettings(jsonDoc[JSONTAG_APPSETTINGS].toObject());
	}

	if (!jsonDoc[JSONTAG_GROUPSETTINGS].isUndefined())
	{
		Logger() << tr("Importing group settings");
		m_groupManager->importSettings(jsonDoc[JSONTAG_GROUPSETTINGS].toObject());
	}

	if (!jsonDoc[JSONTAG_SCHEDETTINGS].isUndefined())
	{
		Logger() << tr("Importing schedule settings");
		m_scheduleManager->importSettings(jsonDoc[JSONTAG_SCHEDETTINGS].toObject());
	}

	if (!jsonDoc[JSONTAG_ALERTSETTINGS].isUndefined())
	{
		Logger() << tr("Importing alert settings");
		m_alertManager->importSettings(jsonDoc[JSONTAG_ALERTSETTINGS].toObject());
	}

	return true;
}


// Generates the settings json data from each manager and combines them
QByteArray CommandInterface::generateSettingsData() const
{
	// Collect the data
	QJsonObject jobject;
	jobject[JSONTAG_GLOBALSETTINGS] = m_globalManager->exportSettings();
	jobject[JSONTAG_APPSETTINGS] = m_appManager->exportSettings();
	jobject[JSONTAG_GROUPSETTINGS] = m_groupManager->exportSettings();
	jobject[JSONTAG_SCHEDETTINGS] = m_scheduleManager->exportSettings();
	jobject[JSONTAG_ALERTSETTINGS] = m_alertManager->exportSettings();

	// Return the json document as json text
	return QJsonDocument(jobject).toJson();
}


void CommandInterface::sendDataToClient(const QSharedPointer<ClientInfo> client, const QByteArray & data) const
{
	if (!QMetaObject::invokeMethod(client->host, "sendDataToClient", Q_ARG(QString, client->clientId), Q_ARG(QByteArray, data)))
	{
		Logger(LOG_ERROR) << tr("Failed to invoke sendDataToClient for clientId '%1'").arg(client->clientId);
	}
}


void CommandInterface::sendDataToClient(const QString &clientId, const QByteArray & data) const
{
	if (!m_clientMap.contains(clientId))
	{
		Logger(LOG_ERROR) << tr("INTERNAL ERROR: Client id '%1' not in client map: sendDataToClient").arg(clientId);
		return;
	}

	sendDataToClient(m_clientMap[clientId], data);
}


QByteArray CommandInterface::variantListData(const QVariantList & vlist) const
{
	static QByteArray sizeArray(sizeof(uint32_t), 0);
	QByteArray dataArray(MsgPack::pack(vlist));
	uint32_t size = qToLittleEndian(dataArray.size());
	memcpy(sizeArray.data(), &size, sizeof(size));
	return sizeArray + dataArray;
}


bool CommandInterface::readServerSettings()
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	m_passwordSalt = settings->value(PROP_SERVER_SALT).toString();
	m_passwordHash = QByteArray::fromHex(settings->value(PROP_SERVER_HASH).toString().toUtf8());

	return true;
}


bool CommandInterface::writeSettings() const
{
#ifdef QT_DEBUG
	//qDebug() << "Writing encrypted TCP server settings";
#endif

	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	settings->setValue(PROP_SERVER_SALT, m_passwordSalt);
	settings->setValue(PROP_SERVER_HASH, QString(m_passwordHash.toHex()));

	return true;
}


bool CommandInterface::createSharedPassword()
{
	// Create shared memory object with special password 
	m_localPassword = GenerateRandomString(SHAREDPASS_LEN);
	//Logger(LOG_DEBUG) << m_localPassword;
	QByteArray passData = m_localPassword.toUtf8();
	QString key = PINHOLE_ORG_DOMAIN;
	for (int n = 0; n < passData.length(); n++)
	{
		passData[n] = passData[n] ^ key[n % key.length()].toLatin1();
	}

#if defined(Q_OS_WIN)
	if (m_settings->runningAsService())
	{
		if (!CreateSharedMemory("Global\\" SHAREDMEM_PASS, passData))
		{
			Logger(LOG_ERROR) << tr("Error creating Windows native shared memory object");
			return false;
		}
		return true;
	}
	else
	{
		m_sharedMemoryPassword.setKey(SHAREDMEM_PASS);
	}
#else
	m_sharedMemoryPassword.setKey(SHAREDMEM_PASS);
#endif

	if (!m_sharedMemoryPassword.create(passData.length()))
	{
		Logger(LOG_ERROR) << tr("Failed to create shared memory object for password: %1")
			.arg(m_sharedMemoryPassword.errorString());
		return false;
	}

	if (!m_sharedMemoryPassword.lock())
	{
		Logger(LOG_ERROR) << tr("Failed to lock shared memory object for password: %1")
			.arg(m_sharedMemoryPassword.errorString());
		return false;
	}

	memcpy(m_sharedMemoryPassword.data(), passData.data(), passData.length());

	if (!m_sharedMemoryPassword.unlock())
	{
		Logger(LOG_ERROR) << tr("Failed to unlock shared memory object for password: %1")
			.arg(m_sharedMemoryPassword.errorString());
	}

#if defined(Q_OS_UNIX)
	if (m_settings->runningAsService())
	{
		// HACK: Override QT permissions on objects so they can be accessed from other user accounts
		QByteArray key = m_sharedMemoryPassword.nativeKey().toUtf8();
		if (0 != chmod(key.data(), 0666))
		{
			Logger(LOG_WARNING) << tr("Error %1 changing permissions on shared memory object '%2': %3")
				.arg(errno)
				.arg(QString::fromUtf8(key))
				.arg(strerror(errno));
		}
		key_t mem_key = ftok(key.data(), 'Q');
		if (-1 == mem_key)
		{
			Logger(LOG_WARNING) << tr("Error %1 getting file token for shared memory object '%2': %3")
				.arg(errno)
				.arg(QString::fromUtf8(key))
				.arg(strerror(errno));
		}
		else
		{
			int mem = shmget(mem_key, 0, 0);
			if (-1 == mem)
			{
				Logger(LOG_WARNING) << tr("Error %1 opening shared memory object: %2")
					.arg(errno)
					.arg(strerror(errno));
			}
			else
			{
				struct shmid_ds shmidds;
				if (-1 == shmctl(mem, IPC_STAT, &shmidds))
				{
					Logger(LOG_WARNING) << tr("Error %1 getting shared memory values: %2")
						.arg(errno)
						.arg(strerror(errno));
				}
				else
				{
					shmidds.shm_perm.mode |= 0666;
					if (-1 == shmctl(mem, IPC_SET, &shmidds))
					{
						Logger(LOG_WARNING) << tr("Error %1 setting shared memory permissions: %2")
							.arg(errno)
							.arg(strerror(errno));
					}
				}
			}
		}

		key = key.mid(23).prepend("/tmp/qipc_systemsem_");
		if (0 != chmod(key.data(), 0666))
		{
			Logger(LOG_WARNING) << tr("Error %1 changing permissions on semaphore obect file '%2': %3")
				.arg(errno)
				.arg(QString::fromUtf8(key))
				.arg(strerror(errno));
		}
		key_t sem_key = ftok(key.data(), 'Q');
		if (-1 == sem_key)
		{
			Logger(LOG_WARNING) << tr("Error %1 getting file token for semaphore object '%2': %3")
				.arg(errno)
				.arg(QString::fromUtf8(key))
				.arg(strerror(errno));
		}
		else
		{
			int sem = semget(sem_key, 1, 0666 | IPC_CREAT);
			if (-1 == sem)
			{
				Logger(LOG_WARNING) << tr("Error %1 opening semaphore: %2")
					.arg(errno)
					.arg(strerror(errno));
			}
			else
			{
				union semun {
					int              val;    /* Value for SETVAL */
					struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
					unsigned short  *array;  /* Array for GETALL, SETALL */
					struct seminfo  *__buf;  /* Buffer for IPC_INFO (Linux-specific) */
				} semopts;
				struct semid_ds mysemds;
				semopts.buf = &mysemds;
				if (-1 == semctl(sem, 0, IPC_STAT, semopts))
				{
					Logger(LOG_WARNING) << tr("Error %1 getting semaphore values: %2")
						.arg(errno)
						.arg(strerror(errno));
				}
				else
				{
					semopts.buf->sem_perm.mode = 0666;
					if (-1 == semctl(sem, 0, IPC_SET, semopts))
					{
						Logger(LOG_WARNING) << tr("Error %1 settings semaphore permissions: %2")
							.arg(errno)
							.arg(strerror(errno));
					}
				}
			}
		}
	}
#endif

	return true;
}


// Returns true if a PinholeHelper client is connected
bool CommandInterface::helperConnected() const
{
	for (const auto& client : m_clientMap)
	{
		if (client->helperClient)
			return true;
	}

	return false;
}


bool CommandInterface::sendToHelper(const QVariantList& vlist) const
{
	for (const auto& client : m_clientMap)
	{
		if (client->helperClient)
		{
			sendDataToClient(client, variantListData(vlist));
			return true;
		}
	}

	return false;
}


bool CommandInterface::sendCmdResponseToWaitingClients(const QVariantList& vlist) const
{
	bool ret = false;
	for (const auto& client : m_clientMap)
	{
		if (client->waitingForCommand && client->waitingCommandGroup == vlist[1] && client->waitingSubCommand == vlist[2])
		{
			sendDataToClient(client, variantListData(vlist));
			client->waitingForCommand = false;
			client->waitingCommandGroup.clear();
			client->waitingSubCommand.clear();
			ret = true;
		}
	}

	return ret;
}


void CommandInterface::sendToAllClients(const QVariantList & vlist) const
{
	QString command = vlist[0].toString();

	for (const auto& client : m_clientMap)
	{
		if (client->subscribedCommands.contains(command))
		{
			if (CMD_VALUE != command || client->subscribedGroups.contains(vlist[1].toString()))
			{
#ifdef QT_DEBUG
				qDebug() << "Sending to " << client->hostName << ": " << vlist[0].toString() << vlist[1].toString() << vlist[2].toString();
#endif
				sendDataToClient(client, variantListData(vlist));
			}
		}
	}
}


void CommandInterface::sendControlWindow(int pid, const QString& display, const QString& command)
{
	QVariantList vlist;
	vlist << CMD_CONTROLWINDOW << pid << display << command;
	sendToHelper(vlist);
}


bool CommandInterface::VariantParser::arg(bool& a)
{
	if (!checkSize())
		return false;

	if (!m_vlist[m_argPos].canConvert(QVariant::Bool))
	{
		m_error = true;
		m_errorString = QObject::tr("Wrong variant type in packet from client %1 subCommand:%2 position %3 should be bool")
			.arg(m_client)
			.arg(m_command)
			.arg(m_argPos);
		return false;
	}

	a = m_vlist[m_argPos].toBool();
	m_argPos++;

	return true;
}

bool CommandInterface::VariantParser::arg(int& a)
{
	if (!checkSize())
		return false;

	if (!m_vlist[m_argPos].canConvert(QVariant::Int))
	{
		m_error = true;
		m_errorString = QObject::tr("Wrong variant type in packet from client %1 subCommand:%2 position %3 should be int")
			.arg(m_client)
			.arg(m_command)
			.arg(m_argPos);
		return false;
	}

	a = m_vlist[m_argPos].toInt();
	m_argPos++;

	return true;
}


bool CommandInterface::VariantParser::arg(QString & a)
{
	if (!checkSize())
		return false;

	if (!m_vlist[m_argPos].canConvert(QVariant::String))
	{
		m_error = true;
		m_errorString = QObject::tr("Wrong variant type in packet from client %1 subCommand:%2 position %3 should be string")
			.arg(m_client)
			.arg(m_command)
			.arg(m_argPos);
		return false;
	}

	a = m_vlist[m_argPos].toString();
	m_argPos++;

	return true;
}


bool CommandInterface::VariantParser::arg(QStringList & a)
{
	if (!checkSize())
		return false;

	if (!m_vlist[m_argPos].canConvert(QVariant::StringList))
	{
		m_error = true;
		m_errorString = QObject::tr("Wrong variant type in packet from client %1 subCommand:%2 position %3 should be string list")
			.arg(m_client)
			.arg(m_command)
			.arg(m_argPos);
		return false;
	}

	a = m_vlist[m_argPos].toStringList();
	m_argPos++;

	return true;
}


bool CommandInterface::VariantParser::arg(QByteArray & a)
{
	if (!checkSize())
		return false;

	if (!m_vlist[m_argPos].canConvert(QVariant::ByteArray))
	{
		m_error = true;
		m_errorString = QObject::tr("Wrong variant type in packet from client %1 command:%2 position %3 should be byte array")
			.arg(m_client)
			.arg(m_command)
			.arg(m_argPos);
		return false;
	}

	a = m_vlist[m_argPos].toByteArray();
	m_argPos++;

	return true;
}


bool CommandInterface::VariantParser::arg(QVariant & a)
{
	if (!checkSize())
		return false;

	a = m_vlist[m_argPos];

	return true;
}


bool CommandInterface::VariantParser::checkSize()
{
	if (m_error)
		return false;

	if (m_argPos > m_vlist.size() - 1)
	{
		m_error = true;
		m_errorString = QObject::tr("Short packet from client %1 command:%2 size:%3")
			.arg(m_client)
			.arg(m_command)
			.arg(m_vlist.size());
		return false;
	}

	return true;
}
