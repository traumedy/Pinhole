#pragma once

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <QSharedMemory>

class Settings;
class AppManager;
class AlertManager;
class GroupManager;
class GlobalManager;
class ScheduleManager;

class CommandInterface : public QObject
{
	Q_OBJECT

	class ClientInfo
	{
	public:
		QObject* host = nullptr;			// Pointer to interface hosting this client
		QString clientId;					// Network interface specific ID
		bool authenticated = false;			// Client has logged in
		QString version;					// Client version
		QString hostName;					// Host name declared by client
		QStringList subscribedCommands;		// Commands the client will receive
		QStringList subscribedGroups;		// Groups the client will receive values for
		bool helperClient = false;			// Client declares self as helper
		bool specialClient = false;			// Client is using local password
		bool waitingForCommand = false;		// Client is waiting for a response (ie screenshot)
		QString waitingCommandGroup;		// The group of the command the client is waiting on
		QString waitingSubCommand;			// The sub command the client is waiting on
	};

	class VariantParser
	{
	public:
		VariantParser(const QString& command, const QString& client, int firstArg, const QVariantList& vlist)
			: m_command(command), m_client(client), m_argPos(firstArg), m_vlist(vlist)
		{
		};
		bool arg(bool& a);
		bool arg(int& a);
		bool arg(QString& a);
		bool arg(QStringList& a);
		bool arg(QByteArray& a);
		bool arg(QVariant& a);
		QString errorString() const { return m_errorString; }

	private:
		bool checkSize();

		QString m_command;
		QString m_client;
		int m_argPos;
		const QVariantList& m_vlist;
		bool m_error = false;
		QString m_errorString;
	};


public:
	CommandInterface(Settings* settings, AlertManager* alertManager,
		AppManager* appManager, GroupManager* groupManager, GlobalManager* globalManager,
		ScheduleManager* scheduleManager, QObject *parent = nullptr);
	~CommandInterface();


public slots:
	void addClient(const QString& clientId);
	void removeClient(const QString& clientId);
	void processData(const QString& clientId, const QByteArray& data, QByteArray& response, bool& disconnect);
	void logMessage(int level, const QString& message);
	void valueChanged(const QString&, const QString&, const QString&, const QVariant&) const;
	void sendControlWindow(int pid, const QString & display, const QString & command);
	void logMessage(int level, const QString & message) const;
	bool writeSettings() const;

private slots:
	void sendToAllClients(const QVariantList& vlist) const;

private:
	bool readServerSettings();
	bool createSharedPassword();
	bool helperConnected() const;
	bool sendToHelper(const QVariantList & vlist) const;
	bool sendCmdResponseToWaitingClients(const QVariantList & vlist) const;
	
	QVariantList handleClientQuery(const QVariantList & vlist, const QString & clientId) const;
	QVariantList handleClientValue(const QVariantList & vlist, const QString & clientId) const;
	QVariantList handleClientCommand(const QVariantList & vlist, const QString & clientId, QSharedPointer<ClientInfo> client);
	QByteArray generateSettingsData() const;
	bool importSettingsData(const QByteArray & data, const QString & clientAddr, const QString & clientHostName) const;
	void sendDataToClient(const QSharedPointer<ClientInfo> client, const QByteArray& data) const;
	void sendDataToClient(const QString& clientId, const QByteArray& data) const;
	QByteArray variantListData(const QVariantList& vlist) const;


	QString m_passwordSalt;
	QByteArray m_passwordHash;
	QMap<QString, QSharedPointer<ClientInfo>> m_clientMap;
	QString m_localPassword;
	QSharedMemory m_sharedMemoryPassword;
	Settings* m_settings = nullptr;
	AlertManager* m_alertManager = nullptr;
	AppManager* m_appManager = nullptr;
	GroupManager* m_groupManager = nullptr;
	GlobalManager* m_globalManager = nullptr;
	ScheduleManager* m_scheduleManager = nullptr;
};


