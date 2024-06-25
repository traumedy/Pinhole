#include "MultiplexSocket.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QtEndian>
#include <QSslSocket>

#include "Utilities.h"

MultiplexSocket::MultiplexSocket(QTcpSocket* socket, QObject* parent) : 
	QTcpServer(parent), m_socket(socket)
{
	connect(socket, &QTcpSocket::readyRead,
		this, &MultiplexSocket::tcpReceiveData);
	connect(socket, &QTcpSocket::disconnected,
		this, &MultiplexSocket::tcpSocketDisconnected);
	connect(this, &QTcpServer::newConnection,
		this, [this]()
	{
		QTcpSocket* socket = nextPendingConnection();
		QString address = HostAddressToString(socket->peerAddress()) + ":" + QString::number(socket->peerPort());
		MultiplexSocketConnection* connection = createConnection(address);
		connect(socket, &QTcpSocket::readyRead,
			this, [socket, connection]()
		{
#if defined(QT_DEBUG)
			qDebug() << "QTcpSocket::readyRead bytes:" << socket->bytesAvailable();
#endif
			connection->writeData(socket->readAll());
		});
		connect(connection, &MultiplexSocketConnection::dataReceived,
			this, [socket, connection](const QByteArray& data)
		{
#if defined(QT_DEBUG)
			qDebug() << "MultiplexSocketConnection::dataReceived bytes:" << data.size();
#endif
			socket->write(data);
			socket->flush();
		});
		connect(socket, &QTcpSocket::disconnected,
			this, [socket, connection]()
		{
			connection->close();
			socket->deleteLater();
		});
		connect(connection, &MultiplexSocketConnection::disconnected,
			this, [socket]()
		{
			socket->close();
		});
	});
}


void MultiplexSocket::incomingConnection(qintptr socketDescriptor)
{
	QSslSocket* sslSocket = new QSslSocket(this);

	connect(sslSocket, (void (QSslSocket::*)(const QList<QSslError>&))&QSslSocket::sslErrors,
		this, [this](const QList<QSslError> &errors)
		{
#ifdef QT_DEBUG
			foreach(const QSslError &error, errors)
			{
				qDebug() << error.errorString();
			}
#else
			Q_UNUSED(errors);
#endif
		});
	connect(sslSocket, &QSslSocket::peerVerifyError,
		this, [this](const QSslError& error)
		{
#ifdef QT_DEBUG
			qWarning() << "Client verify error: " << error;
#else
			Q_UNUSED(error);
#endif
		});
	if (!sslSocket->setSocketDescriptor(socketDescriptor))
	{
		qWarning() << tr("Failed to set QSslSocket descriptor");
	}
	sslSocket->setPrivateKey(*m_key);
	sslSocket->setLocalCertificate(*m_cert);
	sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
	sslSocket->startServerEncryption();

	addPendingConnection(sslSocket);
#if defined(QT_DEBUG)
	qDebug() << "Connection encrypted";
#endif
}


void MultiplexSocket::writeDatagram(unsigned int id, const QByteArray & data)
{
	packetHeader header;
	header.type = TYPE_DATAGRAM;
	header.id = qToLittleEndian(id);
	header.length = qToLittleEndian(data.length());
	m_socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
	m_socket->write(data);
	m_socket->flush();
}


MultiplexSocketConnection* MultiplexSocket::createConnection(const QString& address)
{
	QByteArray addressData = address.toUtf8();
	packetHeader header;
	header.type = TYPE_NEWCONNECTION;
	header.id = qToLittleEndian(m_connectionId);
	header.length = qToLittleEndian(addressData.length());
	MultiplexSocketConnection* connection = new MultiplexSocketConnection(m_connectionId, address, this);
	m_connectionMap[m_connectionId] = connection;
	m_connectionId++;
	connect(connection, &MultiplexSocketConnection::dataWritten,
		this, &MultiplexSocket::connectionData);
	connect(connection, &MultiplexSocketConnection::connectionClosed,
		this, &MultiplexSocket::multiplexConnectionClosed);
	m_socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
	m_socket->write(addressData);
	m_socket->flush();
	return connection;
}


int MultiplexSocket::listen(QSslKey* key, QSslCertificate* cert)
{
	m_key = key;
	m_cert = cert;

	if (!QTcpServer::listen()) {
		qWarning() << tr("Unable to listen on TCP server");
		return 0;
	}

	return serverPort();
}


