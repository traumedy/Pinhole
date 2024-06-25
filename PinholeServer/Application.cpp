#include "Application.h"
#include "Settings.h"
#include "GlobalManager.h"
#include "UserProcess.h"
#include "Logger.h"
#include "Values.h"
#include "qnamedpipe.h"
#include "../common/Utilities.h"
#include "../common/HostClient.h"
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include "LinuxUtil.h"
#endif

#include <QFile>
#include <QTcpSocket>
#include <QTcpServer>

#if defined(Q_OS_WIN)
#include "WinUtil.h"
#include <Windows.h>
#endif


#if !defined(Q_OS_WIN)
QString Application::m_display;
#endif


Application::Application(Settings* settings, GlobalManager* globalManager, const QString& _name)
	: m_name(_name), m_settings(settings), m_globalManager(globalManager)
{
	m_process = new UserProcess(m_settings, this);

	setProperty(PROP_APP_NAME, m_name);
	m_process->setProperty(PROP_APP_NAME, m_name);
	connect(m_process, &QProcess::started,
		this, &Application::processStarted);
	connect(m_process, (void (QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished,
		this, &Application::processFinished);
	connect(m_process, (void (QProcess::*)(QProcess::ProcessError))&QProcess::errorOccurred,
		this, &Application::processError);
	connect(m_process, &QProcess::stateChanged,
		this, &Application::processStateChanged);
	connect(this, &Application::startProcessSignal,
		this, &Application::startProcessSlot);
	connect(this, &Application::killProcessSignal,
		this, &Application::killProcessSlot);
	connect(m_process, &QProcess::readyReadStandardOutput,
		this, &Application::readConsoleStandardOutput);
	connect(m_process, &QProcess::readyReadStandardError,
		this, &Application::readConsoleStandardError);

	m_heartbeatTimer.setInterval(50);
	m_heartbeatTimer.setSingleShot(false);
	connect(&m_heartbeatTimer, &QTimer::timeout,
		this, &Application::heartbeatTimeout);
}


Application::~Application()
{
	delete m_process;
}


QString Application::getName() const
{ 
	return m_name;
}


bool Application::setName(const QString& _name) 
{
	// Special case, emit the old name to indicate rename
	emit valueChanged(PROP_APP_NAME, QVariant(_name));
	m_name = _name;
	setProperty(PROP_APP_NAME, m_name);
	m_process->setProperty(PROP_APP_NAME, m_name);
	return true;
}


QString Application::getExecutable() const 
{ 
	return m_executable;
}


bool Application::setExecutable(const QString& str)
{
	if (m_executable != str)
	{
		m_executable = str;
		emit valueChanged(PROP_APP_EXECUTABLE, QVariant(m_executable));
	}
	return true;
}


QString Application::getArguments() const 
{ 
	return m_arguments;
}


bool Application::setArguments(const QString& str)
{
	if (m_arguments != str)
	{
		m_arguments = str;
		emit valueChanged(PROP_APP_ARGUMENTS, QVariant(m_arguments));
	}
	return true;
}


QString Application::getDirectory() const 
{ 
	return m_directory;
}


bool Application::setDirectory(const QString& str)
{
	if (m_directory != str)
	{
		m_directory = str;
		emit valueChanged(PROP_APP_DIRECTORY, QVariant(m_directory));
	}
	return true;
}


bool Application::getLaunchAtStart() const 
{ 
	return m_launchAtStart;
}


bool Application::setLaunchAtStart(bool b)
{
	if (m_launchAtStart != b)
	{
		m_launchAtStart = b;
		emit valueChanged(PROP_APP_LAUNCHATSTART, QVariant(m_launchAtStart));
	}
	return true;
}


bool Application::getKeepAppRunning() const 
{ 
	return m_keepAppRunning;
}


bool Application::setKeepAppRunning(bool b)
{
	if (m_keepAppRunning != b)
	{
		m_keepAppRunning = b;
		emit valueChanged(PROP_APP_KEEPAPPRUNNING, QVariant(m_keepAppRunning));
	}
	return true;
}


bool Application::getTerminatePrev() const
{
	return m_terminatePrev;
}


bool Application::setTerminatePrev(bool set)
{
	if (m_terminatePrev != set)
	{
		m_terminatePrev = set;
		emit valueChanged(PROP_APP_TERMINATEPREV, QVariant(m_terminatePrev));
	}
	return true;
}


bool Application::getSoftTerminate() const
{
	return m_softTerminate;
}


bool Application::setSoftTerminate(bool set)
{
	if (m_softTerminate != set)
	{
		m_softTerminate = set;
		emit valueChanged(PROP_APP_SOFTTERMINATE, QVariant(m_softTerminate));
	}
	return true;
}


bool Application::getNoCrashThrottle() const
{
	return m_noCrashThrottle;
}


bool Application::setNoCrashThrottle(bool set)
{
	if (m_noCrashThrottle != set)
	{
		m_noCrashThrottle = set;
		emit valueChanged(PROP_APP_NOCRASHTHROTTLE, QVariant(m_noCrashThrottle));
	}
	return true;
}


bool Application::getLockupScreenshot() const
{
	return m_lockupScreenshot;
}


bool Application::setLockupScreenshot(bool set)
{
	if (m_lockupScreenshot != set)
	{
		m_lockupScreenshot = set;
		emit valueChanged(PROP_APP_LOCKUPSCREENSHOT, QVariant(m_lockupScreenshot));
	}
	return true;
}


bool Application::getConsoleCapture() const
{
	return m_consoleCapture;
}


bool Application::setConsoleCapture(bool set)
{
	if (m_consoleCapture != set)
	{
		m_consoleCapture = set;
		emit valueChanged(PROP_APP_CONSOLECAPTURE, QVariant(m_consoleCapture));
	}

	if (getRunning())
	{
		if (set)
		{
			openConsoleOutputFile();
		}
		else
		{
			closeConsoleOutputFile();
		}
	}
	return true;
}


bool Application::getAppendCapture() const
{
	return m_appendCapture;
}


bool Application::setAppendCapture(bool set)
{
	if (m_appendCapture != set)
	{
		m_appendCapture = set;
		emit valueChanged(PROP_APP_APPENDCAPTURE, QVariant(m_appendCapture));
	}
	return true;
}


QString Application::getLaunchDisplay() const
{
	return m_launchDisplay;
}


bool Application::setLaunchDisplay(QString val)
{
	if (m_launchDisplay != val)
	{
		if (!validLaunchDisplay.contains(val))
		{
			Logger(LOG_EXTRA) << tr("Invalid launch display value '%1'").arg(val);
			emit valueChanged(PROP_APP_LAUNCHDISPLAY, QVariant(m_launchDisplay));
			return false;
		}
		m_launchDisplay = val;
		emit valueChanged(PROP_APP_LAUNCHDISPLAY, QVariant(m_launchDisplay));
	}
	return true;
}


int Application::getLaunchDelay() const
{
	return m_launchDelay;
}


bool Application::setLaunchDelay(int val)
{
	if (m_launchDelay != val)
	{
		if (val < 0)
		{
			Logger(LOG_EXTRA) << tr("Invalid launch delay value '%1'").arg(val);
			emit valueChanged(PROP_APP_LAUNCHDELAY, QVariant(m_launchDelay));
			return false;
		}
		m_launchDelay = val;
		emit valueChanged(PROP_APP_LAUNCHDELAY, QVariant(m_launchDelay));
	}
	return true;
}


bool Application::getTcpLoopback() const
{
	return m_tcpLoopback;
}


bool Application::setTcpLoopback(bool b)
{
	if (m_tcpLoopback != b)
	{
		m_tcpLoopback = b;

		if (m_running)
		{
			if (m_tcpLoopback)
			{
				setupLoopback();
			}
			else
			{
				m_tcpServer.clear();
			}
		}

		emit valueChanged(PROP_APP_TCPLOOPBACK, QVariant(m_tcpLoopback));
	}
	return true;
}


int Application::getTcpLoopbackPort() const
{
	return m_tcpLoopbackPort;
}


bool Application::setTcpLoopbackPort(int val)
{
	if (m_tcpLoopbackPort != val)
	{
		if (val < MIN_LISTENINGPORT || val > MAX_PORT)
		{
			Logger(LOG_EXTRA) << tr("Invalid TCP loopback port value '%1'").arg(val);
			emit valueChanged(PROP_APP_TCPLOOPBACKPORT, QVariant(m_tcpLoopbackPort));
			return false;
		}
		m_tcpLoopbackPort = val;
		if (m_running && m_tcpLoopback)
		{
			// Restart TCP server on new port
			setupLoopback();
		}
		emit valueChanged(PROP_APP_TCPLOOPBACKPORT, QVariant(m_tcpLoopbackPort));
	}
	return true;
}


bool Application::getHeartbeats() const
{
	return m_heartbeats;
}


bool Application::setHeartbeats(bool b)
{
	if (m_heartbeats != b)
	{
		m_heartbeats = b;
		emit valueChanged(PROP_APP_HEARTBEATS, QVariant(m_heartbeats));
		m_lastHeartbeat = QDateTime::currentDateTime();
	}
	return true;
}


QStringList Application::getEnvironment() const
{
	return m_environment;
}


bool Application::setEnvironment(const QStringList& list)
{
	if (m_environment != list)
	{
		m_environment = list;
		emit valueChanged(PROP_APP_ENVIRONMENT, QVariant(m_environment));
	}
	return true;
}


QDateTime Application::getLastStarted() const
{ 
	return m_lastStarted;
}


bool Application::setLastStarted(QDateTime time)
{
	m_lastStarted = time;
	emit valueChanged(PROP_APP_LASTSTARTED, QVariant(m_lastStarted.toString()));
	return true;
}


QString Application::getLastStartedString() const
{ 
	return m_lastStarted.toString();
}


bool Application::setLastStartedString(const QString& str)
{
	m_lastStarted = QDateTime::fromString(str);
	emit valueChanged(PROP_APP_LASTSTARTED, QVariant(m_lastStarted.toString()));
	return true;
}


QDateTime Application::getLastExited() const
{ 
	return m_lastExited;
}


bool Application::setLastExited(QDateTime time)
{
	m_lastExited = time;
	emit valueChanged(PROP_APP_LASTEXITED, QVariant(m_lastExited.toString()));
	return true;
}


QString Application::getLastExitedString() const
{ 
	return m_lastExited.toString();
}


bool Application::setLastExitedString(const QString& str)
{
	m_lastExited = QDateTime::fromString(str);
	emit valueChanged(PROP_APP_LASTEXITED, QVariant(m_lastExited.toString()));
	return true;
}


QString Application::getState() const
{
	return m_state;
}


bool Application::setState(const QString& str)
{
	m_state = str;
	emit valueChanged(PROP_APP_STATE, QVariant(m_state));
	return true;
}


bool Application::getRunning() const
{
	return m_running;
}


bool Application::setRunning(bool b)
{
	m_running = b;
	emit valueChanged(PROP_APP_RUNNING, QVariant(m_running));
	return true;
}


int Application::getRestarts() const
{ 
	return m_restarts;
}


void Application::incrementRestarts()
{
	m_restarts++;
	emit valueChanged(PROP_APP_RESTARTS, QVariant(m_restarts));
}


int Application::getLastExitCode() const
{ 
	return m_lastExitCode;
}


bool Application::start(const QStringList& replacementVars)
{
	Logger() << tr("App %1: Starting application").arg(m_name);

	if (m_process->state() != QProcess::ProcessState::NotRunning)
	{
		Logger(LOG_WARNING) << tr("App %1: App already running").arg(m_name);
		return false;
	}

	if (m_executable.isEmpty())
	{
		Logger(LOG_ERROR) << tr("App %1: Empty application path").arg(m_name);
		return false;
	}

	QString fullFilePath = m_executable;
	if (!QFileInfo(m_executable).exists())
	{
		if (m_executable.contains('/') || m_executable.contains('\\'))
		{
			Logger(LOG_ERROR) << tr("App %1: Executable does not exist: %2")
				.arg(m_name)
				.arg(m_executable);
			return false;
		}
		else
		{
			// Try to find executable in path
			fullFilePath = QStandardPaths::findExecutable(m_executable);
			if (fullFilePath.isEmpty())
			{
				Logger(LOG_ERROR) << tr("App %1: Executable does not exist in path: %2")
					.arg(m_name)
					.arg(m_executable);
				return false;
			}
		}
	}

	// Get absolute path in case relative or short name or ?
	fullFilePath = QFileInfo(fullFilePath).absoluteFilePath();

	if (m_terminatePrev)
	{
		Logger(LOG_DEBUG) << tr("App %1: Checking for previous instance of executable")
			.arg(m_name);

		// Find existing instances of executable and terminate
		quint32 pid;
		while (0 != (pid = findProcessWithPath(fullFilePath)))
		{
			Logger() << tr("App %1: Killing existing process with pid %2: %3")
				.arg(m_name)
				.arg(pid)
				.arg(fullFilePath);
			if (!killProcess(pid, 200))
			{
				Logger(LOG_WARNING) << tr("App %1: Failed to terminate process with pid %2")
					.arg(m_name)
					.arg(pid);
				break;
			}
		}
	}

	Logger(LOG_DEBUG) << tr("App %1: Creating application environment")
		.arg(m_name);

	QProcessEnvironment procEnv = QProcessEnvironment::systemEnvironment();

	// Application name
	procEnv.insert(APPENVVAR_APPNAME, m_name);
	// System role
	procEnv.insert(APPENVVAR_ROLE, m_globalManager->getRole());
	// Application logging named pipe name
	procEnv.insert(APPENVVAR_APPLOGPIPE, logPipeName(true));
	// Application TCP loopback port
	if (m_tcpLoopback)
	{
		procEnv.insert(APPENVVAR_APPTCPPORT, QString::number(m_tcpLoopbackPort));
	}
	// Global HTTP server port
	if (m_globalManager->getHttpEnabled())
	{
		procEnv.insert(APPENVVAR_HTTPPORT, QString::number(m_globalManager->getHttpPort()));
	}

	if (m_settings->runningAsService())
	{
		Logger(LOG_DEBUG) << tr("App %1: Setting up user process environment")
			.arg(m_name);

		setupUserProcessEnvironment(procEnv, m_settings->noGui(), false);
	}

	if (!m_environment.isEmpty())
	{
		Logger(LOG_DEBUG) << tr("App %1: Parsing environment strings")
			.arg(m_name);

		for (const auto& envStr : m_environment)
		{
			QStringList envPair = envStr.split('=');
			if (envPair.length() != 2)
			{
				Logger(LOG_WARNING) << tr("App %1: Bad enviornment variable string: '%2'")
					.arg(m_name)
					.arg(envStr);
			}
			else
			{
				QString name = envPair[0];
				QString value = envPair[1];

				// Replace %NAME% with variable values
				replaceEnvironmentStrings(value, procEnv);

				procEnv.insert(name, value);
			}
		}
	}

	Logger(LOG_DEBUG) << tr("App %1: Setting up process settings")
		.arg(m_name);

	m_process->setProcessEnvironment(procEnv);
	m_process->setWorkingDirectory(m_directory);
	m_process->setProgram(m_executable);

	QString tmpArguments = m_arguments;
	// Replace user passed variables
	if (!replacementVars.isEmpty())
	{
		QMap<QString, QString> varMap;
		for (const auto& var : replacementVars)
		{
			QStringList parts = var.split('=');
			if (2 == parts.length())
			{
				varMap[parts[0]] = parts[1];
			}
			else
			{
				Logger(LOG_WARNING) << tr("App %1: Invalid argument replacement: '%2'")
					.arg(m_name)
					.arg(var);
			}
		}
		replaceVariableStrings(tmpArguments, varMap);
	}
	// Replace environment variables
	replaceEnvironmentStrings(tmpArguments, procEnv);
	m_process->setArguments(SplitCommandLine(tmpArguments));

	if (m_consoleCapture)
	{
		Logger(LOG_DEBUG) << tr("App %1: Opening console output file")
			.arg(m_name);

		openConsoleOutputFile();
	}

#if defined(Q_OS_WIN)

	m_process->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments *args)
	{
		if (!m_consoleCapture)
		{
			if (DISPLAY_HIDDEN == m_launchDisplay)
			{
				// For console apps Qt already runs them without a new window and redirects
				// input/output handles so the app is already 'hidden'
				// For GUI apps
				args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
				args->startupInfo->wShowWindow = SW_HIDE;
			}
			else
			{
				// Create new window for console apps
				args->flags |= CREATE_NEW_CONSOLE;
				// Don't redirect stdin/stdout/stderr to Qt
				args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
				if (DISPLAY_MINIMIZE == m_launchDisplay)
				{
					args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
					args->startupInfo->wShowWindow = SW_MINIMIZE;
				}
				else if (DISPLAY_MAXIMIZE == m_launchDisplay)
				{
					args->startupInfo->dwFlags |= STARTF_USESHOWWINDOW;
					args->startupInfo->wShowWindow = SW_SHOWMAXIMIZED;
				}
			}
		}
	});

#endif

	Logger(LOG_DEBUG) << tr("App %1: Signaling start")
		.arg(m_name);

	if (0 == m_launchDelay)
	{
		emit startProcessSignal();
	}
	else
	{
		QTimer::singleShot(m_launchDelay,
			this, &Application::startProcessSignal);
	}

	return true;
}


