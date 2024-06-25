#include "ServiceHandler.h"
#include "Settings.h"
#include "Logger.h"
#include "../common/PinholeCommon.h"
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include "LinuxUtil.h"
#endif
#if defined(Q_OS_WIN)
#include "WinUtil.h"
#endif

#include <QCoreApplication>

#if defined(Q_OS_WIN)
QSemaphore ServiceHandler::startSemaphore;
SERVICE_STATUS_HANDLE ServiceHandler::s_hSvcStatus;  // Service status handle
DWORD ServiceHandler::s_dwHealth;
#endif

ServiceHandler* ServiceHandler::s_instance = nullptr;

ServiceHandler::ServiceHandler(Settings* settings, QObject *parent)
	: QObject(parent), m_settings(settings)
{
	s_instance = this;
}


ServiceHandler::~ServiceHandler()
{
}


bool ServiceHandler::start()
{
#if defined(Q_OS_WIN)
	// HACKY HACKHACK, patch the import address table of the Qt core DLL to intercept the CreateProcessW
	// call	so that we can call CreateProcessAsUser instead when we are running as a service
	if (!OverrideCreateProcess())
	{
		Logger(LOG_ERROR) << QObject::tr("Failed to hook CreateProcess");
	}

	m_handlerThread.start();

	if (!startSemaphore.tryAcquire(1, 30000))
		return false;
	
	if (!m_handlerThread.calledOk())
		return false;
#endif


	return true;
}


#if defined(Q_OS_WIN)

void HandlerThread::run()
{
	Logger(LOG_ALWAYS) << QObject::tr("Starting Windows service...");
	// Notify Windows of starting service
	SERVICE_TABLE_ENTRY SvcEntries[2];
	SvcEntries[0].lpServiceName = TEXT(PINHOLE_SERVICENAME);
	SvcEntries[0].lpServiceProc = ServiceHandler::serviceMain;
	SvcEntries[1].lpServiceName = nullptr;
	SvcEntries[1].lpServiceProc = nullptr;

	if (!StartServiceCtrlDispatcher(SvcEntries))
	{
		DWORD err = GetLastError();

		Logger(LOG_ERROR) << QObject::tr("Error %1 calling StartServiceCtrlDispatcher [%2]")
			.arg(err)
			.arg(qt_error_string(err));

		if (ERROR_FAILED_SERVICE_CONTROLLER_CONNECT == err)
		{
			QString text = "This program is not meant to be run directly, it needs to be installed as a service.";
			//tcout << text << endl;
			//MessageBox(HWND_DESKTOP, text, TEXT("DO NOT RUN THIS PROGRAM DIRECTLY, RUN MONCFG.EXE!"), MB_OK | MB_ICONWARNING);
			console = true;
		}
		else
		{
			Logger(LOG_ERROR) << QString("The Service failed to start [%1]").arg(qt_error_string(GetLastError()));
		}
		ServiceHandler::startSemaphore.release();  // let start() continue, since serviceMain won't be doing it
	}
}


// Sets the 'service status' values for the service control manager
DWORD ServiceHandler::setThisServiceStatus(DWORD dwState, DWORD dwExitCode, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	Logger(LOG_DEBUG) << QObject::tr("Setting Windows service to state %1").arg(dwState);

	DWORD err = 0;
	SERVICE_STATUS SvcStat;

	SvcStat.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
	SvcStat.dwCurrentState = dwState;
	SvcStat.dwWin32ExitCode = dwExitCode;
	SvcStat.dwServiceSpecificExitCode = 0;
	SvcStat.dwCheckPoint = dwCheckPoint;
	SvcStat.dwWaitHint = dwWaitHint;

	switch (dwState)
	{
	case SERVICE_RUNNING:
		SvcStat.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP;
		break;

	default:
		// do not accept controls while starting, stopping, etc
		SvcStat.dwControlsAccepted = 0;
		break;
	}

	if (!SetServiceStatus(s_hSvcStatus, &SvcStat))
	{
		DWORD er = GetLastError();
		Logger(LOG_ERROR) << QObject::tr("Error %1 calling SetServiceStatus(state=%2")
			.arg(er)
			.arg(dwState)
			.arg(qt_error_string(er));
	}

	return err;
}


DWORD WINAPI ServiceHandler::svcHandler(
	__in  DWORD dwControl,
	__in  DWORD dwEventType,
	__in  LPVOID lpEventData,
	__in  LPVOID lpContext
)
{
	Logger(LOG_DEBUG) << QObject::tr("svcHandler service command %1").arg(dwControl);

	DWORD ret = NO_ERROR;
	ServiceHandler* pThis = reinterpret_cast<ServiceHandler*>(lpContext);

	switch (dwControl)
	{
	case SERVICE_CONTROL_SHUTDOWN:
		setThisServiceStatus(SERVICE_STOP_PENDING, 0, s_dwHealth, 5000);
		Logger() << QObject::tr("System shutting down");
		// Signal the app to stop
		QCoreApplication::exit(EXIT_SERVICE);
		emit pThis->reportExit(QObject::tr("System shutting down"));
		break;

	case SERVICE_CONTROL_STOP:
		setThisServiceStatus(SERVICE_STOP_PENDING, 0, s_dwHealth, 5000);
		Logger() << QObject::tr("Stopping service...");
		// Signal the app to stop
		QCoreApplication::exit(EXIT_SERVICE);
		emit pThis->reportExit(QObject::tr("Service stopped"));
		break;

	case SERVICE_CONTROL_INTERROGATE:
		Logger(LOG_DEBUG) << QObject::tr("Received INTERROGATE control.");
		break;

	case SERVICE_CONTROL_DEVICEEVENT:
	case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
	case SERVICE_CONTROL_POWEREVENT:
		// These values should _NOT_ cause a return value of ERROR_CALL_NOT_IMPLEMENTED or it may prevent the action
		break;
	default:
	{
		Logger() << QObject::tr("Received unknown control command: Control:%1 EventType:%2").arg(dwControl).arg(dwEventType);
		ret = ERROR_CALL_NOT_IMPLEMENTED;
	}
	break;
	}

	return ret;
}


VOID WINAPI ServiceHandler::serviceMain(
	__in  DWORD dwArgc,
	__in  LPTSTR *lpszArgv
)
{
	Sleep(10000);

	// Required API to call for service processes
	s_hSvcStatus = RegisterServiceCtrlHandlerEx(TEXT(PINHOLE_SERVICENAME), svcHandler, s_instance);

	if (nullptr == s_hSvcStatus)
	{
		// Bail!
		DWORD err = GetLastError();
		Logger(LOG_ERROR) << QObject::tr("Error %1 calling RegisterServiceCtrlHandlerEx [%2]")
			.arg(err)
			.arg(qt_error_string(err));

		return;
	}

	setThisServiceStatus(SERVICE_START_PENDING, 0, s_dwHealth, 5000);

	ServiceHandler::startSemaphore.release(); // let the qapp creation start
}
#endif

void ServiceHandler::signalStarted()
{
#if defined(Q_OS_WIN)
	setThisServiceStatus(SERVICE_RUNNING, 0, s_dwHealth);
#endif
}


void ServiceHandler::signalExiting()
{
#if defined(Q_OS_WIN)
	setThisServiceStatus(SERVICE_STOPPED, 0, s_dwHealth);
#endif
}

