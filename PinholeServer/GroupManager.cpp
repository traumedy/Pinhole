#include "GroupManager.h"
#include "Settings.h"
#include "AppManager.h"
#include "Logger.h"
#include "Values.h"
#include "../common/Utilities.h"

#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>

GroupManager::GroupManager(Settings* settings, AppManager* appManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_appManager(appManager)
{
	readGroupSettings();

	connect(appManager, &AppManager::valueChanged,
		this, &GroupManager::appManagerValueChanged);

	startStartupGroups();
}


GroupManager::~GroupManager()
{
}


bool GroupManager::readGroupSettings()
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	QStringList groupList = settings->value(SETTINGS_GROUPLIST).toStringList();

	for (const auto& groupName : groupList)
	{
		settings->beginGroup(SETTINGS_GROUPPREFIX + groupName);
		QSharedPointer<Group> newGroup = QSharedPointer<Group>::create(groupName);
		connect(newGroup.data(), &Group::valueChanged,
			this, &GroupManager::groupValueChanged);
		newGroup->setApplications(settings->value(PROP_GROUP_APPLICATIONS).toStringList());
		
		m_groupList[groupName] = newGroup;

		settings->endGroup();
	}

	return true;
}


bool GroupManager::writeGroupSettings() const
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

#ifdef QT_DEBUG
	//qDebug() << "Writing group settings to " << settings->fileName() << settings->isWritable();
#endif

	// Delete old entries
	QStringList groupList = settings->value(SETTINGS_GROUPLIST).toStringList();
	for (const auto& groupName : groupList)
	{
		settings->remove(SETTINGS_GROUPPREFIX + groupName);
	}

	for (const auto& group : m_groupList)
	{
		QString groupName = group->getName();
		settings->beginGroup(SETTINGS_GROUPPREFIX + groupName);
		settings->setValue(PROP_GROUP_NAME, groupName);
		settings->setValue(PROP_GROUP_APPLICATIONS, group->getApplications());
		settings->endGroup();
	}

	settings->setValue(SETTINGS_GROUPLIST, QStringList(m_groupList.keys()));

	return true;
}


bool GroupManager::importSettings(const QJsonObject& root)
{
	if (!root.contains(JSONTAG_GROUPS))
	{
		Logger(LOG_WARNING) << tr("JSON import data missing tag ") << JSONTAG_GROUPS;
		return false;
	}

	QJsonArray groupList = root[JSONTAG_GROUPS].toArray();

	if (root.contains(STATUSTAG_DELETEENTRIES) && root[STATUSTAG_DELETEENTRIES].toBool() == true)
	{
		// Delete entries
		for (const auto& group : groupList)
		{
			if (!group.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON group entry %1 is not object").arg(JSONTAG_GROUPS);
			}
			else
			{
				QJsonObject jgroup = group.toObject();
				QString name = jgroup[PROP_GROUP_NAME].toString();
				m_groupList.remove(name);
				Logger() << tr("Removing group '%1'").arg(name);
			}
		}
	}
	else
	{
		// Modify and add entries
		QStringList groupNameList;
		QStringList appList = m_appManager->getAppNames();

		for (const auto& group : groupList)
		{
			if (!group.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON group entry %1 is not object").arg(JSONTAG_GROUPS);
			}
			else
			{
				QJsonObject jgroup = group.toObject();

				QString name = jgroup[PROP_GROUP_NAME].toString();
				groupNameList.append(name);
				QSharedPointer<Group> newGroup;
				if (m_groupList.contains(name))
				{
					newGroup = m_groupList[name];
				}
				else
				{
					newGroup = QSharedPointer<Group>::create(name);
					connect(newGroup.data(), &Group::valueChanged,
						this, &GroupManager::groupValueChanged);
				}

				QStringList newApplications = ReadJsonValueWithDefault(jgroup, PROP_GROUP_APPLICATIONS, newGroup->getApplications()).toStringList();
				// Remove applications from list that do not exist
				for (const auto& appName : newApplications)
				{
					if (!appList.contains(appName))
					{
						Logger() << tr("Importing group '%1': Removing missing application '%2'")
							.arg(name)
							.arg(appName);
						newApplications.removeAll(appName);
					}
				}
				newGroup->setApplications(newApplications);
				m_groupList[name] = newGroup;
			}
		}

		if (!root.contains(STATUSTAG_NODELETE) || root[STATUSTAG_NODELETE].toBool() == false)
		{
			// Remove groups that are no longer in the list
			for (const auto& groupName : m_groupList.keys())
			{
				if (!groupNameList.contains(groupName))
				{
					m_groupList.remove(groupName);
				}
			}
		}
	}

	emit valueChanged(GROUP_GROUP, "", PROP_GROUP_LIST, QVariant(m_groupList.keys()));

	return true;
}


