#include "AppManager.h"
#include "Settings.h"
#include "GlobalManager.h"
#include "Logger.h"
#include "Values.h"
#include "UserProcess.h"
#if defined(Q_OS_WIN)
#include "WinUtil.h"
#endif
#include "../common/Utilities.h"

#include <QSettings>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QEventLoop>
#include <QDebug>

#if defined(Q_OS_UNIX)
#include <sys/stat.h>
#endif

AppManager::AppManager(Settings* settings, GlobalManager* globalManager, QObject *parent)
	: QObject(parent), m_settings(settings), 
	m_globalManager(globalManager)
{
	readApplicationSettings();
}


AppManager::~AppManager()
{
	// stopAllApps() should be called by main() when shutting down but in case we close for another reason?
	// Probably too late to stop all apps but let's try
	stopAllApps();
}


void AppManager::start()
{
	// Start apps marked to launch at start
	startStartupApps();
}


bool AppManager::readApplicationSettings()
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	QStringList appList = settings->value(SETTINGS_APPLIST).toStringList();
	for (const auto& appName : appList)
	{
		settings->beginGroup(SETTINGS_APPPREFIX + appName);
		QSharedPointer<Application> newApp = newApplication(appName);
		newApp->setExecutable(settings->value(PROP_APP_EXECUTABLE, "").toString());
		newApp->setArguments(settings->value(PROP_APP_ARGUMENTS, "").toString());
		newApp->setDirectory(settings->value(PROP_APP_DIRECTORY, "").toString());
		newApp->setLaunchAtStart(settings->value(PROP_APP_LAUNCHATSTART, false).toBool());
		newApp->setKeepAppRunning(settings->value(PROP_APP_KEEPAPPRUNNING, false).toBool());
		newApp->setTerminatePrev(settings->value(PROP_APP_TERMINATEPREV, false).toBool());
		newApp->setSoftTerminate(settings->value(PROP_APP_SOFTTERMINATE, false).toBool());
		newApp->setNoCrashThrottle(settings->value(PROP_APP_NOCRASHTHROTTLE, false).toBool());
		newApp->setLockupScreenshot(settings->value(PROP_APP_LOCKUPSCREENSHOT, false).toBool());
		newApp->setConsoleCapture(settings->value(PROP_APP_CONSOLECAPTURE, false).toBool());
		newApp->setAppendCapture(settings->value(PROP_APP_APPENDCAPTURE, false).toBool());
		newApp->setLaunchDisplay(settings->value(PROP_APP_LAUNCHDISPLAY, false).toString());
		newApp->setLaunchDelay(settings->value(PROP_APP_LAUNCHDELAY, false).toInt());
		newApp->setTcpLoopback(settings->value(PROP_APP_TCPLOOPBACK, false).toBool());
		newApp->setTcpLoopbackPort(settings->value(PROP_APP_TCPLOOPBACKPORT, DEFAULT_APPLOOPBACKPORT).toInt());
		newApp->setHeartbeats(settings->value(PROP_APP_HEARTBEATS, false).toBool());
		newApp->setEnvironment(settings->value(PROP_APP_ENVIRONMENT).toStringList());

		if (settings->contains(PROP_APP_LASTSTARTED))
			newApp->setLastStartedString(settings->value(PROP_APP_LASTSTARTED).toString());
		if (settings->contains(PROP_APP_LASTEXITED))
			newApp->setLastExitedString(settings->value(PROP_APP_LASTEXITED).toString());

		m_appList[appName] = newApp;

		settings->endGroup();
	}

	return true;
}


bool AppManager::writeApplicationSettings() const
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

#ifdef QT_DEBUG
	//qDebug() << "Writing application settings to " << settings->fileName() << settings->isWritable();