bool Application::stop(bool restart)
{
	if (restart)
		Logger() << tr("App %1: Restarting application").arg(m_name);
	else
		Logger() << tr("App %1: Stopping application").arg(m_name);
	
	if (m_process->state() != QProcess::ProcessState::Running)
	{
		Logger(LOG_WARNING) << tr("App %1: App not running").arg(m_name);
		return false;
	}

	emit killProcessSignal(restart);

	if (!m_tcpServer.isNull())
	{
		m_tcpServer.clear();
	}

	return true;
}


void Application::processStarted()
{
	Logger(LOG_EXTRA) << tr("App %1: Application started")
		.arg(m_name);

	setLastStarted(QDateTime::currentDateTime());

	m_lastHeartbeat = QDateTime::currentDateTime();

	Logger(LOG_DEBUG) << tr("App %1: Starting logging pipe %2")
		.arg(m_name)
		.arg(logPipeName(true));
	m_logPipe = QSharedPointer<QNamedPipe>::create(logPipeName(false), true, this);
	if (!m_logPipe->isValid())
	{
		Logger(LOG_ERROR) << tr("App %1: Failed to create logging pipe %2")
			.arg(m_name)
			.arg(logPipeName(true));
	}
	else
	{
		connect(m_logPipe.data(), &QNamedPipe::received,
			this, [this](QByteArray bytes)
		{
			// Consider any message from the application a heartbeat
			heartbeat();

			QString logMessage = QString::fromUtf8(bytes);
			if (logMessage.startsWith(LOGPREFIX_ALERT))
			{
				// The alert will generate a log message
				emit generateAlert(tr("Application %1 alert: %2")
					.arg(m_name)
					.arg(logMessage.mid(QString(LOGPREFIX_ALERT).length())));
			}
			else if (logMessage.startsWith(LOGPREFIX_TRIGGER))
			{
				// Trigger events
				QStringList eventNames = SplitSemicolonString(logMessage.mid(QString(LOGPREFIX_TRIGGER).length()));
				Logger() << tr("App %1: Application triggering events via named pipe: ").arg(m_name) << eventNames.join(';');
				emit requestTriggerEvents(eventNames);
			}
			else if (logMessage.startsWith(LOGPREFIX_HEARTBEAT))
			{
				// Noop, just don't log anything if it is just a heartbeat message
				// All log messages update heartbeat timer anyway
			}
			else
			{
				// Allow log messages to be prefixed with the logging level
				int logLevel = LOG_NORMAL;
				QMap<QString, int> levels =
				{
					{ LOGPREFIX_LOGERROR, LOG_ERROR },
					{ LOGPREFIX_LOGWARNING, LOG_WARNING },
					{ LOGPREFIX_LOGEXTRA, LOG_EXTRA },
					{ LOGPREFIX_LOGDEBUG, LOG_DEBUG }
				};

				for (const auto& prefix : levels.keys())
				{
					if (logMessage.startsWith(prefix))
					{
						logLevel = levels[prefix];
						// Trim the logging level from the log text
						logMessage = logMessage.mid(prefix.length());
					}
				}

				Logger(logLevel) << tr("App %1 [LOG]: %2")
					.arg(m_name)
					.arg(logMessage);
			}
		});
		m_logPipe->waitAsync();
	}

	if (m_tcpLoopback)
	{
		setupLoopback();
	}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	if (DISPLAY_NORMAL != m_launchDisplay)
	{
		emit(requestControlWindow((int)m_process->processId(), m_display, m_launchDisplay));
	}
#endif
}


