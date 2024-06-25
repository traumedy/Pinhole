#pragma once

#include <QObject>

class Settings;
class StatusInterface;
class QUdpSocket;
class QNetworkDatagram;
class QNetworkInterface;
class QHostAddress;

class HostUdpServer : public QObject
{
	Q_OBJECT

public:
	HostUdpServer(Settings* settings, StatusInterface* statusInterface, QObject *parent = nullptr);
	~HostUdpServer();

public slots:
	void readyRead();
	void start();
	void sendPacketToServers(const QByteArray& packet);

private:
	bool parseDatagram(const QNetworkDatagram& datagram, const QString& MAC);

	QList<QNetworkInterface> m_interfaces;
	Settings* m_settings = nullptr;
	StatusInterface* m_statusInterface = nullptr;
	QUdpSocket* m_sock = nullptr;
	QList<QPair<QHostAddress, int>> m_serverAddresses;
};
