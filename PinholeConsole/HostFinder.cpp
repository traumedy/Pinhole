#include "HostFinder.h"
#include "Global.h"
#include "HostItem.h"
#include "../common/PinholeCommon.h"
#include "../common/Utilities.h"

#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QNetworkConfigurationManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QMessageBox>
#include <QGuiApplication>
#include <QUdpSocket>


#define PREFIX_HOST		"host_"
#define PREFIX_ADDRESS	"address_"
#define PREFIX_PORT		"port_"
#define VALUE_HOSTID	"hostId"
#define VALUE_ADDRESS	"address"
#define VALUE_ADDRESSCOUNT	"addressCount"
#define VALUE_HOSTLIST	"hostList"
#define VALUE_HOSTNAME	"name"
#define VALUE_ROLE		"role"
#define VALUE_VERSION	"version"
#define VALUE_PLATFORM	"platform"
#define VALUE_LASTHEARD	"lastHeard"
#define VALUE_OS		"os"
#define VALUE_MAC		"MAC"

HostFinder::HostFinder(QObject* parent) : 
	QAbstractTableModel(parent)
{
	m_sock = new QUdpSocket(this);

	loadHostList();

	// Brush for marking recently seen hosts in table
	m_hashBrush = new QBrush(Qt::BDiagPattern);
	m_hashBrush->setColor(QColor(170, 64, 64));

	// Create query packet
	QJsonObject jsonObject;
	jsonObject[TAG_COMMAND] = UDPCOMMAND_QUERY;
	QJsonDocument jsonDoc;
	jsonDoc.setObject(jsonObject);
	m_queryPacket = jsonDoc.toJson();

	connect(m_sock, &QUdpSocket::readyRead,
		this, &HostFinder::readyRead);

	m_broadcastTimer.setInterval(FREQ_HOSTBROADCAST);
	m_broadcastTimer.setSingleShot(false);
	connect(&m_broadcastTimer, &QTimer::timeout,
		this, &HostFinder::broadcastHostQuery);

	QTimer* expireTimer = new QTimer(this);
	expireTimer->setInterval(FREQ_HOSTEXPIRECHECK);
	expireTimer->setSingleShot(false);
	connect(expireTimer, &QTimer::timeout,
		this, &HostFinder::expireCheck);
	expireTimer->start();

	QTimer* queryLoopbackTimer = new QTimer(this);
	queryLoopbackTimer->setInterval(FREQ_LOOPBACKQUERY);
	queryLoopbackTimer->setSingleShot(false);
	connect(queryLoopbackTimer, &QTimer::timeout,
		this, &HostFinder::queryLoopback);
	queryLoopbackTimer->start();

	QTimer* queryHostListTimer = new QTimer(this);
	queryHostListTimer->setInterval(FREQ_HOSTQUERY);
	queryHostListTimer->setSingleShot(false);
	connect(queryHostListTimer, &QTimer::timeout,
		this, &HostFinder::queryHostList);
	queryHostListTimer->start();

	// Make list of local addresses and update it when there are changes to configuration
	findLocalAddresses();
	m_networkConfigurationManager = new QNetworkConfigurationManager(this);
	connect(m_networkConfigurationManager, &QNetworkConfigurationManager::configurationChanged,
		this, &HostFinder::findLocalAddresses);
	connect(m_networkConfigurationManager, &QNetworkConfigurationManager::configurationAdded,
		this, &HostFinder::findLocalAddresses);
	connect(m_networkConfigurationManager, &QNetworkConfigurationManager::configurationRemoved,
		this, &HostFinder::findLocalAddresses);
}


HostFinder::~HostFinder()
{
	saveHostList();
	delete m_hashBrush;
}


void HostFinder::clear()
{
	emit beginRemoveRows(QModelIndex(), 0, m_hostList.size() - 1);
	m_hostList.clear();
	emit endRemoveRows();
}


