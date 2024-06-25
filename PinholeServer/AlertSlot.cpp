#include "AlertSlot.h"
#include "Logger.h"

#include <QVariant>

AlertSlot::AlertSlot(const QString& name, QObject *parent)
	: QObject(parent), m_name(name)
{
	setProperty(PROP_ALERT_SLOTNAME, m_name);
}


AlertSlot::~AlertSlot()
{
}


QString AlertSlot::getName() const
{
	return m_name;
}


void AlertSlot::setName(const QString& name)
{
	m_name = name;
	setProperty(PROP_ALERT_SLOTNAME, m_name);
}


bool AlertSlot::getEnabled() const
{
	return m_enabled;
}


bool AlertSlot::setEnabled(bool b)
{
	if (m_enabled != b)
	{
		m_enabled = b;
		emit valueChanged(PROP_ALERT_SLOTENABLED, QVariant(m_enabled));
	}
	return true;
}


QString AlertSlot::getType() const
{
	return m_type;
}


bool AlertSlot::setType(const QString& str)
{
	if (m_type != str)
	{
		if (!m_validAlertSlotTypes.contains(str))
		{
			Logger(LOG_EXTRA) << tr("Invalid alert slot type value '%1'").arg(str);
			emit valueChanged(PROP_ALERT_SLOTTYPE, QVariant(m_type));
			return false;
		}
		m_type = str;
		emit valueChanged(PROP_ALERT_SLOTTYPE, QVariant(m_type));
	}
	return true;
}


QString AlertSlot::getArguments() const
{
	return m_arguments;
}


bool AlertSlot::setArguments(const QString& str)
{
	if (m_arguments != str)
	{
		m_arguments = str;
		emit valueChanged(PROP_ALERT_SLOTARG, QVariant(m_arguments));
	}
	return true;
}

