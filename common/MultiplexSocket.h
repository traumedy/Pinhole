#pragma once

#include <QTcpServer>
#include <QMap>

class QTcpSocket;
class MultiplexSocketConnection;
class QSslError;
class QSslKey;
class QSslCertificate;


class MultiplexSocket : public QTcpServer
{
	Q_OBJECT

public:
	MultiplexSocket(QTcpSocket* socket, QObject* parent);
	void writeDatagram(unsigned int id, const QByteArray& data);
	MultiplexSocketConnection* createConnection(const QString& address);
	int listen(QSslKey* key, QSslCertificate* cert);

signals:
	void newConnection(MultiplexSocketConnection*);
	void datagramReceived(unsigned int id, const QByteArray& data);

protected:
	void incomingConnection(qintptr descriptor) override;

private slots:
	void tcpReceiveData();
	void tcpSocketDisconnected();
	void connectionData(const QByteArray& data);
	void multiplexConnectionClosed();

private:
	enum packetType
	{
		TYPE_DATAGRAM,
		TYPE_NEWCONNECTION,
		TYPE_CONNECTIONCLOSED,
		TYPE_CONNECTIONDATA
	};

	struct packetHeader
	{
		quint8 type;		// one of packetType enum
		quint8 flags[3];	// padding and for future use
		quint32 id;			// connection or datagram id (port)
		quint32 length;	// lenght of data to follow
	};

	void sendClosed(unsigned int id);

	packetHeader m_header;
	unsigned int m_dataLeft = 0;
	QByteArray m_data;
	QTcpSocket* m_socket = nullptr;
	QSslKey* m_key = nullptr;
	QSslCertificate* m_cert = nullptr;
	QMap<unsigned int, MultiplexSocketConnection*> m_connectionMap;
	unsigned int m_connectionId = 1;
	int m_listenPort = 0;
};


class MultiplexSocketConnection : public QObject
{
	Q_OBJECT

public:
	MultiplexSocketConnection(unsigned int id, const QString& address, QObject* parent);
	MultiplexSocketConnection(unsigned int id, QObject* parent);
	unsigned int id() const { return m_id; }
	void writeData(const QByteArray& data);
	void close();
	QString address() const { return m_address; }

signals:
	void disconnected();
	void dataWritten(const QByteArray& data);
	void connectionClosed();
	void dataReceived(const QByteArray& data);


private:
	unsigned int m_id;
	QString m_address;
	QByteArray m_data;
};

