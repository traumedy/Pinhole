#pragma once

#include <QObject>
#include <QSemaphore>
#include <QThread>
#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

class Settings;

#if defined(Q_OS_WIN)
class HandlerThread : public QThread
{
	//Q_OBJECT

public:
	HandlerThread()
		: success(true), console(false), QThread()
	{}

	bool calledOk() const { return success; }
	bool runningAsConsole() const { return console; }

protected:
	bool success, console;
	void run();
};
#endif


class ServiceHandler : public QObject
{
	Q_OBJECT

public:
	ServiceHandler(Settings* settings, QObject *parent = nullptr);
	~ServiceHandler();

	bool start();
	static void signalStarted();
	static void signalExiting();

signals:
	bool reportExit(QString message);

protected:
#if defined(Q_OS_WIN)
	static VOID WINAPI serviceMain(DWORD dwArgc, LPTSTR *lpszArgv);
	static QSemaphore startSemaphore;
#endif

private:
#if defined(Q_OS_WIN)
	static DWORD WINAPI ServiceHandler::svcHandler(
		__in  DWORD dwControl,
		__in  DWORD dwEventType,
		__in  LPVOID lpEventData,
		__in  LPVOID lpContext
	);
	static DWORD setThisServiceStatus(DWORD dwState, DWORD dwExitCode, DWORD dwCheckPoint, DWORD dwWaitHint = 0);

	static SERVICE_STATUS_HANDLE s_hSvcStatus;  // Service status handle
	static DWORD s_dwHealth;

	HandlerThread m_handlerThread;

friend class HandlerThread;
#endif

	Settings* m_settings = nullptr;
	static ServiceHandler* s_instance;
};