QJsonObject GroupManager::exportSettings() const
{
	QJsonArray jgroupList;

	for (const auto& group : m_groupList)
	{
		QJsonObject jgroup;
		jgroup[PROP_GROUP_NAME] = group->getName();
		jgroup[PROP_GROUP_APPLICATIONS] = QJsonArray::fromStringList(group->getApplications());

		jgroupList.append(jgroup);
	}

	QJsonObject root;
	root[JSONTAG_GROUPS] = jgroupList;
	root[STATUSTAG_NODELETE] = false;
	root[STATUSTAG_DELETEENTRIES] = false;

	return root;
}


QStringList GroupManager::getGroupList() const
{
	return m_groupList.keys();
}


QVariant GroupManager::getGroupVariant(const QString& groupName, const QString& propName) const
{
	if (groupName.isEmpty())
	{
		if (PROP_GROUP_LIST == propName)
		{
			return QVariant(QStringList(m_groupList.keys()));
		}
		else
		{
			return QVariant();
		}
	}

	if (!m_groupList.contains(groupName))
	{
		Logger(LOG_WARNING) << tr("Missing group for query: ") << groupName;
		return QVariant();
	}

	if (m_groupStringCallMap.find(propName) != m_groupStringCallMap.end())
	{
		return QVariant(std::get<0>(m_groupStringCallMap.at(propName))(m_groupList[groupName].data()));
	}
	else if (m_groupBoolCallMap.find(propName) != m_groupBoolCallMap.end())
	{
		return QVariant(std::get<0>(m_groupBoolCallMap.at(propName))(m_groupList[groupName].data()));
	}
	else if (m_groupStringListCallMap.find(propName) != m_groupStringListCallMap.end())
	{
		return QVariant(std::get<0>(m_groupStringListCallMap.at(propName))(m_groupList[groupName].data()));
	}

	Logger(LOG_WARNING) << tr("Missing property for group value query: ") << propName;
	return QVariant();
}