bool HostFinder::eraseEntry(const QString& id)
{
	if (!m_hostList.contains(id))
	{
		return false;
	}

	int row = hostIndex(id);
	emit beginRemoveRows(QModelIndex(), row, row);
	m_hostList.remove(id);
	emit endRemoveRows();
	return true;
}


void HostFinder::queryLoopback()
{
	// Unicast queries to local loopback
	m_sock->writeDatagram(m_queryPacket, QHostAddress(QHostAddress::LocalHost), HOST_UDPPORT);
	m_sock->writeDatagram(m_queryPacket, QHostAddress(QHostAddress::LocalHostIPv6), HOST_UDPPORT);
}


// Checks hosts to see if they have become expired and notifies model of background change
void HostFinder::expireCheck()
{
	int row = 0;
	for (auto hostIt = m_hostList.begin(); hostIt != m_hostList.end(); hostIt++)
	{
		if (hostIt.value()->getNeedExpired() &&
			hostIt.value()->getLastHeard().secsTo(QDateTime::currentDateTime()) > SECS_MINHIGHLIGHTHOST)
		{
			hostIt.value()->setNeedExpired(false);
			emit dataChanged(index(row, COL_LASTHEARD), index(row, COL_LASTHEARD), { Qt::BackgroundRole });
		}
		row++;
	}
}


