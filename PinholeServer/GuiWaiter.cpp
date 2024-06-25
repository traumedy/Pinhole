#include "GuiWaiter.h"
#include "Logger.h"
#include "Values.h"
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include "LinuxUtil.h"
#endif
#if defined(Q_OS_WIN)
#include "WinUtil.h"
#endif

#include <QElapsedTimer>

GuiWaiter::GuiWaiter(QObject *parent)
	: QThread(parent)
{
}

GuiWaiter::~GuiWaiter()
{
}


void GuiWaiter::run()
{
	// Start timer counting
	QElapsedTimer timer;
	timer.start();

	// Loop until timer expires or GUI becomes available
	bool success = true;
	do
	{
		success = checkGUI();
		if (!success)
			QThread::msleep(50);
	} while (!success && timer.elapsed() < INTERVAL_GUITIMEOUT * 1000);

	emit finished();
}


bool GuiWaiter::checkGUI()
{
	bool success = true;
#if defined(Q_OS_WIN)
	success = IsUserLoggedIn();
#elif defined(Q_OS_MAC)
	// todo
#elif defined(Q_OS_UNIX)
	success = isX11Running();
#endif
	
	return success;
}