bool GroupManager::setGroupVariant(const QString& groupName, const QString& propName, const QVariant& value) const
{
	if (!m_groupList.contains(groupName))
	{
		Logger(LOG_WARNING) << tr("Missing group for set: ") << groupName;
		return false;
	}

	if (m_groupStringCallMap.find(propName) != m_groupStringCallMap.end())
	{
		if (!value.canConvert(QVariant::String))
		{
			Logger(LOG_WARNING) << tr("Variant for group value set is not string: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_groupStringCallMap.at(propName))(m_groupList[groupName].data(), value.toString());
	}
	else if (m_groupBoolCallMap.find(propName) != m_groupBoolCallMap.end())
	{
		if (!value.canConvert(QVariant::Bool))
		{
			Logger(LOG_WARNING) << tr("Variant for group value set is not bool: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_groupBoolCallMap.at(propName))(m_groupList[groupName].data(), value.toBool());
	}
	else if (m_groupStringListCallMap.find(propName) != m_groupStringListCallMap.end())
	{
		if (!value.canConvert(QVariant::StringList))
		{
			Logger(LOG_WARNING) << tr("Variant for group value set is not stringlist: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_groupStringListCallMap.at(propName))(m_groupList[groupName].data(), value.toStringList());
	}
	else
	{
		Logger(LOG_WARNING) << tr("Missing property for group value set: ") << propName;
		return false;
	}
}


void GroupManager::groupValueChanged(const QString& property, const QVariant& value)
{
	QString groupName = sender()->property("groupName").toString();
	emit valueChanged(GROUP_GROUP, groupName, property, value);
}


bool GroupManager::addGroup(const QString& name)
{
	if (m_groupList.contains(name))
		return false;

	m_groupList[name] = QSharedPointer<Group>::create(name);
	connect(m_groupList[name].data(), &Group::valueChanged,
		this, &GroupManager::groupValueChanged);

	emit valueChanged(GROUP_GROUP, "", PROP_GROUP_LIST, QVariant(m_groupList.keys()));
	return true;
}


bool GroupManager::deleteGroup(const QString& name)
{
	if (!m_groupList.contains(name))
		return false;

	m_groupList.remove(name);

	emit valueChanged(GROUP_GROUP, "", PROP_GROUP_LIST, QVariant(m_groupList.keys()));
	return true;
}


bool GroupManager::renameGroup(const QString& oldName, const QString& newName)
{
	if (!m_groupList.contains(oldName))
		return false;

	QSharedPointer<Group> group = *m_groupList.find(oldName);
	m_groupList.remove(oldName);
	group->setName(newName);
	m_groupList[newName] = group;

	emit valueChanged(GROUP_GROUP, "", PROP_GROUP_LIST, QVariant(m_groupList.keys()));
	return true;
}


bool GroupManager::startGroup(const QString& groupName) const
{
	if (!m_groupList.contains(groupName))
		return false;

	QStringList appList = m_groupList[groupName]->getApplications();

	return m_appManager->startApps(appList);
}


bool GroupManager::stopGroup(const QString& groupName) const
{
	if (!m_groupList.contains(groupName))
		return false;

	QStringList appList = m_groupList[groupName]->getApplications();

	return m_appManager->stopApps(appList);
}


bool GroupManager::switchToApp(const QString& appName) const
{
	// Find the groups this app is a member of and stop them
	QStringList groupList = getGroupList();
	for (const auto& group : groupList)
	{
		QStringList groupMembers = getGroupVariant(group, PROP_GROUP_APPLICATIONS).toStringList();
		if (groupMembers.contains(appName))
		{
			stopGroup(group);
		}
	}

	// Start the app
	return m_appManager->startApps(QStringList(appName));
}


bool GroupManager::startStartupGroups() const
{
	bool ret = true;

	// Start all applications in groups marked as 'launch at start
	QStringList groupList = getGroupList();
	for (const auto& group : groupList)
	{
		if (m_groupList[group]->getLaunchAtStart())
		{

			if (!m_appManager->startApps(m_groupList[group]->getApplications()))
				ret = false;
		}
	}

	return ret;
}


void GroupManager::appManagerValueChanged(const QString& group, const QString& item, const QString& prop, const QVariant& value)
{
	if (GROUP_APP == group)
	{
		if (item.isEmpty())
		{
			if (PROP_APP_LIST == prop)
			{
				// An app may have been deleted, check all the groups and make sure the 
				// apps in their application lists are stil there, remove if not
				QStringList appList = value.toStringList();
				for (const auto& group : m_groupList.keys())
				{
					auto groupAppList = m_groupList[group]->getApplications();
					for (const auto& app : groupAppList)
					{
						if (!appList.contains(app))
						{
							groupAppList.removeAt(groupAppList.indexOf(app));
							m_groupList[group]->setApplications(groupAppList);
						}
					}
				}
			}
		}
		else if (PROP_APP_NAME == prop)
		{
			// An app was renamed, check all the groups and replace it if it is in their application list
			QString newName = value.toString();
			for (const auto& group : m_groupList.keys())
			{
				auto appList = m_groupList[group]->getApplications();
				if (appList.contains(item))
				{
					appList.removeAt(appList.indexOf(item));
					appList.append(newName);
					m_groupList[group]->setApplications(appList);
				}
			}
		}
	}
}

