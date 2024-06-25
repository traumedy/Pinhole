#pragma once

#include "../common/PinholeCommon.h"

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <QSharedPointer>

class QSslError;
class QSslCertificate;
class QSslKey;
class QSslSocket;

class HostClient : public QObject
{
	Q_OBJECT

public:
	HostClient(const QString& address, int port, const QString& hostId, bool specialLoopback, bool helperClient, QObject *parent);
	~HostClient();

	bool isConnected() const;
	QString getHostAddress() const;
	QString getHostName() const;
	QString getHostVersion() const;
	QString getHostId() const;
	void sendVariantList(const QVariantList& vlist) const;
	void startApps(const QStringList& names) const;
	void startStartupApps() const;
	void restartApps(const QStringList& names) const;
	void stopApps(const QStringList& names) const;
	void stopAllApps() const;
	void startAppVariables(const QString& appName, const QStringList& variables) const;
	void addApplication(const QString& name) const;
	void deleteApplication(const QString& name) const;
	void renameApplication(const QString& name, const QString& newName) const;
	void getApplicationConsoleOutput(const QString& name) const;
	void startGroup(const QString& name) const;
	void stopGroup(const QString& name) const;
	void addGroup(const QString& name) const;
	void deleteGroup(const QString& name) const;
	void renameGroup(const QString& oldName, const QString& newName) const;
	void addScheduleEvent(const QString& name) const;
	void deleteScheduleEvent(const QString& name) const;
	void renameScheduleEvent(const QString& oldName, const QString& newName) const;
	void triggerScheduleEvents(const QStringList& names) const;
	void getVariant(const QString& group, const QString& name, const QString& propName) const;
	void setVariant(const QString& group, const QString& name, const QString& propName, const QVariant& value) const;
	void subscribeToCommand(const QString& command) const;
	void subscribeToGroup(const QString& group) const;
	void setPassword(const QString& password) const;
	void shutdownHost() const;
	void rebootHost() const;
	void logMessage(const QString& message, int level = LOG_NORMAL) const;
	void requestScreenshot() const;
	void sendScreenshot(const QByteArray& screenshot) const;
	void requestTerminate() const;
	void requestExportSettings() const;
	void sendImportData(const QByteArray& importData) const;
	void showScreenIds() const;
	void retrieveLog(const QString& startDate, const QString& endDate) const;
	void retrieveSystemInfo() const;
	void retrieveAlertList() const;
	void addAlertSlot(const QString& name) const;
	void deleteAlertSlot(const QString& name) const;
	void renameAlertSlot(const QString& oldName, const QString& newName) const;
	void resetAlertCount() const;
	void executeCommand(const QByteArray& executable, const QString& command, const QString& arguments, const QString& directory, bool capture, bool elevated);

	static void addAddressPassword(const QString& address, const QString& password);

public slots:
	void sslErrors(const QList<QSslError> &errors);
	void socketEncrypted();
	void rx(void);
	void serverDisconnect(void);
	void serverConnect(void);

signals:
	void disconnected(void);
	void connected(void);
	void connectFailed(const QString& reason);
	void passwordFailure();
	void valueUpdate(const QString&, const QString&, const QString&, const QVariant&);
	void valueSet(const QString& group, const QString& item, const QString& property, bool success);
	void missingValue(const QString&, const QString&, const QString&);
	void hostMessage(const QString& hostAddr, const QString& text, const QString& informativeText);
	void hostLog(int level, const QString& message);
	void commandUnknown(const QString&);
	void commandSuccess(const QString&, const QString&);
	void commandError(const QString&, const QString&);
	void commandMissing(const QString&, const QString&);
	void commandData(const QString&, const QString&, const QVariant&);
	void commandScreenshot();
	void commandShowScreenIds();
	void commandControlWindow(int pid, const QString& display, const QString& command);
	void terminationRequested();

private:
	void connectToServer() const;
	bool getSharedPassword(QString& passwordData) const;
	QVariantList makeAuthPacket(const QString& password);

	static QString s_lastPassword;
	static QMap<QString, QString> s_passwordMap;
	static QSharedPointer<QSslCertificate> s_cert;
	static QSharedPointer<QSslKey> s_key;

	QString m_hostAddress;
	int m_hostPort;
	QString m_hostId;
	bool m_specialLoopback = false;
	bool m_helperClient = false;
	QByteArray data;
	uint32_t dataLeft = 0;
	QString m_hostName;
	QString m_hostVersion;
	QSslSocket* m_socket = nullptr;
	QTimer m_reconnectTimer;
	QTimer m_pingTimer;
	bool m_reconnect = true;
	bool m_closing = false;
};
