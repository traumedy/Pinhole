#include "PinholeClient.h"
#include "../common/HostClient.h"
#include "../common/Utilities.h"

#include <QTextStream>
#include <QEventLoop>
#include <QFile>

#include <map>
#include <functional>

QTextStream& qStdOut()
{
	static QTextStream ts(stdout);
	return ts;
}

PinholeClient::PinholeClient(QObject *parent)
	: QObject(parent)
{
}

PinholeClient::~PinholeClient()
{
}

void PinholeClient::listCommands()
{
	qStdOut() << "Server command literal - Argument description:" << endl;
	for (const auto& i : m_commandMap)
	{
		qStdOut() << i.first << " - " << i.second.second << endl;
	}
}

void PinholeClient::listProperties()
{
	qStdOut() << "Global properties:" << endl;
	for (const auto& key : m_propListGlobal.keys())
	{
		qStdOut() << key << " - " << QMetaType::typeName(m_propListGlobal[key]) << endl;
	}
	qStdOut() << PROP_GLOBAL_HOSTLOGLEVEL << " and " << PROP_GLOBAL_REMOTELOGLEVEL << " valid values are:" << endl;
	qStdOut() << LOG_LEVEL_ERROR << ";" << LOG_LEVEL_WARNING << ";" << LOG_LEVEL_NORMAL << ";" << LOG_LEVEL_EXTRA << LOG_LEVEL_DEBUG << endl;

	qStdOut() << endl <<  "Application properties:" << endl;
	for (const auto& key : m_propListApplication.keys())
	{
		qStdOut() << key << " - " << QMetaType::typeName(m_propListApplication[key]) << endl;
	}
	qStdOut() << PROP_APP_LAUNCHDISPLAY << " valid values are:" << endl;
	qStdOut() << DISPLAY_NORMAL << ";" << DISPLAY_HIDDEN << ";" << DISPLAY_MINIMIZE << ";" << DISPLAY_MAXIMIZE << endl;

	qStdOut() << endl << "Group properties:" << endl;
	for (const auto& key : m_propListGroup.keys())
	{
		qStdOut() << key << " - " << QMetaType::typeName(m_propListGroup[key]) << endl;
	}

	qStdOut() << endl << "Schedule properties:" << endl;
	for (const auto& key : m_propListSchedule.keys())
	{
		qStdOut() << key << " - " << QMetaType::typeName(m_propListSchedule[key]) << endl;
	}
	qStdOut() << PROP_SCHED_TYPE << " valid values are:" << endl;
	qStdOut() << SCHED_TYPE_STARTAPPS << ";" << SCHED_TYPE_STOPAPPS << ";" << SCHED_TYPE_RESTARTAPPS << ";" << SCHED_TYPE_STARTGROUP << ";" << endl;
	qStdOut() << SCHED_TYPE_STOPGROUP << ";" << SCHED_TYPE_SHUTDOWN << ";" << SCHED_TYPE_REBOOT << ";" << SCHED_TYPE_SCREENSHOT << ";" << SCHED_TYPE_TRIGGEREVENTS << endl;
	qStdOut() << PROP_SCHED_FREQUENCY << " valid values are: " << endl;
	qStdOut() << SCHED_FREQ_DISABLED << ";" << SCHED_FREQ_WEEKLY << ";" << SCHED_FREQ_DAILY << ";" << SCHED_FREQ_HOURLY << ";" << SCHED_FREQ_ONCE << endl;

	qStdOut() << endl << "Alert properties:" << endl;
	for (const auto& key : m_propListAlert.keys())
	{
		qStdOut() << key << " - " << QMetaType::typeName(m_propListAlert[key]) << endl;
	}
	qStdOut() << PROP_ALERT_SLOTTYPE << " valid values are:" << endl;
	qStdOut() << ALERTSLOT_TYPE_SMPTEMAIL << ";" << ALERTSLOT_TYPE_HTTPGET << ";" << ALERTSLOT_TYPE_HTTPPOST << ";" << ALERTSLOT_TYPE_SLACK << ";" << ALERTSLOT_TYPE_EXTERNAL << endl;

	// Todo more explanation
}