void Application::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	Logger(LOG_EXTRA) << tr("App %1: Application finished exitCode %2 exitStatus %3")
		.arg(m_name)
		.arg(exitCode)
		.arg(QtEnumToString(exitStatus));

	QDateTime now = QDateTime::currentDateTime();
	setLastExited(now);

	// Close console output file
	closeConsoleOutputFile();

	QString runtimeString = MillisecondsToString(getLastStarted().msecsTo(now));

	m_terminateTimer.stop();
	if (!QCoreApplication::closingDown()) // Hack to prevent crash 
		m_logPipe.clear();

	bool restarted = false;
	if (m_exitExpected)
	{
		Logger() << tr("App %1: Application exited after termination (running %2)")
			.arg(m_name)
			.arg(runtimeString);
		m_exitExpected = false;

		if (m_restartAfterExit)
		{
			start();
			restarted = true;
		}
	}
	else
	{
		Logger(LOG_WARNING) << tr("App %1: Application exited unexpectedly exitCode %2 exitStatus %3 (running %4)")
			.arg(m_name)
			.arg(exitCode)
			.arg(QtEnumToString(exitStatus))
			.arg(runtimeString);

		if (m_keepAppRunning)
		{
			if (!m_noCrashThrottle && 
				m_crashInPeriod.increment(static_cast<qint64>(m_globalManager->getCrashPeriod()) * 1000) >= m_globalManager->getCrashCount())
			{
				QString alertText = tr("App %1: Max crashes in period reached, not restarting (%2 restarts in %3 seconds)")
					.arg(m_name)
					.arg(m_globalManager->getCrashCount())
					.arg(m_globalManager->getCrashPeriod());
				Logger(LOG_WARNING) << alertText;
				m_crashInPeriod.reset();
				emit generateAlert(alertText);
			}
			else
			{
				incrementRestarts();
				start();
				restarted = true;
			}
		}
	}

	if (!restarted)
	{
		Logger(LOG_DEBUG) << tr("App %1: Stopping heartbeat timer and TCP server").arg(m_name);

		m_heartbeatTimer.stop();
		m_tcpServer.clear();
		emit applicationExited();
	}
}


