#include "ScheduleEvent.h"
#include "Logger.h"
#include "../common/PinholeCommon.h"


ScheduleEvent::ScheduleEvent(const QString& name, QObject *parent)
	: QObject(parent), m_name(name)
{
	setProperty(PROP_SCHED_NAME, m_name);
}


ScheduleEvent::~ScheduleEvent()
{
}


QString ScheduleEvent::getName() const
{
	return m_name;
}


bool ScheduleEvent::setName(const QString& str)
{
	m_name = str;
	setProperty(PROP_SCHED_NAME, m_name);
	return true;
}


QString ScheduleEvent::getType() const
{
	return m_type;
}


bool ScheduleEvent::setType(const QString& str)
{
	if (m_type != str)
	{
		if (!m_validSchedTypes.contains(str))
		{
			Logger(LOG_EXTRA) << tr("Invalid scheduled event type value '%1'").arg(str);
			emit valueChanged(PROP_SCHED_TYPE, QVariant(m_type));
			return false;
		}
		m_type = str;
		emit valueChanged(PROP_SCHED_TYPE, QVariant(m_type));
	}
	return true;
}


QString ScheduleEvent::getFrequency() const
{
	return m_frequency;
}


bool ScheduleEvent::setFrequency(const QString& str)
{
	if (m_frequency != str)
	{
		if (!m_validSchedFrequencies.contains(str))
		{
			Logger(LOG_EXTRA) << tr("Invalid scheduled event frequency value '%1'").arg(str);
			emit valueChanged(PROP_SCHED_FREQUENCY, QVariant(m_frequency));
			return false;
		}
		m_frequency = str;
		emit valueChanged(PROP_SCHED_FREQUENCY, QVariant(m_frequency));
	}
	return true;
}


QString ScheduleEvent::getArguments() const
{
	return m_arguments;
}


bool ScheduleEvent::setArguments(const QString& str)
{
	if (m_arguments != str)
	{
		m_arguments = str;
		emit valueChanged(PROP_SCHED_ARGUMENTS, QVariant(m_arguments));
	}
	return true;
}


int ScheduleEvent::getOffset() const
{
	return m_offset;
}


bool ScheduleEvent::setOffset(int val)
{
	if (m_offset != val)
	{
		m_offset = val;
		emit valueChanged(PROP_SCHED_OFFSET, QVariant(m_offset));
	}
	return true;
}


QString ScheduleEvent::getLastTriggeredString() const
{
	return m_lastTriggered.toString();
}


QDateTime ScheduleEvent::getLastTriggered() const
{
	return m_lastTriggered;
}


bool ScheduleEvent::setLastTriggered(const QDateTime& time)
{
	m_lastTriggered = time;
	emit valueChanged(PROP_SCHED_LASTTRIGGERED, QVariant(m_lastTriggered.toString()));
	return true;
}


void ScheduleEvent::trigger()
{
	Logger() << tr("Event %1: Triggered; type: %2; arguments: %3")
		.arg(m_name)
		.arg(getType())
		.arg(getArguments());

	setLastTriggered(QDateTime::currentDateTime());

	emit triggered();
}

