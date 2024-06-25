#include "Utilities.h"

#if defined(Q_OS_WIN32)

#include <QLibrary>

#include <Windows.h>
#include <Tlhelp32.h>

const int KDSYSINFO_PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;

struct EnumWindowsProcParam
{
	QList<ProcessInfo> processes;
	QList<quint32> seenIDs;
};

typedef BOOL(WINAPI *QueryFullProcessImageNamePtr)(HANDLE, DWORD, char *, PDWORD);
typedef DWORD(WINAPI *GetProcessImageFileNamePtr)(HANDLE, char *, DWORD);

QList<ProcessInfo> runningProcesses()
{
	EnumWindowsProcParam param;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!snapshot)
		return param.processes;

	QStringList deviceList;
	const DWORD bufferSize = 1024;
	char buffer[bufferSize + 1] = { 0 };
	if (QSysInfo::windowsVersion() <= QSysInfo::WV_5_2) {
		const DWORD size = GetLogicalDriveStringsA(bufferSize, buffer);
		deviceList = QString::fromLatin1(buffer, size).split(QLatin1Char(char(0)), QString::SkipEmptyParts);
	}

	QLibrary kernel32(QLatin1String("Kernel32.dll"));
	kernel32.load();
	QueryFullProcessImageNamePtr pQueryFullProcessImageNamePtr = (QueryFullProcessImageNamePtr)kernel32
		.resolve("QueryFullProcessImageNameA");

	QLibrary psapi(QLatin1String("Psapi.dll"));
	psapi.load();
	GetProcessImageFileNamePtr pGetProcessImageFileNamePtr = (GetProcessImageFileNamePtr)psapi
		.resolve("GetProcessImageFileNameA");

	PROCESSENTRY32 processStruct;
	processStruct.dwSize = sizeof(PROCESSENTRY32);
	bool foundProcess = Process32First(snapshot, &processStruct);
	while (foundProcess) {
		HANDLE procHandle = OpenProcess(QSysInfo::windowsVersion() > QSysInfo::WV_5_2
			? KDSYSINFO_PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_QUERY_INFORMATION, false, processStruct
			.th32ProcessID);

		bool succ = false;
		QString executablePath;
		DWORD bufferSize = 1024;

		if (QSysInfo::windowsVersion() > QSysInfo::WV_5_2) {
			succ = pQueryFullProcessImageNamePtr(procHandle, 0, buffer, &bufferSize);
			executablePath = QString::fromLatin1(buffer);
		}
		else if (pGetProcessImageFileNamePtr) {
			succ = pGetProcessImageFileNamePtr(procHandle, buffer, bufferSize);
			executablePath = QString::fromLatin1(buffer);
			for (int i = 0; i < deviceList.count(); ++i) {
				executablePath.replace(QString::fromLatin1("\\Device\\HarddiskVolume%1\\").arg(i + 1),
					deviceList.at(i));
			}
		}

		if (succ) {
			const quint32 pid = processStruct.th32ProcessID;
			param.seenIDs.append(pid);
			ProcessInfo info;
			info.id = pid;
			info.name = executablePath;
			param.processes.append(info);
		}

		CloseHandle(procHandle);
		foundProcess = Process32Next(snapshot, &processStruct);

	}
	if (snapshot)
		CloseHandle(snapshot);

	kernel32.unload();
	return param.processes;
}


QList<DWORD> processThreads(DWORD procId) 
{
	QList<DWORD> ret;

	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(h, &te))
		{
			do
			{
				if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
					sizeof(te.th32OwnerProcessID))
				{
					if (te.th32OwnerProcessID == procId)
					{
						ret.append(te.th32ThreadID);
					}
				}
				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));
		}
		CloseHandle(h);
	}

	return ret;
}


static BOOL QT_WIN_CALLBACK killProcess_windowCallback(HWND hwnd, LPARAM procId)
{
	DWORD currentProcId = 0;
	GetWindowThreadProcessId(hwnd, &currentProcId);
	if (currentProcId == (DWORD)procId)
		PostMessage(hwnd, WM_CLOSE, 0, 0);

	return TRUE;
}


bool killProcess(quint32 id, int msecs)
{
	EnumWindows(killProcess_windowCallback, (LPARAM)id);

	QList<DWORD> threads = processThreads(id);
	for (auto threadId : threads)
	{
		PostThreadMessage(threadId, WM_CLOSE, 0, 0);
	}

	HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, (DWORD)id);
	if (nullptr == process)
		return true;

	if (WAIT_TIMEOUT == WaitForSingleObjectEx(process, msecs, FALSE))
	{
		TerminateProcess(process, 0);
	}
	CloseHandle(process);

	return IsProcessRunning(id);
}


/*
bool MemoryInformation(unsigned long long& totalMemory, unsigned long long& freeMemory)
{
	MEMORYSTATUSEX memStat;
	ZeroMemory(&memStat, sizeof(MEMORYSTATUSEX));
	memStat.dwLength = sizeof(MEMORYSTATUSEX);

	if (!GlobalMemoryStatusEx(&memStat))
	{
		return false;
	}

	totalMemory = memStat.ullTotalPhys;
	freeMemory = memStat.ullAvailPhys;

	return true;
}
*/


bool IsProcessRunning(qint64 pid)
{
	HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
	if (NULL == process)
		return false;
	CloseHandle(process);
	return true;
}

#endif


