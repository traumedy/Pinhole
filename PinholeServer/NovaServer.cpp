#include "NovaServer.h"
#include "Settings.h"
#include "AppManager.h"
#include "GroupManager.h"
#include "GlobalManager.h"
#include "ScheduleManager.h"
#include "Logger.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QNetworkDatagram>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QTcpServer>
#include <QUdpSocket>


#define RECONNECT_DELAY		500

NovaServer::NovaServer(Settings* settings, AppManager* appManager, GroupManager* groupManager,
	GlobalManager* globalManager, ScheduleManager* scheduleManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_appManager(appManager), m_groupManager(groupManager),
	m_globalManager(globalManager), m_scheduleManager(scheduleManager)
{
	m_reconnectTimer.setInterval(RECONNECT_DELAY);
	m_reconnectTimer.setSingleShot(true);
	connect(&m_reconnectTimer, &QTimer::timeout,
		this, &NovaServer::setupTcp);

	connect(m_globalManager, &GlobalManager::valueChanged,
		this, &NovaServer::globalValueChanged);

	m_novaSite = m_globalManager->getNovaSite();
	m_novaArea = m_globalManager->getNovaArea();
	m_novaDisplay = m_globalManager->getNovaDisplay();

	m_tcpEnabled = m_globalManager->getNovaTcpEnabled();
	m_tcpAddress = m_globalManager->getNovaTcpAddress();
	m_tcpPort = m_globalManager->getNovaTcpPort();

	m_udpEnabled = m_globalManager->getNovaUdpEnabled();
	m_udpAddress = m_globalManager->getNovaUdpAddress();
	m_udpPort = m_globalManager->getNovaUdpPort();
}


NovaServer::~NovaServer()
{
	m_tcpEnabled = false;
	m_udpEnabled = false;
}


void NovaServer::start()
{
	if (m_tcpEnabled)
	{
		setupTcp();
	}

	if (m_udpEnabled)
	{
		setupUdp();
	}
}


void NovaServer::stop()
{
	// TODO
}


void NovaServer::globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value)
{
	Q_UNUSED(itemName);

	if (GROUP_GLOBAL == groupName)
	{
		if (PROP_GLOBAL_NOVATCPENABLED == propName)
		{
			m_tcpEnabled = value.toBool();

			if (m_tcpEnabled)
			{
				setupTcp();
			}
			else
			{
				m_tcpServer.clear();
				m_tcpSocket.clear();
			}
		}
		else if (PROP_GLOBAL_NOVATCPADDRESS == propName)
		{
			m_tcpAddress = value.toString();
			if (m_tcpEnabled)
			{
				setupTcp();
			}
		}
		else if (PROP_GLOBAL_NOVATCPPORT == propName)
		{
			m_tcpPort = value.toInt();
			if (m_tcpEnabled)
			{
				setupTcp();
			}
		}
		else if (PROP_GLOBAL_NOVAUDPENABLED == propName)
		{
			m_udpEnabled = value.toBool();
			if (m_udpEnabled)
			{
				setupUdp();
			}
			else
			{
				m_udpSocket.clear();
			}
		}
		else if (PROP_GLOBAL_NOVAUDPADDRESS == propName)
		{
			m_udpAddress = value.toString();
			if (m_udpEnabled)
			{
				setupUdp();
			}
			else
			{
				m_udpSocket.clear();
			}
		}
		else if (PROP_GLOBAL_NOVAUDPPORT == propName)
		{
			m_udpPort = value.toInt();
			if (m_udpEnabled)
			{
				setupUdp();
			}
		}
		else if (PROP_GLOBAL_NOVASITE == propName)
		{
			m_novaSite = value.toString();
		}
		else if (PROP_GLOBAL_NOVAAREA == propName)
		{
			m_novaArea = value.toString();
		}
		else if (PROP_GLOBAL_NOVADISPLAY == propName)
		{
			m_novaDisplay = value.toString();
		}
	}
}


