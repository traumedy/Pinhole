#include "UdpInterface.h"
#include "Settings.h"
#include "ProxyServer.h"
#include "../common/PinholeCommon.h"

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QJsonDocument>
#include <QJsonObject>

UdpInterface::UdpInterface(Settings* settings, ProxyServer* proxyServer, QObject *parent)
	: QObject(parent), m_settings(settings), m_proxyServer(proxyServer)
{
	// Create UDP server socket
	m_udpSocket = new QUdpSocket(this);
	m_udpSocket->bind(HOST_UDPPORT);
	// Read incomming packets
	connect(m_udpSocket, &QUdpSocket::readyRead,
		this, &UdpInterface::pinholeDatagram);

}


UdpInterface::~UdpInterface()
{
}


void UdpInterface::pinholeDatagram()
{
	while (m_udpSocket->hasPendingDatagrams())
	{
		// Read entire datagram
		QNetworkDatagram datagram = m_udpSocket->receiveDatagram();

		// Parse the data as JSON
		QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram.data());
		// Validate the data
		if (!jsonDoc.isNull())
		{
			QJsonObject jsonObject = jsonDoc.object();
			QString command = jsonObject[QString(TAG_COMMAND)].toString();
			if (command == UDPCOMMAND_QUERY)
			{
				for (const auto& server : m_proxyServer->serverList())
				{
					if (server->isIdentified())
					{
						QJsonObject jsonResponse;
						jsonResponse[TAG_COMMAND] = UDPCOMMAND_REDIRECT;
						jsonResponse[TAG_ID] = server->id();
						jsonResponse[TAG_ADDRESS] = server->addresss();
						jsonResponse[TAG_NAME] = server->name();
						jsonResponse[TAG_ROLE] = server->role();
						jsonResponse[TAG_VERSION] = server->version();
						jsonResponse[TAG_STATUS] = server->status();
						jsonResponse[TAG_PLATFORM] = server->platform();
						jsonResponse[TAG_OS] = server->os();
						jsonResponse[TAG_PORT] = server->port();
						m_udpSocket->writeDatagram(QJsonDocument(jsonResponse).toJson(), datagram.senderAddress(), datagram.senderPort());
					}
				}
			}
		}
	}
}

