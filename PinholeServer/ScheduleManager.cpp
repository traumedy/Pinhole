#include "ScheduleManager.h"
#include "Settings.h"
#include "AppManager.h"
#include "GroupManager.h"
#include "GlobalManager.h"
#include "Logger.h"
#include "Values.h"
#include "../common/Utilities.h"
#include "../common/HostClient.h"

#include <QSettings>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


ScheduleManager::ScheduleManager(Settings* settings, AppManager* appManager, 
	GroupManager* groupManager, GlobalManager* globalManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_appManager(appManager), 
	m_groupManager(groupManager), m_globalManager(globalManager)
{

	readScheduleSettings();

	// Setup the timer that will execute the schedule events
	m_triggerTimer.setInterval(1000 * 60); // Once a minute
	m_triggerTimer.setSingleShot(false);
	connect(&m_triggerTimer, &QTimer::timeout,
		this, &ScheduleManager::triggerTimerCallback);
	m_triggerTimer.start();
}


ScheduleManager::~ScheduleManager()
{
}


void ScheduleManager::triggerTimerCallback()
{
	QDateTime now = QDateTime::currentDateTime();

	for (const auto& event : m_scheduleList)
	{
		QString frequency = event->getFrequency();
		int offset = event->getOffset();
		bool triggerEvent = false;
		if (SCHED_FREQ_ONCE == frequency)
		{
			QDateTime eventTime;
			eventTime.setSecsSinceEpoch(static_cast<qint64>(offset) * 60);

			if (eventTime.date() == now.date() &&
				eventTime.time().hour() == now.time().hour() &&
				eventTime.time().minute() == now.time().minute())
			{
				triggerEvent = true;
			}
		}
		else if (SCHED_FREQ_HOURLY == frequency)
		{
			if (now.time().minute() == offset % 60)
			{
				triggerEvent = true;
			}
		}
		else if (SCHED_FREQ_DAILY == frequency)
		{
			if (now.time().hour() == (offset % 1440) / 60 &&
				now.time().minute() == offset % 60)
			{
				triggerEvent = true;
			}
		}
		else if (SCHED_FREQ_WEEKLY == frequency)
		{
			if (now.date().dayOfWeek() - 1 == (offset % 10080) / 1440 &&
				now.time().hour() == (offset % 1440) / 60 &&
				now.time().minute() == offset % 60)
			{
				triggerEvent = true;
			}
		}

		if (triggerEvent)
		{
			event->trigger();
		}
	}

}


QStringList ScheduleManager::getEventNames() const
{
	return m_scheduleList.keys();
}


bool ScheduleManager::readScheduleSettings()
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	QStringList eventList = settings->value(SETTINGS_EVENTLIST).toStringList();
	for (const auto& eventName : eventList)
	{
		settings->beginGroup(SETTINGS_EVENTPREFIX + eventName);

		QSharedPointer<ScheduleEvent> newEvent = QSharedPointer<ScheduleEvent>::create(eventName);
		newEvent->setType(settings->value(PROP_SCHED_TYPE, SCHED_TYPE_STARTAPPS).toString());
		newEvent->setFrequency(settings->value(PROP_SCHED_FREQUENCY, SCHED_FREQ_DISABLED).toString());
		newEvent->setArguments(settings->value(PROP_SCHED_ARGUMENTS, "").toString());
		newEvent->setOffset(settings->value(PROP_SCHED_OFFSET, 0).toInt());

		if (settings->contains(PROP_SCHED_LASTTRIGGERED))
			newEvent->setLastTriggered(QDateTime::fromString(settings->value(PROP_SCHED_LASTTRIGGERED).toString()));

		connect(newEvent.data(), &ScheduleEvent::valueChanged,
			this, &ScheduleManager::eventValueChanged);
		connect(newEvent.data(), &ScheduleEvent::triggered,
			this, &ScheduleManager::eventTriggered);

		m_scheduleList[eventName] = newEvent;

		settings->endGroup();
	}

	return true;
}


bool ScheduleManager::writeScheduleSettings() const
{
#ifdef QT_DEBUG
	//qDebug() << "Writing schedule settings to " << settings->fileName() << settings->isWritable();
#endif

	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	// Delete old entries
	QStringList eventList = settings->value(SETTINGS_EVENTLIST).toStringList();
	for (const auto& eventName : eventList)
	{
		settings->remove(SETTINGS_EVENTPREFIX + eventName);
	}

	for (const auto& event : m_scheduleList)
	{
		QString eventName = event->getName();
		settings->beginGroup(SETTINGS_EVENTPREFIX + eventName);
		settings->setValue(PROP_SCHED_NAME, eventName);
		settings->setValue(PROP_SCHED_TYPE, event->getType());
		settings->setValue(PROP_SCHED_FREQUENCY, event->getFrequency());
		settings->setValue(PROP_SCHED_ARGUMENTS, event->getArguments());
		settings->setValue(PROP_SCHED_OFFSET, event->getOffset());

		QDateTime time;
		time = event->getLastTriggered();
		if (!time.isNull())
			settings->setValue(PROP_SCHED_LASTTRIGGERED, time.toString());

		settings->endGroup();
	}

	settings->setValue(SETTINGS_EVENTLIST, QStringList(m_scheduleList.keys()));

	return true;
}


