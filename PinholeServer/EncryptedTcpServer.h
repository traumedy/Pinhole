#pragma once

#include <QMap>
#include <QSharedMemory>
#include <QTcpServer>

class Settings;
class AppManager;
class AlertManager;
class GroupManager;
class GlobalManager;
class ScheduleManager;
class QSslError;
class QSslKey;
class QSslCertificate;


class EncryptedTcpServer : public QTcpServer
{
	Q_OBJECT

	class ClientInfo
	{
	public:
		QByteArray data;					// Data received from the client
		uint32_t dataLeft = 0;				// Data left to receive from the client in this packet
		QTcpSocket* socket = nullptr;		// Client socket
	};

public:
	EncryptedTcpServer(Settings* settings, QObject *parent = nullptr);
	~EncryptedTcpServer();

public slots:
	void sslErrors(const QList<QSslError> &errors);
	void link();
	void rx();
	void disconnected();
	void serverAcceptError(QAbstractSocket::SocketError socketError);
	void socketError(QAbstractSocket::SocketError socketError);
	void verifyError(const QSslError& error);
	void start();
	void stop();
	void sendDataToClient(const QString& clientId, const QByteArray& data) const;

signals:
	void terminate();
	void newClient(const QString& clientId);
	void clientRemoved(const QString& clientId);
	void incomingData(const QString& clientId, const QByteArray& data, QByteArray& response, bool& disconnect);

protected:
	void incomingConnection(qintptr descriptor) override;

private:

	QSslKey* m_key = nullptr;
	QSslCertificate* m_cert = nullptr;
	QMap<QString, QSharedPointer<ClientInfo>> m_clientMap;
	Settings* m_settings = nullptr;
};


