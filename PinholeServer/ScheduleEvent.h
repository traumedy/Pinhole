#pragma once

#include "../common/PinholeCommon.h"

#include <QObject>
#include <QDateTime>


class ScheduleEvent : public QObject
{
	Q_OBJECT

public:
	ScheduleEvent(const QString& name, QObject *parent = nullptr);
	~ScheduleEvent();

	QString getName() const;
	bool setName(const QString& str);
	QString getType() const;
	bool setType(const QString& str);
	QString getFrequency() const;
	bool setFrequency(const QString& str);
	QString getArguments() const;
	bool setArguments(const QString& str);
	int getOffset() const;
	bool setOffset(int val);
	QString getLastTriggeredString() const;
	QDateTime getLastTriggered() const;
	bool setLastTriggered(const QDateTime& time);

	void trigger();

signals:
	void valueChanged(const QString&, const QVariant&);
	void triggered();


private:
	const QStringList m_validSchedTypes = 
	{
		SCHED_TYPE_STARTAPPS,
		SCHED_TYPE_STOPAPPS,
		SCHED_TYPE_RESTARTAPPS,
		SCHED_TYPE_STARTGROUP,
		SCHED_TYPE_STOPGROUP,
		SCHED_TYPE_SHUTDOWN,
		SCHED_TYPE_REBOOT,
		SCHED_TYPE_SCREENSHOT,
		SCHED_TYPE_TRIGGEREVENTS,
		SCHED_TYPE_ALERT
	};

	const QStringList m_validSchedFrequencies =
	{
		SCHED_FREQ_DISABLED,
		SCHED_FREQ_WEEKLY,
		SCHED_FREQ_DAILY,
		SCHED_FREQ_HOURLY,
		SCHED_FREQ_ONCE
	};

	QString m_name = "";
	QString m_type = SCHED_TYPE_STARTAPPS;
	QString m_frequency = SCHED_FREQ_DISABLED;
	QString m_arguments = "";
	int m_offset = 0;
	// Offset is the number of minutes from origin:
	// Weekly: Midnight Sunday
	// Daily: Midnight
	// Hourly: Top of the hour
	QDateTime m_lastTriggered;

};