void Application::processError(QProcess::ProcessError error)
{
	if (!m_exitExpected)
	{
		Logger(LOG_WARNING) << tr("App %1: Application process error %2")
			.arg(m_name)
			.arg(QtEnumToString(error));
	}
}


void Application::startProcessSlot()
{
	Logger(LOG_DEBUG) << tr("App %1: Starting process: %2")
		.arg(m_name)
		.arg(m_process->program());

	m_process->start();
}


void Application::killProcessSlot(bool restart)
{
	m_exitExpected = true;
	m_restartAfterExit = restart;

	if (m_softTerminate)
	{
		Logger(LOG_DEBUG) << tr("App %1: Process soft kill application").arg(m_name);

		m_process->terminate();
		int timeout = m_globalManager->getAppTerminateTimeout();
		m_terminateTimer.setInterval(timeout);
		m_terminateTimer.setSingleShot(true);
		connect(&m_terminateTimer, &QTimer::timeout,
			this, [this, timeout]()
		{
			Logger(LOG_EXTRA) << tr("App %1: Process did not terminate gracefully after waiting %2 ms, forcing termination").arg(m_name).arg(timeout);
			m_process->kill();
		});
		m_terminateTimer.start();
	}
	else
	{
		Logger(LOG_DEBUG) << tr("App %1: Process kill application").arg(m_name);

		m_process->kill();
	}
}


