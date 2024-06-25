#pragma once

#include "../common/PinholeCommon.h"

#include <QObject>
#include <QMetaType>
#include <QMap>

class HostClient;

class PinholeClient : public QObject
{
	Q_OBJECT

public:
	PinholeClient(QObject *parent);
	~PinholeClient();

	void setSilent(bool set) { m_silent = set; }
	void setShowId(bool set) { m_showId = set; }

	void listCommands();
	void listProperties();
	bool isCommandValid(const QString& command);
	bool executeCommand(const QString& address, int port, const QString& command, const QString& argument);
	bool getProperty(const QString& address, int port, const QString& item, const QString& property);
	bool setProperty(const QString& address, int port, const QString& item, const QString& property, const QString& value);
	bool logMessage(const QString& address, int port, const QString& message);

private:
	bool None_SetPassword(const QString & address, int port, const QString & argument);
	bool None_GetScreenshot(const QString & address, int port, const QString & argument);
	bool None_ShowScreenIds(const QString & address, int port, const QString & argument);
	bool None_ImportSettings(const QString & address, int port, const QString & argument);
	bool None_ExportSettings(const QString & address, int port, const QString & argument);
	bool App_AddApp(const QString & address, int port, const QString & argument);
	bool App_DeleteApp(const QString & address, int port, const QString & argument);
	bool App_StartApps(const QString & address, int port, const QString & argument);
	bool App_StartStartupApps(const QString & address, int port, const QString & argument);
	bool App_RestartApps(const QString & address, int port, const QString & argument);
	bool App_StopApps(const QString & address, int port, const QString & argument);
	bool App_StopAllApps(const QString & address, int port, const QString & argument);
	bool App_StartAppVariables(const QString & address, int port, const QString & argument);
	bool Group_AddGroup(const QString & address, int port, const QString & argument);
	bool Group_DeleteGroup(const QString & address, int port, const QString & argument);
	bool Group_StartGroup(const QString & address, int port, const QString & argument);
	bool Group_StopGroup(const QString & address, int port, const QString & argument);
	bool Global_Shutdown(const QString & address, int port, const QString & argument);
	bool Gobal_Reboot(const QString & address, int port, const QString & argument);
	bool Global_SysInfo(const QString & address, int port, const QString & argument);
	bool Schedule_AddEvent(const QString & address, int port, const QString & argument);
	bool Schedule_DeleteEvent(const QString & address, int port, const QString & argument);
	bool Schedule_TriggerEvents(const QString & address, int port, const QString & argument);
	bool Alert_AddSlot(const QString & address, int port, const QString & argument);
	bool Alert_DeleteSlot(const QString & address, int port, const QString & argument);
	bool Alert_ResetCount(const QString & address, int port, const QString & argument);

	bool ExecServerCommand(const QString & hostAddress, int port, const QString & commandDescription,
		std::function<void(HostClient*)> func, const QString & outputFile = QString() );

