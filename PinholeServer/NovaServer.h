#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QTimer>
#include <QAbstractSocket>

class Settings;
class AppManager;
class GroupManager;
class GlobalManager;
class ScheduleManager;
class QTcpSocket;
class QTcpServer;
class QUdpSocket;

class NovaServer : public QObject
{
	Q_OBJECT

public:
	NovaServer(Settings* settings, AppManager* appManager, GroupManager* groupManager,
		GlobalManager* globalManager, ScheduleManager* scheduleManager, QObject *parent = nullptr);
	~NovaServer();

public slots:
	void globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value);
	void setupTcp();
	void connected();
	void clientDisconnected();
	void serverDisconnected();
	void tcpError(QAbstractSocket::SocketError socketError);
	void newConnection();
	void rx();
	void start();
	void stop();

private:
	void setupUdp();

	bool m_tcpEnabled = false;
	QSharedPointer<QTcpSocket> m_tcpSocket;
	QSharedPointer<QTcpServer> m_tcpServer;
	QString m_tcpAddress;
	int m_tcpPort = 0;
	bool m_udpEnabled = false;
	QSharedPointer<QUdpSocket> m_udpSocket;
	QString m_udpAddress;
	int m_udpPort = 0;

	QString m_novaSite;
	QString m_novaArea;
	QString m_novaDisplay;

	QTimer m_reconnectTimer;

	Settings* m_settings = nullptr;
	AppManager* m_appManager = nullptr;
	GroupManager* m_groupManager = nullptr;
	GlobalManager* m_globalManager = nullptr;
	ScheduleManager* m_scheduleManager = nullptr;
};
