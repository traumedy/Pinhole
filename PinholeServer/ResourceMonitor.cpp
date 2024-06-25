#include "ResourceMonitor.h"
#include "Settings.h"
#include "GlobalManager.h"
#include "Logger.h"
#include "Values.h"
#include "Sigar.h"
#include "../common/Utilities.h"

#include <QStorageInfo>

ResourceMonitor::ResourceMonitor(Settings* settings, GlobalManager* globalManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_globalManager(globalManager)
{
	m_alertMemory = m_globalManager->getAlertMemory();
	m_minMemory = m_globalManager->getMinMemory();
	m_alertDisk = m_globalManager->getAlertDisk();
	m_minDisk = m_globalManager->getMinDisk();
	m_alertDiskList = m_globalManager->getAlertDiskList();

	connect(m_globalManager, &GlobalManager::valueChanged,
		this, &ResourceMonitor::globalValueChanged);

	m_checkTimer.setInterval(INTERVAL_RESOURCECHECK);
	m_checkTimer.setSingleShot(false);
	connect(&m_checkTimer, &QTimer::timeout,
		this, &ResourceMonitor::checkResources);
	m_checkTimer.start();
}


ResourceMonitor::~ResourceMonitor()
{
}


void ResourceMonitor::globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value)
{
	Q_UNUSED(itemName);

	if (GROUP_GLOBAL == groupName)
	{
		if (PROP_GLOBAL_ALERTMEMORY == propName)
		{
			m_alertMemory = value.toBool();
		}
		else if (PROP_GLOBAL_MINMEMORY == propName)
		{
			m_minMemory = value.toInt();
		}
		if (PROP_GLOBAL_ALERTDISK == propName)
		{
			m_alertDisk = value.toBool();
		}
		else if (PROP_GLOBAL_MINDISK == propName)
		{
			m_minDisk = value.toInt();
		}
		else if (PROP_GLOBAL_ALERTDISKLIST == propName)
		{
			m_alertDiskList = value.toStringList();
		}
	}
}


void ResourceMonitor::checkResources()
{
	// Check for low memory
	if (m_alertMemory && m_minMemory > 0)
	{
		quint64 totalMemory = 0;
		quint64 freeMemory = 0;
		if (MemoryInformation(totalMemory, freeMemory))
		{
			if (!m_memoryAlertSent)
			{
				if (freeMemory < static_cast<quint64>(m_minMemory) * 1048576ULL)
				{
					QString alert = tr("LOW MEMORY: Current %L1 less than minimum %L2")
						.arg(freeMemory)
						.arg(m_minMemory * 1048576LL);
					emit generateAlert(alert);
					m_memoryAlertSent = true;
				}
			}
			else
			{
				if (freeMemory > static_cast<quint64>(m_minMemory) * 943719ULL)
				{
					Logger() << tr("Memory now at %L1, resetting monitoring").arg(freeMemory);
					m_memoryAlertSent = false;
				}
			}
		}
	}

	// Check selected disks for low space
	if (m_alertDisk && m_minDisk > 0)
	{
		for (const auto& volume : QStorageInfo::mountedVolumes())
		{
			QString rootPath = QDir::toNativeSeparators(volume.rootPath());

			if (m_alertDiskList.contains(rootPath))
			{
				if (!m_diskAlertsSent.contains(rootPath))
				{
					if (volume.bytesFree() < m_minDisk * 1048576LL)
					{
						QString alert = tr("LOW DISK: %1 Current %L2 less than minimum %L3")
							.arg(rootPath)
							.arg(volume.bytesFree())
							.arg(m_minDisk * 1048576LL);
						emit generateAlert(alert);
						m_diskAlertsSent.append(rootPath);
					}
				}
				else
				{
					if (volume.bytesFree() > m_minDisk * 943719LL)
					{
						Logger() << tr("Disk %1 now at %L1, resetting monitoring")
							.arg(rootPath)
							.arg(volume.bytesFree());
						m_diskAlertsSent.removeAll(rootPath);
					}
				}
			}
		}
	}
}