void Application::processStateChanged(QProcess::ProcessState newState)
{
	Logger(LOG_DEBUG) << tr("App %1: Process state change: %2")
		.arg(m_name)
		.arg(QtEnumToString(newState));

	QString stateString;
	switch (newState)
	{
	case QProcess::ProcessState::NotRunning:
		stateString = tr("Not running");
		break;
	case QProcess::ProcessState::Starting:
		stateString = tr("Starting");
		break;
	case QProcess::ProcessState::Running:
		stateString = tr("Running");
		break;
	default:
		stateString = tr("Unknown? %1").arg(QtEnumToString(newState));
		break;
	}

	setState(stateString);
	setRunning(newState == QProcess::ProcessState::Running);
}


void Application::applicationDataReceive()
{
	QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
	if (nullptr == clientSocket)
	{
		Logger(LOG_WARNING) << tr("App %1: NULL clientSocket").arg(m_name);
		return;
	}

	// Read client data
	QByteArray data = clientSocket->readAll();
	Logger(LOG_DEBUG) << tr("App %1: Received %1 bytes from client")
		.arg(m_name)
		.arg(data.length());

	if (data.isEmpty())
	{
		Logger(LOG_EXTRA) << tr("App %1: Received empty packet").arg(m_name);
		return;
	}

	switch (data[0])
	{
	case 'h':
		heartbeat();
		break;

	case 'e':
	{
		QString errorMessage = QString::fromUtf8(data.right(data.length() - 1));
		Logger(LOG_ERROR) << tr("App %1 [ERR]: ").arg(m_name) << errorMessage;
	}
		break;
		
	default:
		Logger(LOG_WARNING) << tr("App %1: Unknown data received from loopback").arg(m_name);
		break;
	}
}


