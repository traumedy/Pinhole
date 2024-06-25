#pragma once

#include "TimePeriodCount.h"
#include "../common/PinholeCommon.h"

#include <QObject>
#include <QSharedPointer>
#include <QTimer>
#include <QProcess>

class Settings;
class GlobalManager;
class UserProcess;
class QTcpServer;
class QNamedPipe;
class QFile;

class Application : public QObject
{
	Q_OBJECT

public:
	Application(Settings* settings, GlobalManager* globalManager, const QString& _name);
	~Application();

	QString getName() const;
	bool setName(const QString& _name);
	QString getExecutable() const;
	bool setExecutable(const QString& _path);
	QString getArguments() const;
	bool setArguments(const QString& _arguments);
	QString getDirectory() const;
	bool setDirectory(const QString& _directory);
	bool getLaunchAtStart() const;
	bool setLaunchAtStart(bool set);
	bool getKeepAppRunning() const;
	bool setKeepAppRunning(bool set);
	bool getTerminatePrev() const;
	bool setTerminatePrev(bool set);
	bool getSoftTerminate() const;
	bool setSoftTerminate(bool set);
	bool getNoCrashThrottle() const;
	bool setNoCrashThrottle(bool set);
	bool getLockupScreenshot() const;
	bool setLockupScreenshot(bool set);
	bool getConsoleCapture() const;
	bool setConsoleCapture(bool set);
	bool getAppendCapture() const;
	bool setAppendCapture(bool set);
	QString getLaunchDisplay() const;
	bool setLaunchDisplay(QString val);
	int getLaunchDelay() const;
	bool setLaunchDelay(int val);
	bool getTcpLoopback() const;
	bool setTcpLoopback(bool b);
	int getTcpLoopbackPort() const;
	bool setTcpLoopbackPort(int port);
	bool getHeartbeats() const;
	bool setHeartbeats(bool b);
	QStringList getEnvironment() const;
	bool setEnvironment(const QStringList& _environment);

	QDateTime getLastStarted() const;
	bool setLastStarted(QDateTime time);
	QString getLastStartedString() const;
	bool setLastStartedString(const QString& str);
	QDateTime getLastExited() const;
	bool setLastExited(QDateTime time);
	QString getLastExitedString() const;
	bool setLastExitedString(const QString& str);
	QString getState() const;
	bool setState(const QString& str);
	bool getRunning() const;
	bool setRunning(bool b);

	int getRestarts() const;
	void incrementRestarts();
	int getLastExitCode() const;

	QString logPipeName(bool full) const;
	
	bool start(const QStringList& replacementVars = {});
	bool stop(bool restart = false);
	void openConsoleOutputFile();
	void closeConsoleOutputFile();

	void heartbeat();

	static void setupUserProcessEnvironment(QProcessEnvironment& procEnv, bool noGui, bool elevated);

public slots:
	void processStarted();
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void processError(QProcess::ProcessError error);
	void processStateChanged(QProcess::ProcessState newState);
	void startProcessSlot();
	void killProcessSlot(bool restart);
	void applicationDataReceive();
	void heartbeatTimeout();
	void readConsoleStandardOutput();
	void readConsoleStandardError();

signals:
	void startProcessSignal();
	void killProcessSignal(bool restart);
	void valueChanged(const QString&, const QVariant&);
	void requestTriggerEvents(const QStringList& eventNames);
	void requestControlWindow(int pid, const QString& display, const QString& command);
	void generateAlert(const QString& text);
	void applicationExited();

private:
	void setupLoopback();
	void setupHeartbeats();
	void replaceEnvironmentStrings(QString& str, const QProcessEnvironment& env) const;
	void replaceVariableStrings(QString& str, const QMap<QString, QString>& vars) const;

	const QStringList validLaunchDisplay = 
	{
		DISPLAY_NORMAL,
		DISPLAY_HIDDEN,
		DISPLAY_MINIMIZE,
		DISPLAY_MAXIMIZE
	};

	// Settable properties saved to file
	QString m_name = "";
	QString m_executable = "";
	QString m_arguments = "";
	QString m_directory = "";
	bool m_launchAtStart = false;
	bool m_keepAppRunning = false;
	bool m_terminatePrev = false;
	bool m_softTerminate = false;
	bool m_noCrashThrottle = false;
	bool m_lockupScreenshot = false;
	bool m_consoleCapture = false;
	bool m_appendCapture = false;
	QString m_launchDisplay = DISPLAY_NORMAL;
	int m_launchDelay = 0;
	bool m_tcpLoopback = false;
	int m_tcpLoopbackPort = DEFAULT_APPLOOPBACKPORT;
	bool m_heartbeats = false;
	QStringList m_environment;

	// File console output is saved to
	QFile* m_consoleOutputFile = nullptr;

	// Runtime properties saved to file
	QDateTime m_lastStarted;
	QDateTime m_lastExited;

	// Runtime properties
	TimePeriodCount m_crashInPeriod;
	int m_restarts = 0;
	int m_lastExitCode = 0;
	QString m_state = tr("Not started yet");
	bool m_running = false;

	bool m_exitExpected = false;
	bool m_restartAfterExit = false;
	QDateTime m_lastHeartbeat;
	Settings* m_settings = nullptr;
	GlobalManager* m_globalManager = nullptr;
	UserProcess* m_process = nullptr;
	QSharedPointer<QTcpServer> m_tcpServer;
	QTimer m_heartbeatTimer;
	QTimer m_terminateTimer;
	QSharedPointer<QNamedPipe> m_logPipe;
#if !defined(Q_OS_WIN)
	static QString m_display;		// Stores the X11 display name
#endif
};