#endif

	// Delete old entries
	QStringList appList = settings->value(SETTINGS_APPLIST).toStringList();
	for (const auto& appName : appList)
	{
		settings->remove(SETTINGS_APPPREFIX + appName);
	}

	for (const auto& app : m_appList)
	{
		QString appName = app->getName();
		settings->beginGroup(SETTINGS_APPPREFIX + appName);
		settings->setValue(PROP_APP_NAME, appName);
		settings->setValue(PROP_APP_EXECUTABLE, app->getExecutable());
		settings->setValue(PROP_APP_ARGUMENTS, app->getArguments());
		settings->setValue(PROP_APP_DIRECTORY, app->getDirectory());
		settings->setValue(PROP_APP_LAUNCHATSTART, app->getLaunchAtStart());
		settings->setValue(PROP_APP_KEEPAPPRUNNING, app->getKeepAppRunning());
		settings->setValue(PROP_APP_TERMINATEPREV, app->getTerminatePrev());
		settings->setValue(PROP_APP_SOFTTERMINATE, app->getSoftTerminate());
		settings->setValue(PROP_APP_NOCRASHTHROTTLE, app->getNoCrashThrottle());
		settings->setValue(PROP_APP_LOCKUPSCREENSHOT, app->getLockupScreenshot());
		settings->setValue(PROP_APP_CONSOLECAPTURE, app->getConsoleCapture());
		settings->setValue(PROP_APP_APPENDCAPTURE, app->getAppendCapture());
		settings->setValue(PROP_APP_LAUNCHDISPLAY, app->getLaunchDisplay());
		settings->setValue(PROP_APP_LAUNCHDELAY, app->getLaunchDelay());
		settings->setValue(PROP_APP_TCPLOOPBACK, app->getTcpLoopback());
		settings->setValue(PROP_APP_TCPLOOPBACKPORT, app->getTcpLoopbackPort());
		settings->setValue(PROP_APP_HEARTBEATS, app->getHeartbeats());
		settings->setValue(PROP_APP_ENVIRONMENT, app->getEnvironment());

		QDateTime time;
		time = app->getLastStarted();
		if (!time.isNull())
			settings->setValue(PROP_APP_LASTSTARTED, time.toString());
		
		time = app->getLastExited();
		if (!time.isNull())
			settings->setValue(PROP_APP_LASTEXITED, time.toString());
		settings->endGroup();
	}

	settings->setValue(SETTINGS_APPLIST, QStringList(m_appList.keys()));

	return true;
}