void MultiplexSocket::tcpReceiveData()
{
	do
	{
		if (0 == m_dataLeft)
		{
			quint64 received = m_socket->read(reinterpret_cast<char*>(&m_header), sizeof(m_header));
			if (0 == received)
			{
				// Not sure why this happens
				return;
			}
			if (sizeof(m_header) != received)
			{
				qWarning() << "INTERNAL ERROR, DID NOT RECEIVE ENTIRE MULTIPLEX HEADER, received:" << received;
				return;
			}
			m_dataLeft = qFromLittleEndian(m_header.length);
			m_data.clear();
#if defined(QT_DEBUG)
			qDebug() << "Header type:" << m_header.type << "dataLeft:" << m_dataLeft << "Size:" << m_data.size();
#endif
		}

		if (0 != m_dataLeft)
		{
			m_data.append(m_socket->read(m_dataLeft));
			m_dataLeft = qFromLittleEndian(m_header.length) - m_data.length();
		}

		if (0 == m_dataLeft)
		{
			switch (m_header.type)
			{
			case TYPE_DATAGRAM:
#if defined(QT_DEBUG)
				qDebug() << "MultiplexSocketConnection datagram:" << m_header.id << "Size:" << m_data.size();
#endif
				emit datagramReceived(qFromLittleEndian(m_header.id), m_data);
				break;

			case TYPE_NEWCONNECTION:
			{
				QString remoteAddress = QString::fromUtf8(m_data);
				unsigned int id = qFromLittleEndian(m_header.id);
				MultiplexSocketConnection* connection = new MultiplexSocketConnection(id, remoteAddress, this);
				m_connectionMap[id] = connection;
				m_connectionId = id + 1;
#if defined(QT_DEBUG)
				qDebug() << "MultiplexSocketConnection new connection:" << id << m_connectionMap[id]->address();
#endif
				connect(connection, &MultiplexSocketConnection::dataWritten,
					this, &MultiplexSocket::connectionData);
				emit newConnection(connection);
			}
			break;

			case TYPE_CONNECTIONCLOSED:
			{
				unsigned int id = qFromLittleEndian(m_header.id);
				if (!m_connectionMap.contains(id))
				{
					qWarning() << "INTERNAL ERROR: MULTIPLEX CONNECTION MAP DOES NOT CONTAIN ID FOR CLOSE:" << id;
				}
				else
				{
#if defined(QT_DEBUG)
					qDebug() << "MultiplexSocketConnection connection closed:" << id << m_connectionMap[id]->address();
#endif
					emit m_connectionMap[id]->disconnected();
					m_connectionMap[id]->deleteLater();
					m_connectionMap.remove(id);
				}
			}
			break;

			case TYPE_CONNECTIONDATA:
				unsigned int id = qFromLittleEndian(m_header.id);
				if (!m_connectionMap.contains(id))
				{
					qWarning() << "INTERNAL ERROR: MULTIPLEX CONNECTION MAP DOES NOT CONTAIN ID FOR DATA:" << id;
				}
				else
				{
#if defined(QT_DEBUG)
					qDebug() << "MultiplexSocketConnection data received:" << id << m_connectionMap[id]->address();
#endif
					emit m_connectionMap[id]->dataReceived(m_data);
				}
				break;
			}

			m_data.clear();
		}
#if defined(QT_DEBUG)
		if (m_socket->bytesAvailable())
			qDebug() << "Bytes still available:" << m_socket->bytesAvailable();
#endif
	} while (m_socket->bytesAvailable());
}


void MultiplexSocket::tcpSocketDisconnected()
{
#if defined(QT_DEBUG)
	qDebug() << "MultiplexSocket::disconnected from host" << m_socket->peerAddress().toString();
#endif

	// Make copy of m_connectionMap because it entries will be removed from it
	// as MultiplexSocketConnection::disconnected() is emitted
	QList<MultiplexSocketConnection*> conList;
	for (const auto& con : m_connectionMap)
		conList.append(con);
	for (const auto& con : conList)
		emit con->disconnected();
}


void MultiplexSocket::connectionData(const QByteArray & data)
{
	MultiplexSocketConnection* connection = qobject_cast<MultiplexSocketConnection*>(sender());
	packetHeader header;
	header.type = TYPE_CONNECTIONDATA;
	header.id = qToLittleEndian(connection->id());
	header.length = qToLittleEndian(data.size());
	m_socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
	m_socket->write(data);
	m_socket->flush();
}


void MultiplexSocket::multiplexConnectionClosed()
{
	MultiplexSocketConnection* connection = qobject_cast<MultiplexSocketConnection*>(sender());
	unsigned int id = connection->id();
	sendClosed(id);
	if (m_connectionMap.contains(id))
	{
		m_connectionMap[id]->deleteLater();
		m_connectionMap.remove(id);
	}
}


void MultiplexSocket::sendClosed(unsigned int id)
{
	packetHeader header;
	header.type = TYPE_CONNECTIONCLOSED;
	header.id = qToLittleEndian(id);
	header.length = 0;
	m_socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
	m_socket->flush();
}


MultiplexSocketConnection::MultiplexSocketConnection(unsigned int id, const QString& address, QObject* parent) :
	QObject(parent), m_id(id), m_address(address)
{
}


void MultiplexSocketConnection::writeData(const QByteArray & data)
{
	emit dataWritten(data);
}


void MultiplexSocketConnection::close()
{
	emit connectionClosed();
}