void NovaServer::setupTcp()
{
	m_tcpServer.clear();
	m_tcpSocket.clear();


	if (m_tcpAddress.isEmpty())
	{
		m_tcpServer = QSharedPointer<QTcpServer>::create(this);

		if (!m_tcpServer->listen(QHostAddress::Any, m_tcpPort))
		{
			Logger(LOG_WARNING) << tr("Nova TCP server failed to bind to port ") << m_tcpPort;
		}
		else
		{
			Logger(LOG_EXTRA) << tr("Nova TCP server listening on port ") << m_tcpPort;

			connect(m_tcpServer.data(), &QTcpServer::newConnection,
				this, &NovaServer::newConnection);
		}
	}
	else
	{
		m_tcpSocket = QSharedPointer<QTcpSocket>::create(this);

		QHostAddress novaServer;
		if (!novaServer.setAddress(m_tcpAddress))
		{
			Logger(LOG_ERROR) << tr("Nova server unable to convert TCP server address string to address ") << m_tcpAddress;
			m_tcpSocket.clear();
			return;
		}

		connect(m_tcpSocket.data(), &QTcpSocket::connected,
			this, &NovaServer::connected);
		connect(m_tcpSocket.data(), &QTcpSocket::disconnected,
			this, &NovaServer::clientDisconnected);
		connect(m_tcpSocket.data(), QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
			this, &NovaServer::tcpError);
		connect(m_tcpSocket.data(), &QTcpSocket::readyRead,
			this, &NovaServer::rx);
		Logger(LOG_EXTRA) << tr("Connecting to Nova TCP server ") << m_tcpAddress << ":" << m_tcpPort;

		m_tcpSocket->connectToHost(novaServer, m_tcpPort);
	}
}


void NovaServer::setupUdp()
{
	m_udpSocket = QSharedPointer<QUdpSocket>::create(this);

	m_udpSocket->bind(m_udpPort);

	if (!m_udpAddress.isEmpty())
	{
		QHostAddress multicastAddress;
		if (!multicastAddress.setAddress(m_udpAddress))
		{
			Logger(LOG_ERROR) << tr("Nova server unable to convert UDP multicast address string to address ") << m_udpAddress;
			m_udpSocket.clear();
			return;
		}
		if (!m_udpSocket->joinMulticastGroup(multicastAddress))
		{
			Logger(LOG_ERROR) << tr("Nova server unable to join UDP multicast address ") << m_udpAddress;
			m_udpSocket.clear();
			return;
		}

		Logger() << tr("Nova UDP server joined multicast address %1 listening on port %2")
			.arg(m_udpAddress)
			.arg(m_udpPort);
	}
	else
	{
		Logger() << tr("Nova UDP server listening on port ") << m_udpPort;
	}

	connect(m_udpSocket.data(), &QUdpSocket::readyRead,
		this, &NovaServer::rx);
}


void NovaServer::connected()
{
	Logger() << tr("Connected to Nova TCP server ") << m_tcpAddress << ":" << m_tcpPort;
}


void NovaServer::clientDisconnected()
{
	if (!QCoreApplication::closingDown() && m_tcpEnabled)
	{
		Logger(LOG_EXTRA) << tr("Disconnected from Nova TCP server, reconnecting");
		m_reconnectTimer.start();
	}
}


void NovaServer::serverDisconnected()
{
	QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
	QHostAddress address = socket->peerAddress();
	Logger(LOG_EXTRA) << tr("Nova TCP client disconnected: ") << address.toString();

	socket->deleteLater();
}


void NovaServer::tcpError(QAbstractSocket::SocketError socketError)
{
	switch (socketError)
	{
	case QAbstractSocket::HostNotFoundError:
		Logger(LOG_ERROR) << tr("Nova TCP server not found: ") << m_tcpAddress;
		break;
	case QAbstractSocket::ConnectionRefusedError:
		if (!QCoreApplication::closingDown() && m_tcpEnabled)
		{
			Logger(LOG_EXTRA) << tr("Nova TCP server refused connection, reconnecting");
			m_reconnectTimer.start();
		}
		break;
	case QAbstractSocket::SocketTimeoutError:
		if (!QCoreApplication::closingDown() && m_tcpEnabled)
		{
			Logger(LOG_EXTRA) << tr("Nova TCP server connection timed out, reconnecting");
			m_reconnectTimer.start();
		}
		break;

	case QAbstractSocket::RemoteHostClosedError:
	case QAbstractSocket::SocketAccessError:
	case QAbstractSocket::SocketResourceError:
	case QAbstractSocket::DatagramTooLargeError:
	case QAbstractSocket::NetworkError:
	case QAbstractSocket::AddressInUseError:
	case QAbstractSocket::SocketAddressNotAvailableError:
	case QAbstractSocket::UnsupportedSocketOperationError:
	case QAbstractSocket::UnfinishedSocketOperationError:
	case QAbstractSocket::ProxyAuthenticationRequiredError:
	case QAbstractSocket::SslHandshakeFailedError:
	case QAbstractSocket::ProxyConnectionRefusedError:
	case QAbstractSocket::ProxyConnectionClosedError:
	case QAbstractSocket::ProxyConnectionTimeoutError:
	case QAbstractSocket::ProxyNotFoundError:
	case QAbstractSocket::ProxyProtocolError:
	case QAbstractSocket::OperationError:
	case QAbstractSocket::SslInternalError:
	case QAbstractSocket::SslInvalidUserDataError:
	case QAbstractSocket::TemporaryError:
	case QAbstractSocket::UnknownSocketError:
		break;

	}
}