// Broadcasts a host query packet on each network interface
void HostFinder::broadcastHostQuery()
{
	// Send to broadcast address on each interface
	for (const auto& iface : m_interfaces)
	{
		if (iface.flags() &
			(QNetworkInterface::IsUp |
				QNetworkInterface::IsRunning |
				QNetworkInterface::CanBroadcast)
			&& !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
		{
			for (const auto& address : iface.addressEntries())
			{
				if (address.ip().protocol() == QAbstractSocket::IPv4Protocol)
				{
					m_sock->writeDatagram(m_queryPacket, address.broadcast(), HOST_UDPPORT);
				}
				else if (address.ip().protocol() == QAbstractSocket::IPv6Protocol)
				{
					QNetworkDatagram datagram;
					datagram.setInterfaceIndex(iface.index());
					datagram.setSender(address.ip());
					datagram.setData(m_queryPacket);
					datagram.setDestination(QHostAddress(IPV6_MULTICAST), HOST_UDPPORT);
					m_sock->writeDatagram(datagram);
				}
			}
		}
	}
}


// Sends host query packets to each host in the host list not on a local subnet
void HostFinder::queryHostList()
{
	for (const auto& host : m_hostList)
	{
		QHostAddress addr(host->getAddress());
		// Don't send query if broadcast query would send anyway
		// JEBNOTE: Always query just in case targets can't receive broadcast packets for some reason
		//if (!m_broadcastTimer.isActive() || !isAddressBroadcastable(addr))
		{
			m_sock->writeDatagram(m_queryPacket, addr, HOST_UDPPORT);
		}
	}
}


// Sends a query to a single host address
void HostFinder::queryHostAddress(const QHostAddress & addr) const
{
	m_sock->writeDatagram(m_queryPacket, addr, HOST_UDPPORT);
}


// Reads incomming UDP datagrams
void HostFinder::readyRead()
{
	while (m_sock->hasPendingDatagrams())
	{
		// Receive a datagram from a client
		QNetworkDatagram datagram = m_sock->receiveDatagram();

		// Check for read error (not sure why this happens)
		if (!datagram.isValid())
		{
			//qDebug() << "Error reading datagram" << m_sock->errorString();
			return;
		}

#if 1
		// Don't process datagrams that come from auto-configured or other local interface addresses
		if (isAutoConfiguredAddress(datagram.senderAddress()))
			return;
#else
		// Only process datagram if source is not from a local address (not including loopback)
		if (isLocalAddress(datagram.senderAddress()))
			return;
#endif

		// Parse the data as JSON
#if defined(QT_DEBUG)
		QByteArray jsonData = datagram.data();
		QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
#else
		QJsonDocument jsonDoc = QJsonDocument::fromJson(datagram.data());
#endif
		// Validate the data
		if (!jsonDoc.isNull())
		{
			// Format the address string
			bool preferredAddress = false;
			QString addrStr;
			switch (datagram.senderAddress().protocol())
			{
			case QAbstractSocket::IPv4Protocol:
				addrStr = datagram.senderAddress().toString();
				preferredAddress = true;
				break;

			case QAbstractSocket::IPv6Protocol:
				addrStr = HostAddressToString(datagram.senderAddress(), &preferredAddress);
				break;

			default:
				break;
			}

			// Parse the json data
			QJsonObject jsonObject = jsonDoc.object();
			QString command = jsonObject[TAG_COMMAND].toString();
			QString id, hostAddress, name, role, version, platform, status, os, MAC;
			int port = HOST_TCPPORT;	// Default port unless overridden by redirect
			bool statusOnly = false;
			if (UDPCOMMAND_ANNOUNCE == command)
			{
				id = jsonObject[TAG_ID].toString();
				name = jsonObject[TAG_NAME].toString();
				role = jsonObject[TAG_ROLE].toString();
				version = jsonObject[TAG_VERSION].toString();
				platform = jsonObject[TAG_PLATFORM].toString();
				status = jsonObject[TAG_STATUS].toString();
				os = jsonObject[TAG_OS].toString();
				MAC = jsonObject[TAG_MAC].toString();
			}
			else if (UDPCOMMAND_STATUS == command)
			{
				id = jsonObject[TAG_ID].toString();
				status = jsonObject[TAG_STATUS].toString();
				statusOnly = true;
			}
			else if (UDPCOMMAND_REDIRECT == command)
			{
				id = jsonObject[TAG_ID].toString();
				hostAddress = jsonObject[TAG_ADDRESS].toString();
				port = jsonObject[TAG_PORT].toInt();
				name = jsonObject[TAG_NAME].toString();
				role = jsonObject[TAG_ROLE].toString();
				version = jsonObject[TAG_VERSION].toString();
				platform = jsonObject[TAG_PLATFORM].toString();
				status = jsonObject[TAG_STATUS].toString();
				os = jsonObject[TAG_OS].toString();
			}
			else
			{
				// Unknown command
				return;
			}

			// For compatibility with older servers that don't send ID
			if (id.isEmpty())
			{
				id = addrStr;
			}

			// Find host in list
			QSharedPointer<HostItem> host;
			if (m_hostList.contains(id))
				host = m_hostList[id];
			int hostRow = hostIndex(id);

			if (statusOnly)
			{
				if (!host.isNull())
				{
					host->setStatus(status);
					emit dataChanged(index(hostRow, COL_STATUS), index(hostRow, COL_STATUS), { Qt::DisplayRole });
				}
			}
			else
			{
				if (host.isNull())
				{
					// Host not in list yet
					emit beginInsertRows(QModelIndex(), hostRow, hostRow);
					m_hostList[id] = QSharedPointer<HostItem>::create(name, role, version, platform, status, os, MAC);
					host = m_hostList[id];
					emit endInsertRows();
				}
				else
				{
					// Update host in list
					QPair<int, int> cols = host->update(name, role, version, platform, status, os, MAC);
					emit dataChanged(index(hostRow, cols.first), index(hostRow, cols.second), { Qt::DisplayRole, Qt::BackgroundRole });
				}
				host->addAddress(QPair<QString, int>(addrStr, port), preferredAddress);
				if (!hostAddress.isEmpty())
					host->setHostAddress(hostAddress);
			}
		}
	}
}


int HostFinder::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return COL_LAST + 1;
}


int HostFinder::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return m_hostList.size();
}


