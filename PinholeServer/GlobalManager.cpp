#include "GlobalManager.h"
#include "Settings.h"
#include "Logger.h"
#include "Sigar.h"
#include "Values.h"
#include "../common/Utilities.h"
#include "../common/Version.h"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QSysInfo>
#include <QStorageInfo>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>


#if defined(Q_OS_WIN)
#include "WinUtil.h"
#elif defined(Q_OS_MAC)
#include "MacUtil.h"
#elif defined(Q_OS_UNIX)
#include "LinuxUtil.h"
#endif


GlobalManager::GlobalManager(Settings* settings, QObject *parent)
	: QObject(parent), m_settings(settings)
{
	readGlobalSettings();
}


GlobalManager::~GlobalManager()
{
}


bool GlobalManager::readGlobalSettings()
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	setRole(settings->value(PROP_GLOBAL_ROLE, "").toString());
	setHostLogLevel(settings->value(PROP_GLOBAL_HOSTLOGLEVEL, LOG_LEVEL_NORMAL).toString());
	setRemoteLogLevel(settings->value(PROP_GLOBAL_REMOTELOGLEVEL, LOG_LEVEL_NORMAL).toString());
	setAppTerminateTimeout(settings->value(PROP_GLOBAL_TERMINATETIMEOUT, DEFAULT_TERMINATE_TO).toInt());
	setAppHeartbeatTimeout(settings->value(PROP_GLOBAL_HEARTBEATTIMEOUT, DEFAULT_HEARTBEAT_TO).toInt());
	setCrashPeriod(settings->value(PROP_GLOBAL_CRASHPERIOD, DEFAULT_CRASHPERIOD).toInt());
	setCrashCount(settings->value(PROP_GLOBAL_CRASHCOUNT, DEFAULT_CRASHCOUNT).toInt());
	setTrayLaunch(settings->value(PROP_GLOBAL_TRAYLAUNCH, true).toBool());
	setTrayControl(settings->value(PROP_GLOBAL_TRAYCONTROL, false).toBool());
	setHttpEnabled(settings->value(PROP_GLOBAL_HTTPENABLED, false).toBool());
	setHttpPort(settings->value(PROP_GLOBAL_HTTPPORT, DEFAULT_HTTPPORT).toInt());
	setBackendServer(settings->value(PROP_GLOBAL_BACKENDSERVER, "").toString());
	setNovaSite(settings->value(PROP_GLOBAL_NOVASITE, "").toString());
	setNovaArea(settings->value(PROP_GLOBAL_NOVAAREA, "").toString());
	setNovaDisplay(settings->value(PROP_GLOBAL_NOVADISPLAY, "").toString());
	setNovaTcpEnabled(settings->value(PROP_GLOBAL_NOVATCPENABLED, false).toBool());
	setNovaTcpAddress(settings->value(PROP_GLOBAL_NOVATCPADDRESS, DEFAULT_NOVATCPADDRESS).toString());
	setNovaTcpPort(settings->value(PROP_GLOBAL_NOVATCPPORT, DEFAULT_NOVATCPPORT).toInt());
	setNovaUdpEnabled(settings->value(PROP_GLOBAL_NOVAUDPENABLED, false).toBool());
	setNovaUdpAddress(settings->value(PROP_GLOBAL_NOVAUDPADDRESS, "").toString());
	setNovaUdpPort(settings->value(PROP_GLOBAL_NOVAUDPPORT, DEFAULT_NOVAUDPPORT).toInt());
	setAlertMemory(settings->value(PROP_GLOBAL_ALERTMEMORY, false).toBool());
	setMinMemory(settings->value(PROP_GLOBAL_MINMEMORY, 0).toInt());
	setAlertDisk(settings->value(PROP_GLOBAL_ALERTDISK, false).toBool());
	setMinDisk(settings->value(PROP_GLOBAL_MINDISK, 0).toInt());
	setAlertDiskList(settings->value(PROP_GLOBAL_ALERTDISKLIST, QStringList()).toStringList());

	return true;
}


