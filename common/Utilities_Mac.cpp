#include "Utilities.h"

#if defined(Q_OS_MAC)
#include <Carbon/Carbon.h>

//#include <sys/sysinfo.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <signal.h>

#include <QtCore/QList>

QList<ProcessInfo> runningProcesses()
{
	int mib[4] = {
		CTL_KERN,
		KERN_ARGMAX,
		0,
		0
	};

	int argMax = 0;
	size_t argMaxSize = sizeof(argMax);
	// fetch the maximum process arguments size
	sysctl(mib, 2, &argMax, &argMaxSize, NULL, 0);
	char *processArguments = (char*)malloc(argMax);

	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ALL;
	size_t processTableSize = 0;
	// fetch the kernel process table size
	sysctl(mib, 4, NULL, &processTableSize, NULL, 0);
	struct kinfo_proc *processTable = (kinfo_proc*)malloc(processTableSize);

	// fetch the process table
	sysctl(mib, 4, processTable, &processTableSize, NULL, 0);

	QList<ProcessInfo> processes;
	for (size_t i = 0; i < (processTableSize / sizeof(struct kinfo_proc)); ++i) {
		struct kinfo_proc *process = processTable + i;

		ProcessInfo processInfo;
		processInfo.id = process->kp_proc.p_pid;

		mib[1] = KERN_PROCARGS2;
		mib[2] = process->kp_proc.p_pid;
		mib[3] = 0;

		size_t size = argMax;
		// fetch the process arguments
		if (sysctl(mib, 3, processArguments, &size, NULL, 0) != -1) {
			/*
			* |-----------------| <-- data returned by sysctl()
			* |      argc       |
			* |-----------------|
			* | executable path |
			* |-----------------|
			* |    arguments    |
			* ~~~~~~~~~~~~~~~~~~~
			* |-----------------|
			*/
			processInfo.name = QString::fromLocal8Bit(processArguments + sizeof(int));
		}
		else {
			// if we fail, use the name from the process table
			processInfo.name = QString::fromLocal8Bit(process->kp_proc.p_comm);
		}
		processes.append(processInfo);
	}
	free(processTable);
	free(processArguments);

	return processes;
}

bool killProcess(quint32 id, int msecs)
{
	//Q_UNUSED(process);
	Q_UNUSED(msecs);

	::kill(id, SIGTERM);

	return true;
}

/*
bool MemoryInformation(unsigned long long& totalMemory, unsigned long long& freeMemory)
{
#if 1
	Q_UNUSED(totalMemory);
	Q_UNUSED(freeMemory);
	return false;
#else
	struct sysinfo info;

	if (-1 == sysinfo(&info))
	{
		return false;
	}

	totalMemory = info.totalram * info.mem_unit;
	freeMemory = info.freeram * info.mem_unit;

	return true;
#endif
}
*/


bool IsProcessRunning(qint64 pid)
{
	return 0 == ::kill((pid_t)pid, 0);
}

#endif
