#pragma once

#include "ScheduleEvent.h"

#include <QObject>
#include <QTimer>
#include <QMap>

class Settings;
class AppManager;
class GroupManager;
class GlobalManager;

class ScheduleManager : public QObject
{
	Q_OBJECT

public:
	ScheduleManager(Settings* settings, AppManager* appManager, GroupManager* groupManager, 
		GlobalManager* globalManager, QObject *parent = nullptr);
	~ScheduleManager();

	QStringList getEventNames() const;
	bool readScheduleSettings();
	bool writeScheduleSettings() const;
	bool importSettings(const QJsonObject& root);
	QJsonObject exportSettings() const;
	bool addEvent(const QString& name);
	bool deleteEvent(const QString& name);
	bool renameEvent(const QString& oldName, const QString& newName);
	bool triggerEvents(const QStringList& name) const;
	QVariant getEventVariant(const QString& eventName, const QString& propName) const;
	bool setEventVariant(const QString& eventName, const QString& propName, const QVariant& value) const;

signals:
	void valueChanged(const QString&, const QString&, const QString&, const QVariant&) const;
	void generateAlert(const QString& text);

public slots:
	void triggerTimerCallback();
	void eventValueChanged(const QString&, const QVariant&) const;
	void eventTriggered();
	void triggerEventsSlot(const QStringList eventNames) const;


private:
	void takeScreenshot(const QString& eventName, const QString& filename);

	const std::map<QString, std::pair<std::function<QString(ScheduleEvent*)>, std::function<bool(ScheduleEvent*, const QString&)>>> m_eventStringCallMap =
	{
		{ PROP_SCHED_NAME, { &ScheduleEvent::getName, &ScheduleEvent::setName } },
		{ PROP_SCHED_TYPE, { &ScheduleEvent::getType, &ScheduleEvent::setType } },
		{ PROP_SCHED_FREQUENCY, { &ScheduleEvent::getFrequency, &ScheduleEvent::setFrequency } },
		{ PROP_SCHED_ARGUMENTS, { &ScheduleEvent::getArguments, &ScheduleEvent::setArguments } },
		{ PROP_SCHED_LASTTRIGGERED, { &ScheduleEvent::getLastTriggeredString, nullptr } },
	};

	const std::map<QString, std::pair<std::function<int(ScheduleEvent*)>, std::function<bool(ScheduleEvent*, int)>>> m_eventIntCallMap =
	{
		{ PROP_SCHED_OFFSET, { &ScheduleEvent::getOffset, &ScheduleEvent::setOffset } }
	};

	QTimer m_triggerTimer;
	QMap<QString, QSharedPointer<ScheduleEvent>> m_scheduleList;
	Settings * m_settings = nullptr;
	AppManager * m_appManager = nullptr;
	GroupManager * m_groupManager = nullptr;
	GlobalManager * m_globalManager = nullptr;
};