bool AppManager::importSettings(const QJsonObject& root)
{
	if (!root.contains(JSONTAG_APPLICATIONS))
	{
		Logger(LOG_WARNING) << tr("JSON import data missing tag ") << JSONTAG_APPLICATIONS;
		return false;
	}

	QJsonArray appList = root[JSONTAG_APPLICATIONS].toArray();

	if (root.contains(STATUSTAG_DELETEENTRIES) && root[STATUSTAG_DELETEENTRIES].toBool() == true)
	{
		// Delete entries
		for (const auto& app : appList)
		{
			if (!app.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON app entry %1 is not object").arg(JSONTAG_APPLICATIONS);
			}
			else
			{
				QJsonObject japp = app.toObject();
				QString name = japp[PROP_APP_NAME].toString();
				m_appList.remove(name);
				Logger() << tr("Removing application '%1'").arg(name);
			}
		}
	}
	else
	{
		// Modify and add entries
		QStringList appNameList;

		for (const auto& app : appList)
		{
			if (!app.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON app entry %1 is not object").arg(JSONTAG_APPLICATIONS);
			}
			else
			{
				QJsonObject japp = app.toObject();
				QString name = japp[PROP_APP_NAME].toString();
				appNameList.append(name);
				QSharedPointer<Application> newApp;
				if (m_appList.contains(name))
				{
					newApp = m_appList[name];
				}
				else
				{
					newApp = newApplication(name);
				}
				newApp->setExecutable(ReadJsonValueWithDefault(japp, PROP_APP_EXECUTABLE, newApp->getExecutable()).toString());
				newApp->setArguments(ReadJsonValueWithDefault(japp, PROP_APP_ARGUMENTS, newApp->getArguments()).toString());
				newApp->setDirectory(ReadJsonValueWithDefault(japp, PROP_APP_DIRECTORY, newApp->getDirectory()).toString());
				newApp->setLaunchAtStart(ReadJsonValueWithDefault(japp, PROP_APP_LAUNCHATSTART, newApp->getLaunchAtStart()).toBool());
				newApp->setKeepAppRunning(ReadJsonValueWithDefault(japp, PROP_APP_KEEPAPPRUNNING, newApp->getKeepAppRunning()).toBool());
				newApp->setTerminatePrev(ReadJsonValueWithDefault(japp, PROP_APP_TERMINATEPREV, newApp->getTerminatePrev()).toBool());
				newApp->setSoftTerminate(ReadJsonValueWithDefault(japp, PROP_APP_SOFTTERMINATE, newApp->getSoftTerminate()).toBool());
				newApp->setNoCrashThrottle(ReadJsonValueWithDefault(japp, PROP_APP_NOCRASHTHROTTLE, newApp->getNoCrashThrottle()).toBool());
				newApp->setLockupScreenshot(ReadJsonValueWithDefault(japp, PROP_APP_LOCKUPSCREENSHOT, newApp->getLockupScreenshot()).toBool());
				newApp->setConsoleCapture(ReadJsonValueWithDefault(japp, PROP_APP_CONSOLECAPTURE, newApp->getConsoleCapture()).toBool());
				newApp->setAppendCapture(ReadJsonValueWithDefault(japp, PROP_APP_APPENDCAPTURE, newApp->getAppendCapture()).toBool());
				newApp->setLaunchDisplay(ReadJsonValueWithDefault(japp, PROP_APP_LAUNCHDISPLAY, newApp->getLaunchDisplay()).toString());
				newApp->setLaunchDelay(ReadJsonValueWithDefault(japp, PROP_APP_LAUNCHDELAY, newApp->getLaunchDelay()).toInt());
				newApp->setTcpLoopback(ReadJsonValueWithDefault(japp, PROP_APP_TCPLOOPBACK, newApp->getTcpLoopback()).toBool());
				newApp->setTcpLoopbackPort(ReadJsonValueWithDefault(japp, PROP_APP_TCPLOOPBACKPORT, newApp->getTcpLoopbackPort()).toInt());
				newApp->setHeartbeats(ReadJsonValueWithDefault(japp, PROP_APP_HEARTBEATS, newApp->getHeartbeats()).toBool());
				newApp->setEnvironment(ReadJsonValueWithDefault(japp, PROP_APP_ENVIRONMENT, newApp->getEnvironment()).toStringList());

				if (japp.contains(PROP_APP_LASTSTARTED))
					newApp->setLastStartedString(japp[PROP_APP_LASTSTARTED].toString());
				if (japp.contains(PROP_APP_LASTEXITED))
					newApp->setLastExitedString(japp[PROP_APP_LASTEXITED].toString());

				m_appList[name] = newApp;
			}
		}

		if (!root.contains(STATUSTAG_NODELETE) || root[STATUSTAG_NODELETE].toBool() == false)
		{
			// Remove apps that are no longer in the list
			for (const auto& appName : m_appList.keys())
			{
				if (!appNameList.contains(appName))
				{
					m_appList.remove(appName);
				}
			}
		}
	}

	emit valueChanged(GROUP_APP, "", PROP_APP_LIST, QVariant(m_appList.keys()));

	return true;
}


