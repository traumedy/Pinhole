#pragma once

#define PINHOLE_CONSOLEAPPNAME	"Pinhole Console"
#define PINHOLE_CLIENTAPPNAME	"Pinhole Client"
#define PINHOLE_SERVERAPPNAME	"Pinhole Server"
#define PINHOLE_HELPERAPPNAME	"Pinhole Helper"
#define PINHOLE_BACKENDAPPNAME	"Pinhole Backend"
#define PINHOLE_SERVICENAME		"Pinhole"
#define PINHOLE_ORG_NAME		"Obscura Digital, LLC"
#define PINHOLE_ORG_DOMAIN		"obscuradigital.com"

#define SHAREDMEM_PASS			"PinholeBuff"
#define SHAREDPASS_LEN			64

#define IPV6_MULTICAST			"FF02:0:0:0:0:0:0:175"
#define HOST_UDPPORT			5457
#define HOST_TCPPORT			5457
#define HOST_QUERY_FREQ			2.0
#define PROXY_TCPPORT			5458

#define DEFAULT_APPLOOPBACKPORT	9999
#define DEFAULT_TERMINATE_TO	200
#define DEFAULT_HEARTBEAT_TO	5000
#define DEFAULT_CRASHPERIOD		60
#define DEFAULT_CRASHCOUNT		10
#define DEFAULT_HTTPPORT		8090
#define DEFAULT_NOVATCPPORT		2000
#define DEFAULT_NOVAUDPPORT		2002
#define DEFAULT_NOVATCPADDRESS	""
#define DEFAULT_SMTPPORT		587
#define DEFAULT_SMTPUSER		"no-reply@obscuradigital.com"

//#define MIN_LISTENINGPORT		2049
#define MIN_LISTENINGPORT		1
#define MAX_PORT				65535
#define MIN_HEARTBEATTIMEOUT	100
#define MAX_HEARTBEATTIMEOUT	99999
#define MAX_TERMINATETIMEOUT	120000
#define MIN_CRASHPERIOD			5
#define MAX_CRASHPERIOD			99999
#define MIN_CRASHCOUNT			2
#define MAX_CRASHCOUNT			99

#define TAG_COMMAND				"command"
#define TAG_ID					"ID"
#define TAG_NAME				"name"
#define TAG_ROLE				"role"
#define TAG_VERSION				"version"
#define TAG_PLATFORM			"platform"
#define TAG_OS					"os"
#define TAG_STATUS				"status"
#define TAG_MAC					"MAC"
#define TAG_ADDRESS				"address"
#define TAG_PORT				"port"

#define UDPCOMMAND_QUERY		"query"
#define UDPCOMMAND_ANNOUNCE		"announce"
#define UDPCOMMAND_REDIRECT		"redir"
#define UDPCOMMAND_STATUS		"status"

#define CMD_NOOP				"nop"
#define CMD_TERMINATE			"xxx"
#define CMD_AUTH				"aut"
#define CMD_SUBSCRIBECMD		"sub"
#define CMD_SUBSCRIBEGROUP		"grp"
#define CMD_QUERY				"qry"
#define CMD_VALUE				"val"
#define CMD_VALUESET			"set"
#define CMD_MISSING				"mis"
#define CMD_COMMAND				"cmd"
#define CMD_CMDUNKNOWN			"unk"
#define CMD_CMDRESPONSE			"rsp"
#define CMD_MESSAGE				"msg"
#define CMD_LOG					"log"
#define CMD_SCREENSHOT			"scr"
#define CMD_SHOWSCREENIDS		"ids"
#define CMD_CONTROLWINDOW		"win"

#define CMD_RESPONSE_SUCCESS	0
#define CMD_RESPONSE_ERROR		1
#define CMD_RESPONSE_UNKNOWN	2
#define CMD_RESPONSE_DATA		3

#define GROUP_NONE				""
#define GROUP_APP				"app"
#define GROUP_GROUP				"grp"
#define GROUP_GLOBAL			"glo"
#define GROUP_SCHEDULE			"sch"
#define GROUP_ALERT				"alr"

#define CMD_NONE_SETPASSWORD	"setPassword"
#define CMD_NONE_GETSCREENSHOT	"getScreenshot"
#define CMD_NONE_SHOWSCREENIDS	"showScreenIds"
#define CMD_NONE_IMPORTSETTINGS	"importSettings"
#define CMD_NONE_EXPORTSETTINGS	"exportSettings"
#define CMD_NONE_RETRIEVELOG	"getLog"
#define CMD_NONE_LOGMESSAGE		"logMessage"

