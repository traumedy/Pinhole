#pragma once

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QDateTime>

class QTcpSocket;
class QSslError;
class QSslKey;
class QSslCertificate;
class Settings;
class MultiplexSocket;

class ProxyServer : public QTcpServer
{
	Q_OBJECT
		class Server
	{
	public:
		Server(QTcpSocket* tcpSocket, MultiplexSocket* multiplexSocket, int port) :
			m_tcpSocket(tcpSocket), m_multiplexSocket(multiplexSocket), m_port(port)
		{}
		QTcpSocket* tcpSocket() const { return m_tcpSocket; }
		MultiplexSocket* multiplexSocket() const { return m_multiplexSocket; }
		int port() const { return m_port; }
		QString id() const { return m_id; }
		QString addresss() const { return m_address; }
		QString name() const { return m_name; }
		QString role() const { return m_role; }
		QString version() const { return m_version; }
		QString platform() const { return m_platform; }
		QString status() const { return m_status; }
		QString os() const { return m_os; }
		bool isIdentified() const { return m_identified; }
		void setData(const QString& id, const QString& address, const QString& name,
			const QString& role, const QString& version, const QString& platform,
			const QString& status, const QString& os)
		{
			m_id = id; m_address = address; m_name = name; m_role = role;
			m_version = version; m_platform = platform; m_status = status;
			m_os = os; m_lastHeard = QDateTime::currentDateTime(); m_identified = true;
		}
		void setStatus(const QString& status)
		{
			m_status = status;
			m_lastHeard = QDateTime::currentDateTime();
		}

	private:
		QTcpSocket* m_tcpSocket;
		MultiplexSocket* m_multiplexSocket;
		int m_port;
		QString m_id;
		QString m_address;
		QString m_name;
		QString m_role;
		QString m_version;
		QString m_platform;
		QString m_status;
		QString m_os;
		QDateTime m_lastHeard;
		bool m_identified = false;
	};

public:
	ProxyServer(Settings* settings, QObject *parent);
	~ProxyServer();
	QList<QSharedPointer<Server>>& serverList() { return m_serverList; }

public slots:
	void start();

private slots:
	void serverAcceptError(QAbstractSocket::SocketError socketError);
	void acceptConnection();

protected:
	void incomingConnection(qintptr descriptor) override;

private:
	QByteArray m_queryPacket;
	QSslKey* m_key = nullptr;
	QSslCertificate* m_cert = nullptr;
	QList<QSharedPointer<Server>> m_serverList;
	Settings* m_settings = nullptr;
};
