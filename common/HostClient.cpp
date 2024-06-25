#include "HostClient.h"

#include "../common/Utilities.h"
#include "../qmsgpack/msgpack.h"

#include <QSslSocket>
#include <QSslCertificate>
#include <QSslKey>
#include <QSharedMemory>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QHostInfo>
#include <QVersionNumber>
#include <QtEndian>
#ifndef CONSOLE
#include <QInputDialog>
#endif

#define RECONNECT_FREQUENCY		250
#define PING_FREQUENCY			10000
#define FILENAME_CLIENTKEY		"client.key"
#define FILENAME_CLIENTCERT		"client.pem"

QString HostClient::s_lastPassword;
QMap<QString, QString> HostClient::s_passwordMap;
QSharedPointer<QSslCertificate> HostClient::s_cert = nullptr;
QSharedPointer<QSslKey> HostClient::s_key = nullptr;


HostClient::HostClient(const QString& address, int port, const QString& id, bool specialLoopback, bool helperClient, QObject *parent)
	: QObject(parent), m_hostAddress(address), m_hostPort(port), m_hostId(id), m_specialLoopback(specialLoopback),
	m_helperClient(helperClient)
{
	// Initialize static members
	if (s_cert.isNull())
		s_cert = QSharedPointer<QSslCertificate>::create();
	if (s_key.isNull())
		s_key = QSharedPointer<QSslKey>::create();

	m_socket = new QSslSocket(this);

	connect(m_socket, &QSslSocket::readyRead, 
		this, &HostClient::rx);
	connect(m_socket, &QSslSocket::connected, 
		this, &HostClient::serverConnect);
	connect(m_socket, &QSslSocket::disconnected, 
		this, &HostClient::serverDisconnect);
	connect(m_socket, &QSslSocket::encrypted, 
		this, &HostClient::socketEncrypted);

	connect(m_socket, (void (QSslSocket::*)(const QList<QSslError>&))&QSslSocket::sslErrors,
		this, &HostClient::sslErrors);
	connect(m_socket, (void (QSslSocket::*)(QAbstractSocket::SocketError))&QSslSocket::error,
		this, [this](QAbstractSocket::SocketError socketError)
	{
#ifdef QT_DEBUGxx
		qDebug() << __FUNCTION__ << QtEnumToString(socketError);
#endif

		switch (socketError)
		{
		case QAbstractSocket::ConnectionRefusedError:
		case QAbstractSocket::SocketTimeoutError:
		case QAbstractSocket::NetworkError:
			emit connectFailed(QtEnumToString(socketError));
			if (m_reconnect && !m_closing)
			{
				m_reconnectTimer.start();
			}
			break;

		default:
			break;
		}
	});

	m_reconnectTimer.setInterval(RECONNECT_FREQUENCY);
	m_reconnectTimer.setSingleShot(true);
	connect(&m_reconnectTimer, &QTimer::timeout, 
		this, [this]()
	{
		connectToServer();
	});

	m_pingTimer.setInterval(PING_FREQUENCY);
	m_pingTimer.setSingleShot(false);
	connect(&m_pingTimer, &QTimer::timeout,
		this, [this]()
	{
		// Send noop ping
		QVariantList vlist;
		vlist << CMD_NOOP;
		sendVariantList(vlist);
	});

	QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
	if (!QDir(dataDir).exists())
	{
		QDir().mkpath(dataDir);
	}
	QFile keyFile(dataDir + FILENAME_CLIENTKEY);
	QFile certFile(dataDir + FILENAME_CLIENTCERT);

	// Load keys if we don't have them
	if (s_key->isNull() || s_cert->isNull())
	{
		if (keyFile.exists() && certFile.exists() &&
			keyFile.size() > 0 && certFile.size() > 0)
		{
			keyFile.open(QIODevice::ReadOnly);
			*s_key = QSslKey(keyFile.readAll(), QSsl::Rsa);
			keyFile.close();

			certFile.open(QIODevice::ReadOnly);
			*s_cert = QSslCertificate(certFile.readAll());
			certFile.close();
		}
	}

	// Generate them if we couldn't load them
	if (s_key->isNull() || s_cert->isNull())
	{
		QPair<QSslCertificate, QSslKey> certPair = GenerateCertKeyPair("US", "Obscura", "Obscura LLC");
		*s_key = certPair.second;
		*s_cert = certPair.first;

		// Save to file
		keyFile.open(QIODevice::WriteOnly);
		keyFile.write(s_key->toPem());
		keyFile.close();

		certFile.open(QIODevice::WriteOnly);
		certFile.write(s_cert->toPem());
		certFile.close();
	}

	m_socket->setLocalCertificate(*s_cert);
	m_socket->setPrivateKey(*s_key);
	m_socket->setPeerVerifyMode(QSslSocket::VerifyNone);

	connectToServer();
}


