#include "PasswordReset.h"
#include "../common/HostClient.h"

#include <QDebug>
#include <QEventLoop>


PasswordReset::PasswordReset(QObject *parent)
	: QObject(parent)
{
}


PasswordReset::~PasswordReset()
{
}


bool PasswordReset::exec()
{
	HostClient* hostClient = new HostClient("127.0.0.1", HOST_TCPPORT, QString(), true, false, this);

	bool success = false;

	qDebug() << "Connecting to server to reset password...";

	connect(hostClient, &HostClient::connected,
		this, [hostClient]()
	{
		// Reset password
		qDebug() << "Sending password reset request...";
		hostClient->setPassword("");
	});

	connect(hostClient, &HostClient::connectFailed,
		this, [hostClient](const QString& reason)
	{
		qWarning() << "Failed to connect to localhost:" << reason;
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandError,
		this, [hostClient]()
	{
		qWarning() << "Command error";
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandSuccess,
		this, [hostClient, &success]()
	{
		qDebug() << "Password reset";
		success = true;
		hostClient->deleteLater();
	});

	// Block until we receive a response from the server
	QEventLoop eventLoop(this);
	connect(hostClient, &HostClient::destroyed,
		&eventLoop, &QEventLoop::quit);
	eventLoop.exec();

	return success;
}