bool PinholeClient::isCommandValid(const QString& command)
{
	if (m_commandMap.find(command) == m_commandMap.end())
		return false;

	return true;
}

bool PinholeClient::executeCommand(const QString& address, int port, const QString& command, const QString& argument)
{
	if (m_commandMap.find(command) == m_commandMap.end())
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Invalid command '%1' ' (use -c to list commands)").arg(command) << endl;
		}
		return false;
	}

	return m_commandMap[command].first(this, address, port, argument);
}

bool PinholeClient::getProperty(const QString& address, int port, const QString& item, const QString& property)
{
	QString group;

	if (m_propListGlobal.contains(property))
	{
		group = GROUP_GLOBAL;
	}
	else if (m_propListApplication.contains(property))
	{
		group = GROUP_APP;
	}
	else if (m_propListGroup.contains(property))
	{
		group = GROUP_GROUP;
	}
	else if (m_propListSchedule.contains(property))
	{
		group = GROUP_SCHEDULE;
	}
	else if (m_propListAlert.contains(property))
	{
		group = GROUP_ALERT;
	}
	else
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Unknown property '%1' (use -l to list properties)").arg(property) << endl;
		}
		return false;
	}

	return ExecServerCommand(address, port, tr("get property '%1'").arg(property),
		[group, item, property](HostClient* hostClient) { hostClient->getVariant(group, item, property); });
}

bool PinholeClient::setProperty(const QString& address, int port, const QString& item, const QString& property, const QString& value)
{
	QString group;
	int type;

	if (m_propListGlobal.contains(property))
	{
		group = GROUP_GLOBAL;
		type = m_propListGlobal[property];
	}
	else if (m_propListApplication.contains(property))
	{
		group = GROUP_APP;
		type = m_propListApplication[property];
	}
	else if (m_propListGroup.contains(property))
	{
		group = GROUP_GROUP;
		type = m_propListGroup[property];
	}
	else if (m_propListSchedule.contains(property))
	{
		group = GROUP_SCHEDULE;
		type = m_propListSchedule[property];
	}
	else if (m_propListAlert.contains(property))
	{
		group = GROUP_ALERT;
		type = m_propListAlert[property];
	}
	else
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Unknown property '%1' (use -l to list properties)").arg(property) << endl;
		}
		return false;
	}

	QVariant variant;
	switch (type)
	{
	case QMetaType::Void:
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Property '%1' is read only").arg(property) << endl;
		}
		return false;
		break;

	case QMetaType::Bool:
		if ("true" == value)
			variant = true;
		else if ("false")
			variant = false;
		else
		{
			if (m_silent)
			{
				qStdOut() << tr("failure") << endl;
			}
			else
			{
				qStdOut() << tr("Bool values must be 'true' or 'false' not '%1'.").arg(value) << endl;
			}
			return false;
		}
		break;

	case QMetaType::Int:
	{
		bool ok = false;
		variant = value.toInt(&ok);
		if (!ok)
		{
			if (m_silent)
			{
				qStdOut() << tr("failure") << endl;
			}
			else
			{
				qStdOut() << tr("Value must be an integer, not '%1'.").arg(value) << endl;
			}
			return false;
		}
	}
		break;

	case QMetaType::QString:
		variant = value;
		break;

	case QMetaType::QStringList:
		variant = SplitSemicolonString(value);
		break;
	}

	return ExecServerCommand(address, port, tr("set property '%1'").arg(property),
		[group, item, property, variant](HostClient* hostClient) { hostClient->setVariant(group, item, property, variant); });
}

bool PinholeClient::logMessage(const QString& address, int port, const QString& message)
{
	return ExecServerCommand(address, port, tr("log message"),
		[message](HostClient* hostClient) { hostClient->logMessage(message); });
}

bool PinholeClient::None_SetPassword(const QString& address, int port, const QString& argument)
{
	return ExecServerCommand(address, port, tr("set password"),
		[argument](HostClient* hostClient) { hostClient->setPassword(argument); });
}

