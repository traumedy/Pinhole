#pragma once

#include <QObject>
#include <QSharedPointer>

class Settings;
class AppManager;
class GlobalManager;
class GroupManager;
class ScheduleManager;
class QTcpServer;

class HTTPServer : public QObject
{
	Q_OBJECT

public:
	HTTPServer(Settings* settings, AppManager* appManager, GroupManager* groupManager,
		GlobalManager* globalManager, ScheduleManager* scheduleManager, QObject *parent = nullptr);
	~HTTPServer();

public slots:
	void globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value);
	void setupTcp();
	void newConnection();
	void rx();
	void clientDisconnected();
	void start();

private:
	QString parseHTTPRequest(QString req, QString& auth);
	QString escapeString(const QString& str);
	QByteArray generateResponse(unsigned int errorLevel, const QString& errorText, const QByteArray& data, const QString& contentType = "");
	QByteArray generateHeaderData(const QString& title);
	QByteArray generateAppHeartbeatData(const QString& appName, int& err, QString& errmsg);
	QByteArray generateStartStopAppData(const QString& appName, bool start);
	QByteArray generateStartStopGroupData(const QString & groupName, bool start);
	QByteArray generateTriggerEventData(const QString eventName);
	QByteArray generateShutdownRebootAppData(bool reboot);
	QByteArray generateShowScreenIDsData();
	QByteArray generateScreenshotData();
	QByteArray generateApplicationTable();
	QByteArray generateGroupsTable();
	QByteArray generateScheduleTable();
	QByteArray generateRootData();
	QByteArray generateGroupsData();
	QByteArray generateScheduleData();
	QByteArray generateSystemInfoData();
	QByteArray generateLogData(const QString & arg);

	bool m_tcpEnabled = false;
	QSharedPointer<QTcpServer> m_tcpServer;
	int m_tcpPort = 0;

	Settings* m_settings = nullptr;
	AppManager* m_appManager = nullptr;
	GroupManager* m_groupManager = nullptr;
	GlobalManager* m_globalManager = nullptr;
	ScheduleManager* m_scheduleManager = nullptr;
};