QJsonObject AppManager::exportSettings() const
{
	QJsonArray jappList;

	for (const auto& app : m_appList)
	{
		QJsonObject japp;
		japp[PROP_APP_NAME] = app->getName();
		japp[PROP_APP_EXECUTABLE] = app->getExecutable();
		japp[PROP_APP_ARGUMENTS] = app->getArguments();
		japp[PROP_APP_DIRECTORY] = app->getDirectory();
		japp[PROP_APP_LAUNCHATSTART] = app->getLaunchAtStart();
		japp[PROP_APP_KEEPAPPRUNNING] = app->getKeepAppRunning();
		japp[PROP_APP_TERMINATEPREV] = app->getTerminatePrev();
		japp[PROP_APP_SOFTTERMINATE] = app->getSoftTerminate();
		japp[PROP_APP_NOCRASHTHROTTLE] = app->getNoCrashThrottle();
		japp[PROP_APP_LOCKUPSCREENSHOT] = app->getLockupScreenshot();
		japp[PROP_APP_CONSOLECAPTURE] = app->getConsoleCapture();
		japp[PROP_APP_APPENDCAPTURE] = app->getAppendCapture();
		japp[PROP_APP_LAUNCHDISPLAY] = app->getLaunchDisplay();
		japp[PROP_APP_LAUNCHDELAY] = app->getLaunchDelay();
		japp[PROP_APP_TCPLOOPBACK] = app->getTcpLoopback();
		japp[PROP_APP_TCPLOOPBACKPORT] = app->getTcpLoopbackPort();
		japp[PROP_APP_HEARTBEATS] = app->getHeartbeats();
		japp[PROP_APP_ENVIRONMENT] = QJsonArray::fromStringList(app->getEnvironment());

		QDateTime time;
		time = app->getLastStarted();
		if (!time.isNull())
			japp[PROP_APP_LASTSTARTED] = time.toString();

		time = app->getLastExited();
		if (!time.isNull())
			japp[PROP_APP_LASTEXITED] = time.toString();

		jappList.append(japp);
	}

	QJsonObject root;
	root[JSONTAG_APPLICATIONS] = jappList;
	root[STATUSTAG_NODELETE] = false;
	root[STATUSTAG_DELETEENTRIES] = false;

	return root;
}


QStringList AppManager::getAppNames() const
{
	return m_appList.keys();
}


bool AppManager::addApplication(const QString& appName)
{
	if (m_appList.contains(appName))
		return false;

	m_appList[appName] = newApplication(appName);

	emit valueChanged(GROUP_APP, "", PROP_APP_LIST, QVariant(m_appList.keys()));

	return true;
}


bool AppManager::deleteApplication(const QString& appName)
{
	if (!m_appList.contains(appName))
		return false;

	// Stop app
	if (m_appList[appName]->getRunning())
		m_appList[appName]->stop();

	m_appList.remove(appName);

	emit valueChanged(GROUP_APP, "", PROP_APP_LIST, QVariant(m_appList.keys()));

	return true;
}


bool AppManager::renameApplication(const QString& appName, const QString& newAppName)
{
	if (!m_appList.contains(appName))
		return false;

	// Move pointer to new map key index
	QSharedPointer<Application> app = *m_appList.find(appName);
	m_appList.remove(appName);
	app->setName(newAppName);
	m_appList[newAppName] = app;

	emit valueChanged(GROUP_APP, "", PROP_APP_LIST, QVariant(m_appList.keys()));

	return true;
}