bool PinholeClient::None_GetScreenshot(const QString& address, int port, const QString& argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("get screenshot"),
		[](HostClient* hostClient) { hostClient->requestScreenshot(); }, argument);
}

bool PinholeClient::None_ShowScreenIds(const QString& address, int port, const QString& argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("show screen ids"),
		[](HostClient* hostClient) { hostClient->showScreenIds(); });
}

bool PinholeClient::None_ImportSettings(const QString & address, int port, const QString & argument)
{
	QFile importFile(argument);
	if (!importFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qStdOut() << tr("Unable to open input file: ") << importFile.errorString() << endl;
		return false;
	}

	QByteArray importData = importFile.readAll();
	importFile.close();

	return ExecServerCommand(address, port, tr("import settings"),
		[&importData](HostClient* hostClient) { hostClient->sendImportData(importData); });
}

bool PinholeClient::None_ExportSettings(const QString & address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("export settings"),
		[](HostClient* hostClient) { hostClient->requestExportSettings(); }, argument);
}

bool PinholeClient::App_AddApp(const QString & address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("add application"),
		[argument](HostClient* hostClient) { hostClient->addApplication(argument); });
}

bool PinholeClient::App_DeleteApp(const QString & address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("delete application"),
		[argument](HostClient* hostClient) { hostClient->deleteApplication(argument); });
}

bool PinholeClient::App_StartApps(const QString& address, int port, const QString& argument)
{
	return ExecServerCommand(address, port, tr("start apps"),
		[argument](HostClient* hostClient) { hostClient->startApps(SplitSemicolonString(argument)); });
}

bool PinholeClient::App_StartStartupApps(const QString& address, int port, const QString& argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("start startup apps"),
		[](HostClient* hostClient) { hostClient->startStartupApps(); });
}

bool PinholeClient::App_RestartApps(const QString & address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("restart apps"),
		[argument](HostClient* hostClient) { hostClient->restartApps(SplitSemicolonString(argument)); });
}

bool PinholeClient::App_StopApps(const QString& address, int port, const QString& argument)
{
	return ExecServerCommand(address, port, tr("stop apps"),
		[argument](HostClient* hostClient) { hostClient->stopApps(SplitSemicolonString(argument)); });
}

bool PinholeClient::App_StopAllApps(const QString& address, int port, const QString& argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("stop all apps"),
		[](HostClient* hostClient) { hostClient->stopAllApps(); });
}

bool PinholeClient::App_StartAppVariables(const QString& address, int port, const QString& argument)
{
	int colonPos = argument.indexOf(':');
	if (-1 == colonPos)
		return false;

	QString appName = argument.left(colonPos);
	QStringList variables = SplitSemicolonString(argument.mid(colonPos + 1));

	return ExecServerCommand(address, port, tr("start app with variables"),
		[appName, variables](HostClient* hostClient) { hostClient->startAppVariables(appName, variables); });
}

bool PinholeClient::Group_AddGroup(const QString& address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("add group"),
		[argument](HostClient* hostClient) { hostClient->addGroup(argument); });
}

bool PinholeClient::Group_DeleteGroup(const QString& address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("delete group"),
		[argument](HostClient* hostClient) { hostClient->deleteGroup(argument); });
}

bool PinholeClient::Group_StartGroup(const QString& address, int port, const QString& argument)
{
	return ExecServerCommand(address, port, tr("start group"),
		[argument](HostClient* hostClient) { hostClient->startGroup(argument); });
}

bool PinholeClient::Group_StopGroup(const QString& address, int port, const QString& argument)
{
	return ExecServerCommand(address, port, tr("stop group"),
		[argument](HostClient* hostClient) { hostClient->stopGroup(argument); });
}

bool PinholeClient::Global_Shutdown(const QString& address, int port, const QString& argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("shutdownhost "),
		[](HostClient* hostClient) { hostClient->shutdownHost(); });
}