bool GlobalManager::writeGlobalSettings() const
{
#ifdef QT_DEBUG
	//qDebug() << "Writing global settings" << settings->fileName() << settings->isWritable();
#endif

	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	settings->setValue(PROP_GLOBAL_ROLE, getRole());
	settings->setValue(PROP_GLOBAL_HOSTLOGLEVEL, getHostLogLevel());
	settings->setValue(PROP_GLOBAL_REMOTELOGLEVEL, getRemoteLogLevel());
	settings->setValue(PROP_GLOBAL_TERMINATETIMEOUT, getAppTerminateTimeout());
	settings->setValue(PROP_GLOBAL_HEARTBEATTIMEOUT, getAppHeartbeatTimeout());
	settings->setValue(PROP_GLOBAL_CRASHPERIOD, getCrashPeriod());
	settings->setValue(PROP_GLOBAL_CRASHCOUNT, getCrashCount());
	settings->setValue(PROP_GLOBAL_TRAYLAUNCH, getTrayLaunch());
	settings->setValue(PROP_GLOBAL_TRAYCONTROL, getTrayControl());
	settings->setValue(PROP_GLOBAL_HTTPENABLED, getHttpEnabled());
	settings->setValue(PROP_GLOBAL_HTTPPORT, getHttpPort());
	settings->setValue(PROP_GLOBAL_BACKENDSERVER, getBackendServer());
	settings->setValue(PROP_GLOBAL_NOVASITE, getNovaSite());
	settings->setValue(PROP_GLOBAL_NOVAAREA, getNovaArea());
	settings->setValue(PROP_GLOBAL_NOVADISPLAY, getNovaDisplay());
	settings->setValue(PROP_GLOBAL_NOVATCPENABLED, getNovaTcpEnabled());
	settings->setValue(PROP_GLOBAL_NOVATCPADDRESS, getNovaTcpAddress());
	settings->setValue(PROP_GLOBAL_NOVATCPPORT, getNovaTcpPort());
	settings->setValue(PROP_GLOBAL_NOVAUDPENABLED, getNovaUdpEnabled());
	settings->setValue(PROP_GLOBAL_NOVAUDPADDRESS, getNovaUdpAddress());
	settings->setValue(PROP_GLOBAL_NOVAUDPPORT, getNovaUdpPort());
	settings->setValue(PROP_GLOBAL_ALERTMEMORY, getAlertMemory());
	settings->setValue(PROP_GLOBAL_MINMEMORY, getMinMemory());
	settings->setValue(PROP_GLOBAL_ALERTDISK, getAlertDisk());
	settings->setValue(PROP_GLOBAL_MINDISK, getMinDisk());
	settings->setValue(PROP_GLOBAL_ALERTDISKLIST, getAlertDiskList());

	return true;
}


bool GlobalManager::importSettings(const QJsonObject& root)
{
	setRole(ReadJsonValueWithDefault(root, PROP_GLOBAL_ROLE, getRole()).toString());
	setHostLogLevel(ReadJsonValueWithDefault(root, PROP_GLOBAL_HOSTLOGLEVEL, getHostLogLevel()).toString());
	setRemoteLogLevel(ReadJsonValueWithDefault(root, PROP_GLOBAL_REMOTELOGLEVEL, getRemoteLogLevel()).toString());
	setAppTerminateTimeout(ReadJsonValueWithDefault(root, PROP_GLOBAL_TERMINATETIMEOUT, getAppTerminateTimeout()).toInt());
	setAppHeartbeatTimeout(ReadJsonValueWithDefault(root, PROP_GLOBAL_HEARTBEATTIMEOUT, getAppHeartbeatTimeout()).toInt());
	setCrashPeriod(ReadJsonValueWithDefault(root, PROP_GLOBAL_CRASHPERIOD, getCrashPeriod()).toInt());
	setCrashCount(ReadJsonValueWithDefault(root, PROP_GLOBAL_CRASHCOUNT, getCrashCount()).toInt());
	setTrayControl(ReadJsonValueWithDefault(root, PROP_GLOBAL_TRAYLAUNCH, getTrayLaunch()).toBool());
	setTrayControl(ReadJsonValueWithDefault(root, PROP_GLOBAL_TRAYCONTROL, getTrayControl()).toBool());
	setHttpEnabled(ReadJsonValueWithDefault(root, PROP_GLOBAL_HTTPENABLED, getHttpEnabled()).toBool());
	setHttpPort(ReadJsonValueWithDefault(root, PROP_GLOBAL_HTTPPORT, getHttpPort()).toInt());
	setBackendServer(ReadJsonValueWithDefault(root, PROP_GLOBAL_BACKENDSERVER, getBackendServer()).toString());
	setNovaSite(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVASITE, getNovaSite()).toString());
	setNovaArea(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVAAREA, getNovaArea()).toString());
	setNovaDisplay(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVADISPLAY, getNovaDisplay()).toString());
	setNovaTcpEnabled(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVATCPENABLED, getNovaTcpEnabled()).toBool());
	setNovaTcpAddress(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVATCPADDRESS, getNovaTcpAddress()).toString());
	setNovaTcpPort(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVATCPPORT, getNovaTcpPort()).toInt());
	setNovaUdpEnabled(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVAUDPENABLED, getNovaUdpEnabled()).toBool());
	setNovaUdpAddress(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVAUDPADDRESS, getNovaUdpAddress()).toString());
	setNovaUdpPort(ReadJsonValueWithDefault(root, PROP_GLOBAL_NOVAUDPPORT, getNovaUdpPort()).toInt());
	setAlertMemory(ReadJsonValueWithDefault(root, PROP_GLOBAL_ALERTMEMORY, getAlertMemory()).toBool());
	setMinMemory(ReadJsonValueWithDefault(root, PROP_GLOBAL_MINMEMORY, getMinMemory()).toInt());
	setAlertDisk(ReadJsonValueWithDefault(root, PROP_GLOBAL_ALERTDISK, getAlertDisk()).toBool());
	setMinDisk(ReadJsonValueWithDefault(root, PROP_GLOBAL_MINDISK, getMinDisk()).toInt());
	setAlertDiskList(ReadJsonValueWithDefault(root, PROP_GLOBAL_ALERTDISKLIST, getAlertDiskList()).toStringList());

	return true;
}