void NovaServer::newConnection()
{
	QTcpSocket* socket = m_tcpServer->nextPendingConnection();

	QHostAddress address = socket->peerAddress();
	Logger(LOG_EXTRA) << tr("Nova TCP connection received: ") << address.toString();

	connect(socket, &QTcpSocket::readyRead,
		this, &NovaServer::rx);
	connect(socket, &QTcpSocket::disconnected,
		this, &NovaServer::serverDisconnected);
}


void NovaServer::rx()
{
	QString query;

	if (nullptr != qobject_cast<QTcpSocket*>(sender()))
	{
		query = QString::fromUtf8(qobject_cast<QTcpSocket*>(sender())->readAll());
	}
	else if (nullptr != qobject_cast<QUdpSocket*>(sender()))
	{
		QNetworkDatagram datagram = qobject_cast<QUdpSocket*>(sender())->receiveDatagram();
		query = QString::fromUtf8(datagram.data());
	}

	Logger(LOG_DEBUG) << tr("Nova packet received: ") << query;

	QString source, dest, command;

	int firstColon = query.indexOf(':');
	if (-1 == firstColon)
	{
		Logger(LOG_WARNING) << tr("Bad Nova packet (no first colon): ") << query;
		return;
	}

	int secondColon = query.indexOf(':', firstColon + 1);
	if (-1 == secondColon)
	{
		Logger(LOG_WARNING) << tr("Bad Nova packet (no second colon): ") << query;
		return;
	}

	source = query.left(firstColon);
	dest = query.mid(firstColon + 1, secondColon - firstColon - 1);
	command = query.mid(secondColon + 1);

	QString destSite, destArea, destDisplay;

	if (!dest.isEmpty())
	{
		QStringList parts = dest.split('/');
		if (3 != parts.length())
		{
			Logger(LOG_WARNING) << tr("Bad non-empty destination in Nova packet (does not contain 3 parts): ") << dest;
			return;
		}

		destSite = parts[0];
		destArea = parts[1];
		destDisplay = parts[2];
	}

	if (!((destSite.isEmpty() || destSite == m_novaSite) &&
		(destArea.isEmpty() || destArea == m_novaArea) &&
		(destDisplay.isEmpty() || destDisplay == m_novaDisplay)))
	{
		Logger(LOG_EXTRA) << tr("Nova packet destination not for this server");
		return;
	}

	int leftParen = command.indexOf('(');
	int rightParen = command.lastIndexOf(')');

	if (-1 == leftParen || -1 == rightParen || leftParen > rightParen)
	{
		Logger(LOG_WARNING) << tr("Bad Nova command (missing or bad paranetheses): ") << command;
		return;
	}

	QString action = command.left(leftParen);
	QString arg = command.mid(leftParen + 1, rightParen - leftParen - 1);

	if (NOVA_CMD_RESTART == action)
	{
		Logger(LOG_WARNING) << tr("Reboot requested via Nova interface");
		m_globalManager->reboot();
	}
	else if (NOVA_CMD_SHUTDOWN == action)
	{
		Logger(LOG_WARNING) << tr("Shutdown requested via Nova interface");
		m_globalManager->shutdown();
	}
	else if (NOVA_CMD_START_APP == action)
	{
		m_appManager->startApps(SplitSemicolonString(arg));
	}
	else if (NOVA_CMD_START_APP_VARS == action)
	{
		QStringList argList = SplitSemicolonString(arg);
		if (!argList.isEmpty())
		{
			QString appName = argList[0];
			argList.removeAt(0);
			m_appManager->startAppVariables(appName, argList);
		}
	}
	else if (NOVA_CMD_STOP_APP == action)
	{
		m_appManager->stopApps(SplitSemicolonString(arg));
	}
	else if (NOVA_CMD_SWITCH_APP == action)
	{
		m_groupManager->switchToApp(arg);
	}
	else if (NOVA_CMD_RESTART_APP == action)
	{
		m_appManager->restartApps(SplitSemicolonString(arg));
	}
	else if (NOVA_CMD_START_GROUP == action)
	{
		m_groupManager->startGroup(arg);
	}
	else if (NOVA_CMD_STOP_GROUP == action)
	{
		m_groupManager->stopGroup(arg);
	}
	else if (NOVA_CMD_LOGWARNING == action)
	{
		Logger(LOG_WARNING) << "[Nova " << source << "] " << arg;
	}
	else
	{
		Logger(LOG_WARNING) << tr("Unknown command received from Nova client: ") << command;
		Logger(LOG_DEBUG) << tr("Nova action '") << action << "' arg '" << arg << "'";
	}
}


