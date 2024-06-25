#pragma once

#include "Application.h"
#include "../common/PinholeCommon.h"

#include <QObject>
#include <QMap>
#include <QTimer>

class Settings;
class GlobalManager;

class AppManager : public QObject
{
	Q_OBJECT

public:
	AppManager(Settings* settings, GlobalManager* globalManager, QObject *parent = nullptr);
	~AppManager();

	QStringList getAppNames() const;
	bool readApplicationSettings();
	bool writeApplicationSettings() const;
	bool importSettings(const QJsonObject& root);
	QJsonObject exportSettings() const;
	bool addApplication(const QString& appName);
	bool deleteApplication(const QString& appName);
	bool renameApplication(const QString& appName, const QString& newAppName);
	QVariant getAppVariant(const QString& appName, const QString& propName) const;
	bool setAppVariant(const QString& appName, const QString& propName, const QVariant& value) const;
	bool startApps(const QStringList& appNames);
	bool stopApps(const QStringList& appNames);
	bool restartApps(const QStringList& appNames);
	bool startAllApps();
	bool startStartupApps();
	bool stopAllApps();
	bool startAppVariables(const QString& appName, const QStringList& vars);
	int runningAppCount() const;
	bool heartbeatApp(const QString& appName) const;
	QByteArray getConsoleOutputFile(const QString& appName) const;
	bool executeCommand(const QByteArray& file, const QString& command, const QString& args, const QString& directory, bool capture, bool elevated, QVariant& data);

signals:
	void valueChanged(const QString&, const QString&, const QString&, const QVariant&) const;
	void requestTriggerEvents(const QStringList& eventNames);
	void requestControlWindow(int pid, const QString& display, const QString& command);
	void generateAlert(const QString& text);

public slots:
	void appValueChanged(const QString&, const QVariant&) const;
	void start();

private:
	const std::map<QString, std::pair<std::function<QString(Application*)>, std::function<bool(Application*, const QString&)>>> m_appStringCallMap =
	{
		{ PROP_APP_EXECUTABLE, { &Application::getExecutable, &Application::setExecutable } },
		{ PROP_APP_ARGUMENTS, { &Application::getArguments, &Application::setArguments } },
		{ PROP_APP_DIRECTORY, { &Application::getDirectory, &Application::setDirectory } },
		{ PROP_APP_LAUNCHDISPLAY, { &Application::getLaunchDisplay, &Application::setLaunchDisplay } },
		{ PROP_APP_LASTSTARTED, { &Application::getLastStartedString, &Application::setLastStartedString } },
		{ PROP_APP_LASTEXITED, { &Application::getLastExitedString, &Application::setLastExitedString } },
		{ PROP_APP_STATE, { &Application::getState, &Application::setState } },
	};

	const std::map<QString, std::pair<std::function<bool(Application*)>, std::function<bool(Application*, bool)>>> m_appBoolCallMap =
	{
		{ PROP_APP_LAUNCHATSTART, { &Application::getLaunchAtStart, &Application::setLaunchAtStart } },
		{ PROP_APP_KEEPAPPRUNNING, { &Application::getKeepAppRunning, &Application::setKeepAppRunning } },
		{ PROP_APP_RUNNING, { &Application::getRunning, &Application::setRunning } },
		{ PROP_APP_TERMINATEPREV, { &Application::getTerminatePrev, &Application::setTerminatePrev } },
		{ PROP_APP_SOFTTERMINATE, { &Application::getSoftTerminate, &Application::setSoftTerminate } },
		{ PROP_APP_NOCRASHTHROTTLE, { &Application::getNoCrashThrottle, &Application::setNoCrashThrottle } },
		{ PROP_APP_LOCKUPSCREENSHOT, { &Application::getLockupScreenshot, &Application::setLockupScreenshot } },
		{ PROP_APP_CONSOLECAPTURE, { &Application::getConsoleCapture, &Application::setConsoleCapture } },
		{ PROP_APP_APPENDCAPTURE, { &Application::getAppendCapture, &Application::setAppendCapture } },
		{ PROP_APP_TCPLOOPBACK, { &Application::getTcpLoopback, &Application::setTcpLoopback } },
		{ PROP_APP_HEARTBEATS, { &Application::getHeartbeats, &Application::setHeartbeats } },
	};

	const std::map<QString, std::pair<std::function<int(Application*)>, std::function<bool(Application*, int)>>> m_appIntCallMap =
	{
		{ PROP_APP_LAUNCHDELAY, { &Application::getLaunchDelay, &Application::setLaunchDelay } },
		{ PROP_APP_TCPLOOPBACKPORT, { &Application::getTcpLoopbackPort, &Application::setTcpLoopbackPort } },
		{ PROP_APP_RESTARTS, { &Application::getRestarts, nullptr } },
	};

	const std::map<QString, std::pair<std::function<QStringList(Application*)>, std::function<bool(Application*, const QStringList&)>>> m_appStringListCallMap =
	{
		{ PROP_APP_ENVIRONMENT, { &Application::getEnvironment, &Application::setEnvironment } },
	};

	QSharedPointer<Application> newApplication(const QString& name);
	
	Settings* m_settings = nullptr;
	GlobalManager* m_globalManager = nullptr;
	QMap<QString, QSharedPointer<Application>> m_appList;
#if defined(Q_OS_LINUX)
	bool m_rootAddedToXhost = false;
#endif
};

