#pragma once
/* Sigar.h - Wrappers around libSigar */

#include <QtGlobal>
#include <QString>
#include <QHostAddress>

struct CpuInfo
{
	QString Vendor;
	QString Model;
	int mhz;
	int mhzMin;
	int mhzMax;
	quint64 cacheSize;
};

struct SystemProcessesInfo
{
	quint64 total;
	quint64 sleeping;
	quint64 running;
	quint64 zombie;
	quint64 stopped;
	quint64 idle;
	quint64 threads;
};

struct RouteEntry
{
	QString interfaceName;
	QHostAddress destination;
	QHostAddress gateway;
	QHostAddress mask;
	quint64	flags;
	quint64 refcount;
	quint64 use;
	quint64 metric;
	quint64 mtu;
	quint64 window;
	quint64 irtt;
};

struct WhoEntry
{
	QString user;
	QString device;
	QString host;
	quint64 time;
};

// Returns memory information
bool MemoryInformation(quint64& totalMemory, quint64& freeMemory);

// Returns system uptime in seconds
bool SystemUptime(qint64& uptime);

// Returns a list of the CPUs
QList<CpuInfo> CpuInformation();

// Returns info about processes running on the system
bool GetSystemProcessesInfo(SystemProcessesInfo& procInfo);

// Returns the network route list
QList<RouteEntry> GetRouteList();

// Returns list of used devices
QList<WhoEntry> GetWhoList();