QString Application::logPipeName(bool full) const
{
	QString pipeName = "PINHOLE-" + m_name + "-LOG";
	if (full)
	{
#ifdef Q_OS_WIN
		return QString("\\\\.\\pipe\\%1").arg(pipeName);
#else
		return QString("/tmp/.%1.fifo").arg(pipeName);
#endif
	}
	
	return pipeName;
}


void Application::setupLoopback()
{
	Logger(LOG_DEBUG) << tr("App %1: Sarting TCP server on port %2")
		.arg(m_name)
		.arg(m_tcpLoopbackPort);

	m_tcpServer = QSharedPointer<QTcpServer>::create();
	if (!m_tcpServer->listen(QHostAddress::LocalHost, m_tcpLoopbackPort))
	{
		Logger(LOG_ERROR) << tr("App %1: Failed to listen for loopback connections on TCP port %2")
			.arg(m_name)
			.arg(m_tcpLoopbackPort);
		m_tcpServer.clear();
	}
	else
	{
		connect(m_tcpServer.data(), &QTcpServer::newConnection,
			this, [this]()
		{
			Logger(LOG_DEBUG) << tr("App %1: Loopback connection received")
				.arg(m_name);

			QTcpSocket* socket = m_tcpServer->nextPendingConnection();

			connect(socket, &QTcpSocket::readyRead,
				this, &Application::applicationDataReceive);
		});

		if (m_globalManager->getAppHeartbeatTimeout() <= 0)
		{
			Logger(LOG_WARNING) << tr("Global app heartbeat timer value set to ") << m_globalManager->getAppHeartbeatTimeout();
		}

		m_heartbeatTimer.start();
	}
}


