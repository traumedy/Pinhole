#include "EncryptedTcpServer.h"
#include "Settings.h"
#include "AppManager.h"
#include "AlertManager.h"
#include "GroupManager.h"
#include "GlobalManager.h"
#include "ScheduleManager.h"
#include "Logger.h"
#include "Values.h"
#include "WinUtil.h"
#include "../common/PinholeCommon.h"
#include "../common/Utilities.h"

#include <QSharedPointer>
#include <QFile>
#include <QtEndian>
#include <QSslKey>
#include <QSslCertificate>
#include <QSslSocket>


EncryptedTcpServer::EncryptedTcpServer(Settings* settings, QObject *parent)
	: QTcpServer(parent), m_settings(settings)
{
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

	connect(this, (void (QTcpServer::*)(QAbstractSocket::SocketError))&QTcpServer::acceptError,
		this, &EncryptedTcpServer::serverAcceptError);
	connect(this, &EncryptedTcpServer::newConnection, 
		this, &EncryptedTcpServer::link);
}


EncryptedTcpServer::~EncryptedTcpServer()
{
	delete m_key;
	delete m_cert;
}


void EncryptedTcpServer::start()
{
	// Make server listen
	if (!listen(QHostAddress::Any, HOST_TCPPORT)) {
		Logger(LOG_ERROR) << tr("Unable to start the TCP server");
		emit terminate();
	}
}


void EncryptedTcpServer::stop()
{
	// TODO
}


void EncryptedTcpServer::sendDataToClient(const QString & clientId, const QByteArray & data) const
{
	if (!m_clientMap.contains(clientId))
	{
		Logger(LOG_ERROR) << tr("EncryptedTcpServer client map does not contain client id '%1'").arg(clientId);
		return;
	}

	auto client = m_clientMap[clientId];
	client->socket->write(data);
	client->socket->flush();
}


void EncryptedTcpServer::incomingConnection(qintptr socketDescriptor)
{
	QSslSocket *sslSocket = new QSslSocket(this);

	connect(sslSocket, (void (QSslSocket::*)(const QList<QSslError>&))&QSslSocket::sslErrors,
		this, &EncryptedTcpServer::sslErrors);
	connect(sslSocket, &QSslSocket::peerVerifyError,
		this, &EncryptedTcpServer::verifyError);
	if (!sslSocket->setSocketDescriptor(socketDescriptor))
	{
		Logger(LOG_WARNING) << tr("Failed to set QSslSocket descriptor");
	}
	sslSocket->setPrivateKey(*m_key);
	sslSocket->setLocalCertificate(*m_cert);
	sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
	sslSocket->startServerEncryption();

	addPendingConnection(sslSocket);
}


void EncryptedTcpServer::sslErrors(const QList<QSslError> &errors)
{
	foreach(const QSslError &error, errors)
	{
		Logger(LOG_WARNING) << tr("SSL error: ") << error.errorString();
#ifdef QT_DEBUG
		qDebug() << error.errorString();
#endif
	}
}


void EncryptedTcpServer::link()
{
	QTcpSocket *clientSocket = nextPendingConnection();

	connect(clientSocket, &QTcpSocket::readyRead, 
		this, &EncryptedTcpServer::rx);
	connect(clientSocket, &QTcpSocket::disconnected, 
		this, &EncryptedTcpServer::disconnected);
	connect(clientSocket, (void (QTcpSocket::*)(QAbstractSocket::SocketError))&QTcpSocket::error,
		this, &EncryptedTcpServer::socketError);

	QString clientAddr = "TCP:" + clientSocket->peerAddress().toString() + ":" + QString::number(clientSocket->peerPort());
	// Save as a property for the socket object so we can use it as a key to m_clientMap
	clientSocket->setProperty(PROPERTY_ADDRESS, clientAddr);

	m_clientMap[clientAddr] = QSharedPointer<ClientInfo>::create();
	m_clientMap[clientAddr]->socket = clientSocket;
	emit newClient(clientAddr);
}


void EncryptedTcpServer::rx()
{
	QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());

	QString clientAddr = clientSocket->property(PROPERTY_ADDRESS).toString();
	QSharedPointer<ClientInfo> client = m_clientMap[clientAddr];

	do
	{
		if (0 == client->dataLeft)
		{
			QByteArray sizeArray = clientSocket->read(sizeof(uint32_t));
			uint32_t size;
			memcpy(&size, sizeArray.data(), sizeof(size));
			client->dataLeft = qFromLittleEndian(size);
		}

		QByteArray data = clientSocket->read(client->dataLeft);
		client->data += data;
		client->dataLeft -= data.size();

		if (!(0 == client->dataLeft && !client->data.isEmpty()))
		{
			// More data expected
			return;
		}

		QByteArray response;
		bool disconnect = false;
		emit incomingData(clientAddr, client->data, response, disconnect);

		// Reset client data
		client->data.clear();

		if (!response.isEmpty())
		{
			clientSocket->write(response);
			clientSocket->flush();
		}

		if (disconnect)
		{
			clientSocket->disconnectFromHost();
		}
	} while (clientSocket->bytesAvailable());
}


void EncryptedTcpServer::disconnected()
{
	QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
	QString clientAddr = clientSocket->property(PROPERTY_ADDRESS).toString();

	// Remove entry from m_clientMap
	m_clientMap.remove(clientAddr);
	emit clientRemoved(clientAddr);

	Logger(LOG_DEBUG) << tr("Client Disconnected: ") << clientAddr;

	clientSocket->deleteLater();
}


void EncryptedTcpServer::serverAcceptError(QAbstractSocket::SocketError socketError)
{
#ifdef QT_DEBUG
	qDebug() << "Server accept error: " << socketError;
#else
	Q_UNUSED(socketError);
#endif
}


void EncryptedTcpServer::socketError(QAbstractSocket::SocketError socketError)
{
#ifdef QT_DEBUG
	qDebug() << "Client socket error: " << socketError;
#else
	Q_UNUSED(socketError);
#endif
}


void EncryptedTcpServer::verifyError(const QSslError& error)
{
#ifdef QT_DEBUG
	qDebug() << "Client verify error: " << error;
#else
	Q_UNUSED(error);
#endif
}






