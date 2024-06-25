#pragma once

// Server global constants

#define MAX_LOG_FILESIZE			1048576		// Max size in bytes of log files8
#define MAX_LOG_FILES				50			// Max number of log files
#define HELPER_THROTTLE_COUNT		10			// Max number of times helper can crash in throttle period
#define HELPER_THROTTLE_PERIOD		300000		// Crash throttle period for helper
#define HELPER_RELAUNCH_DELAY		5000		// Delay before relaunching crashed helper
#define INTERVAL_AUTOSAVE			600000		// How often settings are automatically saved
#define INTERVAL_RESOURCECHECK		100000		// How often resources (disk/mem) is checked
#define INTERVAL_HELPERSTARTDELAY	1000		// Milliseconds after PinholeHelper starts to start launching apps, gives a chance for helper to connect
#define INTERVAL_GUITIMEOUT			30			// Number of seconds to wait for x11/Login
#define INTERVAL_APPHEARTBEAT		1000		// How often the application heartbeats itself to detect lockups
#define INTERVAL_APPTIMEOUT			30000		// App lockup timeout

#define ARG_RESETPASSWORD			"RESETPASSWORD"	// Command line argument to reset password

#define FILENAME_LOGFILE			"pinholelog.txt"	// The base name of the log file
#define FILENAME_ALERTLOG			"pinholealerts.txt"		// Log of alerts
#define FILENAME_KEYFILE			"host.key"	// The server encryption private key file name
#define FILENAME_CERTFILE			"host.pem"	// The serevr encryption public key file name

#define SUBDIR_APPOUTPUT			"appoutput/"	// Where app console output is stored

#define PROP_SERVER_SALT			"salt"
#define PROP_SERVER_HASH			"hash"

#define PROPERTY_ADDRESS			"clientAddress"

// Settings names and prefixes used when reading QSettings
#define SETTINGS_APPLIST			"appList"
#define SETTINGS_APPPREFIX			"app_"

#define SETTINGS_GROUPLIST			"groupList"
#define SETTINGS_GROUPPREFIX		"group_"

#define SETTINGS_EVENTLIST			"eventList"
#define SETTINGS_EVENTPREFIX		"event_"

#define SETTINGS_ALERTSLOTLIST		"alertSlotList"
#define SETTINGS_ALERTSLOTPREFIX	"alertSlot_"