bool PinholeClient::Gobal_Reboot(const QString& address, int port, const QString& argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("reboot host"),
		[](HostClient* hostClient) { hostClient->rebootHost(); });
}

bool PinholeClient::Global_SysInfo(const QString& address, int port, const QString & argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("retrieve system info"),
		[](HostClient* hostClient) { hostClient->retrieveSystemInfo(); });
}

bool PinholeClient::Schedule_AddEvent(const QString& address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("add event"),
		[argument](HostClient* hostClient) { hostClient->addScheduleEvent(argument); });
}

bool PinholeClient::Schedule_DeleteEvent(const QString& address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("delete event"),
		[argument](HostClient* hostClient) { hostClient->deleteScheduleEvent(argument); });
}

bool PinholeClient::Schedule_TriggerEvents(const QString& address, int port, const QString& argument)
{
	return ExecServerCommand(address, port, tr("trigger events"),
		[argument](HostClient* hostClient) { hostClient->triggerScheduleEvents(SplitSemicolonString(argument)); });
}

bool PinholeClient::Alert_AddSlot(const QString& address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("add alert slot"),
		[argument](HostClient* hostClient) { hostClient->addAlertSlot(argument); });
}

bool PinholeClient::Alert_DeleteSlot(const QString& address, int port, const QString & argument)
{
	return ExecServerCommand(address, port, tr("delete alert slot"),
		[argument](HostClient* hostClient) { hostClient->deleteAlertSlot(argument); });
}

bool PinholeClient::Alert_ResetCount(const QString& address, int port, const QString & argument)
{
	Q_UNUSED(argument);
	return ExecServerCommand(address, port, tr("reset alert count"),
		[](HostClient* hostClient) { hostClient->resetAlertCount(); });
}

