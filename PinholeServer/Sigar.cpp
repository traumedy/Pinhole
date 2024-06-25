#include "Sigar.h"

#include "../libSigar/sigar.h"

#include <QtEndian>
#include <QDebug>



QHostAddress SigarAddressToQHostAddress(sigar_net_address_t& sigarAddr)
{
	switch (sigarAddr.family)
	{
	case sigar_net_address_t::SIGAR_AF_INET:
		return QHostAddress(static_cast<quint32>(qFromBigEndian(sigarAddr.addr.in)));
		break;

	case sigar_net_address_t::SIGAR_AF_INET6:
		return QHostAddress(reinterpret_cast<const quint8*>(sigarAddr.addr.in6));
		break;

	default:
		return QHostAddress();
		break;
	}
}


bool MemoryInformation(quint64& totalMemory, quint64& freeMemory)
{
	sigar_t* t;
	if (SIGAR_OK != sigar_open(&t))
		return false;

	sigar_mem_t mem;

	bool ret = false;
	if (SIGAR_OK == sigar_mem_get(t, &mem))
	{
		totalMemory = mem.total;
		freeMemory = mem.free;
		ret = true;
	}

	sigar_close(t);
	return ret;
}


bool SystemUptime(qint64& sysuptime)
{
	sigar_t* t;
	if (SIGAR_OK != sigar_open(&t))
		return false;

	bool ret = false;

	sigar_uptime_t uptime;

	if (SIGAR_OK == sigar_uptime_get(t, &uptime))
	{
		sysuptime = uptime.uptime;
		//qDebug() << sysuptime << uptime.uptime;
		ret = true;
	}

	sigar_close(t);
	return ret;
}


QList<CpuInfo> CpuInformation()
{
	sigar_t* t;
	if (SIGAR_OK != sigar_open(&t))
		return {};

	QList<CpuInfo> ret;

	sigar_cpu_info_list_t cpuinfo;

	if (SIGAR_OK == sigar_cpu_info_list_get(t, &cpuinfo))
	{
		for (unsigned long i = 0; i < cpuinfo.number; i++)
		{
			CpuInfo cpu;
			cpu.Vendor = cpuinfo.data[i].vendor;
			cpu.Model = cpuinfo.data[i].model;
			cpu.mhz = cpuinfo.data[i].mhz;
			cpu.mhzMin = cpuinfo.data[i].mhz_min;
			cpu.mhzMax = cpuinfo.data[i].mhz_max;
			cpu.cacheSize = cpuinfo.data[i].cache_size;
			ret.push_back(cpu);
		}

		sigar_cpu_info_list_destroy(t, &cpuinfo);
	}

	sigar_close(t);
	return ret;
}


bool GetSystemProcessesInfo(SystemProcessesInfo& procInfo)
{
	sigar_t* t;
	if (SIGAR_OK != sigar_open(&t))
		return false;

	sigar_proc_stat_t procstats;

	bool ret = false;
	if (SIGAR_OK == sigar_proc_stat_get(t, &procstats))
	{
		procInfo.total = procstats.total;
		procInfo.sleeping = procstats.sleeping;
		procInfo.running = procstats.running;
		procInfo.zombie = procstats.zombie;
		procInfo.stopped = procstats.stopped;
		procInfo.idle = procstats.idle;
		procInfo.threads = procstats.threads;
		ret = true;
	}

	sigar_close(t);
	return ret;
}


QList<RouteEntry> GetRouteList()
{
	sigar_t* t;
	if (SIGAR_OK != sigar_open(&t))
		return {};

	sigar_net_route_list_t routelist;

	QList<RouteEntry> ret;
	if (SIGAR_OK == sigar_net_route_list_get(t, &routelist))
	{
		for (unsigned long i = 0; i < routelist.number; i++)
		{
			RouteEntry entry;
			entry.interfaceName = routelist.data[i].ifname;
			entry.destination = SigarAddressToQHostAddress(routelist.data[i].destination);
			entry.gateway = SigarAddressToQHostAddress(routelist.data[i].gateway);
			entry.mask = SigarAddressToQHostAddress(routelist.data[i].mask);
			entry.flags = routelist.data[i].flags;
			entry.refcount = routelist.data[i].refcnt;
			entry.use = routelist.data[i].use;
			entry.metric = routelist.data[i].metric;
			entry.mtu = routelist.data[i].mtu;
			entry.window = routelist.data[i].window;
			entry.irtt = routelist.data[i].irtt;
			ret.push_back(entry);
		}

		sigar_net_route_list_destroy(t, &routelist);
	}

	sigar_close(t);
	return ret;
}


QList<WhoEntry> GetWhoList()
{
	sigar_t* t;
	if (SIGAR_OK != sigar_open(&t))
		return {};

	sigar_who_list_t wholist;

	QList<WhoEntry> ret;
	if (SIGAR_OK == sigar_who_list_get(t, &wholist))
	{
		for (unsigned long i = 0; i < wholist.number; i++)
		{
			WhoEntry entry;
			entry.user = wholist.data[i].user;
			entry.device = wholist.data[i].device;
			entry.host = wholist.data[i].host;
			entry.time = wholist.data[i].time;
			ret.push_back(entry);
		}

		sigar_who_list_destroy(t, &wholist);
	}

	sigar_close(t);
	return ret;
}

