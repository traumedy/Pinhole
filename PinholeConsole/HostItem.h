#pragma once

#include <QDateTime>

class HostItem
{
public:

	HostItem() {}
	HostItem(const QString& name, const QString& role, const QString& version,
		const QString& platform, const QString& status, const QString& os,
		const QString& mac, QDateTime lastHeard = QDateTime::currentDateTime());
	QPair<int, int> update(const QString& name, const QString& role, const QString& version,
		const QString& platform, const QString& status, const QString& os,
		const QString& mac);
	QString getAddress() const { if (m_addressList.isEmpty()) return ""; return m_addressList[0].first; }
	int getPort() const { if (m_addressList.isEmpty()) return 0; return m_addressList[0].second; }
	QString getName() const { return m_name; }
	QString getRole() const { return m_role; }
	QString getVersion() const { return m_version; }
	QString getPlatform() const { return m_platform; }
	void setStatus(const QString& status) { m_status = status; }
	QString getStatus() const { return m_status; }
	QDateTime getLastHeard() const { return m_lastHeard; }
	QString getOs() const { return m_os; }
	void setMAC(const QString& MAC) { m_mac = MAC; }
	QString getMAC() const { return m_mac; }
	void setHostAddress(const QString& hostAddress) { m_hostAddress = hostAddress; }
	QString getHostAddress() const { return m_hostAddress; }
	bool getNeedExpired() const { return m_needExpiredNotification; }
	void setNeedExpired(bool b) { m_needExpiredNotification = b; }
	void addAddress(const QPair<QString, int>& address, bool IPv4);
	void setPreferredAddress(const QString& address, int port);
	void removeAddress(const QString& address, int port);
	QList<QPair<QString, int>> getAddressList() const { return m_addressList; }
	int getAddressCount() const { return m_addressList.size(); }

private:
	QString m_name;
	QString m_role;
	QString m_version;
	QString m_platform;
	QString m_status;
	QString m_os;
	QString m_mac;
	QDateTime m_lastHeard;
	QString m_hostAddress;
	QList<QPair<QString, int>> m_addressList;
	bool m_needExpiredNotification = true;
};

