#pragma once

#include <QObject>

class Settings;
class ProxyServer;

class WebInterface : public QObject
{
	Q_OBJECT

public:
	WebInterface(Settings* settings, ProxyServer* proxyServer, QObject *parent);
	~WebInterface();

private:
	Settings* m_settings = nullptr;
	ProxyServer* m_proxyServer = nullptr;
};
