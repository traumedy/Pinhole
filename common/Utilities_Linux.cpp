#include "Utilities.h"

#if (defined(Q_OS_UNIX)) && !defined(Q_OS_MAC)

#include <QDir>
#include <QFileInfo>
#include <QDebug>

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#ifdef Q_OS_FREEBSD
#include <sys/sysctl.h>
#endif

QList<ProcessInfo> runningProcesses()
{
	QList<ProcessInfo> processes;
	QDir procDir(QLatin1String("/proc"));
	const QFileInfoList procCont = procDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
	QRegExp validator(QLatin1String("[0-9]+"));
	Q_FOREACH(const QFileInfo &info, procCont) {
		if (validator.exactMatch(info.fileName())) {
			const QString linkPath = QDir(info.absoluteFilePath()).absoluteFilePath(QLatin1String("exe"));
			const QFileInfo linkInfo(linkPath);
			if (linkInfo.exists()) {
				ProcessInfo processInfo;
				processInfo.name = linkInfo.symLinkTarget();
				processInfo.id = info.fileName().toInt();
				processes.append(processInfo);
			}
		}
	}
	return processes;
}


bool killProcess(quint32 id, int msecs)
{
	Q_UNUSED(msecs);
	::kill(id, SIGTERM);
	return true;
}

/*
bool MemoryInformation(unsigned long long& totalMemory, unsigned long long& freeMemory)
{
	struct sysinfo info;

	if (-1 == sysinfo(&info))
	{
		return false;
	}

	totalMemory = info.totalram * info.mem_unit;
	freeMemory = info.freeram * info.mem_unit;

	return true;
}
*/

bool IsProcessRunning(qint64 pid)
{
	return 0 == ::kill((pid_t)pid, 0);
}

#endif