QJsonObject GlobalManager::exportSettings() const
{
	QJsonObject root;
	root[PROP_GLOBAL_ROLE] = getRole();
	root[PROP_GLOBAL_HOSTLOGLEVEL] = getHostLogLevel();
	root[PROP_GLOBAL_REMOTELOGLEVEL] = getRemoteLogLevel();
	root[PROP_GLOBAL_TERMINATETIMEOUT] = getAppTerminateTimeout();
	root[PROP_GLOBAL_HEARTBEATTIMEOUT] = getAppHeartbeatTimeout();
	root[PROP_GLOBAL_CRASHPERIOD] = getCrashPeriod();
	root[PROP_GLOBAL_CRASHCOUNT] = getCrashCount();
	root[PROP_GLOBAL_TRAYLAUNCH] = getTrayLaunch();
	root[PROP_GLOBAL_TRAYCONTROL] = getTrayControl();
	root[PROP_GLOBAL_HTTPENABLED] = getHttpEnabled();
	root[PROP_GLOBAL_HTTPPORT] = getHttpPort();
	root[PROP_GLOBAL_BACKENDSERVER] = getBackendServer();
	root[PROP_GLOBAL_NOVASITE] = getNovaSite();
	root[PROP_GLOBAL_NOVAAREA] = getNovaArea();
	root[PROP_GLOBAL_NOVADISPLAY] = getNovaDisplay();
	root[PROP_GLOBAL_NOVATCPENABLED] = getNovaTcpEnabled();
	root[PROP_GLOBAL_NOVATCPADDRESS] = getNovaTcpAddress();
	root[PROP_GLOBAL_NOVATCPPORT] = getNovaTcpPort();
	root[PROP_GLOBAL_NOVAUDPENABLED] = getNovaUdpEnabled();
	root[PROP_GLOBAL_NOVAUDPADDRESS] = getNovaUdpAddress();
	root[PROP_GLOBAL_NOVAUDPPORT] = getNovaUdpPort();
	root[PROP_GLOBAL_ALERTMEMORY] = getAlertMemory();
	root[PROP_GLOBAL_MINMEMORY] = getMinMemory();
	root[PROP_GLOBAL_ALERTDISK] = getAlertDisk();
	root[PROP_GLOBAL_MINDISK] = getMinDisk();
	root[PROP_GLOBAL_ALERTDISKLIST] = QJsonArray::fromStringList(getAlertDiskList());

	return root;
}


QString GlobalManager::getRole() const
{
	return m_role;
}


bool GlobalManager::setRole(const QString& str)
{
	if (m_role != str)
	{
		m_role = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_ROLE, QVariant(m_role));
	}
	return true;
}


QString GlobalManager::getHostLogLevel() const
{
	return m_hostLogLevel;
}


bool GlobalManager::setHostLogLevel(const QString& str)
{
	if (m_hostLogLevel != str)
	{
		if (!m_validLogLevels.contains(str))
		{
			Logger(LOG_EXTRA) << tr("Invalid host log level value '%1'").arg(str);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HOSTLOGLEVEL, QVariant(m_hostLogLevel));
			return false;
		}
		m_hostLogLevel = str;
		Logger::setHostLogLevel(m_hostLogLevel);
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HOSTLOGLEVEL, QVariant(m_hostLogLevel));
	}
	return true;
}


QString GlobalManager::getRemoteLogLevel() const
{
	return m_remoteLogLevel;
}


bool GlobalManager::setRemoteLogLevel(const QString& str)
{
	if (m_remoteLogLevel != str)
	{
		if (!m_validLogLevels.contains(str))
		{
			Logger(LOG_EXTRA) << tr("Invalid remote log level value '%1'").arg(str);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_REMOTELOGLEVEL, QVariant(m_remoteLogLevel));
			return false;
		}
		m_remoteLogLevel = str;
		Logger::setRemoteLogLevel(m_remoteLogLevel);
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_REMOTELOGLEVEL, QVariant(m_remoteLogLevel));
	}
	return true;
}


int GlobalManager::getAppTerminateTimeout() const
{
	return m_appTerminateTimeout;
}