HostClient::~HostClient()
{
	m_closing = true;

	if (m_socket->state() == QAbstractSocket::SocketState::ConnectedState)
	{
		m_socket->flush();
		m_socket->disconnectFromHost();
	}
}


bool HostClient::isConnected() const
{
	return m_socket->state() == QAbstractSocket::SocketState::ConnectedState;
}


QString HostClient::getHostAddress() const
{
	return m_hostAddress;
}


QString HostClient::getHostName() const
{
	return m_hostName;
}


QString HostClient::getHostVersion() const
{
	return m_hostVersion;
}


QString HostClient::getHostId() const
{
	return m_hostId;
}


void HostClient::startApps(const QStringList& names) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_STARTAPPS << names;
	sendVariantList(vlist);
}


void HostClient::startStartupApps() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_STARTSTARTUPAPPS;
	sendVariantList(vlist);
}


void HostClient::restartApps(const QStringList& names) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_RESTARTAPPS << names;
	sendVariantList(vlist);
}


void HostClient::stopApps(const QStringList& names) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_STOPAPPS << names;
	sendVariantList(vlist);
}


void HostClient::stopAllApps() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_STOPALLAPPS;
	sendVariantList(vlist);
}


void HostClient::startAppVariables(const QString& appName, const QStringList & variables) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_STARTVARS << appName << variables;
	sendVariantList(vlist);
}


void HostClient::addApplication(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_ADDAPP << name;
	sendVariantList(vlist);
}


void HostClient::deleteApplication(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_DELETEAPP << name;
	sendVariantList(vlist);
}


void HostClient::renameApplication(const QString& name, const QString& newName) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_RENAMEAPP << name << newName;
	sendVariantList(vlist);
}


void HostClient::getApplicationConsoleOutput(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_GETCONSOLE << name;
	sendVariantList(vlist);
}


void HostClient::startGroup(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GROUP << CMD_GROUP_STARTGROUP << name;
	sendVariantList(vlist);
}


void HostClient::stopGroup(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GROUP << CMD_GROUP_STOPGROUP << name;
	sendVariantList(vlist);
}


void HostClient::addGroup(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GROUP << CMD_GROUP_ADDGROUP << name;
	sendVariantList(vlist);
}

void HostClient::deleteGroup(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GROUP << CMD_GROUP_DELETEGROUP << name;
	sendVariantList(vlist);
}

void HostClient::renameGroup(const QString& oldName, const QString& newName) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GROUP << CMD_GROUP_RENAMEGROUP << oldName << newName;
	sendVariantList(vlist);
}


void HostClient::addScheduleEvent(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_SCHEDULE << CMD_SCHED_ADDEVENT << name;
	sendVariantList(vlist);
}


void HostClient::deleteScheduleEvent(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_SCHEDULE << CMD_SCHED_DELETEEVENT << name;
	sendVariantList(vlist);
}


void HostClient::renameScheduleEvent(const QString& oldName, const QString& newName) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_SCHEDULE << CMD_SCHED_RENAMEEVENT << oldName << newName;
	sendVariantList(vlist);
}


void HostClient::triggerScheduleEvents(const QStringList& names) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_SCHEDULE << CMD_SCHED_TRIGGEREVENTS << names;
	sendVariantList(vlist);
}


void HostClient::getVariant(const QString& group, const QString& name, const QString& propName) const
{
	QVariantList vlist;
	vlist << CMD_QUERY << group << name << propName;
	sendVariantList(vlist);
}


void HostClient::setVariant(const QString& group, const QString& name, const QString& propName, const QVariant& value) const
{
	QVariantList vlist;
	vlist << CMD_VALUE << group << name << propName << value;
	sendVariantList(vlist);
}


void HostClient::subscribeToCommand(const QString& command) const
{
	QVariantList vlist;
	vlist << CMD_SUBSCRIBECMD << command;
	sendVariantList(vlist);
}