#define CMD_APP_ADDAPP			"addApp"
#define CMD_APP_DELETEAPP		"delApp"
#define CMD_APP_RENAMEAPP		"renApp"
#define CMD_APP_STARTAPPS		"startApps"
#define CMD_APP_STARTSTARTUPAPPS	"startStartupApps"
#define CMD_APP_RESTARTAPPS		"restartApps"
#define CMD_APP_STOPAPPS		"stopApps"
#define CMD_APP_STOPALLAPPS		"stopAllApps"
#define CMD_APP_GETCONSOLE		"getConsole"
#define CMD_APP_EXECUTE			"execute"
#define CMD_APP_STARTVARS		"startAppVars"

#define PROP_APP_LIST			"appList"
#define PROP_APP_NAME			"appName"
#define PROP_APP_EXECUTABLE		"executable"
#define PROP_APP_ARGUMENTS		"arguments"
#define PROP_APP_DIRECTORY		"directory"
#define PROP_APP_LAUNCHATSTART	"launchAtStart"
#define PROP_APP_KEEPAPPRUNNING	"keepAppRunning"
#define PROP_APP_TERMINATEPREV	"terminatePrevApp"
#define PROP_APP_SOFTTERMINATE	"softTerminate"
#define PROP_APP_NOCRASHTHROTTLE	"noCrashThrottle"
#define PROP_APP_LOCKUPSCREENSHOT	"lockupScreenshot"
#define PROP_APP_CONSOLECAPTURE	"consoleCapture"
#define PROP_APP_APPENDCAPTURE	"appendCapture"
#define PROP_APP_LAUNCHDISPLAY	"launchDisplay"
#define PROP_APP_LAUNCHDELAY	"launchDelay"
#define PROP_APP_TCPLOOPBACK	"tcpLoopback"
#define PROP_APP_TCPLOOPBACKPORT	"tcpLoopbackPort"
#define PROP_APP_HEARTBEATS		"heartbeats"
#define PROP_APP_ENVIRONMENT	"environment"

#define PROP_APP_LASTSTARTED	"lastStarted"
#define PROP_APP_LASTEXITED		"lastExited"
#define PROP_APP_RESTARTS		"restarts"
#define PROP_APP_STATE			"state"
#define PROP_APP_RUNNING		"running"

#define CMD_GROUP_ADDGROUP		"addGroup"
#define CMD_GROUP_DELETEGROUP	"delGroup"
#define CMD_GROUP_RENAMEGROUP	"renGroup"
#define CMD_GROUP_STARTGROUP	"startGroup"
#define CMD_GROUP_STOPGROUP		"stopGroup"

#define PROP_GROUP_LIST			"groupList"
#define PROP_GROUP_NAME			"groupName"
#define PROP_GROUP_APPLICATIONS	"applications"
#define PROP_GROUP_LAUNCHATSTART	"launchAtStart"

#define CMD_GLOBAL_SHUTDOWN		"shutdown"
#define CMD_GLOBAL_REBOOT		"reboot"
#define CMD_GLBOAL_SYSINFO		"sysinfo"

#define PROP_GLOBAL_ROLE		"role"
#define PROP_GLOBAL_REMOTELOGLEVEL	"remoteLogLevel"
#define PROP_GLOBAL_HOSTLOGLEVEL	"hostLogLevel"
#define PROP_GLOBAL_TERMINATETIMEOUT	"terminateTimeout"
#define PROP_GLOBAL_HEARTBEATTIMEOUT	"heatbeatTimeout"
#define PROP_GLOBAL_CRASHPERIOD	"crashPeriod"
#define PROP_GLOBAL_CRASHCOUNT	"crashCount"
#define PROP_GLOBAL_TRAYCONTROL	"trayControl"
#define PROP_GLOBAL_TRAYLAUNCH	"trayLaunch"
#define PROP_GLOBAL_HTTPENABLED		"httpEnabled"
#define PROP_GLOBAL_HTTPPORT		"httpPort"
#define PROP_GLOBAL_BACKENDSERVER	"backendServer"
#define PROP_GLOBAL_NOVATCPENABLED	"novaTcpEnabled"
#define PROP_GLOBAL_NOVATCPADDRESS	"novaTcpAddress"
#define PROP_GLOBAL_NOVATCPPORT		"novaTcpPort"
#define PROP_GLOBAL_NOVAUDPENABLED	"novaUdpEnabled"
#define PROP_GLOBAL_NOVAUDPADDRESS	"novaUdpAddress"
#define PROP_GLOBAL_NOVAUDPPORT		"novaUdpPort"
#define PROP_GLOBAL_NOVASITE		"novaSite"
#define PROP_GLOBAL_NOVAAREA		"novaArea"
#define PROP_GLOBAL_NOVADISPLAY		"novaDisplay"
#define PROP_GLOBAL_ALERTMEMORY		"alertMemory"
#define PROP_GLOBAL_MINMEMORY		"minMemory"
#define PROP_GLOBAL_ALERTDISK		"alertDisk"
#define PROP_GLOBAL_MINDISK			"minDisk"
#define PROP_GLOBAL_ALERTDISKLIST	"alertDiskList"

