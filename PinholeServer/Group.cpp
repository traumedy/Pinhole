#include "Group.h"
#include "../common/PinholeCommon.h"

#include <QVariant>

Group::Group(QString name, QObject *parent)
	: QObject(parent), m_name(name)
{
	setProperty("groupName", m_name);
}


Group::~Group()
{
}


QString Group::getName() const
{
	return m_name;
}


void Group::setName(const QString& str)
{
	emit valueChanged(PROP_GROUP_NAME, str);
	m_name = str;
	setProperty("groupName", m_name);
}


QStringList Group::getApplications() const
{
	return m_applications;
}


bool Group::setApplications(const QStringList& list)
{
	if (m_applications != list)
	{
		m_applications = list;
		emit valueChanged(PROP_GROUP_APPLICATIONS, QVariant(m_applications));
	}
	return true;
}


bool Group::getLaunchAtStart() const
{
	return m_launchAtStart;
}


bool Group::setLaunchAtStart(bool set)
{
	if (m_launchAtStart != set)
	{
		m_launchAtStart = set;
		emit valueChanged(PROP_GROUP_LAUNCHATSTART, QVariant(m_launchAtStart));
	}
	return true;
}
