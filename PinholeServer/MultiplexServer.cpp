#include "MultiplexServer.h"
#include "GlobalManager.h"
#include "Settings.h"
#include "Logger.h"
#include "Values.h"
#include "StatusInterface.h"
#include "CommandInterface.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"
#include "../common/MultiplexSocket.h"

#include <QSslSocket>
#include <QSslKey>
#include <QSslCertificate>
#include <QFile>
#include <QtEndian>
#include <QTimer>

#define FREQ_RECONNECT			5000
#define FREQCOUNT_RETRYWARN		250

MultiplexServer::MultiplexServer(Settings* settings, StatusInterface* statusInterface, 
	GlobalManager* globalManager, QObject *parent)
	: QObject(parent), m_statusInterface(statusInterface), m_globalManager(globalManager),
	m_settings(settings)
{
	connect(m_globalManager, &GlobalManager::valueChanged,
		this, &MultiplexServer::globalValueChanged);

	m_serverAddress = m_globalManager->getBackendServer();

	QFile keyFile(settings->dataDir() + FILENAME_KEYFILE);
	QFile certFile(settings->dataDir() + FILENAME_CERTFILE);

	if (keyFile.exists() && certFile.exists() &&
		keyFile.size() > 0 && certFile.size() > 0)
	{
		keyFile.open(QIODevice::ReadOnly);
			m_key = new QSslKey(keyFile.readAll(), QSsl::Rsa);
			keyFile.close();

			certFile.open(QIODevice::ReadOnly);
			m_cert = new QSslCertificate(certFile.readAll());
			certFile.close();

			Logger(LOG_DEBUG) << tr("Key files read: %1 %2")
			.arg(keyFile.fileName())
			.arg(certFile.fileName());
}

	// If no keys or bad keys, generate keys
	if (m_cert == nullptr || m_key == nullptr || m_cert->isNull() || m_key->isNull())
	{
		Logger() << tr("Generating key/cert pair...");

		if (m_cert != nullptr)
			delete m_cert;
		if (m_key != nullptr)
			delete m_key;

		QPair<QSslCertificate, QSslKey> certPair = GenerateCertKeyPair("US", "Obscura", "Obscura LLC");

		m_cert = new QSslCertificate(certPair.first);
		m_key = new QSslKey(certPair.second);

		if (!keyFile.open(QIODevice::WriteOnly))
		{
			Logger(LOG_ERROR) << tr("Failed to open key file '%1': %2")
				.arg(keyFile.fileName())
				.arg(keyFile.errorString());
		}
		else
		{
			keyFile.write(m_key->toPem());
			keyFile.close();
		}

		if (!certFile.open(QIODevice::WriteOnly))
		{
			Logger(LOG_ERROR) << tr("Failed to open cert file '%1': %2")
				.arg(keyFile.fileName())
				.arg(keyFile.errorString());
		}
		else
		{
			certFile.write(m_cert->toPem());
			certFile.close();
		}

		Logger() << tr("Key pair written: %1 %2")
			.arg(keyFile.fileName())
			.arg(certFile.fileName());
	}

	// This is BAD
	if (m_cert->isNull())
		Logger(LOG_ALWAYS) << tr("WARNING:  Server certificate is null!!!");
	if (m_key->isNull())
		Logger(LOG_ALWAYS) << tr("WARNING:  Server key is null!!!");

	m_socket = new QSslSocket(this);
	connect(m_socket, (void (QSslSocket::*)(QAbstractSocket::SocketError))&QSslSocket::error,
		this, [this](QAbstractSocket::SocketError socketError)
		{
#ifdef QT_DEBUG
			qDebug() << "MultiplexServer::QSslSocket::error" << QtEnumToString(socketError);
#endif

			switch (socketError)
			{
			case QAbstractSocket::ConnectionRefusedError:
			case QAbstractSocket::SocketTimeoutError:
			case QAbstractSocket::NetworkError:
			case QAbstractSocket::SslHandshakeFailedError:
			case QAbstractSocket::RemoteHostClosedError:
				if (m_retryCount % FREQCOUNT_RETRYWARN == 0)
				{
					Logger(LOG_WARNING) << tr("Failed to connect to backend server '%1' (%2) retries: %3")
						.arg(m_serverAddress)
						.arg(QtEnumToString(socketError))
						.arg(m_retryCount);
				}
				QTimer::singleShot(FREQ_RECONNECT, this, &MultiplexServer::connectToServer);
				m_retryCount++;
				break;

			default:
				break;
			}
		});
	connect(m_socket, &QSslSocket::encrypted,
		this, [this]()
		{
			m_multiplexSocket = new MultiplexSocket(m_socket, this);
			connect(m_multiplexSocket, &MultiplexSocket::datagramReceived,
				this, [this](unsigned int id, const QByteArray& datagram)
				{
					if (HOST_UDPPORT == id)
					{
						QByteArray response = m_statusInterface->processPacket(datagram.data(), "");
						if (!response.isEmpty())
						{
							m_multiplexSocket->writeDatagram(HOST_UDPPORT, response);
						}
					}
				});
			connect(m_multiplexSocket, &MultiplexSocket::newConnection,
				this, [this](MultiplexSocketConnection* connection)
				{
					QString address = "PXY:" + connection->address();
					m_connectionMap[address] = connection;
					connect(connection, &MultiplexSocketConnection::disconnected,
						this, [this, address, connection]()
						{
							emit clientRemoved(address);
							m_connectionMap.remove(address);
						});
					connect(connection, &MultiplexSocketConnection::dataReceived,
						this, [this, address, connection](const QByteArray& data)
						{
							uint32_t size;
							memcpy(&size, data.data(), sizeof(size));
							unsigned int dataSize = qFromLittleEndian(size);
							if (dataSize != data.size() - sizeof(uint32_t))
							{
								Logger(LOG_WARNING) << tr("MultiplexServer received bad sized data packet");
							}
							else
							{
								QByteArray response;
								bool disconnect = false;
								emit incomingData(address, data.mid(4), response, disconnect);

								if (!response.isEmpty())
								{
									connection->writeData(response);
								}

								if (disconnect)
								{
									connection->close();
								}
							}
						});
					emit newClient(address);
				});
		});
	connect(m_socket, (void (QSslSocket::*)(const QList<QSslError>&))&QSslSocket::sslErrors,
		this, [this](const QList<QSslError> errors)
		{
			for (const auto& err : errors)
				qDebug() << __FUNCTION__ << err;
		});

	// TODO - Verify remote cert?
	m_socket->setLocalCertificate(*m_cert);
	m_socket->setPrivateKey(*m_key);
	m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);
}


