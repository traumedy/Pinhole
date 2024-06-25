#include "WebInterface.h"
#include "ProxyServer.h"

WebInterface::WebInterface(Settings* settings, ProxyServer* proxyServer, QObject *parent)
	: QObject(parent), m_settings(settings), m_proxyServer(proxyServer)
{
}


WebInterface::~WebInterface()
{
}