void Application::heartbeatTimeout()
{
	QDateTime now = QDateTime::currentDateTime();
	if (m_running && m_heartbeats &&
		m_lastHeartbeat.msecsTo(now) > m_globalManager->getAppHeartbeatTimeout())
	{
		// heartbeat timeout
		Logger(LOG_WARNING) << tr("App %1: heartbeat timeout ").arg(m_name) <<
			m_globalManager->getAppHeartbeatTimeout() << " ms";

		bool restartApp = m_keepAppRunning;

		if (m_keepAppRunning)
		{
			if (!m_noCrashThrottle && m_crashInPeriod.increment(static_cast<qint64>(m_globalManager->getCrashPeriod()) * 1000) >= m_globalManager->getCrashCount())
			{
				QString alertText = tr("App %1: Max crashes in period reached, not restarting (%2 restarts in %3 seconds)")
					.arg(m_name)
					.arg(m_globalManager->getCrashCount())
					.arg(m_globalManager->getCrashPeriod());
				Logger(LOG_WARNING) << alertText;
				m_crashInPeriod.reset();
				emit generateAlert(alertText);
				restartApp = false;
			}
			else
			{
				incrementRestarts();
			}
		}

		if (!m_lockupScreenshot)
		{
			// Stop or restart app
			stop(restartApp);
		}
		else
		{
			// Stop the heartbeat timer because so we don't retrigger while
			// we are retrieving the screenshot
			m_heartbeatTimer.stop();

			// Take screenshot then restart app
			Logger(LOG_EXTRA) << tr("App %1: Taking screenshot").arg(m_name);

			HostClient* hostClient = new HostClient("127.0.0.1", HOST_TCPPORT, QString(), true, false, this);

			connect(hostClient, &HostClient::connected,
				this, [hostClient]()
			{
				// Request screenshot
				hostClient->requestScreenshot();
			});

			connect(hostClient, &HostClient::connectFailed,
				this, [this, hostClient, restartApp](const QString& reason)
			{
				Logger(LOG_WARNING) << tr("App %1: Failed to aquire screenshot (failed to connect to localhost? %2)")
					.arg(m_name)
					.arg(reason);
				stop(restartApp);
				hostClient->deleteLater();
				m_heartbeatTimer.start();
			});

			connect(hostClient, &HostClient::commandError,
				this, [this, hostClient, restartApp]()
			{
				Logger(LOG_WARNING) << tr("App %1: Failed to aquire screenshot (command error, helper not running?)").arg(m_name);
				stop(restartApp);
				hostClient->deleteLater();
				m_heartbeatTimer.start();
			});

			connect(hostClient, &HostClient::commandData,
				this, [this, hostClient, restartApp](const QString& group, const QString& subCommand, const QVariant& data)
			{
				if (GROUP_NONE != group || CMD_NONE_GETSCREENSHOT != subCommand)
				{
					Logger(LOG_WARNING) << tr("App %1: Failed to aquire screenshot (internal error, wrong data from server)").arg(m_name);
				}
				else
				{
					// Write screenshot to file
					QString filename = m_settings->dataDir() + tr("Lockup-%1-%2.png")
						.arg(m_name)
						.arg(currentDateTimeFilenameString());
					QFile outfile(filename);
					if (!outfile.open(QIODevice::WriteOnly))
					{
						Logger(LOG_WARNING) << tr("App %1: Failed to create screenshot file ").arg(m_name) << filename;
					}
					else
					{
						outfile.write(data.toByteArray());
						outfile.close();
						Logger() << tr("App %1: Lockup screenshot written to ").arg(m_name) << filename;
					}
				}

				stop(restartApp);
				hostClient->deleteLater();
				m_heartbeatTimer.start();
			});
		}
	}
}


void Application::heartbeat()
{
	m_lastHeartbeat = QDateTime::currentDateTime();
	Logger(LOG_DEBUG) << tr("App %1: heartbeat received").arg(m_name);
}


void Application::replaceEnvironmentStrings(QString& str, const QProcessEnvironment& env) const
{
	// Replace %NAME% with variable values
	QStringList variableList;
	int startPos = -1;
	int endPos = -1;
	do
	{
		startPos = str.indexOf('%', endPos + 1);
		if (-1 != startPos)
		{
			endPos = str.indexOf('%', startPos + 1);
			if (-1 != endPos)
			{
				variableList.append(str.mid(startPos + 1, endPos - startPos - 1));
				startPos = endPos + 1;
			}
			else
			{
				startPos = -1;
			}
		}
	} while (-1 != startPos);

	variableList.removeDuplicates();

	for (const auto& variable : variableList)
	{
		if (variable.isEmpty())
		{
			str.replace("%%", "%");
		}
		else
		{
			str.replace("%" + variable + "%", env.value(variable));
		}
	}
}


void Application::replaceVariableStrings(QString & str, const QMap<QString, QString>& vars) const
{
	// Replace %NAME% with variable values
	QStringList variableList;
	int startPos = -1;
	int endPos = -1;
	do
	{
		startPos = str.indexOf('%', endPos + 1);
		if (-1 != startPos)
		{
			endPos = str.indexOf('%', startPos + 1);
			if (-1 != endPos)
			{
				variableList.append(str.mid(startPos + 1, endPos - startPos - 1));
				startPos = endPos + 1;
			}
			else
			{
				startPos = -1;
			}
		}
	} while (-1 != startPos);

	variableList.removeDuplicates();

	for (const auto& variable : variableList)
	{
		if (!variable.isEmpty() && vars.contains(variable))
		{
			str.replace("%" + variable + "%", vars[variable]);
		}
	}
}


