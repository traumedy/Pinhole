#pragma once

#include <QObject>

class Settings;
class QUdpSocket;
class ProxyServer;

class UdpInterface : public QObject
{
	Q_OBJECT

public:
	UdpInterface(Settings* settings, ProxyServer* proxyServer, QObject *parent);
	~UdpInterface();

private slots:
	void pinholeDatagram();

private:
	QUdpSocket* m_udpSocket = nullptr;
	Settings* m_settings = nullptr;
	ProxyServer* m_proxyServer = nullptr;
};
