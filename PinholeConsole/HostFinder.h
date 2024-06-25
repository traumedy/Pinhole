#pragma once

#include <QAbstractTableModel>
#include <QMap>
#include <QThread>
#include <QTimer>

class QNetworkConfigurationManager;
class QNetworkInterface;
class QUdpSocket;
class QHostAddress;
class HostItem;

class HostFinder : public QAbstractTableModel
{
	Q_OBJECT

public:
	HostFinder(QObject* parent = nullptr);
	~HostFinder();
	void clear();
	bool eraseEntry(const QString& id);
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation,
		int role = Qt::DisplayRole) const override;
	void queryHostAddress(const QHostAddress& addr) const;
	bool loadHostList();
	bool saveHostList() const;
	bool importHostList(const QJsonObject& root);
	QJsonObject exportHostList() const;
	void enableScanning(bool enable);
	QList<QPair<QString, int>> getHostAddressList(const QString& id);
	void setHostPreferredAddress(const QString& id, const QString& address, int port);
	void removeHostAddress(const QString& id, const QString& address, int port);

public slots:
	void readyRead();

private slots:
	void expireCheck();
	void broadcastHostQuery();
	void queryLoopback();
	void queryHostList();
	void findLocalAddresses();

private:
	QStringList m_colDescriptions = 
	{
		tr("Network address"),
		tr("Host name"),
		tr("Assigned role"),
		tr("Pinhole version"),
		tr("OS Platform"),
		tr("Machine status"),
		tr("Last time heard from"),
		tr("Operating system"),
		tr("Hardware MAC address")
	};

	int hostIndex(const QString& id) const;
	bool isAddressBroadcastable(const QHostAddress& addr) const;
	bool isLocalAddress(const QHostAddress& addr) const;
	bool isAutoConfiguredAddress(const QHostAddress& addr) const;

	QUdpSocket* m_sock;
	QTimer m_broadcastTimer;
	QByteArray m_queryPacket;
	QNetworkConfigurationManager* m_networkConfigurationManager = nullptr;
	QBrush* m_hashBrush;
	QMap<QString, QSharedPointer<HostItem>> m_hostList;
	// List of Addresses, prefix length
	QList<QPair<QHostAddress, int>> m_localAddresses;
	QList<QNetworkInterface> m_interfaces;
};