QVariant HostFinder::data(const QModelIndex &index, int role) const
{
	// Return quick if not a role we care about (this function gets called a BUNCH)
	switch (role)
	{
	case Qt::ToolTipRole:
	case Qt::DisplayRole:
	case HOSTROLE_ID:
	case HOSTROLE_ADDRESS:
	case HOSTROLE_PORT:
		break;

	case Qt::BackgroundRole:
		// Only color the lastHeard column
		if (index.column() != COL_LASTHEARD)
			return QVariant();
		break;

	default:
		return QVariant();
	}

	// Row number is index into QMap
	QString id(m_hostList.keys()[index.row()]);

	if (HOSTROLE_ID == role)
		return id;

	// Get host at index
	auto host = m_hostList[id];

	switch (role)
	{
	case Qt::ToolTipRole:
		if (COL_ADDRESS == index.column())
		{
			// Show address list
			auto addressList = host->getAddressList();
			QString ret;
			for (const auto& address : addressList)
			{
				ret.append(QString("Address: %1 port %2\n").arg(address.first).arg(address.second));
			}
			ret.append(m_colDescriptions[index.column()]);
			return ret;
		}
		else
		{
			return QString("%1 (%2)\n%3")
				.arg(host->getName())
				.arg(host->getAddress())
				.arg(m_colDescriptions[index.column()]);
		}
		break;

	case Qt::DisplayRole:
		switch (index.column())
		{
		case COL_ADDRESS:
		{
			QString hostAddress = host->getHostAddress();
			int addressCount = host->getAddressCount();
			QString address = host->getAddress();
			if (!hostAddress.isEmpty())
				address += " (" + hostAddress + ")";
			if (addressCount > 1)
				address += " +" + QString::number(addressCount - 1);
			return address;
		}
			break;
		case COL_NAME:
			return host->getName();
			break;
		case COL_ROLE:
			return host->getRole();
			break;
		case COL_VERSION:
			return host->getVersion();
			break;
		case COL_PLATFORM:
			return host->getPlatform();
			break;
		case COL_STATUS:
			return host->getStatus();
			break;
		case COL_LASTHEARD:
			return host->getLastHeard();
			break;
		case COL_OS:
			return host->getOs();
			break;
		case COL_MAC:
			return host->getMAC();
			break;
		}
		break;

	case Qt::BackgroundRole:
		// Slash out hosts that haven't been seen recently
		if (host->getLastHeard().secsTo(QDateTime::currentDateTime()) > SECS_MINHIGHLIGHTHOST)
		{
			return QVariant(*m_hashBrush);
		}
		break;

	case HOSTROLE_ADDRESS:
		return host->getAddress();
		break;

	case HOSTROLE_PORT:
		return host->getPort();
		break;

	default:
		break;
	}
	return QVariant();
}


