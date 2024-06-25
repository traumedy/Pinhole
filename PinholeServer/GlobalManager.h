#pragma once

#include "../common/PinholeCommon.h"

#include <QObject>

class Settings;

class GlobalManager : public QObject
{
	Q_OBJECT

public:
	GlobalManager(Settings* settings, QObject *parent = nullptr);
	~GlobalManager();

	bool readGlobalSettings();
	bool writeGlobalSettings() const;
	bool importSettings(const QJsonObject& root);
	QJsonObject exportSettings() const;
	QString getRole() const;
	bool setRole(const QString& str);
	QString getHostLogLevel() const;
	bool setHostLogLevel(const QString& str);
	QString getRemoteLogLevel() const;
	bool setRemoteLogLevel(const QString& str);
	int getAppTerminateTimeout() const;
	bool setAppTerminateTimeout(int val);
	int getAppHeartbeatTimeout() const;
	bool setAppHeartbeatTimeout(int val);
	int getCrashPeriod() const;
	bool setCrashPeriod(int val);
	int getCrashCount() const;
	bool setCrashCount(int val);
	bool getTrayLaunch() const;
	bool setTrayLaunch(bool b);
	bool getTrayControl() const;
	bool setTrayControl(bool b);
	bool getHttpEnabled() const;
	bool setHttpEnabled(bool b);
	int getHttpPort() const;
	bool setHttpPort(int val);
	QString getBackendServer() const;
	bool setBackendServer(const QString& str);
	bool getNovaTcpEnabled() const;
	bool setNovaTcpEnabled(bool b);
	QString getNovaTcpAddress() const;
	bool setNovaTcpAddress(const QString& str);
	int getNovaTcpPort() const;
	bool setNovaTcpPort(int n);
	bool getNovaUdpEnabled() const;
	bool setNovaUdpEnabled(bool b);
	QString getNovaUdpAddress() const;
	bool setNovaUdpAddress(const QString& str);
	int getNovaUdpPort() const;
	bool setNovaUdpPort(int n);
	QString getNovaSite() const;
	bool setNovaSite(const QString& str);
	QString getNovaArea() const;
	bool setNovaArea(const QString& str);
	QString getNovaDisplay() const;
	bool setNovaDisplay(const QString& str);
	bool getAlertMemory() const;
	bool setAlertMemory(bool b);
	int getMinMemory() const;
	bool setMinMemory(int val);
	bool getAlertDisk() const;
	bool setAlertDisk(bool b);
	int getMinDisk() const;
	bool setMinDisk(int val);
	QStringList getAlertDiskList() const;
	bool setAlertDiskList(const QStringList& val);

	QVariant getGlobalVariant(const QString& appName, const QString& propName) const;
	bool setGlobalVariant(const QString& appName, const QString& propName, const QVariant& value);

	bool reboot();
	bool shutdown();
	QByteArray getSysInfoData() const;

signals:
	void valueChanged(const QString&, const QString&, const QString&, const QVariant&);
	void reportShutdown(const QString& message);

private:
	const QStringList m_validLogLevels = 
	{
		LOG_LEVEL_ERROR,
		LOG_LEVEL_WARNING,
		LOG_LEVEL_NORMAL,
		LOG_LEVEL_EXTRA,
		LOG_LEVEL_DEBUG
	};

	QString m_role = "";
	QString m_hostLogLevel = LOG_LEVEL_NORMAL;
	QString m_remoteLogLevel = LOG_LEVEL_NORMAL;
	int m_appTerminateTimeout = DEFAULT_TERMINATE_TO;
	int m_appHeartbeatTimeout = DEFAULT_HEARTBEAT_TO;
	int m_crashPeriod = DEFAULT_CRASHPERIOD;
	int m_crashCount = DEFAULT_CRASHCOUNT;
	bool m_trayLaunch = true;
	bool m_trayControl = false;
	bool m_httpEnabled = false;
	int m_httpPort = DEFAULT_HTTPPORT;
	QString m_backendServer = "";
	bool m_novaTcpEnabled = false;
	QString m_novaTcpAddress = DEFAULT_NOVATCPADDRESS;
	int m_novaTcpPort = DEFAULT_NOVATCPPORT;
	bool m_novaUdpEnabled = false;
	QString m_novaUdpAddress = "";
	int m_novaUdpPort = DEFAULT_NOVAUDPPORT;
	QString m_novaSite = "";
	QString m_novaArea = "";
	QString m_novaDisplay = "";
	bool m_alertMemory = false;
	int m_minMemory = 0;
	bool m_alertDisk = false;
	int m_minDisk = 0;
	QStringList m_alertDiskList;

	Settings* m_settings = nullptr;
};