void HostClient::subscribeToGroup(const QString& group) const
{
	QVariantList vlist;
	vlist << CMD_SUBSCRIBEGROUP << group;
	sendVariantList(vlist);
}


void HostClient::setPassword(const QString& password) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_SETPASSWORD << password;
	sendVariantList(vlist);
}


void HostClient::shutdownHost() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GLOBAL << CMD_GLOBAL_SHUTDOWN;
	sendVariantList(vlist);
}


void HostClient::rebootHost() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GLOBAL << CMD_GLOBAL_REBOOT;
	sendVariantList(vlist);
}


void HostClient::logMessage(const QString& message, int level) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_LOGMESSAGE << level << message;
	sendVariantList(vlist);
}


void HostClient::requestScreenshot() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_GETSCREENSHOT;
	sendVariantList(vlist);
}


void HostClient::sendScreenshot(const QByteArray& screenshot) const
{
	QVariantList vlist;
	vlist << CMD_SCREENSHOT << screenshot;
	sendVariantList(vlist);
}


void HostClient::requestTerminate() const
{
	QVariantList vlist;
	vlist << CMD_TERMINATE;
	sendVariantList(vlist);
}


void HostClient::requestExportSettings() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_EXPORTSETTINGS;
	sendVariantList(vlist);
}


void HostClient::sendImportData(const QByteArray& importData) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_IMPORTSETTINGS << importData;
	sendVariantList(vlist);
}


void HostClient::showScreenIds() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_SHOWSCREENIDS;
	sendVariantList(vlist);
}


void HostClient::retrieveLog(const QString& startDate, const QString& endDate) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_NONE << CMD_NONE_RETRIEVELOG << startDate << endDate;
	sendVariantList(vlist);
}


void HostClient::retrieveSystemInfo() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_GLOBAL << CMD_GLBOAL_SYSINFO;
	sendVariantList(vlist);
}


void HostClient::retrieveAlertList() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_ALERT << CMD_ALERT_RETRIEVELIST;
	sendVariantList(vlist);
}


void HostClient::addAlertSlot(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_ALERT << CMD_ALERT_ADDSLOT << name;
	sendVariantList(vlist);
}


void HostClient::deleteAlertSlot(const QString& name) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_ALERT << CMD_ALERT_DELETESLOT << name;
	sendVariantList(vlist);
}


void HostClient::renameAlertSlot(const QString& oldName, const QString& newName) const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_ALERT << CMD_ALERT_RENAMESLOT << oldName << newName;
	sendVariantList(vlist);
}


void HostClient::resetAlertCount() const
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_ALERT << CMD_ALERT_RESETCOUNT;
	sendVariantList(vlist);
}


void HostClient::executeCommand(const QByteArray& executable, const QString& command, const QString& arguments, const QString& directory, bool capture, bool elevated)
{
	QVariantList vlist;
	vlist << CMD_COMMAND << GROUP_APP << CMD_APP_EXECUTE << executable << command << arguments << directory << capture << elevated;
	sendVariantList(vlist);
}


void HostClient::connectToServer() const
{
#ifdef QT_DEBUGxx
	qDebug() << "Connecting to host " << m_hostAddress;
#endif
	m_socket->connectToHostEncrypted(m_hostAddress, m_hostPort);
}


void HostClient::socketEncrypted()
{
	// Connected and encrypted, send auth packet
	// todo OS specific stuff
	QString password;

	if (m_specialLoopback)
	{
		if (!getSharedPassword(password))
		{
			// Problem getting shared password
			m_socket->disconnectFromHost();
			m_reconnect = false;
			return;
		}
	}
	else
	{
		if (!s_passwordMap.contains(m_hostAddress))
		{
			s_passwordMap[m_hostAddress] = s_lastPassword;
		}

		password = s_passwordMap[m_hostAddress];
	}

	sendVariantList(makeAuthPacket(password));
}


void HostClient::sslErrors(const QList<QSslError> &errors)
{
#ifdef QT_DEBUG
	foreach(const QSslError &error, errors)
		qDebug() << __FUNCTION__ << error.errorString();
#else
	Q_UNUSED(errors);
#endif
}


void HostClient::serverDisconnect(void)
{
#ifdef QT_DEBUGxx
	qDebug() << "Host disconnected";
#endif
	emit disconnected();
	// Reconnect
	if (m_reconnect && !m_closing)
	{
		m_reconnectTimer.start();
	}

	// Stop pinging
	m_pingTimer.stop();
}