	std::map<QString, std::pair<std::function<bool(PinholeClient*, const QString& address, int port, const QString& argument)>, QString>> m_commandMap =
	{
		{ CMD_NONE_SETPASSWORD, { &PinholeClient::None_SetPassword, tr("Password string") } },
		{ CMD_NONE_GETSCREENSHOT, { &PinholeClient::None_GetScreenshot, tr("Local filename to save PNG to") } },
		{ CMD_NONE_SHOWSCREENIDS, { &PinholeClient::None_ShowScreenIds, "" } },
		{ CMD_NONE_IMPORTSETTINGS, { &PinholeClient::None_ImportSettings, tr("Local filename to read settings JSON from") } },
		{ CMD_NONE_EXPORTSETTINGS, { &PinholeClient::None_ExportSettings, tr("Local filename to write settings JSON to") } },
		{ CMD_APP_ADDAPP, { &PinholeClient::App_AddApp, tr("Name of new application") } },
		{ CMD_APP_DELETEAPP, { &PinholeClient::App_DeleteApp, tr("Name of application to delete") } },
		{ CMD_APP_STARTAPPS, { &PinholeClient::App_StartApps, tr("List of application names to start") } },
		{ CMD_APP_STARTSTARTUPAPPS, { &PinholeClient::App_StartStartupApps, "" } },
		{ CMD_APP_RESTARTAPPS, { &PinholeClient::App_RestartApps, tr("List of application names to restart") } },
		{ CMD_APP_STOPAPPS, { &PinholeClient::App_StopApps, tr("List of application names to stop") } },
		{ CMD_APP_STOPALLAPPS, { &PinholeClient::App_StopAllApps, "" } },
		{ CMD_APP_STARTVARS, { &PinholeClient::App_StartAppVariables, "APPNAME:Name=Value list" } },
		{ CMD_GROUP_ADDGROUP, { &PinholeClient::Group_AddGroup, tr("Name of new group") } },
		{ CMD_GROUP_DELETEGROUP, { &PinholeClient::Group_DeleteGroup, tr("Name of group to delete") } },
		{ CMD_GROUP_STARTGROUP, { &PinholeClient::Group_StartGroup, tr("Name of group to start") } },
		{ CMD_GROUP_STOPGROUP, { &PinholeClient::Group_StopGroup, tr("Name of group to stop") } },
		{ CMD_GLOBAL_SHUTDOWN, { &PinholeClient::Global_Shutdown, "" } },
		{ CMD_GLOBAL_REBOOT, { &PinholeClient::Gobal_Reboot, "" } },
		{ CMD_GLBOAL_SYSINFO, { &PinholeClient::Global_SysInfo, "" } },
		{ CMD_SCHED_ADDEVENT, { &PinholeClient::Schedule_AddEvent, tr("Name of new scheduled event") } },
		{ CMD_SCHED_DELETEEVENT, { &PinholeClient::Schedule_DeleteEvent, tr("Name of scheduled event to delete") } },
		{ CMD_SCHED_TRIGGEREVENTS, { &PinholeClient::Schedule_TriggerEvents, tr("List of scheduled event names to trigger") } },
		{ CMD_ALERT_ADDSLOT, { &PinholeClient::Alert_AddSlot, tr("Name of new alert slot") } },
		{ CMD_ALERT_DELETESLOT, { &PinholeClient::Alert_DeleteSlot, tr("Name of alert slot to delete") } },
		{ CMD_ALERT_RESETCOUNT, { &PinholeClient::Alert_ResetCount, "" } },
	};

	const QMap<QString, int> m_propListGlobal =
	{
		{ PROP_GLOBAL_ROLE, QMetaType::QString },
		{ PROP_GLOBAL_REMOTELOGLEVEL, QMetaType::QString },
		{ PROP_GLOBAL_HOSTLOGLEVEL, QMetaType::QString },
		{ PROP_GLOBAL_TERMINATETIMEOUT, QMetaType::Int },
		{ PROP_GLOBAL_HEARTBEATTIMEOUT, QMetaType::Int },
		{ PROP_GLOBAL_CRASHPERIOD, QMetaType::Int },
		{ PROP_GLOBAL_CRASHCOUNT, QMetaType::Int },
		{ PROP_GLOBAL_TRAYLAUNCH, QMetaType::Bool },
		{ PROP_GLOBAL_TRAYCONTROL, QMetaType::Bool },
		{ PROP_GLOBAL_HTTPENABLED, QMetaType::Bool },
		{ PROP_GLOBAL_HTTPPORT, QMetaType::Int },
		{ PROP_GLOBAL_BACKENDSERVER, QMetaType::QString },
		{ PROP_GLOBAL_NOVATCPENABLED, QMetaType::Bool },
		{ PROP_GLOBAL_NOVATCPADDRESS, QMetaType::QString },
		{ PROP_GLOBAL_NOVATCPPORT, QMetaType::Int },
		{ PROP_GLOBAL_NOVAUDPENABLED, QMetaType::Bool },
		{ PROP_GLOBAL_NOVAUDPADDRESS, QMetaType::QString },
		{ PROP_GLOBAL_NOVAUDPPORT, QMetaType::Int },
		{ PROP_GLOBAL_NOVASITE, QMetaType::QString },
		{ PROP_GLOBAL_NOVAAREA , QMetaType::QString },
		{ PROP_GLOBAL_NOVADISPLAY, QMetaType::QString },
		{ PROP_GLOBAL_ALERTMEMORY, QMetaType::Bool },
		{ PROP_GLOBAL_MINMEMORY, QMetaType::Int },
		{ PROP_GLOBAL_ALERTDISK, QMetaType::Bool },
		{ PROP_GLOBAL_MINDISK, QMetaType::Int },
		{ PROP_GLOBAL_ALERTDISKLIST, QMetaType::QStringList }
	};