QVariant HostFinder::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		switch (section)
		{
		case COL_ADDRESS:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Address");
			case Qt::ToolTipRole:
				return tr("The IP address of the host");
			case Qt::WhatsThisRole:
				return tr("This column shows the IPv4 or IPv6 address of the remote host "
					"interface that responded to the query packet.  The same "
					"host may appear more than once in the host list with "
					"different addresses if it has multiple addresses and/or "
					"network interfaces.  If a host has both IPv4 and IPv6 "
					"enabled then both addresses may appear as entries in the "
					"host list.");
			default:
				break;
			}

		case COL_NAME:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Name");
			case Qt::ToolTipRole:
				return tr("The host name");
			case Qt::WhatsThisRole:
				return tr("This column shows the host name of the remote host.  This "
					"can be the Windows machine name or the network host name.");
			default:
				break;
			}

		case COL_ROLE:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Role");
			case Qt::ToolTipRole:
				return tr("The role assigned to the host");
			case Qt::WhatsThisRole:
				return tr("This column shows the role assigned to this "
					"host in the Globals tab of the Pinhole configuration.");
			default:
				break;
			}

		case COL_VERSION:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Ver");
			case Qt::ToolTipRole:
				return tr("The PinholeServer version");
			case Qt::WhatsThisRole:
				return tr("This column shows the version of PinholeServer "
					"the remote host is running.");
			default:
				break;
			}

		case COL_PLATFORM:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Plat");
			case Qt::ToolTipRole:
				return tr("The platform of the host");
			case Qt::WhatsThisRole:
				return tr("This column shows a code for the operating system "
					"platform; WIN for Windows, NIX for Unix/Linux based OSs, "
					"MAC for Mac OS, etc.");
			default:
				break;
			}

		case COL_STATUS:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Status");
			case Qt::ToolTipRole:
				return tr("The current status of the host");
			case Qt::WhatsThisRole:
				return tr("This column shows the state reported by the host.  If this "
					"field is blank Pinhole Console has not heard from this "
					"host during this execution.  If the host is idle this "
					"will show how long the server has been running, otherwise "
					"it will show how many applications are currently running.");
			default:
				break;
			}

		case COL_LASTHEARD:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("Last heard");
			case Qt::ToolTipRole:
				return tr("The last time the host was heard from");
			case Qt::WhatsThisRole:
				return tr("This column shows the date and time of the last time Pinhole "
					"Console received a response to a query from this host.  "
					"Pinhole Console sends queries to known hosts every few "
					"seconds.");
			default:
				break;
			}

		case COL_OS:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("OS");
			case Qt::ToolTipRole:
				return tr("The host operating system");
			case Qt::WhatsThisRole:
				return tr("This column shows the operating system, version and build as reported "
					"by the remote host.");
			default:
				break;
			}

		case COL_MAC:
			switch (role)
			{
			case Qt::DisplayRole:
				return tr("MAC");
			case Qt::ToolTipRole:
				return tr("The hardware address of the ethernet card");
			case Qt::WhatsThisRole:
				return tr("If present, the MAC address can be used to remotely "
					"wake the computer using the Wake-On-LAN button (if the computer "
					"supports this functionality).");
			default:
				break;
			}
		}
	}

	return QVariant();
}


// Returns the index (row) of a host from the id, 
// even if the host is not currently in the host list
int HostFinder::hostIndex(const QString& id) const
{
	QStringList keys = m_hostList.keys();
	if (!m_hostList.contains(id))
	{
		// This assumes that QMap sorts the keys alphabetically
		keys.append(id);
		keys.sort();
	}
	return keys.indexOf(id);
}


// Returns true if addr is in broadcast range of any bound interface addresses
bool HostFinder::isAddressBroadcastable(const QHostAddress& addr) const
{
	if (addr.protocol() != QAbstractSocket::IPv4Protocol)
		return false;

	for (const auto& address : m_localAddresses)
	{
		if (addr.isInSubnet(address.first, address.second))
		{
			return true;
		}
	}

	return false;
}


// Returns true if address is one bound to the local machine, not including loopback
bool HostFinder::isLocalAddress(const QHostAddress& address) const
{
	for (const auto& addr : m_localAddresses)
	{
		if (addr.first.isEqual(address,
			QHostAddress::ConvertV4MappedToIPv4 | QHostAddress::ConvertV4CompatToIPv4))
		{
			return true;
		}
	}

	return false;
}


// Returns true if IPv4 address and not link local
bool HostFinder::isAutoConfiguredAddress(const QHostAddress& addr) const
{
	bool convertOk;
	addr.toIPv4Address(&convertOk);

	if (addr.isLinkLocal() && convertOk)
		return true;

	return false;
}