bool ScheduleManager::importSettings(const QJsonObject& root)
{
	if (!root.contains(JSONTAG_EVENTS))
	{
		Logger(LOG_WARNING) << tr("JSON import data missing tag ") << JSONTAG_EVENTS;
		return false;
	}

	QJsonArray eventList = root[JSONTAG_EVENTS].toArray();

	if (root.contains(STATUSTAG_DELETEENTRIES) && root[STATUSTAG_DELETEENTRIES].toBool() == true)
	{
		// Delete entries
		for (const auto& event : eventList)
		{
			if (!event.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON import data event entry %1 is not object").arg(JSONTAG_EVENTS);
			}
			else
			{
				QJsonObject jevent = event.toObject();
				QString name = jevent[PROP_SCHED_NAME].toString();
				m_scheduleList.remove(name);
				Logger() << tr("Removing event '%1'").arg(name);
			}
		}
	}
	else
	{
		// Modify and add entries
		QStringList eventNameList;

		for (const auto& event : eventList)
		{
			if (!event.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON import data event entry %1 is not object").arg(JSONTAG_EVENTS);
			}
			else
			{
				QJsonObject jevent = event.toObject();
				QString name = jevent[PROP_SCHED_NAME].toString();
				eventNameList.append(name);
				QSharedPointer<ScheduleEvent> newEvent;
				if (m_scheduleList.contains(name))
				{
					newEvent = m_scheduleList[name];
				}
				else
				{
					newEvent = QSharedPointer<ScheduleEvent>::create(name);
					connect(newEvent.data(), &ScheduleEvent::valueChanged,
						this, &ScheduleManager::eventValueChanged);
					connect(newEvent.data(), &ScheduleEvent::triggered,
						this, &ScheduleManager::eventTriggered);
				}
				newEvent->setType(ReadJsonValueWithDefault(jevent, PROP_SCHED_TYPE, newEvent->getType()).toString());
				newEvent->setFrequency(ReadJsonValueWithDefault(jevent, PROP_SCHED_FREQUENCY, newEvent->getFrequency()).toString());
				newEvent->setArguments(ReadJsonValueWithDefault(jevent, PROP_SCHED_ARGUMENTS, newEvent->getArguments()).toString());
				newEvent->setOffset(ReadJsonValueWithDefault(jevent, PROP_SCHED_OFFSET, newEvent->getOffset()).toInt());

				if (jevent.contains(PROP_SCHED_LASTTRIGGERED))
					newEvent->setLastTriggered(QDateTime::fromString(jevent[PROP_SCHED_LASTTRIGGERED].toString()));

				m_scheduleList[name] = newEvent;
			}
		}

		if (!root.contains(STATUSTAG_NODELETE) || root[STATUSTAG_NODELETE].toBool() == false)
		{
			// Remove entries that are no longer in the list
			for (const auto& eventName : m_scheduleList.keys())
			{
				if (!eventNameList.contains(eventName))
				{
					m_scheduleList.remove(eventName);
				}
			}
		}
	}

	emit valueChanged(GROUP_SCHEDULE, "", PROP_SCHED_LIST, QVariant(m_scheduleList.keys()));

	return true;
}


QJsonObject ScheduleManager::exportSettings() const
{
	QJsonArray jeventList;

	for (const auto& event : m_scheduleList)
	{
		QJsonObject jevent;
		jevent[PROP_SCHED_NAME] = event->getName();
		jevent[PROP_SCHED_TYPE] = event->getType();
		jevent[PROP_SCHED_FREQUENCY] = event->getFrequency();
		jevent[PROP_SCHED_ARGUMENTS] = event->getArguments();
		jevent[PROP_SCHED_OFFSET] = event->getOffset();

		QDateTime time;
		time = event->getLastTriggered();
		if (!time.isNull())
			jevent[PROP_SCHED_LASTTRIGGERED] = time.toString();

		jeventList.append(jevent);
	}

	QJsonObject root;
	root[JSONTAG_EVENTS] = jeventList;
	root[STATUSTAG_NODELETE] = false;
	root[STATUSTAG_DELETEENTRIES] = false;

	return root;
}


