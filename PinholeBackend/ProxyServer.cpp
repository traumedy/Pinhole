#include "ProxyServer.h"
#include "Settings.h"
#include "../common/Utilities.h"
#include "../common/MultiplexSocket.h"
#include "../common/PinholeCommon.h"

#include <QFile>
#include <QtEndian>
#include <QSslKey>
#include <QSslCertificate>
#include <QSslSocket>
#include <QUdpSocket>
#include <QDebug>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QNetworkDatagram>

#define FILENAME_KEYFILE	"backend.key"
#define FILENAME_CERTFILE	"backend.pem"

#define FREQ_SERVERQUERY	10000

ProxyServer::ProxyServer(Settings* settings, QObject *parent)
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

		qInfo() << tr("Key files read: %1 %2")
			.arg(keyFile.fileName())
			.arg(certFile.fileName());
	}

	// If no keys or bad keys, generate keys
	if (m_cert == nullptr || m_key == nullptr || m_cert->isNull() || m_key->isNull())
	{
		qInfo() << tr("Generating key/cert pair...");

		if (m_cert != nullptr)
			delete m_cert;
		if (m_key != nullptr)
			delete m_key;

		QPair<QSslCertificate, QSslKey> certPair = GenerateCertKeyPair("US", "Obscura", "Obscura LLC");

		m_cert = new QSslCertificate(certPair.first);
		m_key = new QSslKey(certPair.second);

		if (!keyFile.open(QIODevice::WriteOnly))
		{
			qWarning() << tr("Failed to open key file '%1': %2")
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
			qWarning() << tr("Failed to open cert file '%1': %2")
				.arg(keyFile.fileName())
				.arg(keyFile.errorString());
		}
		else
		{
			certFile.write(m_cert->toPem());
			certFile.close();
		}

		qInfo() << tr("Key pair written: %1 %2")
			.arg(keyFile.fileName())
			.arg(certFile.fileName());
	}

	// This is BAD
	if (m_cert->isNull())
		qWarning() << tr("WARNING:  Server certificate is null!!!");
	if (m_key->isNull())
		qWarning() << tr("WARNING:  Server key is null!!!");

	connect(this, (void (QTcpServer::*)(QAbstractSocket::SocketError))&QTcpServer::acceptError,
		this, &ProxyServer::serverAcceptError);
	connect(this, &ProxyServer::newConnection,
		this, &ProxyServer::acceptConnection);

	// Create TCP server socket
	if (!listen(QHostAddress::Any, PROXY_TCPPORT)) 
	{
		qFatal("Unable to start the TCP server");
		QCoreApplication::exit();
	}

	// Create query packet
	QJsonObject jsonObject;
	jsonObject[TAG_COMMAND] = UDPCOMMAND_QUERY;
	QJsonDocument jsonDoc;
	jsonDoc.setObject(jsonObject);
	m_queryPacket = jsonDoc.toJson();

	// Query servers on a timer
	QTimer* queryTimer = new QTimer(this);
	queryTimer->setInterval(FREQ_SERVERQUERY);
	queryTimer->setSingleShot(false);
	connect(queryTimer, &QTimer::timeout,
		this, [this]()
		{
			for (const auto& client : m_serverList)
			{
				client->multiplexSocket()->writeDatagram(HOST_UDPPORT, m_queryPacket);
			}
		});
	queryTimer->start();

	qInfo() << "ProxyServer created";
}


ProxyServer::~ProxyServer()
{
}


void ProxyServer::start()
{

}


void ProxyServer::serverAcceptError(QAbstractSocket::SocketError socketError)
{
#ifdef QT_DEBUG
	qWarning() << "Server accept error: " << socketError;
#else
	Q_UNUSED(socketError);
#endif
}


void ProxyServer::acceptConnection()
{
	QTcpSocket *tcpSocket = nextPendingConnection();
	MultiplexSocket* multiplexSocket = new MultiplexSocket(tcpSocket, this);
	int port = multiplexSocket->listen(m_key, m_cert);
	auto server = QSharedPointer<Server>::create(tcpSocket, multiplexSocket, port);
	QString serverAddress = HostAddressToString(server->tcpSocket()->peerAddress());
	connect(multiplexSocket, &MultiplexSocket::datagramReceived,
		this, [this, server, serverAddress](int id, const QByteArray& datagram)
		{
			Q_UNUSED(id);

			// Parse the data as JSON
			QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram.data());
			// Validate the data
			if (!jsonDoc.isNull())
			{
				// Parse the json data
				QJsonObject jsonObject = jsonDoc.object();
				QString command = jsonObject[TAG_COMMAND].toString();
				if (UDPCOMMAND_ANNOUNCE == command)
				{
					QString address = serverAddress;
					QString serverId = jsonObject[TAG_ID].toString();
					QString name = jsonObject[TAG_NAME].toString();
					QString role = jsonObject[TAG_ROLE].toString();
					QString version = jsonObject[TAG_VERSION].toString();
					QString platform = jsonObject[TAG_PLATFORM].toString();
					QString status = jsonObject[TAG_STATUS].toString();
					QString os = jsonObject[TAG_OS].toString();
					server->setData(serverId, address, name, role, version, platform, status, os);
				}
				else if (UDPCOMMAND_STATUS == command)
				{
					server->setStatus(jsonObject[TAG_STATUS].toString());
				}
			}
		});
	connect(tcpSocket, &QTcpSocket::disconnected,
		this, [this, server, serverAddress]
	{
		qInfo() << "Server disconnected:" << serverAddress;
		m_serverList.removeAll(server);
	});

	m_serverList.append(server);
	qInfo() << "New server connection from:" << serverAddress << "listening on proxy port:" << port;
}


void ProxyServer::incomingConnection(qintptr socketDescriptor)
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
		qWarning() << "Peer verify error: " << error;
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