	const QMap<QString, int> m_propListApplication =
	{
		{ PROP_APP_LIST, QMetaType::Void },
		//{ PROP_APP_NAME, QMetaType::Void },
		{ PROP_APP_EXECUTABLE, QMetaType::QString },
		{ PROP_APP_ARGUMENTS, QMetaType::QString },
		{ PROP_APP_DIRECTORY, QMetaType::QString },
		{ PROP_APP_LAUNCHATSTART, QMetaType::Bool },
		{ PROP_APP_KEEPAPPRUNNING, QMetaType::Bool },
		{ PROP_APP_TERMINATEPREV, QMetaType::Bool },
		{ PROP_APP_SOFTTERMINATE,  QMetaType::Bool },
		{ PROP_APP_NOCRASHTHROTTLE, QMetaType::Bool },
		{ PROP_APP_LOCKUPSCREENSHOT, QMetaType::Bool },
		{ PROP_APP_LAUNCHDISPLAY, QMetaType::QString },
		{ PROP_APP_LAUNCHDELAY, QMetaType::Int },
		{ PROP_APP_TCPLOOPBACK, QMetaType::Bool },
		{ PROP_APP_TCPLOOPBACKPORT, QMetaType::Int },
		{ PROP_APP_HEARTBEATS, QMetaType::Bool },
		{ PROP_APP_ENVIRONMENT, QMetaType::QStringList },
		{ PROP_APP_LASTSTARTED, QMetaType::Void },
		{ PROP_APP_LASTEXITED, QMetaType::Void },
		{ PROP_APP_RESTARTS, QMetaType::Void },
		{ PROP_APP_STATE, QMetaType::Void },
		{ PROP_APP_RUNNING, QMetaType::Void }
	};

	const QMap<QString, int> m_propListGroup =
	{
		{ PROP_GROUP_LIST, QMetaType::Void },
		//{ PROP_GROUP_NAME, QMetaType::Void },
		{ PROP_GROUP_APPLICATIONS, QMetaType::QStringList },
		{ PROP_GROUP_LAUNCHATSTART, QMetaType::Bool }
	};

	const QMap<QString, int> m_propListSchedule =
	{
		{ PROP_SCHED_LIST, QMetaType::Void },
		//{ PROP_SCHED_NAME, QMetaType::Void },
		{ PROP_SCHED_TYPE, QMetaType::QString },
		{ PROP_SCHED_FREQUENCY, QMetaType::QString },
		{ PROP_SCHED_ARGUMENTS, QMetaType::QString },
		{ PROP_SCHED_OFFSET, QMetaType::Int },
		{ PROP_SCHED_LASTTRIGGERED, QMetaType::Void }
	};

	const QMap<QString, int> m_propListAlert = 
	{
		{ PROP_ALERT_SMTPSERVER, QMetaType::QString },
		{ PROP_ALERT_SMTPPORT, QMetaType::Int },
		{ PROP_ALERT_SMTPUSER, QMetaType::QString },
		{ PROP_ALERT_SMTPPASS, QMetaType::QString },
		{ PROP_ALERT_SMTPEMAIL, QMetaType::QString },
		{ PROP_ALERT_SMTPNAME, QMetaType::QString },
		{ PROP_ALERT_SLOTLIST, QMetaType::Void },
		//{ PROP_ALERT_SLOTNAME, QMetaType::Void },
		{ PROP_ALERT_SLOTENABLED, QMetaType::Bool },
		{ PROP_ALERT_SLOTTYPE, QMetaType::QString },
		{ PROP_ALERT_SLOTARG, QMetaType::QString }
	};

	bool m_silent = false;
	bool m_showId = false;
};