bool PinholeClient::ExecServerCommand(const QString& hostAddress, int port, const QString& commandDescription,
	std::function<void(HostClient*)> func, const QString & outputFile)
{
	if (!m_silent)
	{
		qStdOut() << tr("Connecting to %1 port %2...").arg(hostAddress).arg(port) << endl;
	}

	HostClient* hostClient = new HostClient(hostAddress, port, QString(), false, false, this);

	bool success = false;

	connect(hostClient, &HostClient::connected,
		this, [this, hostClient, func]()
	{
		if (!m_silent)
		{
			qStdOut() << tr("Connected to %1, executing operation...").arg(hostClient->getHostName()) << endl;
		}

		if (m_showId)
		{
			qStdOut() << tr("Server ID: %1").arg(hostClient->getHostId()) << endl;
		}

		func(hostClient);
	});

	connect(hostClient, &HostClient::disconnected,
		this, [hostClient]()
	{
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::connectFailed,
		this, [this, hostClient, commandDescription]()
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Failed to connect to host %1")
				.arg(hostClient->getHostAddress())
				<< endl;
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::passwordFailure,
		this, [this, hostClient, commandDescription]()
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Incorrect password for host %1")
				.arg(hostClient->getHostAddress())
				<< endl;
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandSuccess,
		this, [hostClient, &success]()
	{
		success = true;
		qStdOut() << tr("success") << endl;
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandError,
		this, [this, hostClient, commandDescription]()
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("An error occured while attempting %1 on host %2")
				.arg(commandDescription)
				.arg(hostClient->getHostAddress())
				<< endl;
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandUnknown,
		this, [this, hostClient, commandDescription]()
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("The host %1 (%2) is unable to %3 because it doesn't know that command (host version %4)")
				.arg(hostClient->getHostAddress())
				.arg(hostClient->getHostName())
				.arg(commandDescription)
				.arg(hostClient->getHostVersion())
				<< endl;
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandMissing,
		this, [this, hostClient, commandDescription]()
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("The host %1 (%2) is unable to %3 because it doesn't know that command (host version %4)")
				.arg(hostClient->getHostAddress())
				.arg(hostClient->getHostName())
				.arg(commandDescription)
				.arg(hostClient->getHostVersion())
				<< endl;
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandData,
		this, [hostClient, outputFile](const QString& group, const QString& subCommand, const QVariant& data)
	{
		if (GROUP_NONE == group)
		{
			if (CMD_NONE_GETSCREENSHOT == subCommand)
			{
				qStdOut() << tr("Writing screenshot to %1").arg(outputFile) << endl;
				QFile exportFile(outputFile);
				if (!exportFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
				{
					qStdOut() << tr("Error opening output file: ") << exportFile.errorString() << endl;
				}
				else
				{
					exportFile.write(data.toByteArray());
				}
			}
			else if (CMD_NONE_EXPORTSETTINGS == subCommand)
			{
				qStdOut() << tr("Writing settings to %1").arg(outputFile) << endl;
				QFile exportFile(outputFile);
				if (!exportFile.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text))
				{
					qStdOut() << tr("Error opening output file: ") << exportFile.errorString() << endl;
				}
				else
				{
					exportFile.write(data.toByteArray());
				}
			}
			else if (CMD_NONE_RETRIEVELOG == subCommand)
			{
				qStdOut() << tr("Received log") << endl;
			}
			else if (CMD_ALERT_RETRIEVELIST == subCommand)
			{
				qStdOut() << tr("Recevied alert list") << endl;
			}
		}
		else if (GROUP_GLOBAL == group)
		{
			if (CMD_GLBOAL_SYSINFO == subCommand)
			{
				qStdOut() << QString::fromUtf8(qUncompress(data.toByteArray()));
			}
		}

		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::missingValue,
		this, [this, hostClient](const QString& group, const QString& item, const QString& property)
	{
		if (m_silent)
		{
			qStdOut() << tr("failure") << endl;
		}
		else
		{
			qStdOut() << tr("Missing property '%1' '%2' '%3'")
				.arg(group)
				.arg(item)
				.arg(property);
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::valueUpdate,
		this, [this, hostClient](const QString& group, const QString& item, const QString& property, const QVariant& value)
	{
		if (!m_silent)
		{
			qStdOut() << tr("Group '%1' item '%2' property '%3':")
				.arg(group)
				.arg(item)
				.arg(property)
				<< endl;
		}

		switch (value.type())
		{
		case QVariant::Bool:
			if (m_silent)
				qStdOut() << (value.toBool() ? tr("true") : tr("false")) << endl;
			else
				qStdOut() << "bool " << (value.toBool() ? tr("true") : tr("false")) << endl;
			break;

		case QVariant::Int:
			if (m_silent)
				qStdOut() << value.toInt() << endl;
			else
				qStdOut() << "int " << value.toInt() << endl;
			break;

		case QVariant::String:
			if (m_silent)
				qStdOut() << value.toString() << endl;
			else
				qStdOut() << "string '" << value.toString() << "'" << endl;
			break;

		case QVariant::StringList:
		case QVariant::List:
			if (m_silent)
			{
				for (const auto s : value.toStringList())
				{
					qStdOut() << s << endl;
				}
			}
			else
			{
				qStdOut() << "string list " << value.toStringList().length() << " entries:" << endl;
				for (const auto s : value.toStringList())
				{
					qStdOut() << "'" << s << "'" << endl;
				}
			}
			break;

		default:
			break;
		}
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::valueSet,
		this, [this, hostClient](const QString& group, const QString& item, const QString& property, bool success)
	{
		if (m_silent)
		{
			if (success)
			{
				qStdOut() << tr("success") << endl;
			}
			else
			{
				qStdOut() << tr("failure") << endl;
			}
		}
		else
		{
			qStdOut() << tr("Group '%1' item '%2' property '%3': %4")
				.arg(group)
				.arg(item)
				.arg(property)
				.arg(success ? "Property set" : "Error setting property")
				<< endl;
		}
		hostClient->deleteLater();
	});

	// Block until we receive a response from the server
	QEventLoop eventLoop(this);
	connect(hostClient, &HostClient::destroyed,
		&eventLoop, &QEventLoop::quit);
	eventLoop.exec();

	return success;
}