bool ScheduleManager::addEvent(const QString& name)
{
	if (m_scheduleList.contains(name))
	{
		return false;
	}

	m_scheduleList[name] = QSharedPointer<ScheduleEvent>::create(name);

	connect(m_scheduleList[name].data(), &ScheduleEvent::valueChanged,
		this, &ScheduleManager::eventValueChanged);
	connect(m_scheduleList[name].data(), &ScheduleEvent::triggered,
		this, &ScheduleManager::eventTriggered);

	emit valueChanged(GROUP_SCHEDULE, "", PROP_SCHED_LIST, QVariant(m_scheduleList.keys()));

	return true;
}


bool ScheduleManager::deleteEvent(const QString& name)
{
	if (!m_scheduleList.contains(name))
	{
		return false;
	}

	m_scheduleList.remove(name);

	emit valueChanged(GROUP_SCHEDULE, "", PROP_SCHED_LIST, QVariant(m_scheduleList.keys()));

	return true;
}


bool ScheduleManager::renameEvent(const QString& oldName, const QString& newName)
{
	if (!m_scheduleList.contains(oldName))
		return false;

	// Move pointer to new map key index
	QSharedPointer<ScheduleEvent> event = *m_scheduleList.find(oldName);
	m_scheduleList.remove(oldName);
	event->setName(newName);
	m_scheduleList[newName] = event;

	emit valueChanged(GROUP_SCHEDULE, "", PROP_SCHED_LIST, QVariant(m_scheduleList.keys()));
	return true;
}


bool ScheduleManager::triggerEvents(const QStringList& names) const
{
	bool ret = true;
	for (const auto& name : names)
	{
		if (!m_scheduleList.contains(name))
		{
			ret = false;
		}
		else
		{
			m_scheduleList[name]->trigger();
		}
	}

	return ret;
}


QVariant ScheduleManager::getEventVariant(const QString& eventName, const QString& propName) const
{
	if (eventName.isEmpty())
	{
		if (PROP_SCHED_LIST == propName)
		{
			return getEventNames();
		}
		else
		{
			return QVariant();
		}
	}

	if (!m_scheduleList.contains(eventName))
	{
		Logger(LOG_WARNING) << tr("Missing event for query: ") << eventName;
		return QVariant();
	}

	if (m_eventStringCallMap.find(propName) != m_eventStringCallMap.end())
	{
		return QVariant(std::get<0>(m_eventStringCallMap.at(propName))(m_scheduleList[eventName].data()));
	}
	else if (m_eventIntCallMap.find(propName) != m_eventIntCallMap.end())
	{
		return QVariant(std::get<0>(m_eventIntCallMap.at(propName))(m_scheduleList[eventName].data()));
	}

	Logger(LOG_WARNING) << tr("Missing property for event value query: ") << propName;

	return QVariant();
}


