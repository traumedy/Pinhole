// Creates a dummy Win32 window to receive WM_CLOSE message and shutdown the app
// For applications that have no main window like PinholeServer and PinholeHelper
// Allows them to terminate gracefully and quickly when terminate() is called from
// the installer
// Also creates a console control handler callback

#include "DummyWindow.h"
#include "PinholeCommon.h"

#include <QCoreApplication>
#include <QDebug>

#if defined(Q_OS_WIN)
#include <Windows.h>

#define WINDOWCLASSNAME "PinholeDummyWindow"

LRESULT CALLBACK DummyWindowProc(
	HWND hWnd,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (Msg)
	{
	case WM_CLOSE:
		QCoreApplication::exit(EXIT_WMCLOSE);
		break;

	case WM_QUERYENDSESSION:
		// For some reason we get the QUERYENDSESSION message but get terminated 
		// before the ENDSESSION message.  This means we will terminate if the
		// shutdown is aborted, but is the only way I can find to intercept
		// system shutdown.
		QCoreApplication::exit(EXIT_WMENDSESSION);
		return 1;
		break;

	case WM_ENDSESSION:
		if (TRUE == wParam)
		{
			QCoreApplication::exit(EXIT_WMENDSESSION);
		}
		return 0;
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}


BOOL WINAPI ConsoleHandlerRoutine(DWORD ctrlType)
{
	// The system shutdown event CTRL_SHUTDOWN_EVENT does not seem to 
	// happen:  
	// "Interactive applications are not present by the time the system 
	// sends this signal, therefore it can be received only be services 
	// in this situation"
	// However, if it is the service, let the service handler handle it
	if (CTRL_SHUTDOWN_EVENT != ctrlType)
	{
		QCoreApplication::exit(EXIT_CONSOLE);
	}
	return TRUE;
}
#endif


DummyWindow::DummyWindow(QObject *parent)
	: QObject(parent)
{
#if defined(Q_OS_WIN)

	// Yet another way to receive termination events
	SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

	WNDCLASSEXA wndclass;
	wndclass.cbSize = sizeof(wndclass);
	wndclass.style = 0;
	wndclass.lpfnWndProc = DummyWindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = nullptr;
	wndclass.hIcon = nullptr;
	wndclass.hCursor = nullptr;
	wndclass.hbrBackground = nullptr;
	wndclass.lpszMenuName = nullptr;
	wndclass.lpszClassName = WINDOWCLASSNAME;
	wndclass.hIconSm = nullptr;

	if (0 == RegisterClassExA(&wndclass))
	{
		qDebug() << "Failed to register Windows window class";
	}
	else
	{
		HWND hwndWin = CreateWindowExA(0, WINDOWCLASSNAME, WINDOWCLASSNAME,
			WS_POPUP,
			CW_USEDEFAULT, 0,
			CW_USEDEFAULT, 0,
			GetDesktopWindow(),
			nullptr,
			nullptr,
			nullptr);

		if (nullptr == hwndWin)
		{
			DWORD d = GetLastError();
			qDebug() << "Failed to create Windows window: " << d;
		}
	}
#endif
}

DummyWindow::~DummyWindow()
{
}
