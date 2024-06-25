#include "HTTPServer.h"
#include "Settings.h"
#include "AppManager.h"
#include "GroupManager.h"
#include "GlobalManager.h"
#include "ScheduleManager.h"
#include "Logger.h"
#include "../common/Utilities.h"
#include "../common/HostClient.h"
#include "../common/Version.h"

#include <QUrl>
#include <QTcpServer>
#include <QTcpSocket>
#include <QEventLoop>
#include <QHostInfo>


HTTPServer::HTTPServer(Settings* settings, AppManager* appManager, GroupManager* groupManager,
	GlobalManager* globalManager, ScheduleManager* scheduleManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_appManager(appManager), m_groupManager(groupManager),
	m_globalManager(globalManager), m_scheduleManager(scheduleManager)
{
	connect(m_globalManager, &GlobalManager::valueChanged,
		this, &HTTPServer::globalValueChanged);

	m_tcpEnabled = m_globalManager->getHttpEnabled();
	m_tcpPort = m_globalManager->getHttpPort();
}


HTTPServer::~HTTPServer()
{
}


void HTTPServer::start()
{
	if (m_tcpEnabled)
	{
		setupTcp();
	}
}


void HTTPServer::globalValueChanged(const QString& groupName, const QString& itemName, const QString& propName, const QVariant& value)
{
	Q_UNUSED(itemName);

	if (GROUP_GLOBAL == groupName)
	{
		if (PROP_GLOBAL_HTTPENABLED == propName)
		{
			m_tcpEnabled = value.toBool();

			if (m_tcpEnabled)
			{
				setupTcp();
			}
			else
			{
				Logger(LOG_EXTRA) << tr("Stopping HTTP server");
				m_tcpServer.clear();
			}
		}
		else if (PROP_GLOBAL_HTTPPORT == propName)
		{
			m_tcpPort = value.toInt();

			if (m_tcpEnabled)
			{
				setupTcp();
			}
		}
	}
}


void HTTPServer::setupTcp()
{
	m_tcpServer.clear();

	m_tcpServer = QSharedPointer<QTcpServer>::create(this);

	if (!m_tcpServer->listen(QHostAddress::Any, m_tcpPort))
	{
		Logger(LOG_WARNING) << tr("HTTP server failed to bind to TCP port ") << m_tcpPort;
	}
	else
	{
		Logger(LOG_EXTRA) << tr("HTTP server listening on TCP port ") << m_tcpPort;

		connect(m_tcpServer.data(), &QTcpServer::newConnection,
			this, &HTTPServer::newConnection);
	}
}


void HTTPServer::newConnection()
{
	QTcpSocket* socket = m_tcpServer->nextPendingConnection();
	Logger(LOG_DEBUG) << tr("HTTP connection received: ") << socket->peerAddress().toString();

	connect(socket, &QTcpSocket::readyRead,
		this, &HTTPServer::rx);
	connect(socket, &QTcpSocket::disconnected,
		this, &HTTPServer::clientDisconnected);
}


void HTTPServer::clientDisconnected()
{
	QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
	if (nullptr != socket)
	{
		Logger(LOG_DEBUG) << tr("HTTP client disconnected: ") << socket->peerAddress().toString();
		socket->deleteLater();
	}
}


QString HTTPServer::parseHTTPRequest(QString req, QString& auth)
{
	// We only care about the first line
	QTextStream reqStream(&req);
	QString firstline = reqStream.readLine();
	if (firstline.size() < 13)
	{
		Logger(LOG_WARNING) << tr("Short HTTP request");
		return "";
	}
	else if (!firstline.startsWith("GET "))
	{
		Logger(LOG_WARNING) << tr("HTTP request missing GET");
		return "";
	}

	int httpPos = firstline.indexOf(" HTTP/1.1");
	if (-1 == httpPos)
	{
		Logger(LOG_WARNING) << tr("HTTP request missing HTTP/1.1");
		return "";
	}

	// Read the rest of the headers looking for the authentication
	QString line;
	while (reqStream.readLineInto(&line))
	{
		if (line.startsWith("Authorization: Basic "))
		{
			auth = QString(QByteArray::fromBase64(line.mid(21).toUtf8()));
			break;
		}
	}

	// The path is everything between the GET and the HTTP
	return firstline.mid(4, httpPos - 4);
}


// Percent encodes a string
QString HTTPServer::escapeString(const QString& str)
{
	return QUrl::toPercentEncoding(str);
}


// generates a response with headers, returns the response
QByteArray HTTPServer::generateResponse(unsigned int errorLevel, const QString& errorText, const QByteArray& data, const QString& contentType)
{
	QByteArray response;
	response += "HTTP/1.1 " + QString::number(errorLevel) + " " + errorText + "\r\n";
	response += "Server: Pinhole Server\r\n"\
		"WWW-Authenticate: Basic realm=\"Pinhole Server\"\r\n"\
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"\
		"Access-Control-Allow-Origin: *\r\n"\
		"Pragma: no-cache\r\n"\
		"Expires: 0\r\n";
	response += "Content-Length: " + QString::number(data.length()) + "\r\n";
	response += "Content-Type: ";
	if (contentType.isEmpty())
		response += "text/html; charset=UTF-8";
	else
		response += contentType;
	response += "\r\n\r\n";
	response += data;
	return response;
}


QByteArray HTTPServer::generateHeaderData(const QString& title)
{
	return ("<!DOCTYPE html><html><head><title>" + title + "</title>"\
		"<style>"\
		"table, th, td{ border: 1px solid black; }"\
		"tr:nth-child(even) {background-color: #dddddd;}"\
		"ul#menu li{display: inline; float: none; padding: 5px 0px 5px 20px;}"\
		"ul#menu li.first-item, ul#menu li.last-item{padding-left: 0px;}"\
		"</style>"\
		"<center>Pinhole Server v" PINHOLE_VERSION " on " + QHostInfo::localHostName() +
		"<ul id='menu' class='cols'>"\
		"<li><a href=\"/\">Applications</a></li>"\
		"<li><a href=\"/groups\">Groups</a></li>"\
		"<li><a href=\"/schedule\">Schedule</a></li>"\
		"<li><a href=\"/sysinfo\">System info</a></li>"\
		"<li><a href='/viewlog?0'>View log</a></li>"\
		"<li><a href='/screenshot.png' target=\"_blank\">Screenshot</a></li>"\
		"</ul>"\
		"<ul id='menu' class='cols'>"\
		"<li><button onclick=\"showIDs()\">Show Screen IDs</button></li>"
		"<li><button onclick=\"rebootShutdown('reboot')\">Reboot</button></li>"
		"<li><button onclick=\"rebootShutdown('shutdown')\">Shutdown</button></li>"
		"</ul>"\
		"</center>"\
		"<script>"\
		"function rebootShutdown(cmd) {"\
			"var r = confirm(\"Do you want to \" + cmd + \" this computer?\");"\
			"if (r == true) {"\
				"document.getElementById(\"status\").innerHTML = \"Initiating \" + cmd;"\
				"document.getElementById(\"exec\").src=\"/\" + cmd + \"/\";"\
			"}"\
		"}"\
		"function showIDs() {"\
			"document.getElementById(\"status\").innerHTML = \"Screen IDs shown\";"\
			"document.getElementById(\"exec\").src=\"/showids\";"\
		"}"\
		"</script>"\
		"</head><body>"\
		"<iframe id=\"exec\" src=\"\" style=\"width:0;height:0;border:0;border:none;\"></iframe>"\
		"<center><p id=\"status\"></p></center>"\
		).toUtf8();
}


QByteArray HTTPServer::generateRootData()
{
	QByteArray data = generateHeaderData("Pinhole applications");
	data += generateApplicationTable();
	data += "<script>"\
		"function reloadPage() {"\
		"location.reload();"\
		"}"\
		"function appStop(app) {"\
		"document.getElementById(\"status\").innerHTML = \"Stopping application \" + app;"\
		"document.getElementById(\"exec\").src=\"/appstop?\" + app;"\
		"setTimeout(reloadPage, 500);"
		"}"\
		"function appStart(app) {"\
		"document.getElementById(\"status\").innerHTML = \"Starting application \" + app;"\
		"document.getElementById(\"exec\").src=\"/appstart?\" + app;"\
		"setTimeout(reloadPage, 500);"
		"}"\
		"</script></body></html>";
	return data;
}


QByteArray HTTPServer::generateGroupsData()
{
	QByteArray data = generateHeaderData("Pinhole groups");
	data += generateGroupsTable();
	data += "<script>"\
		"function reloadPage() {"\
		"location.reload();"\
		"}"\
		"function groupStop(group) {"\
		"document.getElementById(\"status\").innerHTML = \"Stopping group \" + group;"\
		"document.getElementById(\"exec\").src=\"/groupstop?\" + group;"\
		"setTimeout(reloadPage, 500);"
		"}"\
		"function groupStart(group) {"\
		"document.getElementById(\"status\").innerHTML = \"Starting group \" + group;"\
		"document.getElementById(\"exec\").src=\"/groupstart?\" + group;"\
		"setTimeout(reloadPage, 500);"
		"}"\
		"</script></body></html>";
	return data;
}


QByteArray HTTPServer::generateScheduleData()
{
	QByteArray data = generateHeaderData("Pinhole schedule");
	data += generateScheduleTable();
	data += "<script>"\
		"function reloadPage() {"\
		"location.reload();"\
		"}"\
		"function eventTrigger(event) {"\
		"document.getElementById(\"status\").innerHTML = \"Triggering scheduled event \" + event;"\
		"document.getElementById(\"exec\").src=\"/eventtrigger?\" + event;"\
		"setTimeout(reloadPage, 500);"
		"}"\
		"</script></body></html>";
	return data;
}


QByteArray HTTPServer::generateSystemInfoData()
{
	QByteArray data = generateHeaderData("Pinhole system information");
	data += "<pre>";
	data += m_globalManager->getSysInfoData();
	data += "</pre></body></html>";
	return data;
}


QByteArray HTTPServer::generateLogData(const QString& arg)
{
	QByteArray data = generateHeaderData("Pinhole log");
	int daysAgo = arg.toInt();
	data += "<table><caption>Days ago</caption>";
	for (int n = 0; n < 30; n++)
	{
		data += "<td>";
		if (n == daysAgo)
		{
			data += QString::number(n);
		}
		else
		{
			data += "<a href='/viewlog?" + QString::number(n) + "'>" + QString::number(n) + "</a>";
		}
		data += "</td>";
	}
	data += "</table>";

	QDateTime current = QDateTime::currentDateTime();
	QDateTime end = current.addDays(0 - static_cast<qint64>(daysAgo));
	QDateTime start = current.addDays(-1 - static_cast<qint64>(daysAgo));
	data += "<pre>";
	data += Logger::getLogFileData(
		start.toString(DATETIME_STRINGFORMAT), 
		end.toString(DATETIME_STRINGFORMAT));
	data += "</pre></body></html>";
	return data;
}


// Sends a heartbeat to an application
QByteArray HTTPServer::generateAppHeartbeatData(const QString& appName, int& err, QString& errmsg)
{
	QByteArray data = generateHeaderData("Pinhole application heartbeat");

	if (!m_appManager->heartbeatApp(appName))
	{
		err = 500;
		errmsg = "App not found";
		data += "App " + appName + " not found";
	}
	else
	{
		err = 200;
		errmsg = "OK";
		data += "Heartbeat sent to application " + appName;
	}

	data += "<p></body></html>";
	return data;
}


// generates an HTML table of application status
QByteArray HTTPServer::generateApplicationTable()
{
	QByteArray ret;
	QStringList appNames = m_appManager->getAppNames();
	if (appNames.isEmpty())
	{
		ret = "<p>No applications configured</p>";
	}
	else
	{
		ret += "<table><caption>Applications</caption><tr>"
			"<th>Action</th>"
			"<th>Name</th>"
			"<th>State</th>"
			"<th>Restarts</th>"
			"<th>Executable</th>"
			"<th>Arguments</th>"
			"<th>Directory</th>"
			"<th>Last started</th>"
			"<th>Last exited</th>"
			"</tr>";

		for (const auto & app : appNames)
		{
			bool appRunning = m_appManager->getAppVariant(app, PROP_APP_RUNNING).toBool();
			QString state = m_appManager->getAppVariant(app, PROP_APP_STATE).toString();
			int restarts = m_appManager->getAppVariant(app, PROP_APP_RESTARTS).toInt();
			QString executable = m_appManager->getAppVariant(app, PROP_APP_EXECUTABLE).toString();
			QString arguments = m_appManager->getAppVariant(app, PROP_APP_ARGUMENTS).toString();
			QString directory = m_appManager->getAppVariant(app, PROP_APP_DIRECTORY).toString();
			QString lastStarted = m_appManager->getAppVariant(app, PROP_APP_LASTSTARTED).toString();
			QString lastExited = m_appManager->getAppVariant(app, PROP_APP_LASTEXITED).toString();

			ret += "<tr><td><button onclick=\"";
			if (appRunning)
			{
				ret += "appStop('" + escapeString(app) + "')\">Stop";
			}
			else
			{
				ret += "appStart('" + escapeString(app) + "')\">Start";
			}
			ret += "</button></td><td>" + app + "</td>";
			ret += "<td>" + state + "</td>";
			ret += "<td>" + QString::number(restarts) + "</td>";
			ret += "<td>" + executable + "</td>";
			ret += "<td>" + arguments + "</td>";
			ret += "<td>" + directory + "</td>";
			ret += "<td>" + lastStarted + "</td>";
			ret += "<td>" + lastExited + "</td>";
			ret += "</tr>";
		}
		ret += "</table>";
	}

	return ret;
}


QByteArray HTTPServer::generateGroupsTable()
{
	QByteArray ret;
	QStringList groupNames = m_groupManager->getGroupList();
	if (groupNames.isEmpty())
	{
		ret = "<p>No groups configured</p>";
	}
	else
	{
		ret += "<table><caption>Groups</caption><tr>"
			"<th>Start</th>"
			"<th>Stop</th>"
			"<th>Name</th>"
			"<th>Launch at start</th>"
			"<th>Members</th>"
			"</tr>";

		for (const auto& group : groupNames)
		{
			bool launchAtStart = m_groupManager->getGroupVariant(group, PROP_GROUP_LAUNCHATSTART).toBool();
			QStringList members = m_groupManager->getGroupVariant(group, PROP_GROUP_APPLICATIONS).toStringList();

			ret += "<tr><td><button onclick=\"";
			ret += "groupStart('" + escapeString(group) + "')\">Start";
			ret += "</button></td>";
			ret += "<td><button onclick=\"";
			ret += "groupStop('" + escapeString(group) + "')\">Stop";
			ret += "</button></td>";
			ret += "<td>" + group + "</td>";
			ret += "<td>" + QString(launchAtStart ? "Yes" : "No") + "</td>";
			ret += "<td>" + members.join(" ") + "</td>";
			ret += "</tr>";
		}

		ret += "</table>";
	}

	return ret;
}


QByteArray HTTPServer::generateScheduleTable()
{
	QByteArray ret;
	QStringList eventNames = m_scheduleManager->getEventNames();
	if (eventNames.isEmpty())
	{
		ret = "<p>No scheduled events configured</p>";
	}
	else
	{
		ret += "<table><caption>Scheduled events</caption><tr>"
			"<th>Trigger</th>"
			"<th>Name</th>"
			"<th>Type</th>"
			"<th>Frequency</th>"
			"<th>When</th>"
			"<th>Last triggered</th>"
			"<th>Arguments</th>"
			"</tr>";

		for (const auto& event : eventNames)
		{
			QString type = m_scheduleManager->getEventVariant(event, PROP_SCHED_TYPE).toString();
			QString frequency = m_scheduleManager->getEventVariant(event, PROP_SCHED_FREQUENCY).toString();
			int offset = m_scheduleManager->getEventVariant(event, PROP_SCHED_OFFSET).toInt();
			QString lastTriggered = m_scheduleManager->getEventVariant(event, PROP_SCHED_LASTTRIGGERED).toString();
			QString arguments = m_scheduleManager->getEventVariant(event, PROP_SCHED_ARGUMENTS).toString();

			ret += "<tr><td><button onclick=\"";
			ret += "eventTrigger('" + escapeString(event) + "')\">Trigger";
			ret += "</button></td>";
			ret += "<td>" + event + "</td>";
			ret += "<td>" + type + "</td>";
			ret += "<td>" + frequency + "</td>";
			ret += "<td>" + EventOffsetToString(frequency, offset) + "</td>";
			ret += "<td>" + lastTriggered + "</td>";
			ret += "<td>" + arguments + "</td>";
			ret += "</td>";
		}

		ret += "</table>";
	}

	return ret;
}


// generates response for starting or stopping an app
QByteArray HTTPServer::generateStartStopAppData(const QString& appName, bool start)
{
	QByteArray data = generateHeaderData("Pinhole application control");

	bool success = true;
	if (start)
		success = m_appManager->startApps(QStringList(appName));
	else
		success = m_appManager->stopApps(QStringList(appName));
	
	if (!success)
	{
		data += "Failed to " + QString(start ? "start" : "stop") + " app " + appName;
	}
	else
	{
		data += "App " + appName + QString(start ? " started" : " stopped");
	}

	data += "<p><a href=\"/\">Back</a></body></html>";
	return data;
}


QByteArray HTTPServer::generateStartStopGroupData(const QString& groupName, bool start)
{
	QByteArray data = generateHeaderData("Pinhole group control");

	bool success = true;
	if (start)
		success = m_groupManager->startGroup(groupName);
	else
		success = m_groupManager->stopGroup(groupName);

	if (!success)
	{
		data += "Failed to " + QString(start ? "start" : "stop") + " group " + groupName;
	}
	else
	{
		data += "Group " + groupName + QString(start ? " started" : " stopped");
	}

	data += "<p><a href=\"/groups\">Back</a></body></html>";
	return data;
}


QByteArray HTTPServer::generateTriggerEventData(const QString eventName)
{
	QByteArray data = generateHeaderData("Pinhole schedule event control");

	if (!m_scheduleManager->triggerEvents(QStringList(eventName)))
	{
		data += "Failed to trigger scheduled event " + eventName;
	}
	else
	{
		data += "Scheduled event " + eventName + " triggered";
	}

	data += "<p><a href=\"/schedule\">Back</a></body></html>";
	return data;
}


QByteArray HTTPServer::generateShutdownRebootAppData(bool reboot)
{
	Logger(LOG_ALWAYS) << "Rebotting or shutting down machine via HTTP server";

	QByteArray data = generateHeaderData("Shutdown/Reboot computer");
	bool success = false;
	if (reboot)
		success = m_globalManager->reboot();
	else
		success = m_globalManager->shutdown();

	if (!success)
	{
		Logger(LOG_ERROR) << "Failed to " << (reboot ? "reboot" : "shut down") << " the system";
		data += "Failed to " + QString(reboot ? "restart" : "shut down") + " the system";
	}
	else
	{
		data += "System " + QString(reboot ? "restarting" : "shutting down");
	}
	data += "<p><a href=\"/\">Back</a></body></html>";
	return data;
}


QByteArray HTTPServer::generateShowScreenIDsData()
{
	HostClient* hostClient = new HostClient("127.0.0.1", HOST_TCPPORT, QString(), true, false, this);

	bool success = false;

	connect(hostClient, &HostClient::connected,
		this, [hostClient]()
	{
		// Request show screen IDs
		hostClient->showScreenIds();
	});

	connect(hostClient, &HostClient::connectFailed,
		this, [hostClient](const QString& reason)
	{
		Logger(LOG_WARNING) << tr("HTTP server: Failed to request show screen IDs (failed to connect to localhost? %1)")
			.arg(reason);
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandError,
		this, [hostClient]()
	{
		Logger(LOG_WARNING) << "HTTP server: Failed to request show screen IDs (command error, helper not running?)";
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandSuccess,
		this, [hostClient, &success]()
	{
		success = true;
		hostClient->deleteLater();
	});

	// Block until we receive a response from the server
	QEventLoop eventLoop(this);
	connect(hostClient, &HostClient::destroyed,
		&eventLoop, &QEventLoop::quit);
	eventLoop.exec();

	QByteArray data = generateHeaderData("Show screen IDs");
	if (!success)
	{
		data += "Failed to show screen IDs";
	}
	else
	{
		data += "Screen IDs shown";
	}

	data += "<p><a href=\"/schedule\">Back</a></body></html>";
	return data;
}


QByteArray HTTPServer::generateScreenshotData()
{
	QByteArray screenshot;

	HostClient* hostClient = new HostClient("127.0.0.1", HOST_TCPPORT, QString(), true, false, this);

	connect(hostClient, &HostClient::connected,
		this, [hostClient]()
	{
		// Request screenshot
		hostClient->requestScreenshot();
	});

	connect(hostClient, &HostClient::connectFailed,
		this, [hostClient](const QString& reason)
	{
		Logger(LOG_WARNING) << tr("HTTP server: Failed to aquire screenshot (failed to connect to localhost? %1)")
			.arg(reason);
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandError,
		this, [hostClient]()
	{
		Logger(LOG_WARNING) << tr("HTTP server: Failed to aquire screenshot (command error, helper not running?)");
		hostClient->deleteLater();
	});

	connect(hostClient, &HostClient::commandData,
		this, [hostClient, &screenshot](const QString& group, const QString& subCommand, const QVariant& data)
	{
		if (GROUP_NONE != group || CMD_NONE_GETSCREENSHOT != subCommand)
		{
			Logger(LOG_WARNING) << tr("HTTP server: Failed to aquire screenshot (internal error, wrong data from server)");
		}
		else
		{
			screenshot = data.toByteArray();
		}

		hostClient->deleteLater();
	});

	// Block until we receive a response from the server
	QEventLoop eventLoop(this);
	connect(hostClient, &HostClient::destroyed,
		&eventLoop, &QEventLoop::quit);
	eventLoop.exec();
	return screenshot;
}


void HTTPServer::rx()
{
	QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
	QByteArray data = socket->readAll();

	QString auth;
	QString path = QUrl::fromPercentEncoding(parseHTTPRequest(data, auth).toUtf8());
	QByteArray response;

	if (path.isEmpty())
	{
		// Bad request, do nothing
		Logger(LOG_WARNING) << "Bad HTTP request (no path)";
	}
	else
	{
		// Parse out the argument if it exists
		QString arg;
		int qPos = path.indexOf("?");
		if (-1 != qPos)
		{
			arg = path.mid(qPos + 1);
			path = path.left(qPos);
		}
		// Parse out just the password
		int colPos = auth.indexOf(":");
		if (-1 != colPos)
		{
			auth = auth.mid(colPos + 1);
		}

		Logger(LOG_DEBUG) << "HTTP request received, path '" << path << "' arg '" << arg << "'";

		if ("/" == path)
		{
			// Root page (applications)
			response = generateResponse(200, "OK", generateRootData());
		}
		else if ("/groups" == path)
		{
			response = generateResponse(200, "OK", generateGroupsData());
		}
		else if ("/schedule" == path)
		{
			response = generateResponse(200, "OK", generateScheduleData());
		}
		else if ("/sysinfo" == path)
		{
			response = generateResponse(200, "OK", generateSystemInfoData());
		}
		else if ("/viewlog" == path)
		{
			response = generateResponse(200, "OK", generateLogData(arg));
		}
		else if ("/heartbeat" == path)
		{
			int err;
			QString errMsg;
			QByteArray data = generateAppHeartbeatData(arg, err, errMsg);
			response = generateResponse(err, errMsg, data);
		}
		else if ("/appstart" == path)
		{
			response = generateResponse(200, "OK", generateStartStopAppData(arg, true));
		}
		else if ("/appstop" == path)
		{
			response = generateResponse(200, "OK", generateStartStopAppData(arg, false));
		}
		else if ("/groupstart" == path)
		{
			response = generateResponse(200, "OK", generateStartStopGroupData(arg, true));
		}
		else if ("/groupstop" == path)
		{
			response = generateResponse(200, "OK", generateStartStopGroupData(arg, false));
		}
		else if ("/eventtrigger" == path)
		{
			response = generateResponse(200, "OK", generateTriggerEventData(arg));
		}
		else if ("/reboot/" == path)
		{
			response = generateResponse(200, "OK", generateShutdownRebootAppData(true));
		}
		else if ("/shutdown/" == path)
		{
			response = generateResponse(200, "OK", generateShutdownRebootAppData(false));
		}
		else if ("/showids" == path)
		{
			response = generateResponse(200, "OK", generateShowScreenIDsData());
		}
		else if ("/screenshot.png" == path)
		{
			QByteArray data = generateScreenshotData();
			if (data.isEmpty())
			{
				response = generateResponse(500, "Internal server error", "There was a problem reading the screenshot image");
			}
			else
			{
				response = generateResponse(200, "OK", data, "image/png");
			}
		}
		else
		{
			response = generateResponse(404, "Not found", "<html>No such page</html>\r\n");
			Logger(LOG_WARNING) << "Bad HTTP request path: '" << path << "'";
		}

		// Send response
		if (!response.isEmpty())
		{
			// todo - will data send all in one packet?
			// Seems to break it up/buffer
			socket->write(response);
		}
	}
}