bool GlobalManager::setAppTerminateTimeout(int val)
{
	if (m_appTerminateTimeout != val)
	{
		if (val < 0)
		{
			Logger(LOG_EXTRA) << tr("Invalid global application termination timeout value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_TERMINATETIMEOUT, QVariant(m_appTerminateTimeout));
			return false;
		}
		m_appTerminateTimeout = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_TERMINATETIMEOUT, QVariant(m_appTerminateTimeout));
	}
	return true;
}


int GlobalManager::getAppHeartbeatTimeout() const
{
	return m_appHeartbeatTimeout;
}


bool GlobalManager::setAppHeartbeatTimeout(int val)
{
	if (m_appHeartbeatTimeout != val)
	{
		if (val < MIN_HEARTBEATTIMEOUT || val > MAX_HEARTBEATTIMEOUT)
		{
			Logger(LOG_EXTRA) << tr("Invalid global application heartbeat timeout value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HEARTBEATTIMEOUT, QVariant(m_appHeartbeatTimeout));
			return false;
		}
		m_appHeartbeatTimeout = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HEARTBEATTIMEOUT, QVariant(m_appHeartbeatTimeout));
	}
	return true;
}


int GlobalManager::getCrashPeriod() const
{
	return m_crashPeriod;
}


bool GlobalManager::setCrashPeriod(int val)
{
	if (m_crashPeriod != val)
	{
		if (val < MIN_CRASHPERIOD || val > MAX_CRASHPERIOD)
		{
			Logger(LOG_EXTRA) << tr("Invalid global application crash period value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_CRASHPERIOD, QVariant(m_crashPeriod));
			return false;
		}
		m_crashPeriod = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_CRASHPERIOD, QVariant(m_crashPeriod));
	}
	return true;
}


int GlobalManager::getCrashCount() const
{
	return m_crashCount;
}


bool GlobalManager::setCrashCount(int val)
{
	if (m_crashCount != val)
	{
		if (val < MIN_CRASHCOUNT || val > MAX_CRASHCOUNT)
		{
			Logger(LOG_EXTRA) << tr("Invalid global application crash count value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_CRASHCOUNT, QVariant(m_crashCount));
			return false;
		}
		m_crashCount = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_CRASHCOUNT, QVariant(m_crashCount));
	}
	return true;
}


bool GlobalManager::getTrayLaunch() const
{
	return m_trayLaunch;
}


bool GlobalManager::setTrayLaunch(bool b)
{
	if (m_trayLaunch != b)
	{
		m_trayLaunch = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_TRAYLAUNCH, QVariant(m_trayLaunch));
	}
	return true;
}


bool GlobalManager::getTrayControl() const
{
	return m_trayControl;
}


bool GlobalManager::setTrayControl(bool b)
{
	if (m_trayControl != b)
	{
		m_trayControl = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_TRAYCONTROL, QVariant(m_trayControl));
	}
	return true;
}


bool GlobalManager::getHttpEnabled() const
{
	return m_httpEnabled;
}


bool GlobalManager::setHttpEnabled(bool b)
{
	if (m_httpEnabled != b)
	{
		m_httpEnabled = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HTTPENABLED, QVariant(m_httpEnabled));
	}
	return true;
}


int GlobalManager::getHttpPort() const
{
	return m_httpPort;
}


bool GlobalManager::setHttpPort(int val)
{
	if (m_httpPort != val)
	{
		if (val < MIN_LISTENINGPORT || val > MAX_PORT)
		{
			Logger(LOG_EXTRA) << tr("Invalid HTTP port value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HTTPPORT, QVariant(m_httpPort));
			return false;
		}
		m_httpPort = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_HTTPPORT, QVariant(m_httpPort));
	}
	return true;
}


QString GlobalManager::getBackendServer() const
{
	return m_backendServer;
}


bool GlobalManager::setBackendServer(const QString& str)
{
	if (m_backendServer != str)
	{
		m_backendServer = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_BACKENDSERVER, QVariant(m_backendServer));
	}
	return true;
}


bool GlobalManager::getNovaTcpEnabled() const
{
	return m_novaTcpEnabled;
}


bool GlobalManager::setNovaTcpEnabled(bool b)
{
	if (m_novaTcpEnabled != b)
	{
		m_novaTcpEnabled = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVATCPENABLED, QVariant(m_novaTcpEnabled));
	}
	return true;
}


QString GlobalManager::getNovaTcpAddress() const
{
	return m_novaTcpAddress;
}


bool GlobalManager::setNovaTcpAddress(const QString& str)
{
	if (m_novaTcpAddress != str)
	{
		m_novaTcpAddress = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVATCPADDRESS, QVariant(m_novaTcpAddress));
	}
	return true;
}


int GlobalManager::getNovaTcpPort() const
{
	return m_novaTcpPort;
}