bool HostFinder::loadHostList()
{
	QSettings settings;

	// Clear the current list just in case
	m_hostList.clear();

	QStringList keys = settings.value(VALUE_HOSTLIST).toStringList();
	for (const auto& key : keys)
	{
		settings.beginGroup(PREFIX_HOST + key);
		QString name = settings.value(VALUE_HOSTNAME).toString();
		QString role = settings.value(VALUE_ROLE).toString();
		QString version = settings.value(VALUE_VERSION).toString();
		QString platform = settings.value(VALUE_PLATFORM).toString();
		QDateTime lastHeard = settings.value(VALUE_LASTHEARD).toDateTime();
		QString os = settings.value(VALUE_OS).toString();
		QString MAC = settings.value(VALUE_MAC).toString();
		int addressCount = settings.value(VALUE_ADDRESSCOUNT).toInt();
		// Add new item to map
		m_hostList[key] = QSharedPointer<HostItem>::create(name, role, version, platform, QString(), os, MAC, lastHeard);
		for (int x = 0; x < addressCount; x++)
		{
			QString address = settings.value(PREFIX_ADDRESS + QString::number(x)).toString();
			int port = settings.value(PREFIX_PORT + QString::number(x)).toInt();
			m_hostList[key]->addAddress(QPair<QString, int>(address, port), false);
		}

		settings.endGroup();
	}

	return true;
}


bool HostFinder::saveHostList() const
{
	QSettings settings;

	// Remove existing hosts
	QStringList keys = settings.value(VALUE_HOSTLIST).toStringList();
	for (const auto& key : keys)
	{
		settings.remove(PREFIX_HOST + key);
	}

	// Write each host entry group
	for (const auto& key : m_hostList.keys())
	{
		settings.beginGroup(PREFIX_HOST + key);
		settings.setValue(VALUE_HOSTNAME, m_hostList[key]->getName());
		settings.setValue(VALUE_ROLE, m_hostList[key]->getRole());
		settings.setValue(VALUE_VERSION, m_hostList[key]->getVersion());
		settings.setValue(VALUE_PLATFORM, m_hostList[key]->getPlatform());
		settings.setValue(VALUE_LASTHEARD, m_hostList[key]->getLastHeard());
		settings.setValue(VALUE_OS, m_hostList[key]->getOs());
		settings.setValue(VALUE_MAC, m_hostList[key]->getMAC());
		auto addressList = m_hostList[key]->getAddressList();
		settings.setValue(VALUE_ADDRESSCOUNT, addressList.size());
		for (int x = 0; x < addressList.size(); x++)
		{
			settings.setValue(PREFIX_ADDRESS + QString::number(x), addressList[x].first);
			settings.setValue(PREFIX_PORT + QString::number(x), addressList[x].second);
		}

		settings.endGroup();
	}
	
	// Write new key list
	settings.setValue(VALUE_HOSTLIST, QStringList(m_hostList.keys()));

	return true;
}


bool HostFinder::importHostList(const QJsonObject& root)
{
	if (!root.contains(VALUE_HOSTLIST))
	{
		QMessageBox::warning(qobject_cast<QWidget*>(parent()), QGuiApplication::applicationDisplayName(),
			tr("Host list JSON import data missing tag %1")
			.arg(VALUE_HOSTLIST));
		return false;
	}

	try
	{
		QJsonArray jhostList = root[VALUE_HOSTLIST].toArray();
		for (const auto& host : jhostList)
		{
			QJsonObject jhost = host.toObject();
			QString hostId = jhost[VALUE_HOSTID].toString();
			QString hostName = jhost[VALUE_HOSTNAME].toString();
			QString MAC = jhost[VALUE_MAC].toString();
			QJsonValue addressVal = jhost[VALUE_ADDRESS];

			if (hostId.isEmpty())
			{
				// Old style host list?
				hostId = addressVal.toString();
			}

			if (hostId.isEmpty())
			{
				// Bad entry
				continue;
			}

			int hostRow = hostIndex(hostId);
			if (m_hostList.contains(hostId))
			{
				// If address already exists in list update MAC address
				m_hostList[hostId]->setMAC(MAC);
				emit dataChanged(index(hostRow, COL_MAC), index(hostRow, COL_MAC), { Qt::DisplayRole });
			}
			else
			{
				emit beginInsertRows(QModelIndex(), hostRow, hostRow);
				// Use address as the key temporarily, a new entry will get created if the server responds with an ID
				m_hostList[hostId] = QSharedPointer<HostItem>::create(hostName, QString(), QString(), QString(), QString(), QString(), MAC, QDateTime());
				emit endInsertRows();
			}

			if (addressVal.isObject())
			{
				// Address list is stored in JSON object
				QJsonObject addressObj = addressVal.toObject();
				// Add in reverse order
				auto keys = addressObj.keys();
				for (auto key = keys.rbegin(); key != keys.rend(); key++)
				{
					int port = addressObj[*key].toInt();
					if (0 != port)
					{
						m_hostList[hostId]->addAddress(QPair<QString, int>(*key, port), false);
					}
				}
			}
			else
			{
				// Old style host list
				QString address = addressVal.toString();
				if (!address.isEmpty())
				{
					m_hostList[hostId]->addAddress(QPair<QString, int>(address, HOST_TCPPORT), false);
				}
			}

		}
	}
	catch (...)
	{
		QMessageBox::warning(qobject_cast<QWidget*>(parent()), QGuiApplication::applicationDisplayName(),
			tr("Error parsing host import data"));
		return false;
	}

	return true;
}