bool ScheduleManager::setEventVariant(const QString& eventName, const QString& propName, const QVariant& value) const
{
	if (!m_scheduleList.contains(eventName))
	{
		Logger(LOG_WARNING) << tr("Missing event for query: ") << eventName;
		return false;
	}

	if (m_eventStringCallMap.find(propName) != m_eventStringCallMap.end())
	{
		if (!value.canConvert(QVariant::String))
		{
			Logger(LOG_WARNING) << tr("Variant for scheduled event value set is not string: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_eventStringCallMap.at(propName))(m_scheduleList[eventName].data(), value.toString());
	}
	else if (m_eventIntCallMap.find(propName) != m_eventIntCallMap.end())
	{
		if (!value.canConvert(QVariant::Int))
		{
			Logger(LOG_WARNING) << tr("Variant for scheduled event value set is not int: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_eventIntCallMap.at(propName))(m_scheduleList[eventName].data(), value.toInt());
	}
	else
	{
		Logger(LOG_WARNING) << tr("Missing property for event value set: ") << propName;
		return false;
	}
}


void ScheduleManager::eventValueChanged(const QString& property, const QVariant& value) const
{
	QString eventName = sender()->property(PROP_SCHED_NAME).toString();
	emit valueChanged(GROUP_SCHEDULE, eventName, property, value);
}


void ScheduleManager::eventTriggered()
{
	// Reset global idle timer
	m_settings->resetIdle();

	ScheduleEvent* event = qobject_cast<ScheduleEvent*>(sender());

	QString eventType = event->getType();

	if (SCHED_TYPE_STARTAPPS == eventType)
	{
		QStringList appList = SplitSemicolonString(event->getArguments());
		if (!appList.isEmpty())
		{
			m_appManager->startApps(appList);
		}
	}
	else if (SCHED_TYPE_STOPAPPS == eventType)
	{
		QStringList appList = SplitSemicolonString(event->getArguments());
		if (!appList.isEmpty())
		{
			m_appManager->stopApps(appList);
		}
	}
	else if (SCHED_TYPE_RESTARTAPPS == eventType)
	{
		QStringList appList = SplitSemicolonString(event->getArguments());
		if (!appList.isEmpty())
		{
			m_appManager->restartApps(appList);
		}
	}
	else if (SCHED_TYPE_STARTGROUP == eventType)
	{
		QString groupName = event->getArguments();
		m_groupManager->startGroup(groupName);
	}
	else if (SCHED_TYPE_STOPGROUP == eventType)
	{
		QString groupName = event->getArguments();
		m_groupManager->stopGroup(groupName);
	}
	else if (SCHED_TYPE_SHUTDOWN == eventType)
	{
		QString logMessage = event->getArguments();
		Logger(LOG_ALWAYS) << tr("Scheduled shutdown event: ") << logMessage;
		m_globalManager->shutdown();
	}
	else if (SCHED_TYPE_REBOOT == eventType)
	{
		QString logMessage = event->getArguments();
		Logger(LOG_ALWAYS) << tr("Scheduled reboot event: ") << logMessage;
		m_globalManager->reboot();
	}
	else if (SCHED_TYPE_SCREENSHOT == eventType)
	{
		takeScreenshot(event->getName(), event->getArguments());
	}
	else if (SCHED_TYPE_TRIGGEREVENTS == eventType)
	{
		QStringList eventList = SplitSemicolonString(event->getArguments());
		if (!eventList.isEmpty())
		{
			// Prevent recursive triggers
			if (eventList.contains(event->getName()))
			{
				Logger(LOG_WARNING) << tr("Event %1: Recursive trigger: '%2'")
					.arg(event->getName())
					.arg(event->getArguments());
				eventList.removeAll(event->getName());
			}
			triggerEvents(eventList);
		}
	}
	else if (SCHED_TYPE_ALERT == eventType)
	{
		QString alertText = QString("[Event %1] %2")
			.arg(event->getName())
			.arg(event->getArguments());

		emit generateAlert(alertText);
	}
}


void ScheduleManager::takeScreenshot(const QString& eventName, const QString& filename)
{
	HostClient* hostClient = new HostClient("127.0.0.1", HOST_TCPPORT, QString(), true, false, this);

	connect(hostClient, &HostClient::connected,
		this, [hostClient]()
	{
		// Request screenshot
		hostClient->requestScreenshot();
	});

	connect(hostClient, &HostClient::connectFailed,
		this, [hostClient, eventName](const QString& reason)
	{
		Logger(LOG_WARNING) << tr("Event %1: Failed to aquire screenshot (failed to connect to localhost? %2)")
			.arg(eventName)
			.arg(reason);
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandError,
		this, [hostClient, eventName]()
	{
		Logger(LOG_WARNING) << tr("Event %1: Failed to aquire screenshot (command error, helper not running?)")
			.arg(eventName);
		hostClient->deleteLater();
	});

	
	QString newFilename;
	if (filename.isEmpty())
	{
		// If no filename was supplied, build one from the event name
		newFilename = m_settings->dataDir() + eventName + "-" + currentDateTimeFilenameString() + ".png";
	}
	else
	{
		// Replace %DATE% with date/time stamp
		newFilename = filename;
		newFilename.replace(QString("%DATE%"), currentDateTimeFilenameString());
	}

	connect(hostClient, &HostClient::commandData,
		this, [hostClient, eventName, newFilename](const QString& group, const QString& subCommand, const QVariant& data)
	{
		if (GROUP_NONE != group || CMD_NONE_GETSCREENSHOT != subCommand)
		{
			Logger(LOG_WARNING) << tr("Event %1: Failed to aquire screenshot (internal error, wrong data from server)")
				.arg(eventName);
		}
		else
		{
			// Write screenshot to file
		
			QFile outfile(newFilename);
			if (!outfile.open(QIODevice::WriteOnly))
			{
				Logger(LOG_WARNING) << tr("Event %1: Failed to create screenshot file %2")
					.arg(eventName )
					.arg(newFilename);
			}
			else
			{
				outfile.write(data.toByteArray());
				outfile.close();
				Logger() << tr("Event %1: Screenshot written to %2")
					.arg(eventName)
					.arg(newFilename);
			}
		}

		hostClient->deleteLater();
	});
}


void ScheduleManager::triggerEventsSlot(const QStringList eventNames) const
{
	triggerEvents(eventNames);
}