#define CMD_SCHED_ADDEVENT		"addEvent"
#define CMD_SCHED_DELETEEVENT	"deleteEvent"
#define CMD_SCHED_RENAMEEVENT	"renameEvent"
#define CMD_SCHED_TRIGGEREVENTS	"triggerEvents"

#define PROP_SCHED_LIST			"schedList"
#define PROP_SCHED_NAME			"schedName"
#define PROP_SCHED_TYPE			"schedType"
#define PROP_SCHED_FREQUENCY	"frequency"
#define PROP_SCHED_ARGUMENTS	"schedArgs"
#define PROP_SCHED_OFFSET		"offset"
#define PROP_SCHED_LASTTRIGGERED	"lastTriggered"

#define SCHED_TYPE_STARTAPPS	"startApps"
#define SCHED_TYPE_STOPAPPS		"stopApps"
#define SCHED_TYPE_RESTARTAPPS	"restartApps"
#define SCHED_TYPE_STARTGROUP	"startGroup"
#define SCHED_TYPE_STOPGROUP	"stopGroup"
#define SCHED_TYPE_SHUTDOWN		"shutdown"
#define SCHED_TYPE_REBOOT		"reboot"
#define SCHED_TYPE_SCREENSHOT	"screenshot"
#define SCHED_TYPE_TRIGGEREVENTS	"triggerEvents"
#define SCHED_TYPE_ALERT		"alert"

#define SCHED_FREQ_DISABLED		"disabled"
#define SCHED_FREQ_WEEKLY		"weekly"
#define SCHED_FREQ_DAILY		"daily"
#define SCHED_FREQ_HOURLY		"hourly"
#define SCHED_FREQ_ONCE			"once"

#define PROP_ALERT_ALERTCOUNT	"alertCount"
#define PROP_ALERT_SMTPSERVER	"smptServer"
#define PROP_ALERT_SMTPPORT		"smptPort"
#define PROP_ALERT_SMTPSSL		"smtpSSL"
#define PROP_ALERT_SMTPTLS		"smtpTLS"
#define PROP_ALERT_SMTPUSER		"smptUser"
#define PROP_ALERT_SMTPPASS		"smptPass"
#define PROP_ALERT_SMTPEMAIL	"smptEmail"
#define PROP_ALERT_SMTPNAME		"smptName"
#define PROP_ALERT_SLOTLIST		"alertSlotList"
#define PROP_ALERT_SLOTNAME		"alertSlotName"
#define PROP_ALERT_SLOTENABLED	"alertSlotEnabled"
#define PROP_ALERT_SLOTTYPE		"alertSlotType"
#define PROP_ALERT_SLOTARG		"alertSlotArg"

#define CMD_ALERT_ADDSLOT		"addAlertSlot"
#define CMD_ALERT_DELETESLOT	"delAlertSlot"
#define CMD_ALERT_RENAMESLOT	"renAlertSlot"
#define CMD_ALERT_RESETCOUNT	"resetAlerts"
#define CMD_ALERT_RETRIEVELIST	"getAlerts"

#define ALERTSLOT_TYPE_SMPTEMAIL	"smtpEmail"
#define ALERTSLOT_TYPE_HTTPGET		"httpGet"
#define ALERTSLOT_TYPE_HTTPPOST		"httpPost"
#define ALERTSLOT_TYPE_SLACK		"slack"
#define ALERTSLOT_TYPE_EXTERNAL		"external"

#define ALERT_REPLACE_TEXT		"$ALERT$"

#define LOG_LEVEL_ERROR			"error"
#define LOG_LEVEL_WARNING		"warning"
#define LOG_LEVEL_NORMAL		"normal"
#define LOG_LEVEL_EXTRA			"extra"
#define LOG_LEVEL_DEBUG			"debug"

