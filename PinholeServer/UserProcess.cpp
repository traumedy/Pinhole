#include "UserProcess.h"
#include "Settings.h"
#include "Logger.h"
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include "LinuxUtil.h"
#endif


UserProcess::UserProcess(Settings* settings, QObject *parent)
	: QProcess(parent), m_settings(settings)
{
}


UserProcess::~UserProcess()
{
}


void UserProcess::setupChildProcess()
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
	if (m_settings->runningAsService() && !m_elevated)
	{
		if (!ChangeToInteractiveUser(!m_settings->noGui()))
		{
			Logger(LOG_WARNING) << tr("Failed to lower privileges");
		}
	}
#endif
}