void HostClient::serverConnect(void)
{
#ifdef QT_DEBUGxx
	qDebug() << "Host connected";
#endif
	//emit connected();
}


void HostClient::rx(void)
{
	do
	{
		if (dataLeft == 0)
		{
			QByteArray sizeArray = m_socket->read(sizeof(uint32_t));
			uint32_t size;
			memcpy(&size, sizeArray.data(), sizeof(size));
			dataLeft = qFromLittleEndian(size);
		}

		QByteArray newdata = m_socket->read(dataLeft);
		data += newdata;
		dataLeft -= newdata.size();

		if (!(0 == dataLeft && !data.isEmpty()))
		{
			// More data expected
			return;
		}

		// Parse as msgpack data
		QVariant var = MsgPack::unpack(data);

		// Clear data
		data.clear();

		// Validate data
		if (!var.isValid())
		{
			qWarning() << "Data is invalid from host " << m_hostAddress;
			return;
		}

		if (var.isNull())
		{
			qWarning() << "Data is null from host " << m_hostAddress;
			return;
		}

		if (!var.canConvert<QVariantList>())
		{
			qWarning() << "Data is not QVariantList from host " << m_hostAddress;
			return;
		}

		QVariantList vlist = var.value<QVariantList>();

		if (!vlist[0].canConvert<QString>())
		{
			qWarning() << "First data item not string from host " << m_hostAddress;
			return;
		}

		QString command = vlist[0].toString();

		if (CMD_TERMINATE == command)
		{
			emit terminationRequested();
		}
		else  if (CMD_AUTH == command)
		{
			int success = vlist[1].toInt();
			QString message = vlist[2].toString();

			if (0 == success)
			{
				if (m_specialLoopback)
				{
					emit connectFailed(tr("Password error"));
					m_reconnect = false;
					m_socket->disconnectFromHost();
				}
				else
				{
#ifdef CONSOLE
					emit passwordFailure();
#else
					bool ok;
					QString password = QInputDialog::getText(qobject_cast<QWidget*>(parent()), message,
						tr("Enter password for host ") + m_hostAddress, QLineEdit::EchoMode::Password,
						QString(), &ok);

					if (ok)
					{
						s_lastPassword = password;
						s_passwordMap[m_hostAddress] = password;
						sendVariantList(makeAuthPacket(password));
					}
					else
					{
						m_reconnect = false;
						m_socket->disconnectFromHost();
						emit connectFailed(tr("Password error"));
						return;
					}
#endif
				}
			}
			else
			{
				// Server returns host name in message
				m_hostName = message;
				m_hostVersion = vlist[3].toString();

				QVersionNumber serverVer = QVersionNumber::fromString(m_hostVersion);
				if (serverVer >= QVersionNumber(QVector<int>({ 0, 7, 5 })))
				{
					// Keep-alive pings for server versions that support it
					m_pingTimer.start();
				}

				if (serverVer >= QVersionNumber(QVector<int>({ 0, 9, 0 })))
				{
					// Verify this is the correct server
					QString actualHostId = vlist[4].toString();
					if (m_hostId.isEmpty())
					{
						// Blank host id allows connection without verification 
						// (console client, special clients)
						m_hostId = actualHostId;
					} 
					else if (actualHostId != m_hostId)
					{
						// Not the correct server
						emit connectFailed(tr("Incorrect server"));
						return;
					}
				}

				emit connected();
			}
		}
		else if (CMD_VALUE == command)
		{
			QString group = vlist[1].toString();
			QString item = vlist[2].toString();
			QString property = vlist[3].toString();
			QVariant value = vlist[4];
			emit valueUpdate(group, item, property, value);
		}
		else if (CMD_VALUESET == command)
		{
			QString group = vlist[1].toString();
			QString item = vlist[2].toString();
			QString property = vlist[3].toString();
			bool success = vlist[4].toBool();
			emit valueSet(group, item, property, success);
		}
		else if (CMD_MISSING == command)
		{
			QString group = vlist[1].toString();
			QString item = vlist[2].toString();
			QString property = vlist[3].toString();
			emit missingValue(group, item, property);
		}
		else if (CMD_MESSAGE == command)
		{
			QString text1 = vlist[1].toString();
			QString text2 = vlist[2].toString();
			emit hostMessage(m_hostAddress, text1, text2);
		}
		else if (CMD_LOG == command)
		{
			int level = vlist[1].toInt();
			QString message = vlist[2].toString();
			emit hostLog(level, message);
		}
		else if (CMD_CMDUNKNOWN == command)
		{
			QString unknownCommand = vlist[1].toString();
			emit commandUnknown(unknownCommand);
		}
		else if (CMD_CMDRESPONSE == command)
		{
			QString group = vlist[1].toString();
			QString subCommand = vlist[2].toString();
			int errorCode = vlist[3].toInt();
			switch (errorCode)
			{
			case CMD_RESPONSE_SUCCESS:
				emit commandSuccess(group, subCommand);
				break;
			case CMD_RESPONSE_ERROR:
				emit commandError(group, subCommand);
				break;
			case CMD_RESPONSE_UNKNOWN:
				emit commandMissing(group, subCommand);
				break;
			case CMD_RESPONSE_DATA:
				emit commandData(group, subCommand, vlist[4]);
				break;
			}
		}
		else if (CMD_SCREENSHOT == command)
		{
			emit commandScreenshot();
		}
		else if (CMD_SHOWSCREENIDS == command)
		{
			emit commandShowScreenIds();
		}
		else if (CMD_CONTROLWINDOW == command)
		{
			int pid = vlist[1].toInt();
			QString display = vlist[2].toString();
			QString command = vlist[3].toString();
			emit commandControlWindow(pid, display, command);
		}
	} while (m_socket->bytesAvailable());
}


