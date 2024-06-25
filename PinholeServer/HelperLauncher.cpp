#include "HelperLauncher.h"
#include "Settings.h"
#include "GlobalManager.h"
#include "UserProcess.h"
#include "Logger.h"
#include "Values.h"
#include "../common/Utilities.h"
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include "LinuxUtil.h"
#endif

#include <QCoreApplication>
#include <QDir>

HelperLauncher::HelperLauncher(Settings* settings, GlobalManager* globalManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_globalManager(globalManager)
{
	m_helperProcess = new UserProcess(m_settings, this);

	connect(m_helperProcess, &QProcess::started,
		this, &HelperLauncher::helperStarted);
	connect(m_helperProcess, (void (QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished,
		this, &HelperLauncher::helperFinished);
	connect(m_helperProcess, (void (QProcess::*)(QProcess::ProcessError))&QProcess::errorOccurred,
		this, &HelperLauncher::helperError);

	connect(m_globalManager, &GlobalManager::valueChanged,
		this, &HelperLauncher::globalValueChanged);

	m_trayLaunch = m_globalManager->getTrayLaunch();
}


HelperLauncher::~HelperLauncher()
{
	delete m_helperProcess;
}


void HelperLauncher::start()
{
	if (m_trayLaunch)
	{
		startHelper();
	}
	else
	{
		started = true;
		emit finishedStarting();
	}
}


void HelperLauncher::stop()
{
	if (m_helperProcess->state() != QProcess::NotRunning)
	{
		stopHelper();
		if (!m_helperProcess->waitForFinished(1000))
		{
			Logger(LOG_EXTRA) << tr("Waiting for helper to exit failed");
		}
	}
}


void HelperLauncher::startHelper()
{
	// If no GUI, never launch helper
	if (m_settings->noGui())
		return;

	if (m_helperProcess->state() != QProcess::NotRunning)
	{
		Logger(LOG_EXTRA) << tr("Helper already running");
		return;
	}

	QString currentDirectory = QDir::toNativeSeparators(ApplicationBaseDir());
	QString helperPath;

#if defined(Q_OS_WIN)
	helperPath = currentDirectory + "\\PinholeHelper.exe";
#elif defined(Q_OS_MAC)
	helperPath = currentDirectory + "/PinholeHelper.app/Contents/MacOS/PinholeHelper";
#else
	helperPath = currentDirectory + "/PinholeHelper";
#endif

	// Kill previous instances of helper just in case 
	// it is already running for some reason
	for (const auto& process : runningProcesses())
	{
		if (helperPath == process.name)
		{
			Logger() << tr("Killing previous instance of helper process");
			killProcess(process.id);
		}
	}

	m_helperProcess->setProgram(helperPath);
	m_helperProcess->setWorkingDirectory(currentDirectory);
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
	uid_t uid;
	QString device;
	if (getX11User(&uid, nullptr, nullptr, &device))
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert("DISPLAY", device);
		env.insert("DBUS_SESSION_BUS_ADDRESS", QString("unix:path=/run/user/%1/bus").arg(uid));
		m_helperProcess->setProcessEnvironment(env);
	}
	else
	{
		Logger(LOG_WARNING) << tr("Failed to get x11 user info for helper");
	}
#endif

	Logger(LOG_EXTRA) << tr("Starting helper process");
	launchHelperProcess();
}


void HelperLauncher::stopHelper()
{
	if (m_helperProcess->state() == QProcess::NotRunning)
	{
		Logger(LOG_EXTRA) << tr("Helper not running");
		return;
	}

	Logger(LOG_EXTRA) << tr("Stopping helper process");
	m_exitExpected = true;
#if defined(Q_OS_WIN)
	if (m_settings->runningAsService())
		m_helperProcess->kill();
	else
#endif
		m_helperProcess->terminate();
}


void HelperLauncher::helperStarted()
{
	Logger(LOG_EXTRA) << tr("Helper process started: ") << m_helperProcess->program();

	if (!started)
	{
		started = true;
		// Delay starting of applications so PinholeHelper has a chance to 
		// connect to EncryptedTcpServer
		QTimer::singleShot(INTERVAL_HELPERSTARTDELAY,
			[this]()
		{
			emit finishedStarting();
		});
	}
}


void HelperLauncher::helperFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (!m_exitExpected)
	{
		Logger(LOG_ERROR) << tr("Helper process exited unexpectedly with exit code ") << 
			exitCode << ": " << QtEnumToString(exitStatus);
#if 1
		// Debug output if helper is crashing
		QByteArray output = m_helperProcess->readAllStandardOutput();
		output += m_helperProcess->readAllStandardError();
		Logger(LOG_DEBUG) << QString::fromUtf8(output);
#endif

		if (m_trayLaunch)
		{
			if (m_restartInPeriod.increment(HELPER_THROTTLE_PERIOD) >= HELPER_THROTTLE_COUNT)
			{
				QString alertText = tr("Helper: Max crashes in period reached, not restarting (%1 restarts in %2 seconds)")
					.arg(HELPER_THROTTLE_COUNT)
					.arg(HELPER_THROTTLE_PERIOD / 1000);
				Logger(LOG_WARNING) << alertText;
			}
			else
			{
				QTimer::singleShot(HELPER_RELAUNCH_DELAY,
					[this] { launchHelperProcess(); });
			}
		}
	}
	else
	{
		Logger(LOG_EXTRA) << tr("Helper process exited as expected");
	}

	m_exitExpected = false;
}


void HelperLauncher::helperError(QProcess::ProcessError error)
{
	Logger(LOG_ERROR) << tr("Helper process error ") << QtEnumToString(error);
	if (!started)
	{
		started = true;
		emit finishedStarting();
	}
}


void HelperLauncher::globalValueChanged(const QString& group, const QString& item, const QString& prop, const QVariant& value)
{
	Q_UNUSED(group);
	Q_UNUSED(item);

	if (PROP_GLOBAL_TRAYLAUNCH == prop)
	{
		m_trayLaunch = value.toBool();

		if (m_trayLaunch)
		{
			m_restartInPeriod.reset();
			startHelper();
		}
		else
		{
#if 0
			// Extra debug output if helper is acting strangely
			QByteArray output = m_helperProcess->readAllStandardOutput();
			output += m_helperProcess->readAllStandardError();
			Logger(LOG_DEBUG) << QString::fromUtf8(output);
#endif
			stopHelper();
		}
	}
}


void HelperLauncher::launchHelperProcess()
{
	m_helperProcess->start();
}