#define LOG_ALWAYS				6
#define LOG_ERROR				5
#define LOG_WARNING				4
#define LOG_NORMAL				3
#define LOG_EXTRA				2
#define LOG_DEBUG				1

#define DISPLAY_NORMAL			"normal"
#define DISPLAY_HIDDEN			"hidden"
#define DISPLAY_MINIMIZE		"minimize"
#define DISPLAY_MAXIMIZE		"maximize"

#define NOVA_CMD_SWITCH_APP		"switch_app"	// Switch between apps in a group
#define NOVA_CMD_START_APP		"start_app"		// Start an app, can be semicolon seperated list
#define NOVA_CMD_START_APP_VARS	"start_app_vars"	// Start an app with variables
#define NOVA_CMD_STOP_APP		"stop_app"		// Stop an app, can be semicolon seperated list
#define NOVA_CMD_RESTART_APP	"restart_app"	// Stop and then restart an app, can be semicolon seperated list
#define NOVA_CMD_STOP_GROUP		"stop_group"	// Stop an entire group
#define NOVA_CMD_START_GROUP	"start_group"	// Start an group, or one app in a group if they are set to 'exclusive'
#define NOVA_CMD_STOP_ALL		"stop_all"		// Stop all apps
#define NOVA_CMD_START_ALL		"start_all"		// Start all apps and groups
#define NOVA_CMD_RESTART		"restart"		// Restart the computer
#define NOVA_CMD_SHUTDOWN		"shutdown"		// Shutdown the computer
#define NOVA_CMD_LOGWARNING		"log"			// Log a warning message
/*
#define NOVA_CMD_SETVOLUME		"set_volume"	// Param is the volume level in percent or "mute" or "unmute"
#define NOVA_CMD_MONITORS		"monitors"		// Turns monitors on or off, param is "on" or "off"
*/

#define DATETIME_STRINGFORMAT	"yyyy-MM-dd HH:mm:ss"	// String format used for transfering DateTime values

#define APPENVVAR_APPNAME		"PINHOLEAPPNAME"	// The application name
#define APPENVVAR_ROLE			"PINHOLEROLE"		// The system role
#define APPENVVAR_APPLOGPIPE	"PINHOLELOGPIPE"	// The named pipe for logging and alerts and heartbeats, etc for the app
#define APPENVVAR_APPTCPPORT	"PINHOLEAPPPORT"	// If enabled, the TCP loopback port for the app
#define APPENVVAR_HTTPPORT		"PINHOLEHTTPPORT"	// If enabled, the TCP port for the Pinhole HTTP server

// Prefixes for log messages sent to the applicaiton logging named pipe
#define LOGPREFIX_ALERT			"ALERT "			// Trigger an alert
#define LOGPREFIX_TRIGGER		"TRIGGER "			// Trigger schedule events
#define LOGPREFIX_HEARTBEAT		"HEARTBEAT"			// Heartbeat the current application
#define LOGPREFIX_LOGERROR		"ERROR "			// Log level error
#define LOGPREFIX_LOGWARNING	"WARNING "			// Log level warning
#define LOGPREFIX_LOGEXTRA		"EXTRA "			// Log level extra
#define LOGPREFIX_LOGDEBUG		"DEBUG "			// Log level debug

// JSON tags in import/export files
#define JSONTAG_GLOBALSETTINGS		"GlobalSettings"
#define JSONTAG_APPSETTINGS			"ApplicationSettings"
#define JSONTAG_GROUPSETTINGS		"GroupSettings"
#define JSONTAG_SCHEDETTINGS		"ScheduleSettings"
#define JSONTAG_ALERTSETTINGS		"AlertSettings"
#define JSONTAG_APPLICATIONS		"Applications"
#define JSONTAG_GROUPS				"Groups"
#define JSONTAG_EVENTS				"Events"
#define JSONTAG_ALERTSLOTS			"AlertSlots"
#define STATUSTAG_NODELETE			"nodelete"		// Set in a status json group to prevent items from being deleted so they are only updated or added
#define STATUSTAG_DELETEENTRIES		"deleteentires"	// Set in a status json group to delete the group items listed, only the name tag is required


// Exit values triggered from various places
enum {
	EXIT_REMOTEDEBUG = 1,
	EXIT_SIGNAL,
	EXIT_CONSOLE,
	EXIT_WMCLOSE,
	EXIT_WMENDSESSION,
	EXIT_SERVICE,
	EXIT_LOCKUP,
	EXIT_CRASH
};
