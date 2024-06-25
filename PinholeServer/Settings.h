#pragma once

#include <QElapsedTimer>
#include <QSharedPointer>

class QSettings;

class Settings
{
public:
	Settings()
	{
		m_upTimer.start();
		m_idleTimer.start();
	}

	QSharedPointer<QSettings> getScopedSettings() const;
	void setApplication() {}
	bool runningAsService() const { return m_runningAsService; }
	void setRunningAsService(bool b) { m_runningAsService = b; }
	bool noGui() const { return m_noGui; }
	void setNoGui(bool b) { m_noGui = b; }
	QString serverId() const { return m_serverId; }
	void setServerId(const QString& id) { m_serverId = id; }
	QString dataDir() const { return m_dataDir; }
	void setDataDir(const QString& dir) { m_dataDir = dir; }
	qint64 uptime() const { return m_upTimer.elapsed(); }
	qint64 idleTime() const { return m_idleTimer.elapsed(); }
	void resetIdle() { m_idleTimer.start(); }

private:
	QElapsedTimer m_upTimer;
	QElapsedTimer m_idleTimer;
	QString m_serverId;
	QString m_dataDir;
	bool m_runningAsService = false;
	bool m_noGui = false;
};