bool GlobalManager::setNovaTcpPort(int val)
{
	if (m_novaTcpPort != val)
	{
		if (val < MIN_LISTENINGPORT || val > MAX_PORT)
		{
			Logger(LOG_EXTRA) << tr("Invalid Nova TCP port value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVATCPPORT, QVariant(m_novaTcpPort));
			return false;
		}
		m_novaTcpPort = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVATCPPORT, QVariant(m_novaTcpPort));
	}
	return true;
}


bool GlobalManager::getNovaUdpEnabled() const
{
	return m_novaUdpEnabled;
}


bool GlobalManager::setNovaUdpEnabled(bool b)
{
	if (m_novaUdpEnabled != b)
	{
		m_novaUdpEnabled = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVAUDPENABLED, QVariant(m_novaUdpEnabled));
	}
	return true;
}


QString GlobalManager::getNovaUdpAddress() const
{
	return m_novaUdpAddress;
}


bool GlobalManager::setNovaUdpAddress(const QString& str)
{
	if (m_novaUdpAddress != str)
	{
		m_novaUdpAddress = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVAUDPADDRESS, QVariant(m_novaUdpAddress));
	}
	return true;
}


int GlobalManager::getNovaUdpPort() const
{
	return m_novaUdpPort;
}


bool GlobalManager::setNovaUdpPort(int val)
{
	if (m_novaUdpPort != val)
	{
		if (val < MIN_LISTENINGPORT || val > MAX_PORT)
		{
			Logger(LOG_EXTRA) << tr("Invalid Nova UDP port value '%1'").arg(val);
			emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVAUDPPORT, QVariant(m_novaUdpPort));
			return false;
		}
		m_novaUdpPort = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVAUDPPORT, QVariant(m_novaUdpPort));
	}
	return true;
}


QString GlobalManager::getNovaSite() const
{
	return m_novaSite;
}


bool GlobalManager::setNovaSite(const QString& str)
{
	if (m_novaSite != str)
	{
		m_novaSite = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVASITE, QVariant(m_novaSite));
	}
	return true;
}


QString GlobalManager::getNovaArea() const
{
	return m_novaArea;
}


bool GlobalManager::setNovaArea(const QString& str)
{
	if (m_novaArea != str)
	{
		m_novaArea = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVAAREA, QVariant(m_novaArea));
	}
	return true;
}


QString GlobalManager::getNovaDisplay() const
{
	return m_novaDisplay;
}


bool GlobalManager::setNovaDisplay(const QString& str)
{
	if (m_novaDisplay != str)
	{
		m_novaDisplay = str;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_NOVADISPLAY, QVariant(m_novaDisplay));
	}
	return true;
}


bool GlobalManager::getAlertMemory() const
{
	return m_alertMemory;
}


bool GlobalManager::setAlertMemory(bool b)
{
	if (m_alertMemory != b)
	{
		m_alertMemory = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_ALERTMEMORY, QVariant(m_alertMemory));
	}
	return true;
}


int GlobalManager::getMinMemory() const
{
	return m_minMemory;
}


bool GlobalManager::setMinMemory(int val)
{
	if (m_minMemory != val)
	{
		m_minMemory = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_MINMEMORY, QVariant(m_minMemory));
	}
	return true;
}


bool GlobalManager::getAlertDisk() const
{
	return m_alertDisk;
}


bool GlobalManager::setAlertDisk(bool b)
{
	if (m_alertDisk != b)
	{
		m_alertDisk = b;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_ALERTDISK, QVariant(m_alertDisk));
	}
	return true;
}


int GlobalManager::getMinDisk() const
{
	return m_minDisk;
}


bool GlobalManager::setMinDisk(int val)
{
	if (m_minDisk != val)
	{
		m_minDisk = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_MINDISK, QVariant(m_minDisk));
	}
	return true;
}


QStringList GlobalManager::getAlertDiskList() const
{
	return m_alertDiskList;
}


bool GlobalManager::setAlertDiskList(const QStringList& val)
{
	if (m_alertDiskList != val)
	{
		m_alertDiskList = val;
		emit valueChanged(GROUP_GLOBAL, QString(), PROP_GLOBAL_ALERTDISKLIST, QVariant(m_alertDiskList));
	}
	return true;
}


