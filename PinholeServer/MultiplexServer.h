#pragma once

#include <QObject>
#include <QMap>

class StatusInterface;
class GlobalManager;
class Settings;
class MultiplexSocket;
class MultiplexSocketConnection;
class QSslSocket;
class QSslKey;
class QSslCertificate;


class MultiplexServer : public QObject
{
	Q_OBJECT

public:
	MultiplexServer(Settings* settings, StatusInterface* statusInterface, 
		GlobalManager* globalManager, QObject *parent = nullptr);
	~MultiplexServer();

public slots:
	void start();	
	void stop();
	void sendDataToClient(const QString& clientId, const QByteArray& data) const;
	void sendPacketToServers(const QByteArray& packet);

signals:
	void newClient(const QString& clientId);
	void clientRemoved(const QString& clientId);
	void incomingData(const QString& clientId, const QByteArray& data, QByteArray& response, bool& disconnect);

private slots:
	void connectToServer();
	void globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value);

private:

	int m_retryCount = 0;
	QString m_serverAddress;
	QSslSocket* m_socket = nullptr;
	QSslKey* m_key = nullptr;
	QSslCertificate* m_cert = nullptr;
	QMap<QString, MultiplexSocketConnection*> m_connectionMap;
	MultiplexSocket* m_multiplexSocket = nullptr;
	StatusInterface* m_statusInterface = nullptr;
	GlobalManager* m_globalManager = nullptr;
	Settings* m_settings = nullptr;
};
