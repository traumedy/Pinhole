#pragma once

#include "Group.h"
#include "../common/PinholeCommon.h"

#include <QObject>
#include <QMap>

class Settings;
class AppManager;

class GroupManager : public QObject
{
	Q_OBJECT

public:
	GroupManager(Settings* settings, AppManager* appManager, QObject *parent = nullptr);
	~GroupManager();

	bool readGroupSettings();
	bool writeGroupSettings() const;
	bool importSettings(const QJsonObject& root);
	QJsonObject exportSettings() const;
	QStringList getGroupList() const;
	QVariant getGroupVariant(const QString& name, const QString& propName) const;
	bool setGroupVariant(const QString& name, const QString& propName, const QVariant& value) const;
	bool addGroup(const QString& name);
	bool deleteGroup(const QString& name);
	bool renameGroup(const QString& oldName, const QString& newName);
	bool startGroup(const QString& groupName) const;
	bool stopGroup(const QString& groupName) const;
	bool switchToApp(const QString& appName) const;
	bool startStartupGroups() const;

public slots:
	void groupValueChanged(const QString&, const QVariant&);
	void appManagerValueChanged(const QString&, const QString&, const QString&, const QVariant&);

signals:
	void valueChanged(const QString&, const QString&, const QString&, const QVariant&);

private:

	const std::map<QString, std::pair<std::function<QString(Group*)>, std::function<bool(Group*, const QString&)>>> m_groupStringCallMap =
	{
	};

	const std::map<QString, std::pair<std::function<bool(Group*)>, std::function<bool(Group*, bool)>>> m_groupBoolCallMap =
	{
		{ PROP_GROUP_LAUNCHATSTART, { &Group::getLaunchAtStart, &Group::setLaunchAtStart } },
	};

	const std::map<QString, std::pair<std::function<QStringList(Group*)>, std::function<bool(Group*, const QStringList&)>>> m_groupStringListCallMap =
	{
		{ PROP_GROUP_APPLICATIONS, { &Group::getApplications, &Group::setApplications } },
	};

	Settings* m_settings = nullptr;
	AppManager* m_appManager = nullptr;

	QMap<QString, QSharedPointer<Group>> m_groupList;
};