void HostClient::sendVariantList(const QVariantList& vlist) const
{
	static QByteArray sizeArray(sizeof(uint32_t), 0);
	QByteArray dataArray(MsgPack::pack(vlist));
	uint32_t size = qToLittleEndian(dataArray.size());
	memcpy(sizeArray.data(), &size, sizeof(size));
	// Send size
	m_socket->write(sizeArray);
	// Send data
	m_socket->write(dataArray);
	m_socket->flush();
}


void HostClient::addAddressPassword(const QString& address, const QString& password)
{
	s_passwordMap[address] = password;
}


bool HostClient::getSharedPassword(QString& password) const
{
	// Read password from shared memory object
	QSharedMemory sharedMem(SHAREDMEM_PASS);

	bool lock = true;

	if (!sharedMem.attach(QSharedMemory::ReadOnly))
	{
		qDebug() << tr("'%1' error %2 connecting to shared memory '%3'")
			.arg(sharedMem.errorString())
			.arg(sharedMem.error())
			.arg(sharedMem.nativeKey());

#if defined(Q_OS_WIN)
		if (sharedMem.error() == QSharedMemory::NotFound)
		{
			// Try to connect to global shared password if server is running as service
			sharedMem.setNativeKey("Global\\" SHAREDMEM_PASS);
			if (!sharedMem.attach(QSharedMemory::ReadOnly))
			{
				qDebug() << tr("'%1' error %2 connecting to shared memory '%3'")
					.arg(sharedMem.errorString())
					.arg(sharedMem.error())
					.arg(sharedMem.nativeKey());
				return false;
			}
			else
			{
				lock = false;
			}
		}
#else
		return false;
#endif

	}

	if (lock)
	{
		if (!sharedMem.lock())
		{
			qDebug() << tr("'%1' error %2 locking shared memory '%3'")
				.arg(sharedMem.errorString())
				.arg(sharedMem.error())
				.arg(sharedMem.nativeKey());
			return false;
		}
	}

	QByteArray passData;
	passData.resize(SHAREDPASS_LEN);
	memcpy(passData.data(), sharedMem.data(), SHAREDPASS_LEN);

	if (lock)
	{
		if (!sharedMem.unlock())
		{
			qDebug() << tr("'%1' error %2 unlocking shared memory '%3'")
				.arg(sharedMem.errorString())
				.arg(sharedMem.error())
				.arg(sharedMem.nativeKey());
		}
	}

	QString key = PINHOLE_ORG_DOMAIN;
	for (int n = 0; n < passData.length(); n++)
	{
		passData[n] = passData[n] ^ key[n % key.length()].toLatin1();
	}

	password = QString::fromUtf8(passData);

	return true;
}


QVariantList HostClient::makeAuthPacket(const QString& password)
{
	QVariantList vlist;
	vlist << CMD_AUTH << QCoreApplication::applicationVersion() << QHostInfo::localHostName() << password << m_helperClient;
	return vlist;
}