MultiplexServer::~MultiplexServer()
{
	delete m_key;
	delete m_cert;
}


void MultiplexServer::start()
{
	connectToServer();
}


void MultiplexServer::stop()
{
}


void MultiplexServer::sendDataToClient(const QString & clientId, const QByteArray & data) const
{
	if (!m_connectionMap.contains(clientId))
	{
		Logger(LOG_ERROR) << tr("Internal error: MultiplexServer connection map does not contain client id '%1'").arg(clientId);
		return;
	}

	m_connectionMap[clientId]->writeData(data);
}


void MultiplexServer::sendPacketToServers(const QByteArray & packet)
{
	m_multiplexSocket->writeDatagram(HOST_UDPPORT, packet);
}


void MultiplexServer::connectToServer()
{
	if (!m_serverAddress.isEmpty())
	{
		Logger(LOG_EXTRA) << tr("Connecting to backend server: %1 port %2").arg(m_serverAddress).arg(PROXY_TCPPORT);
		m_socket->connectToHostEncrypted(m_serverAddress, PROXY_TCPPORT);
	}
}


void MultiplexServer::globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value)
{
	Q_UNUSED(itemName);

	if (GROUP_GLOBAL == groupName)
	{
		if (PROP_GLOBAL_BACKENDSERVER == propName)
		{
			m_serverAddress = value.toString();

			m_socket->close();
			connectToServer();
		}
	}
}