QVariant AppManager::getAppVariant(const QString& appName, const QString& propName) const
{
	if (appName.isEmpty())
	{
		if (PROP_APP_LIST == propName)
		{
			return getAppNames();
		}
		else
		{
			return QVariant();
		}
	}

	if (!m_appList.contains(appName))
	{
		Logger(LOG_WARNING) << tr("Missing app for query: ") << appName;
		return QVariant();
	}

	if (m_appStringCallMap.find(propName) != m_appStringCallMap.end())
	{
		return QVariant(std::get<0>(m_appStringCallMap.at(propName))(m_appList[appName].data()));
	}
	else if (m_appBoolCallMap.find(propName) != m_appBoolCallMap.end())
	{
		return QVariant(std::get<0>(m_appBoolCallMap.at(propName))(m_appList[appName].data()));
	}
	else if (m_appIntCallMap.find(propName) != m_appIntCallMap.end())
	{
		return QVariant(std::get<0>(m_appIntCallMap.at(propName))(m_appList[appName].data()));
	}
	else if (m_appStringListCallMap.find(propName) != m_appStringListCallMap.end())
	{
		return QVariant(std::get<0>(m_appStringListCallMap.at(propName))(m_appList[appName].data()));
	}

	Logger(LOG_WARNING) << tr("Missing property for app value query: ") << propName;
	return QVariant();
}