QVariant GlobalManager::getGlobalVariant(const QString& item, const QString& propName) const
{
	Q_UNUSED(item);

	// todo change to QMap<QString, ..>>
	if (PROP_GLOBAL_ROLE == propName)
	{
		return getRole();
	}
	else if (PROP_GLOBAL_HOSTLOGLEVEL == propName)
	{
		return getHostLogLevel();
	}
	else if (PROP_GLOBAL_REMOTELOGLEVEL == propName)
	{
		return getRemoteLogLevel();
	}
	else if (PROP_GLOBAL_TERMINATETIMEOUT == propName)
	{
		return getAppTerminateTimeout();
	}
	else if (PROP_GLOBAL_HEARTBEATTIMEOUT == propName)
	{
		return getAppHeartbeatTimeout();
	}
	else if (PROP_GLOBAL_CRASHPERIOD == propName)
	{
		return getCrashPeriod();
	}
	else if (PROP_GLOBAL_CRASHCOUNT == propName)
	{
		return getCrashCount();
	}
	else if (PROP_GLOBAL_TRAYLAUNCH == propName)
	{
		return getTrayLaunch();
	}
	else if (PROP_GLOBAL_TRAYCONTROL == propName)
	{
		return getTrayControl();
	}
	else if (PROP_GLOBAL_HTTPENABLED == propName)
	{
		return getHttpEnabled();
	}
	else if (PROP_GLOBAL_HTTPPORT == propName)
	{
		return getHttpPort();
	}
	else if (PROP_GLOBAL_BACKENDSERVER == propName)
	{
		return getBackendServer();
	}
	else if (PROP_GLOBAL_NOVATCPENABLED == propName)
	{
		return getNovaTcpEnabled();
	}
	else if (PROP_GLOBAL_NOVATCPADDRESS == propName)
	{
		return getNovaTcpAddress();
	}
	else if (PROP_GLOBAL_NOVATCPPORT == propName)
	{
		return getNovaTcpPort();
	}
	else if (PROP_GLOBAL_NOVAUDPENABLED == propName)
	{
		return getNovaUdpEnabled();
	}
	else if (PROP_GLOBAL_NOVAUDPADDRESS == propName)
	{
		return getNovaUdpAddress();
	}
	else if (PROP_GLOBAL_NOVAUDPPORT == propName)
	{
		return getNovaUdpPort();
	}
	else if (PROP_GLOBAL_NOVASITE == propName)
	{
		return getNovaSite();
	}
	else if (PROP_GLOBAL_NOVAAREA == propName)
	{
		return getNovaArea();
	}
	else if (PROP_GLOBAL_NOVADISPLAY == propName)
	{
		return getNovaDisplay();
	}
	else if (PROP_GLOBAL_ALERTMEMORY == propName)
	{
		return getAlertMemory();
	}
	else if (PROP_GLOBAL_MINMEMORY == propName)
	{
		return getMinMemory();
	}
	else if (PROP_GLOBAL_ALERTDISK == propName)
	{
		return getAlertDisk();
	}
	else if (PROP_GLOBAL_MINDISK == propName)
	{
		return getMinDisk();
	}
	else if (PROP_GLOBAL_ALERTDISKLIST == propName)
	{
		return getAlertDiskList();
	}

	return QVariant();
}


bool GlobalManager::setGlobalVariant(const QString& item, const QString& propName, const QVariant& value)
{
	Q_UNUSED(item);

	// todo change to QMap<QString, ..>>
	if (PROP_GLOBAL_ROLE == propName)
	{
		return setRole(value.toString());
	}
	else if (PROP_GLOBAL_HOSTLOGLEVEL == propName)
	{
		return setHostLogLevel(value.toString());
	}
	else if (PROP_GLOBAL_REMOTELOGLEVEL == propName)
	{
		return setRemoteLogLevel(value.toString());
	}
	else if (PROP_GLOBAL_TERMINATETIMEOUT == propName)
	{
		return setAppTerminateTimeout(value.toInt());
	}
	else if (PROP_GLOBAL_HEARTBEATTIMEOUT == propName)
	{
		return setAppHeartbeatTimeout(value.toInt());
	}
	else if (PROP_GLOBAL_CRASHPERIOD == propName)
	{
		return setCrashPeriod(value.toInt());
	}
	else if (PROP_GLOBAL_CRASHCOUNT == propName)
	{
		return setCrashCount(value.toInt());
	}
	else if (PROP_GLOBAL_TRAYLAUNCH == propName)
	{
		return setTrayLaunch(value.toBool());
	}
	else if (PROP_GLOBAL_TRAYCONTROL == propName)
	{
		return setTrayControl(value.toBool());
	}
	else if (PROP_GLOBAL_HTTPENABLED == propName)
	{
		return setHttpEnabled(value.toBool());
	}
	else if (PROP_GLOBAL_HTTPPORT == propName)
	{
		return setHttpPort(value.toInt());
	}
	else if (PROP_GLOBAL_BACKENDSERVER == propName)
	{
		return setBackendServer(value.toString());
	}
	else if (PROP_GLOBAL_NOVATCPENABLED == propName)
	{
		return setNovaTcpEnabled(value.toBool());
	}
	else if (PROP_GLOBAL_NOVATCPADDRESS == propName)
	{
		return setNovaTcpAddress(value.toString());
	}
	else if (PROP_GLOBAL_NOVATCPPORT == propName)
	{
		return setNovaTcpPort(value.toInt());
	}
	else if (PROP_GLOBAL_NOVAUDPENABLED == propName)
	{
		return setNovaUdpEnabled(value.toBool());
	}
	else if (PROP_GLOBAL_NOVAUDPADDRESS == propName)
	{
		return setNovaUdpAddress(value.toString());
	}
	else if (PROP_GLOBAL_NOVAUDPPORT == propName)
	{
		return setNovaUdpPort(value.toInt());
	}
	else if (PROP_GLOBAL_NOVASITE == propName)
	{
		return setNovaSite(value.toString());
	}
	else if (PROP_GLOBAL_NOVAAREA == propName)
	{
		return setNovaArea(value.toString());
	}
	else if (PROP_GLOBAL_NOVADISPLAY == propName)
	{
		return setNovaDisplay(value.toString());
	}
	else if (PROP_GLOBAL_ALERTMEMORY == propName)
	{
		return setAlertMemory(value.toBool());
	}
	else if (PROP_GLOBAL_MINMEMORY == propName)
	{
		return setMinMemory(value.toInt());
	}
	else if (PROP_GLOBAL_ALERTDISK == propName)
	{
		return setAlertDisk(value.toBool());
	}
	else if (PROP_GLOBAL_MINDISK == propName)
	{
		return setMinDisk(value.toInt());
	}
	else if (PROP_GLOBAL_ALERTDISKLIST == propName)
	{
		return setAlertDiskList(value.toStringList());
	}
	else
	{
		Logger(LOG_WARNING) << tr("Missing property for global value set: ") << propName;
		return false;
	}

	return true;
}


