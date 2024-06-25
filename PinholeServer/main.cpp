#include "Values.h"
#include "Settings.h"
#include "StatusInterface.h"
#include "HostUdpServer.h"
#include "CommandInterface.h"
#include "EncryptedTcpServer.h"
#include "MultiplexServer.h"
#include "AlertManager.h"
#include "GroupManager.h"
#include "GlobalManager.h"
#include "AppManager.h"
#include "ScheduleManager.h"
#include "NovaServer.h"
#include "HTTPServer.h"
#include "Logger.h"
#include "HelperLauncher.h"
#include "PasswordReset.h"
#include "ServiceHandler.h"
#include "GuiWaiter.h"
#include "ResourceMonitor.h"
#include "HeartbeatThread.h"
#include "../common/DummyWindow.h"
#include "../common/Utilities.h"
#include "../common/Version.h"

#include <QtCore/QCoreApplication>
#include <QTimer>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QDir>
#include <QMap>
#include <QSettings>
#include <QUuid>

#include <iostream>
#include <future>
#include <exception>
#include <signal.h>

#if defined(Q_OS_WIN)
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#endif


#define VALUE_SERVERID		"serverId"

void stackTrace()
{
#if defined(Q_OS_WIN)
	unsigned int   i;
	void         * stack[128];
	unsigned short frames;
	SYMBOL_INFO  * symbol;
	HANDLE         process;

	process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	frames = CaptureStackBackTrace(1, 128, stack, NULL);
	symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	if (nullptr == symbol)
		return;
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	Logger(LOG_ALWAYS) << "Base address: " << GetModuleHandle(NULL);
	for (i = 0; i < frames; i++) {
		
		if (!SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
			Logger(LOG_ALWAYS) << frames - i - 1 << ": ??? - " << stack[i];
		else
			Logger(LOG_ALWAYS) << frames - i - 1 << ": " << symbol->Name << " - " << reinterpret_cast<void*>(symbol->Address) << " (" << stack[i] << ")";
	}

	free(symbol);
#else
	void* arr[128];
	size_t size;
	char** strings;

	size = backtrace(arr, 128);
	strings = backtrace_symbols(arr, size);
	for (size_t i = 0; i < size; i++)
	{
		Logger(LOG_ALWAYS) << size - i - 1 << ": " << strings[i] << arr[i];
	}

	free(strings);
#endif
}


// Signal handler
void signal_handler(int param)
{
	switch (param)
	{
	case SIGABRT:
	case SIGFPE:
	case SIGSEGV:
	case SIGILL:
		Logger(LOG_ALWAYS) << QObject::tr("CRASH: SIGNAL ERROR %1").arg(param);
		stackTrace();
		abort();
		break;

	default:
		Logger() << QObject::tr("Signaled (%1), exiting").arg(param);
		QCoreApplication::exit(EXIT_SIGNAL);
		break;
	}

}


// Unhandled exception handler
void termination_handler()
{
	Logger(LOG_ALWAYS) << "CRASH: UNHANDLED EXCEPTION OCCURED";
	stackTrace();
	abort();
}


// Main application entry point
int main(int argc, char *argv[])
{
	// Catch ^C and termination requests
	signal(SIGINT, signal_handler);
	//signal(SIGABRT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGILL, signal_handler);
	//signal(SIGBREAK, signal_handler);

	// Catch unhandled exceptions
	std::set_terminate(termination_handler);

	QMap<int, QString> exitNames =
	{
		{EXIT_REMOTEDEBUG, QObject::tr("remote/debug")},
		{EXIT_SIGNAL, QObject::tr("signal")},
		{EXIT_CONSOLE, QObject::tr("console")},
		{EXIT_WMCLOSE, QObject::tr("WM_CLOSE window message")},
		{EXIT_WMENDSESSION, QObject::tr("WM_ENDSESSION window message")},
		{EXIT_SERVICE, QObject::tr("Service shutdown")},
		{EXIT_LOCKUP, QObject::tr("Application lockup")},
		{EXIT_CRASH, QObject::tr("CRASH")}
	};

	QCoreApplication::setApplicationVersion(PINHOLE_VERSION);
	QCoreApplication::setApplicationName(PINHOLE_SERVERAPPNAME);
	QCoreApplication::setOrganizationName(PINHOLE_ORG_NAME);
	QCoreApplication::setOrganizationDomain(PINHOLE_ORG_DOMAIN);

	QCoreApplication application(argc, argv);

	bool resetPassword = false;

	if (2 == argc)
	{
		if (ARG_RESETPASSWORD == QString(argv[1]))
		{
			resetPassword = true;
		}
	}

	// If run with command line option to reset password, reset password and exit
	if (resetPassword)
	{
		PasswordReset passwordReset(&application);
		bool success = passwordReset.exec();
		return success ? 0 : 1;
	}

	// Prevent program from running more than once
	if (IsThisProgramAlreadyRunning())
	{
		std::cerr << "Pinhole Server already running" << std::endl;
		return 0;
	}

	// Register Msgpack types
	RegisterMsgpackTypes();

	// Settings object stores global settings values
	Settings settings;
	settings.setApplication();
	QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/";
	settings.setDataDir(dataDir);
	
	if (dataDir.isEmpty())
	{
		qWarning() << "Data dir empty!";
	}

	if (!QDir(dataDir).exists())
	{
		//qWarning() << "Creating data directory: " << settings.dataDir;
		if (!QDir().mkpath(dataDir))
		{
			qWarning() << "Failed to create data directory";
		}
	}
	else
	{
		qDebug() << "Using data directory: " << dataDir;
	}

	if (!QDir(dataDir + SUBDIR_APPOUTPUT).exists())
	{
		if (!QDir().mkpath(dataDir + SUBDIR_APPOUTPUT))
		{
			qWarning() << "Failed to create data app output subdirectory";
		}
	}

	// Let logger know where data directory is
	Logger::setSettings(&settings);

	// Process command line
	QCommandLineParser parser;
	QCommandLineOption serviceOption(QStringList() << "s" << "service",
		QObject::tr("Run as service."));
	parser.addOption(serviceOption);
	QCommandLineOption noGuiOption(QStringList() << "nogui", QObject::tr("Running on a system with no GUI"));
	parser.addOption(noGuiOption);
	parser.process(application);
	settings.setRunningAsService(parser.isSet(serviceOption));
	settings.setNoGui(parser.isSet(noGuiOption));

	{
		// Get or create server ID
		QSharedPointer<QSettings> pSettings = settings.getScopedSettings();
		if (!pSettings->isWritable())
		{
			Logger(LOG_ERROR) << QObject::tr("WARNING: SETTINGS ARE NOT WRITABLE, NO CHANGES TO CONFIGURATION WILL BE STORED");
			Logger(LOG_ERROR) << pSettings->fileName();
		}

		QString serverId = pSettings->value(VALUE_SERVERID, "").toString();
		if (serverId.isEmpty())
		{
			serverId = QUuid::createUuid().toString();
			pSettings->setValue(VALUE_SERVERID, serverId);
			Logger(LOG_ALWAYS) << QObject::tr("Server ID created: %1").arg(serverId);
		}
		settings.setServerId(serverId);
	}

	Logger(LOG_ALWAYS) << QObject::tr("Server ID: %1").arg(settings.serverId());
	Logger(LOG_ALWAYS) << "Pinhole Server " << PINHOLE_VERSION << " startup " 
		<< (settings.runningAsService() ? "(as service)" : "(as user)") 
		<< " Qt " << qVersion();

	Logger(LOG_EXTRA) << QObject::tr("Qt %1 (%2)")
		.arg(QLibraryInfo::version().toString())
		.arg(QDir::toNativeSeparators(QLibraryInfo::location(QLibraryInfo::LibrariesPath)));

	// Helper class if running as service
	ServiceHandler serviceHandler(&settings);

	// Global manager
	GlobalManager globalManager(&settings);

	// Start service handler if running as service
	if (settings.runningAsService())
	{
		serviceHandler.start();
	}

	// Alert manager
	AlertManager alertManager(&settings);
	Logger(LOG_DEBUG) << "AlertManager created";

	// Application manager
	AppManager appManager(&settings, &globalManager);
	Logger(LOG_DEBUG) << "AppManager created";

	// Application group manager
	GroupManager groupManager(&settings, &appManager);
	Logger(LOG_DEBUG) << "GroupManager created";

	// Scheduled event manager
	ScheduleManager scheduleManager(&settings, &appManager, &groupManager,
		&globalManager);
	Logger(LOG_DEBUG) << "ScheduleManager created";

	// Status and locator interface
	StatusInterface statusInterface(&settings, &alertManager, &appManager,
		&globalManager);
	Logger(LOG_DEBUG) << "StatusInterface created";

	// UDP server interface
	HostUdpServer udpServer(&settings, &statusInterface);
	QObject::connect(&statusInterface, &StatusInterface::sendPacket,
		&udpServer, &HostUdpServer::sendPacketToServers);
	Logger(LOG_DEBUG) << "HostUdpServer created";

	// Command processing interface
	CommandInterface commandInterface(&settings, &alertManager, &appManager, 
		&groupManager, &globalManager, &scheduleManager);
	Logger::setCommandInterface(&commandInterface);
	Logger(LOG_DEBUG) << "CommandInterface created";

	// TCP server interface
	EncryptedTcpServer tcpServer(&settings);
	QObject::connect(&tcpServer, &EncryptedTcpServer::newClient,
		&commandInterface, &CommandInterface::addClient);
	QObject::connect(&tcpServer, &EncryptedTcpServer::clientRemoved,
		&commandInterface, &CommandInterface::removeClient);
	QObject::connect(&tcpServer, &EncryptedTcpServer::incomingData,
		&commandInterface, &CommandInterface::processData);
	Logger(LOG_DEBUG) << "EncryptedTcpServer created";

	// Backend server interface
	MultiplexServer multiplexServer(&settings, &statusInterface, &globalManager);
	QObject::connect(&multiplexServer, &MultiplexServer::newClient,
		&commandInterface, &CommandInterface::addClient);
	QObject::connect(&multiplexServer, &MultiplexServer::clientRemoved,
		&commandInterface, &CommandInterface::removeClient);
	QObject::connect(&multiplexServer, &MultiplexServer::incomingData,
		&commandInterface, &CommandInterface::processData);
	Logger(LOG_DEBUG) << "MultiplexServer created";

	// Nova server interface
	NovaServer novaServer(&settings, &appManager, &groupManager, &globalManager,
		&scheduleManager);
	Logger(LOG_DEBUG) << "NovaServer created";

	// HTTP server interface
	HTTPServer httpServer(&settings, &appManager, &groupManager, &globalManager,
		&scheduleManager);
	Logger(LOG_DEBUG) << "HTTPServer created";

	// Helper launcher, launches the helper executable
	HelperLauncher helperLauncher(&settings, &globalManager);
	Logger(LOG_DEBUG) << "AlertManager created";

	// Resource monitor, watches memory and disk space and generates alerts
	ResourceMonitor resourceMonitor(&settings, &globalManager);
	Logger(LOG_DEBUG) << "ResourceMonitor created";

	// Dummy window used to intercept window messages
	DummyWindow dummyWindow;
	Logger(LOG_DEBUG) << "DummyWindow created";

	// Helper to wait for GUI to be present so applications can be launched
	GuiWaiter guiWaiter;
	Logger(LOG_DEBUG) << "GuiWaiter created";

	// HeartbeatThread checks to make sure the main thread hasn't locked up
	HeartbeatThread heartbeatThread;
	heartbeatThread.start();

	// Allow AppManager Applications to request events to be triggered
	QObject::connect(&appManager, &AppManager::requestTriggerEvents,
		&scheduleManager, &ScheduleManager::triggerEventsSlot);
	// Allow AppManager Applications to request control commands to be sent to helper
	QObject::connect(&appManager, &AppManager::requestControlWindow,
		&commandInterface, &CommandInterface::sendControlWindow);
	// Allow AppManager to generate alerts
	QObject::connect(&appManager, &AppManager::generateAlert,
		&alertManager, &AlertManager::generateAlert);
	// Allow ResourceMonitor to generate alerts
	QObject::connect(&resourceMonitor, &ResourceMonitor::generateAlert,
		&alertManager, &AlertManager::generateAlert);
	// Allow ScheduleManager to generate alerts
	QObject::connect(&scheduleManager, &ScheduleManager::generateAlert,
		&alertManager, &AlertManager::generateAlert);
	// Start applications after helper is running
	QObject::connect(&helperLauncher, &HelperLauncher::finishedStarting,
		&appManager, &AppManager::start);
	// Let alert manager send status messages
	QObject::connect(&alertManager, &AlertManager::sendStatus,
		&statusInterface, &StatusInterface::sendStatus);

	bool exitExplained = false;

	if (!settings.noGui())
	{
		if (settings.runningAsService())
		{
			// If running as service make sure we are logged in befure launching helper
			QObject::connect(&guiWaiter, &GuiWaiter::finished,
				&helperLauncher, &HelperLauncher::start);
			guiWaiter.start();
		}
		else
		{
			QTimer::singleShot(0, &helperLauncher, &HelperLauncher::start);
		}
	}

	// Settings save functions
	auto saveAllSettings = [&]()
	{
		commandInterface.writeSettings();
		appManager.writeApplicationSettings();
		groupManager.writeGroupSettings();
		scheduleManager.writeScheduleSettings();
		globalManager.writeGlobalSettings();
		alertManager.writeAlertSettings();
	};

	// Save all settings on a timer (in case app crashes)
	QTimer saveSettingsTimer;
	saveSettingsTimer.setInterval(INTERVAL_AUTOSAVE);
	saveSettingsTimer.setSingleShot(false);
	QObject::connect(&saveSettingsTimer, &QTimer::timeout,
		saveAllSettings);
	saveSettingsTimer.start();

	QObject::connect(&application, &QCoreApplication::aboutToQuit,
		[&]()
		{
			Logger(LOG_EXTRA) << QObject::tr("About to quit, writing settings");

			// Make sure we stop all apps on exit
			appManager.stopAllApps();

			// Stop the helper
			helperLauncher.stop();

			if (!exitExplained)
			{
				// Send message to all clients who have communicated to us we are shutting down
				statusInterface.sendStatus(QObject::tr("Pinhole exiting"));
			}

			// Stop TCP interfaces
			tcpServer.stop();
			multiplexServer.stop();

			// Save all settings when about to quit
			saveAllSettings();
		});

	// Allow service handler to override exit message
	QObject::connect(&serviceHandler, &ServiceHandler::reportExit,
		[&](QString message)
		{
			exitExplained = true;
			statusInterface.sendStatus(message);
		});

	// Save all settings and notify remote clients when requested by GlobalManager
	QObject::connect(&globalManager, &GlobalManager::reportShutdown,
		[&](const QString& message)
	{
		// Send message to all clients who have communicated to us we are shutting down
			statusInterface.sendStatus(message);
		exitExplained = true;
		Logger(LOG_EXTRA) << QObject::tr("System is possibly %1, writing settings").arg(message);
		saveAllSettings();
	});

	// Execute when the execution loop starts
	QTimer::singleShot(0, [&]()
	{
		if (settings.runningAsService())
		{
			serviceHandler.signalStarted();
		}

		// Start servers with network components AFTER reporting service started
		// Windows boot seems to stop if IP networking is attempted before service finishes starting??
		udpServer.start();
		tcpServer.start();
		multiplexServer.start();
		novaServer.start();
		httpServer.start();

		std::cout << "Server started, use CTRL-C quit..." << std::endl;
		Logger(LOG_ALWAYS) << QObject::tr("Server started");
	});

	// Output some debug info
	Logger(LOG_EXTRA) << "Server executable: " << QCoreApplication::applicationFilePath();
	Logger(LOG_EXTRA) << "Working directory: " << QDir::currentPath();
	QLocale locale = QLocale::system();
	QString bcp47 = locale.bcp47Name();
	QString country = QLocale::countryToString(locale.country());
	QString language = QLocale::languageToString(locale.language());
	QString script = QLocale::scriptToString(locale.script());
	Logger(LOG_EXTRA) << "Locale: " << QString("%1 - %2 - %3 - %4")
		.arg(bcp47)
		.arg(country)
		.arg(language)
		.arg(script);

	Logger(LOG_DEBUG) << QObject::tr("Entering main execution loop");

	// Enter execution loop
	int ret = QCoreApplication::exec();

	Logger(LOG_ALWAYS) << QObject::tr("Pinhole server shutting down (%1)")
		.arg(ret >= 0 && exitNames.contains(ret) ? exitNames[ret] : QString::number(ret));

	if (settings.runningAsService())
	{
		serviceHandler.signalExiting();
	}

	return ret;
}

