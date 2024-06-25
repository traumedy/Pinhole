#include "HostItem.h"
#include "Global.h"


HostItem::HostItem(const QString& name, const QString& role, const QString& version,
	const QString& platform, const QString& status, const QString& os,
	const QString& mac, QDateTime lastHeard)
	: m_name(name), m_role(role), m_version(version), m_platform(platform),
	m_status(status), m_os(os), m_mac(mac), m_lastHeard(lastHeard) 
{
}


QPair<int, int> HostItem::update(const QString& name, const QString& role, const QString& version,
	const QString& platform, const QString& status, const QString& os,
	const QString& mac)
{
	int firstColDifferent = COL_LASTHEARD;

	if (m_status != status)
	{
		m_status = status;
		firstColDifferent = COL_STATUS;
	}

	if (m_platform != platform)
	{
		m_platform = platform;
		firstColDifferent = COL_PLATFORM;
	}

	if (m_version != version)
	{
		m_version = version;
		firstColDifferent = COL_VERSION;
	}

	if (m_role != role)
	{
		m_role = role;
		firstColDifferent = COL_ROLE;
	}

	if (m_name != name)
	{
		m_name = name;
		firstColDifferent = COL_NAME;
	}

	int lastColDifferent = COL_LASTHEARD;

	if (m_os != os)
	{
		m_os = os;
		lastColDifferent = COL_OS;
	}

	if (!mac.isEmpty() && m_mac != mac)
	{
		m_mac = mac;
		lastColDifferent = COL_MAC;
	}

	m_lastHeard = QDateTime::currentDateTime();
	m_needExpiredNotification = true;

	return QPair<int, int>(firstColDifferent, lastColDifferent);
}


void HostItem::addAddress(const QPair<QString, int>& address, bool IPv4)
{
	// TODO - Find a better way of determing the best IP address to keep as
	// the 'preferred' address without having to expinsively check each address
	// in the list each time an entry is added
	// IPv4 Loopback
	// IPv6 Loopback
	// IPv4 address external address
	// IPv6 address external address
	// Check default route?
	if (!m_addressList.contains(address))
	{
		// Remove entries with the same address on different port
		for (auto addrEntry = m_addressList.begin(); addrEntry != m_addressList.end(); )
		{
			if (address.first == addrEntry->first)
				addrEntry = m_addressList.erase(addrEntry);
			else
				addrEntry++;
		}
		if (IPv4)
			m_addressList.push_front(address);
		else
			m_addressList.push_back(address);
	}
}


void HostItem::setPreferredAddress(const QString& address, int port)
{
	for (int x = 0; x < m_addressList.size(); x++)
	{
		if (m_addressList[x].first == address && m_addressList[x].second == port)
		{
			m_addressList.swapItemsAt(0, x);
			return;
		}
	}
}


void HostItem::removeAddress(const QString& address, int port)
{
	m_addressList.removeAll(qMakePair(address, port));
}