void Application::readConsoleStandardOutput()
{
	if (m_consoleCapture && m_consoleOutputFile != nullptr)
	{
		m_consoleOutputFile->write(m_process->readAllStandardOutput());
		m_consoleOutputFile->flush();
	}
}


void Application::readConsoleStandardError()
{
	if (m_consoleCapture && m_consoleOutputFile != nullptr)
	{
		m_consoleOutputFile->write(m_process->readAllStandardError());
		m_consoleOutputFile->flush();
	}
}


void Application::openConsoleOutputFile()
{
	QString outFilename = m_settings->dataDir() + SUBDIR_APPOUTPUT + m_name + ".output";
	m_consoleOutputFile = new QFile(outFilename, this);
	QIODevice::OpenMode mode = QIODevice::WriteOnly;
	if (m_appendCapture)
		mode |= QIODevice::Append;
	if (!m_consoleOutputFile->open(mode))
	{
		Logger(LOG_ERROR) << tr("App %1: Error creating console output file '%2': '%3'")
			.arg(m_name)
			.arg(outFilename)
			.arg(m_consoleOutputFile->errorString());
	}
}


void Application::closeConsoleOutputFile()
{
	if (nullptr != m_consoleOutputFile)
	{
		m_consoleOutputFile->close();
		delete m_consoleOutputFile;
		m_consoleOutputFile = nullptr;
	}
}


void Application::setupUserProcessEnvironment(QProcessEnvironment& procEnv, bool noGui, bool elevated)
{
#if defined(Q_OS_WIN)
	QString user;
	QString sid;
	if (!GetInteractiveUsername(user, sid))
	{
		Logger(LOG_WARNING) << tr("Failed to get interactive user");
	}
	else
	{
		Logger(LOG_DEBUG) << tr("Interactive user: %1 sid:%2").arg(user).arg(sid);

		procEnv.insert("USERNAME", user);
		QString userDir = procEnv.value("PUBLIC", "C:\\Users\\Public").replace("Public", user);
		procEnv.insert("HOMEPATH", userDir);
		procEnv.insert("USERPROFILE", userDir);

		// Read user paths from registry
		QSettings shellFolders("HKEY_USERS\\" + sid + "\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", QSettings::NativeFormat);
		if (QSettings::NoError == shellFolders.status())
		{
			procEnv.insert("APPDATA", SubstituteEnvironmentstring(shellFolders.value("AppData", "").toString(), procEnv));
			procEnv.insert("LOCALAPPDATA", SubstituteEnvironmentstring(shellFolders.value("Local AppData", "").toString(), procEnv));
		}

		Logger(LOG_DEBUG) << tr("Shell folders contians %1 entries").arg(shellFolders.allKeys().size());

		// Read user environment
		QSettings userEnv("HKEY_USERS\\" + sid + "\\Environment", QSettings::NativeFormat);
		if (QSettings::NoError == userEnv.status())
		{
			Logger(LOG_DEBUG) << tr("User enviroment contains %1 entries").arg(userEnv.allKeys().size());

			for (const auto& key : userEnv.allKeys())
			{
				if (key.compare("PATH", Qt::CaseInsensitive) == 0)
				{
					//QString value = 

					// Concatenate path instead of replacing
					procEnv.insert(key, procEnv.value(key) + ";" + SubstituteEnvironmentstring(userEnv.value(key, "").toString(), procEnv));
				}
				else
				{
					procEnv.insert(key, SubstituteEnvironmentstring(userEnv.value(key, "").toString(), procEnv));
				}
			}
		}
	}
#elif !defined(Q_OS_MAC)

	if (noGui && elevated)
		return;

	uid_t uid = (uid_t)-1;
	QString user;
	bool success = false;
	if (noGui)
	{
		if (elevated)
		{
			success = true;
			user = "root";
		}
		else
		{
			success = pickUser(&uid, nullptr, user);
		}
	}
	else
	{
		success = getX11User(&uid, nullptr, &user, &m_display);
	}

	if (!success)
	{
		Logger(LOG_WARNING) << tr("Failed to get user info for application");
	}
	else
	{
		if (elevated)
			user = "root";

		procEnv.insert("USER", user);
		procEnv.insert("USERNAME", user);
		procEnv.insert("HOME", "/home/" + user);

		if (!noGui)
		{
			procEnv.insert("DISPLAY", m_display);
			procEnv.insert("DBUS_SESSION_BUS_ADDRESS", QString("unix:path=/run/user/%1/bus").arg(uid));
		}
	}
#else
	Q_UNUSED(procEnv);
	Q_UNUSED(noGui);
	Q_UNUSED(elevated);
#endif
}

