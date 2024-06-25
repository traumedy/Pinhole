#include "HostUdpServer.h"
#include "Settings.h"
#include "StatusInterface.h"
#include "Logger.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QNetworkConfigurationManager>

#include <thread>

HostUdpServer::HostUdpServer(Settings* settings, StatusInterface* statusInterface, 
	QObject *parent) : QObject(parent), m_settings(settings), m_statusInterface(statusInterface)
{
	// Store (and update) the network interface list so we don't need to 
	//  call QNetworkInterface::allInterfaces() often
	m_interfaces = QNetworkInterface::allInterfaces();
	auto networkConfigurationManager = new QNetworkConfigurationManager(this);
	connect(networkConfigurationManager, &QNetworkConfigurationManager::configurationChanged,
		this, [this] { m_interfaces = QNetworkInterface::allInterfaces(); });
	connect(networkConfigurationManager, &QNetworkConfigurationManager::configurationAdded,
		this, [this] { m_interfaces = QNetworkInterface::allInterfaces(); });
	connect(networkConfigurationManager, &QNetworkConfigurationManager::configurationRemoved,
		this, [this] { m_interfaces = QNetworkInterface::allInterfaces(); });

	// Create socket
	m_sock = new QUdpSocket(this);

	// Read incomming packets
	connect(m_sock, &QUdpSocket::readyRead,
		this, &HostUdpServer::readyRead);
}


HostUdpServer::~HostUdpServer()
{
}


void HostUdpServer::start()
{
	//Logger(LOG_DEBUG) << "HostUdpServer: Binding to UDP port";
	if (!m_sock->bind(QHostAddress::Any, HOST_UDPPORT, QUdpSocket::ReuseAddressHint))
	{
		Logger(LOG_ERROR) << tr("Failed to bind to UDP port ") << HOST_UDPPORT;
	}
	else
	{
		//Logger(LOG_DEBUG) << "HostUdpServer: Bound to UDP port";
	}

	//Logger(LOG_DEBUG) << "HostUdpServer: Joining multicast group";
	if (!m_sock->joinMulticastGroup(QHostAddress(IPV6_MULTICAST)))
	{
		Logger(LOG_WARNING) << tr("Failed to join multicast group %1").arg(IPV6_MULTICAST);
	}
	else
	{
		//Logger(LOG_DEBUG) << "HostUdpServer: Multicast group joined";
	}
}


bool HostUdpServer::parseDatagram(const QNetworkDatagram& datagram, const QString& MAC)
{
	QByteArray response = m_statusInterface->processPacket(datagram.data(), MAC);

	if (response.isEmpty())
		return false;

	QHostAddress senderAddress = datagram.senderAddress();
	int senderPort = datagram.senderPort();

	// Store a list of any hosts we have received packets from
	if (!m_serverAddresses.contains(QPair<QHostAddress, int>(senderAddress, senderPort)))
	{
		m_serverAddresses.append(QPair<QHostAddress, int>(senderAddress, senderPort));
	}

	// Send response packet
	m_sock->writeDatagram(response, senderAddress, senderPort);
	//qInfo() << "Response sent " << QTime::currentTime().toString();

	return true;
}


void HostUdpServer::readyRead()
{
	while (m_sock->hasPendingDatagrams())
	{
		// Read entire datagram
		QNetworkDatagram datagram = m_sock->receiveDatagram();
		uint ifaceIndex = datagram.interfaceIndex();

		// Determine the interface the datagram was received on
		QNetworkInterface iface;
		if (0 != ifaceIndex)
		{
			iface = QNetworkInterface::interfaceFromIndex(ifaceIndex);
		}
		else if (!datagram.senderAddress().isLoopback())
		{
			// 0 means the interface index is unknown, greaaaaaaaaaaaat....
			static QNetworkInterface s_defaultIface;
			if (!s_defaultIface.isValid())
			{
				// Just find the first ethernet adapter that's plugged in
				for (auto& ifaceiter : m_interfaces)
				{
					if (ifaceiter.isValid() &&
						ifaceiter.type() == QNetworkInterface::Ethernet &&
						ifaceiter.flags().testFlag(QNetworkInterface::IsUp) &&
						ifaceiter.flags().testFlag(QNetworkInterface::IsRunning) &&
						!ifaceiter.flags().testFlag(QNetworkInterface::IsLoopBack))
					{
						s_defaultIface = ifaceiter;
						break;
					}
				}
			}

			iface = s_defaultIface;
		}

		// Get the MAC address for the interface
		QString MAC;
		if (!iface.isValid())
		{
			//MAC = tr("(No valid adapters found)");
		}
		else if (QNetworkInterface::Ethernet == iface.type())
		{
			MAC = iface.hardwareAddress();
		}
		else if (QNetworkInterface::Loopback == iface.type())
		{
			// Leave loopback blank
			//MAC = tr("Loopback");
		}
		else
		{
			MAC = QtEnumToString(iface.type()) + " " + iface.humanReadableName() + "(" + iface.name() + ")";
		}
		//qInfo() << "Packet received length " << datagram.data().length();

		if (!parseDatagram(datagram, MAC))
		{
			Logger(LOG_DEBUG) << tr("Bad UDP packet received from %1:%2")
				.arg(datagram.senderAddress().toString())
				.arg(datagram.senderPort());
		}
	}
}


void HostUdpServer::sendPacketToServers(const QByteArray& packet)
{
	for (const auto& server : m_serverAddresses)
	{
		m_sock->writeDatagram(packet, server.first, server.second);
	}
}