bool GlobalManager::reboot()
{
	Logger(LOG_ALWAYS) << tr("System reboot requested");
	emit reportShutdown(tr("Rebooting"));
	return ShutdownOrReboot(true);
}


bool GlobalManager::shutdown()
{
	Logger(LOG_ALWAYS) << tr("System shutdown requested");
	emit reportShutdown(tr("Shutting down"));
	return ShutdownOrReboot(false);
}


// Returns compressed UTF8 system report text
QByteArray GlobalManager::getSysInfoData() const
{
	QString data;

	data += tr("\r\nHost name:          %1").arg(QHostInfo::localHostName());
	data += tr("\r\nDomain name:        %1").arg(QHostInfo::localDomainName());

	data += "\r\n";

	data += tr("\r\nPinhole version:    %1").arg(PINHOLE_VERSION);
	data += tr("\r\nQt version:         %1").arg(qVersion());
	data += tr("\r\nPinhole path:       %1").arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
	data += tr("\r\nData directory:     %1").arg(QDir::toNativeSeparators(m_settings->dataDir()));
	data += tr("\r\nPinhole uptime:     %1").arg(MillisecondsToString(m_settings->uptime()));
	qint64 sysuptime = 0;
	if (SystemUptime(sysuptime))
	{
		data += tr("\r\nSystem uptime:      %1").arg(MillisecondsToString(sysuptime * 1000));
	}
	data += tr("\r\nLocal datetime:     %1").arg(QDateTime::currentDateTime().toString());

	data += "\r\n";

	// CPU and OS
	data += tr("\r\nOS product name:    %1").arg(QSysInfo::prettyProductName());
	data += tr("\r\nOS product type:    %1").arg(QSysInfo::productType());
	data += tr("\r\nOS product version: %1").arg(QSysInfo::productVersion());
	data += tr("\r\nOS kernel type:     %1").arg(QSysInfo::kernelType());
	data += tr("\r\nOS krnel version:   %1").arg(QSysInfo::kernelVersion());
	data += tr("\r\nCPU architecture:   %1").arg(QSysInfo::currentCpuArchitecture());
	
	quint64 totalMemory = 0;
	quint64 freeMemory = 0;
	if (MemoryInformation(totalMemory, freeMemory))
	{
		data += tr("\r\nTotal memory:       %L1").arg(totalMemory);
		data += tr("\r\nFree memory:        %L1").arg(freeMemory);
	}

	data += "\r\n";

	QList<CpuInfo> cpuInfo = CpuInformation();
	if (!cpuInfo.isEmpty())
	{
		data += tr("\r\nNum CPUs cores:     %1").arg(cpuInfo.size());
		for (int n = 0; n < cpuInfo.size(); n++)
		{
			data += tr("\r\nCPU number %1 info:").arg(n + 1);
			data += tr("\r\nCPU vendor:         %1").arg(cpuInfo[n].Vendor);
			data += tr("\r\nCPU model:          %1").arg(cpuInfo[n].Model);
			data += tr("\r\nCPU MHz:            %1 (Min %2 Max %3)").arg(cpuInfo[n].mhz).arg(cpuInfo[n].mhzMin).arg(cpuInfo[n].mhzMax);
			data += tr("\r\nCPU cache size:     %L1").arg(cpuInfo[n].cacheSize);
			data += "\r\n";
		}
	}

	data += "\r\n";

	// Network interface list
	QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
	data += tr("\r\nNetwork interfaces:");
	for (const auto& iface : interfaces)
	{
		auto flags = iface.flags();

		if (flags & (QNetworkInterface::IsUp | QNetworkInterface::IsRunning))
		{
			data += tr("\r\nInterface type:     %1").arg(QtEnumToString(iface.type()));
			data += tr("\r\nInterface name:     %1 (%2)").arg(iface.humanReadableName()).arg(iface.name());
			data += tr("\r\nHardware address:   %1").arg(iface.hardwareAddress());
			data += tr("\r\nMTU:                %L1").arg(iface.maximumTransmissionUnit());
			data += tr("\r\nNetwork addresses:");
			QList<QNetworkAddressEntry> addresses = iface.addressEntries();
			for (const auto& address : addresses)
			{
				data += tr("\r\nAddress:            %1").arg(HostAddressToString(address.ip()));
				data += tr("\r\nNetmask:            %1").arg(HostAddressToString(address.netmask()));
			}
		}

		data += "\r\n";
	}

	data += "\r\n";

	// Route list
	QList<RouteEntry> routeList = GetRouteList();
	if (!routeList.isEmpty())
	{
		data += tr("\r\nRoute list:");
		data += QString("\r\n%1%2%3%4%5%6%7%8")
			.arg(tr("Destination"), -40)
			.arg(tr("Gateway"), -40)
			.arg(tr("Mask"), -40)
			.arg(tr("Metric"), -10)
			.arg(tr("MTU"), -10)
			.arg(tr("Window"), -10)
			.arg(tr("Use"), -10)
			.arg(tr("Interface"));
		for (const auto& route : routeList)
		{
			data += QString("\r\n%1%2%3%L4%L5%L6%L7%8")
				.arg(route.destination.toString(), -40)
				.arg(route.gateway.toString(), -40)
				.arg(route.mask.toString(), -40)
				.arg(route.metric, -10)
				.arg(route.mtu, -10)
				.arg(route.window, -10)
				.arg(route.use, -10)
				.arg(route.interfaceName);
		}
	}

	data += "\r\n";

	// Storage
	data += tr("\r\nStorage volumes:");
	for (const auto& volume : QStorageInfo::mountedVolumes())
	{
		if (volume.isValid())
		{
			data += tr("\r\nName:               %1 (%2)").arg(volume.displayName()).arg(volume.name());
			data += tr("\r\nDevice:             %1").arg(QString(volume.device()));
			data += tr("\r\nRoot:               %1").arg(QDir::toNativeSeparators(volume.rootPath()));
			data += tr("\r\nFile system:        %1").arg(QString(volume.fileSystemType()));
			data += tr("\r\nBytes total:        %L1").arg(volume.bytesTotal());
			data += tr("\r\nBytes free:         %L1").arg(volume.bytesFree());
			data += tr("\r\nBytes available:    %L1").arg(volume.bytesAvailable());
			data += tr("\r\nRead only:          %1").arg(volume.isReadOnly() ? tr("Yes") : tr("No"));
			data += tr("\r\nReady:              %1").arg(volume.isReady() ? tr("Yes") : tr("No"));
			data += "\r\n";
		}
	}

	data += "\r\n";

	SystemProcessesInfo procInfo;
	if (GetSystemProcessesInfo(procInfo))
	{
		data += tr("\r\nTotal processes:    %L1").arg(procInfo.total);
		data += tr("\r\nSleeping processes: %L1").arg(procInfo.sleeping);
		data += tr("\r\nRunning processes:  %L1").arg(procInfo.running);
		data += tr("\r\nZombie proesses:    %L1").arg(procInfo.zombie);
		data += tr("\r\nStopped proesses:   %L1").arg(procInfo.stopped);
		data += tr("\r\nIdle processes:     %L1").arg(procInfo.idle);
		data += tr("\r\nTotal threads:      %L1").arg(procInfo.threads);
	}

	// Process list
	data += tr("\r\nProcess list:");
	auto procList = runningProcesses();
	std::sort(procList.begin(), procList.end(), [](ProcessInfo& procA, ProcessInfo& procB) { return procA.id < procB.id; });
	for (const auto& proc : procList)
	{
		data += QString("\r\n%1 %2").arg(proc.id).arg(proc.name);
	}
	data += "\r\n";

	QList<WhoEntry> wholist = GetWhoList();
	if (!wholist.isEmpty())
	{
		data += tr("\r\nUsed devices:");
		for (const auto& who : wholist)
		{
			data += tr("\r\nUser:               %1").arg(who.user);
			data += tr("\r\nDevice:             %1").arg(who.device);
			data += tr("\r\nHost:               %1").arg(who.host);
			//data += tr("\r\nTime:               %1").arg(MillisecondsToString(who.time));
			data += "\r\n";
		}
	}

	data += "\r\n";

	return data.toUtf8();
}