QJsonObject HostFinder::exportHostList() const
{
	QJsonArray jhostList;
	for (const auto& key : m_hostList.keys())
	{
		QJsonObject jhost;
		jhost[VALUE_HOSTID] = key;
		jhost[VALUE_HOSTNAME] = m_hostList[key]->getName();
		jhost[VALUE_MAC] = m_hostList[key]->getMAC();
		QJsonObject addressObj;
		for (const auto& addr : m_hostList[key]->getAddressList())
		{
			// Don't output loopback addresses
			if (!QHostAddress(addr.first).isLoopback())
			{
				addressObj[addr.first] = addr.second;
			}
		}
		jhost[VALUE_ADDRESS] = addressObj;
		jhostList.append(jhost);
	}

	QJsonObject root;
	root[VALUE_HOSTLIST] = jhostList;
	return root;
}


// Starts/stops broadcasting for additional hosts
void HostFinder::enableScanning(bool enable)
{
	if (enable)
	{
		m_broadcastTimer.start();
		broadcastHostQuery();
	}
	else
	{
		m_broadcastTimer.stop();
	}
}


// Builds list of local addresses 
void HostFinder::findLocalAddresses()
{
	// Store this so we don't need to call QNetworkInterface::allInterfaces() all the time
	m_interfaces = QNetworkInterface::allInterfaces();

	m_localAddresses.clear();
	for (const auto& iface : m_interfaces)
	{
		if (iface.flags() &
			(QNetworkInterface::IsUp |
				QNetworkInterface::IsRunning |
				QNetworkInterface::CanBroadcast)
			&& !iface.flags().testFlag(QNetworkInterface::IsLoopBack)
			)
		{
			for (const auto& address : iface.addressEntries())
			{
				// Store local addresses so we can filter the response packets
				m_localAddresses.append(QPair<QHostAddress, int>(address.ip(), address.prefixLength()));
			}
		}
	}
}


// Returns the address list for a specific host
QList<QPair<QString, int>> HostFinder::getHostAddressList(const QString& id)
{
	if (!m_hostList.contains(id))
		return {};
	return m_hostList[id]->getAddressList();
}


// Sets the preferred address for a specific host
void HostFinder::setHostPreferredAddress(const QString& id, const QString& address, int port)
{
	if (!m_hostList.contains(id))
		return;

	m_hostList[id]->setPreferredAddress(address, port);
	int hostRow = hostIndex(id);
	emit dataChanged(index(hostRow, COL_ADDRESS), index(hostRow, COL_ADDRESS), { Qt::DisplayRole });
}


void HostFinder::removeHostAddress(const QString& id, const QString& address, int port)
{
	if (!m_hostList.contains(id))
		return;

	m_hostList[id]->removeAddress(address, port);
	int hostRow = hostIndex(id);
	emit dataChanged(index(hostRow, COL_ADDRESS), index(hostRow, COL_ADDRESS), { Qt::DisplayRole });
}