bool AppManager::setAppVariant(const QString& appName, const QString& propName, const QVariant& value) const
{
	if (!m_appList.contains(appName))
	{
		Logger(LOG_WARNING) << tr("Missing app for set: ") << appName;
		return false;
	}

	if (m_appStringCallMap.find(propName) != m_appStringCallMap.end())
	{
		if (!value.canConvert(QVariant::String))
		{
			Logger(LOG_WARNING) << tr("Variant for application value set is not string: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_appStringCallMap.at(propName))(m_appList[appName].data(), value.toString());
	}
	else if (m_appBoolCallMap.find(propName) != m_appBoolCallMap.end())
	{
		if (!value.canConvert(QVariant::Bool))
		{
			Logger(LOG_WARNING) << tr("Variant for application value set is not bool: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_appBoolCallMap.at(propName))(m_appList[appName].data(), value.toBool());
	}
	else if (m_appIntCallMap.find(propName) != m_appIntCallMap.end())
	{
		if (!value.canConvert(QVariant::Int))
		{
			Logger(LOG_WARNING) << tr("Variant for application value set is not int: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_appIntCallMap.at(propName))(m_appList[appName].data(), value.toInt());
	}
	else if (m_appStringListCallMap.find(propName) != m_appStringListCallMap.end())
	{
		if (!value.canConvert(QVariant::StringList))
		{
			Logger(LOG_WARNING) << tr("Variant for application value set is not stringlist: %1").arg(propName);
			return false;
		}
		return std::get<1>(m_appStringListCallMap.at(propName))(m_appList[appName].data(), value.toStringList());
	}
	else
	{
		Logger(LOG_WARNING) << tr("Missing property for app value set: ") << propName;
		return false;
	}
}


// Returns false if any errors occur
bool AppManager::startApps(const QStringList& appNames)
{
	bool ret = true;

	for (const auto& name : appNames)
	{
		if (!m_appList.contains(name))
		{
			ret = false;
		}
		else
		{
			if (!m_appList[name]->start())
				ret = false;
		}
	}

	return ret;
}


// Returns false if any errors occur
bool AppManager::stopApps(const QStringList& appNames)
{
	bool ret = true;

	for (const auto& name : appNames)
	{
		if (!m_appList.contains(name))
		{
			ret = false;
		}
		else
		{
			if (!m_appList[name]->stop())
				ret = false;
		}
	}

	return ret;
}


bool AppManager::restartApps(const QStringList& appNames)
{
	bool ret = true;

	for (const auto& name : appNames)
	{
		if (!m_appList.contains(name))
		{
			ret = false;
		}
		else
		{
			if (!m_appList[name]->stop(true))
				ret = false;
		}
	}

	return ret;
}


bool AppManager::startAllApps()
{
	bool ret = true;

	for (const auto& app : m_appList)
	{
		if (!app->getRunning())
		{
			if (!app->start())
				ret = false;
		}
	}

	return ret;
}


bool AppManager::stopAllApps()
{
	bool ret = true;

	for (const auto& app : m_appList)
	{
		if (app->getRunning())
		{
			if (!app->stop())
				ret = false;
		}
	}

	return ret;
}


bool AppManager::startAppVariables(const QString & appName, const QStringList & vars)
{
	if (!m_appList.contains(appName))
		return false;

	if (m_appList[appName]->getRunning())
		return false;

	return m_appList[appName]->start(vars);
}


bool AppManager::startStartupApps()
{
	bool ret = true;

	for (const auto& app : m_appList)
	{
		if (app->getLaunchAtStart() && !app->getRunning())
		{
			if (!app->start())
				ret = false;
		}
	}

	return ret;
}


// Called by Application objects when a value changes
void AppManager::appValueChanged(const QString& property, const QVariant& value) const
{
	QString appName = sender()->property(PROP_APP_NAME).toString();
	emit valueChanged(GROUP_APP, appName, property, value);
}


int AppManager::runningAppCount() const
{
	int count = 0;

	for (const auto& app : m_appList)
	{
		if (app->getRunning())
			count++;
	}

	return count;
}


bool AppManager::heartbeatApp(const QString& appName) const
{
	if (!m_appList.contains(appName))
	{
		Logger(LOG_WARNING) << tr("Heartbeat requested for non-existant app '%1'").arg(appName);
		return false;
	}

	m_appList[appName]->heartbeat();
	return true;
}


QSharedPointer<Application> AppManager::newApplication(const QString& name)
{
	QSharedPointer<Application> newApp = QSharedPointer<Application>::create(m_settings, m_globalManager, name);
	connect(newApp.data(), &Application::valueChanged,
		this, &AppManager::appValueChanged);
	connect(newApp.data(), &Application::requestTriggerEvents,
		this, [this](const QStringList& eventNames)
	{ 
		emit requestTriggerEvents(eventNames); 
	});
	connect(newApp.data(), &Application::requestControlWindow,
		this, [this](int pid, const QString& display, const QString& command)
	{
		emit requestControlWindow(pid, display, command);
	});
	connect(newApp.data(), &Application::generateAlert,
		this, [this](const QString& text)
	{
		emit generateAlert(text);
	});
	connect(newApp.data(), &Application::applicationExited,
		this, [this]()
	{
		// Start the timer if we've gone idle
		if (runningAppCount() == 0)
			m_settings->resetIdle();
	});

	return newApp;
}


QByteArray AppManager::getConsoleOutputFile(const QString& appName) const
{
	if (!m_appList.contains(appName))
		return tr("No such app: '%1'").arg(appName).toUtf8();

	QString outFilename = m_settings->dataDir() + SUBDIR_APPOUTPUT + appName + ".output";
	QFile consoleOutput(outFilename);
	if (!consoleOutput.open(QIODevice::ReadOnly))
		return tr("Error opening console output file: %1").arg(consoleOutput.errorString()).toUtf8();

	return consoleOutput.readAll();
}


bool AppManager::executeCommand(const QByteArray& file, const QString& command, const QString& args, const QString& directory, bool capture, bool elevated, QVariant& data)
{
	// Can't execute elevated unless running as service
	if (elevated && !m_settings->runningAsService())
	{
		Logger(LOG_ERROR) << tr("An attempt was made to execute an elevated command while not running as a service");
		return false;
	}

	QStringList argList = SplitCommandLine(args);
	UserProcess *process = new UserProcess(m_settings, this);
	process->setArguments(argList);
	process->setWorkingDirectory(directory);
	process->setElevated(elevated);

	QProcessEnvironment procEnv = QProcessEnvironment::systemEnvironment();

	if (m_settings->runningAsService())
	{
#if defined(Q_OS_WIN)
		if (elevated)
		{
			Logger(LOG_DEBUG) << tr("[%1] Elevating process").arg(command);
			ElevateNextCreateProcess();
		}
#endif

		Application::setupUserProcessEnvironment(procEnv, m_settings->noGui(), elevated);
		process->setProcessEnvironment(procEnv);
	}

#if defined(Q_OS_WIN)
	QString tempPath = procEnv.value("TEMP");
#else
	QString tempPath = "/tmp";
#endif

	QByteArray captureData;
	bool ret = true;
	QEventLoop* eventLoop = nullptr;

	if (capture)
	{
		connect(process, &QProcess::readyReadStandardOutput,
			this, [process, &captureData]()
		{
			captureData.append(process->readAllStandardOutput());
		});
		connect(process, &QProcess::readyReadStandardError,
			this, [process, &captureData]()
		{
			captureData.append(process->readAllStandardError());
		});

		// Parent event loop to process so it gets deleted when process is deleted
		eventLoop = new QEventLoop(process);
	}

	QString executable;
	bool deleteFile = false;

	if (!file.isNull() && !file.isEmpty())
	{
		deleteFile = true;
		executable = tempPath + "/" + GenerateRandomString(8, true);
#if defined(Q_OS_WIN)
		executable += ".EXE";
#endif
		QFile exe(executable);
		if (!exe.open(QFile::WriteOnly))
		{
			Logger(LOG_ERROR) << "Failed to open temp executable '" << executable << "' for write:" << exe.errorString();
			return false;
		}
		exe.write(file);
		exe.close();
#if defined(Q_OS_UNIX)
		chmod(executable.toLocal8Bit().data(), 0x777);
#endif
	}
	else
	{
		executable = command;
	}

	process->setProgram(executable);

	connect(process, &QProcess::errorOccurred,
		[process, &ret, executable](QProcess::ProcessError e)
	{
		Logger(LOG_DEBUG) << QString("[%1] Error: %2")
			.arg(executable)
			.arg(QtEnumToString(e));

		if (QProcess::FailedToStart == e)
		{
			ret = false;
			process->deleteLater();
		}
	});

	connect(process, (void (QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished,
		[executable, process, eventLoop, deleteFile](int retcode, QProcess::ExitStatus exitStatus)
	{
		Logger() << tr("[%1] Finished with status %2, return code %3")
			.arg(executable)
			.arg(QtEnumToString(exitStatus))
			.arg(retcode);

		if (eventLoop)
		{
			//Don't delete event loop without quiting it first
			eventLoop->quit();
		}
		process->deleteLater();

		if (deleteFile)
		{
			QFile f(executable);
			if (!f.remove())
			{
				Logger(LOG_ERROR) << tr("Failed to delete temporary executable '%1': '%2'").arg(executable).arg(f.errorString());
			}
			else
			{
				Logger() << tr("[%1] Temporary executable deleted").arg(executable);
			}
		}
	});

#if defined(Q_OS_LINUX)
	if (m_settings->runningAsService() && 
		!m_settings->noGui() && 
		!m_rootAddedToXhost && 
		elevated)
	{
		// Add root to Xauthority so it can run interactive processes
		Logger(LOG_EXTRA) << tr("Executing xhost as user to allow root access to X server");
		UserProcess xhostProc(m_settings);
		QProcessEnvironment procEnv = QProcessEnvironment::systemEnvironment();
		Application::setupUserProcessEnvironment(procEnv, false, false);
		xhostProc.setProcessEnvironment(procEnv);
		xhostProc.start("xhost", QStringList("si:localuser:root"));
		if (!xhostProc.waitForFinished())
		{
			Logger(LOG_ERROR) << "xhost process did not exit";
		}
		else if (0 != xhostProc.exitCode())
		{
			Logger(LOG_WARNING) << "xhost returned %1" << xhostProc.exitCode();
		}
	}
#endif

	Logger() << tr("[%1] Lanching with arguments: %2")
		.arg(executable)
		.arg(args);

	process->start();

	if (ret && eventLoop)
	{
		// Block until the process finishes
		eventLoop->exec();
		if (ret)
		{
			data = QVariant(qCompress(captureData));
		}
	}

	return ret;
}
