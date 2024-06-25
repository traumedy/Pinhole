#include "HostConfigGlobalsWidget.h"
#include "Global.h"
#include "ListWidgetEx.h"
#include "GuiUtil.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QResizeEvent>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QListWidget>


HostConfigGlobalsWidget::HostConfigGlobalsWidget(QWidget *parent)
	: HostConfigWidgetTab(parent)
{
	QGridLayout* mainLayout = new QGridLayout(this);
	QScrollArea* detailsScrollArea = new QScrollArea;
	mainLayout->addWidget(detailsScrollArea);

	QWidget* detailsScrollAreaWidgetContents = new QWidget;
	detailsScrollAreaWidgetContents->setProperty("borderless", true);
	detailsScrollAreaWidgetContents->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
	detailsScrollAreaWidgetContents->setMinimumWidth(MINIMUM_DETAILS_WIDTH);
	QFormLayout* detailsLayout = new QFormLayout(detailsScrollAreaWidgetContents);
	detailsLayout->setSizeConstraint(QLayout::SetFixedSize);
	m_role = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("Role")), m_role);
	m_hostLogLevel = new QComboBox;
	m_hostLogLevel->setEditable(false);
	m_hostLogLevel->addItem(tr("Errors"), LOG_LEVEL_ERROR);
	m_hostLogLevel->addItem(tr("Warnings"), LOG_LEVEL_WARNING);
	m_hostLogLevel->addItem(tr("Normal"), LOG_LEVEL_NORMAL);
	m_hostLogLevel->addItem(tr("Extra"), LOG_LEVEL_EXTRA);
	m_hostLogLevel->addItem(tr("Debug"), LOG_LEVEL_DEBUG);
	m_hostLogLevel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	detailsLayout->addRow(tr("Host log level"), m_hostLogLevel);
	m_remoteLogLevel = new QComboBox;
	m_remoteLogLevel->setEditable(false);
	m_remoteLogLevel->addItem(tr("Errors"), LOG_LEVEL_ERROR);
	m_remoteLogLevel->addItem(tr("Warnings"), LOG_LEVEL_WARNING);
	m_remoteLogLevel->addItem(tr("Normal"), LOG_LEVEL_NORMAL);
	m_remoteLogLevel->addItem(tr("Extra"), LOG_LEVEL_EXTRA);
	m_remoteLogLevel->addItem(tr("Debug"), LOG_LEVEL_DEBUG);
	m_remoteLogLevel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	detailsLayout->addRow(tr("Remote log level"), m_remoteLogLevel);
	m_appTerminateTimeout = new QSpinBox;
	m_appTerminateTimeout->setMinimum(0);
	m_appTerminateTimeout->setMaximum(MAX_TERMINATETIMEOUT);
	m_appTerminateTimeout->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Application terminate timeout"), m_appTerminateTimeout);
	m_appHeartbeatTimeout = new QSpinBox;
	m_appHeartbeatTimeout->setMinimum(MIN_HEARTBEATTIMEOUT);
	m_appHeartbeatTimeout->setMaximum(MAX_HEARTBEATTIMEOUT);
	m_appHeartbeatTimeout->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Application heartbeat timeout"), m_appHeartbeatTimeout);
	m_appCrashPeriod = new QSpinBox;
	m_appCrashPeriod->setMinimum(MIN_CRASHPERIOD);
	m_appCrashPeriod->setMaximum(MAX_CRASHPERIOD);
	m_appCrashPeriod->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Crash throttle period"), m_appCrashPeriod);
	m_appCrashCount = new QSpinBox;
	m_appCrashCount->setMinimum(MIN_CRASHCOUNT);
	m_appCrashCount->setMaximum(MAX_CRASHCOUNT);
	m_appCrashCount->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Crash throttle max count"), m_appCrashCount);
	m_trayLaunch = new QCheckBox(tr("Automatically launch tray application"));
	detailsLayout->addRow(nullptr, m_trayLaunch);
	m_trayControl = new QCheckBox(tr("Allow server control from system tray icon"));
	detailsLayout->addRow(nullptr, m_trayControl);
	m_httpEnabled = new QCheckBox(tr("Enable HTTP interface"));
	detailsLayout->addRow(nullptr, m_httpEnabled);
	m_httpPort = new QSpinBox;
	m_httpPort->setMinimum(MIN_LISTENINGPORT);
	m_httpPort->setMaximum(MAX_PORT);
	m_httpPort->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("HTTP port"), m_httpPort);
	m_backendServer = new QLineEdit;
	detailsLayout->addRow(tr("Pinhole backend server"), m_backendServer);
	m_novaSite = new QLineEdit;
	detailsLayout->addRow(tr("Nova site"), m_novaSite);
	m_novaArea = new QLineEdit;
	detailsLayout->addRow(tr("Nova area"), m_novaArea);
	m_novaDisplay = new QLineEdit;
	detailsLayout->addRow(tr("Nova display"), m_novaDisplay);
	m_novaTcpEnabled = new QCheckBox(tr("Nova TCP enabled"));
	detailsLayout->addRow(nullptr, m_novaTcpEnabled);
	m_novaTcpAddress = new QLineEdit;
	detailsLayout->addRow(tr("Nova TCP server"), m_novaTcpAddress);
	m_novaTcpPort = new QSpinBox;
	m_novaTcpPort->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_novaTcpPort->setMinimum(MIN_LISTENINGPORT);
	m_novaTcpPort->setMaximum(MAX_PORT);
	detailsLayout->addRow(tr("Nova TCP port"), m_novaTcpPort);
	m_novaUdpEnabled = new QCheckBox(tr("Nova UDP enabled"));
	detailsLayout->addRow(nullptr, m_novaUdpEnabled);
	m_novaUdpAddress = new QLineEdit;
	detailsLayout->addRow(tr("Nova UDP multicast"), m_novaUdpAddress);
	m_novaUdpPort = new QSpinBox;
	m_novaUdpPort->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_novaUdpPort->setMinimum(MIN_LISTENINGPORT);
	m_novaUdpPort->setMaximum(MAX_PORT);
	detailsLayout->addRow(tr("Nova UDP port"), m_novaUdpPort);
	m_alertMemory = new QCheckBox(tr("Alert on low memory"));
	detailsLayout->addRow(nullptr, m_alertMemory);
	m_minMemory = new QSpinBox;
	m_minMemory->setMinimum(0);
	m_minMemory->setMaximum(999999);
	m_minMemory->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Minimum memory (MB)"), m_minMemory);
	m_alertDisk = new QCheckBox(tr("Alert on low disk space"));
	detailsLayout->addRow(nullptr, m_alertDisk);
	m_minDisk = new QSpinBox;
	m_minDisk->setMinimum(0);
	m_minDisk->setMaximum(999999);
	m_minDisk->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	detailsLayout->addRow(tr("Minimum disk space (MB)"), m_minDisk);
	m_alertDiskList = new ListWidgetEx(tr("disk"), "C:\\");
	connect(m_alertDiskList, &ListWidgetEx::valueChanged,
		this, [this]() { emit widgetValueChanged(m_alertDiskList); });
	detailsLayout->addRow(tr("Disks to monitor"), m_alertDiskList);

	detailsScrollArea->setWidget(detailsScrollAreaWidgetContents);
	detailsScrollArea->show();

	m_widgetMap = 
	{
		{ PROP_GLOBAL_ROLE , m_role },
		{ PROP_GLOBAL_HOSTLOGLEVEL, m_hostLogLevel },
		{ PROP_GLOBAL_REMOTELOGLEVEL, m_remoteLogLevel },
		{ PROP_GLOBAL_TERMINATETIMEOUT, m_appTerminateTimeout },
		{ PROP_GLOBAL_HEARTBEATTIMEOUT, m_appHeartbeatTimeout },
		{ PROP_GLOBAL_CRASHPERIOD, m_appCrashPeriod },
		{ PROP_GLOBAL_CRASHCOUNT, m_appCrashCount },
		{ PROP_GLOBAL_TRAYLAUNCH, m_trayLaunch },
		{ PROP_GLOBAL_TRAYCONTROL, m_trayControl },
		{ PROP_GLOBAL_HTTPENABLED, m_httpEnabled },
		{ PROP_GLOBAL_HTTPPORT, m_httpPort },
		{ PROP_GLOBAL_BACKENDSERVER, m_backendServer },
		{ PROP_GLOBAL_NOVASITE, m_novaSite },
		{ PROP_GLOBAL_NOVAAREA, m_novaArea },
		{ PROP_GLOBAL_NOVADISPLAY, m_novaDisplay },
		{ PROP_GLOBAL_NOVATCPENABLED, m_novaTcpEnabled },
		{ PROP_GLOBAL_NOVATCPADDRESS, m_novaTcpAddress },
		{ PROP_GLOBAL_NOVATCPPORT, m_novaTcpPort },
		{ PROP_GLOBAL_NOVAUDPENABLED, m_novaUdpEnabled },
		{ PROP_GLOBAL_NOVAUDPADDRESS, m_novaUdpAddress },
		{ PROP_GLOBAL_NOVAUDPPORT, m_novaUdpPort },
		{ PROP_GLOBAL_ALERTMEMORY, m_alertMemory },
		{ PROP_GLOBAL_MINMEMORY, m_minMemory },
		{ PROP_GLOBAL_ALERTDISK, m_alertDisk },
		{ PROP_GLOBAL_MINDISK, m_minDisk },
		{ PROP_GLOBAL_ALERTDISKLIST, m_alertDiskList }
	};

	for (const auto& key : m_widgetMap.keys())
	{
		m_widgetMap[key]->setProperty(PROPERTY_GROUPNAME, GROUP_GLOBAL);
		m_widgetMap[key]->setProperty(PROPERTY_ITEMNAME, "");
		m_widgetMap[key]->setProperty(PROPERTY_PROPNAME, key);
	}

	QString novaHtml = LocalHtmlDoc(DOC_NOVA, tr("Details about Nova"));
	QString httpHtml = LocalHtmlDoc(DOC_HTTPSERVER, tr("Details about HTTP server"));
	QString loggingHtml = LocalHtmlDoc(DOC_LOGGING, tr("More information about logging"));
	QString loopbackHtml = LocalHtmlDoc(DOC_LOOPBACK, tr("More information about the TCP loopback"));
	QString resourceHtml = LocalHtmlDoc(DOC_RESOURCE, tr("More information about the resource monitoring"));
	QString backendHtml = LocalHtmlDoc(DOC_RESOURCE, tr("More information about PinholeBackend"));
	
	m_role->setToolTip(tr("The assigned role for this host"));
	m_role->setWhatsThis(tr("This is where you enter the role for this host.  This value is "
		"displayed in the Pinhole Console host list and is passed to the application in "
		"an environment variable named <b>" APPENVVAR_ROLE "</b>."));
	m_hostLogLevel->setToolTip(tr("The log level detail for the log file stored on the host"));
	m_hostLogLevel->setWhatsThis(tr("This sets the level of detail for log entries written "
		"to the log file on the server host.  This can be set to <b>EXTRA</b> or "
		"<b>DEBUG</b> for troubleshooting purposes or to <b>WARNINGS</b> to reduce the "
		"size of the log generated file."));
	m_remoteLogLevel->setToolTip(tr("The log level detail for the log viewed remotely"));
	m_remoteLogLevel->setWhatsThis(tr("This sets te level of detail for log entries sent to "
		"Pinhole Console remote log dialogs.  This can be set to <b>EXTRA</b> or "
		"<b>DEBUG</b> for troubleshooting purposes"));
	m_appTerminateTimeout->setToolTip(tr("The time in milliseconds to wait for an app to exit before killing it"));
	m_appTerminateTimeout->setWhatsThis(tr("If <b>Soft terminate</b> is enabled for an application, Pinhole will first "
		"send it a message to terminate (SIGTERM on Unix/Linux/Mac, WM_CLOSE on Windows) allowing it to cleanup and "
		"close gracefully.  This timeout is how long to wait (in milliseconds) before the process is forcefully "
		"terminated."));
	m_appHeartbeatTimeout->setToolTip(tr("The number of milliseconds to wait for a heartbeat from an application before restarting it"));
	m_appHeartbeatTimeout->setWhatsThis(tr("This is where you set the application heartbeat "
		"timeout (in milliseconds).  This is the amount of time Pinhole will wait without "
		"receiving a heartbeat from an application before assuming that application has "
		"become unresponsive, at which point it will terminate and restart it.  Heartbeats "
		"can be sent via the TCP loopback, the logging named pipe or the HTTP interface.") 
		+ loopbackHtml + httpHtml + loggingHtml);
	m_appCrashPeriod->setToolTip(tr("Restart throttle in seconds"));
	m_appCrashPeriod->setWhatsThis(tr("Stop restarting an application if it crashes "
		"<b>Crash throttle count</b> times in this period, specified in seconds."));
	m_appCrashCount->setToolTip(tr("Restart throttle crash count"));
	m_appCrashCount->setWhatsThis(tr("Stop restarting an applciation if it crashes this number "
		"times in <b>Crash throttle period</b> seconds."));
	m_trayLaunch->setToolTip(tr("PinholeServer will automatically start and stop PinholeHelper"));
	m_trayLaunch->setWhatsThis(tr("If checked, PinholeServer will launch PinholeHelper when it "
		"starts and terminate it when it stops.  PinholeHelper is the user interface portion of "
		"the server providing a system tray icon the user can interact with."));
	m_trayControl->setToolTip(tr("Allow the interactive user to execute commands on the server using the system tray icon"));
	m_trayControl->setWhatsThis(tr("If checked, the system tray icon menu will "
		"allow starting and stopping of applications.  For installations, this should be "
		"disabled before deployment to prevent clients from interfering with the intended "
		"operation.  Clients will still be able to enter log messages."));
	m_httpEnabled->setToolTip(tr("Enable control via HTTP interface"));
	m_httpEnabled->setWhatsThis(tr("If checked, Pinhole will listen for HTTP connections "
		"and serve an interface allowing viewing of the configuration as well as being "
		"able to start/stop applications/groups, and trigger events.  The log can also "
		"be viewed as well as a system information summary.  The system can also be "
		"shut down or rebooted throug this intreface.<br>"
		"Applciations can also send heartbeats through this interface.") + httpHtml);
	m_httpPort->setToolTip(tr("The TCP port for the HTTP interface to listen on"));
	m_httpPort->setWhatsThis(tr("To connect to the HTTP server from a browser, this port "
		"number must be specifed in the URL such as <pre>http://127.0.0.1:8090</pre>.  "
		"This value is exported to applications in the environment variable <b>" 
		APPENVVAR_HTTPPORT "</b>."));
	m_backendServer->setToolTip(tr("Host name or IP of a PinholeBackend server"));
	m_backendServer->setWhatsThis(tr("PinholeServer can be configured to communicate through "
		"a proxy server using PinholeBackend as a secondary network interface.  Enter the "
		"host name or IP address of the machine running PinholeBackend here.  Point "
		"PinholeConsole to the same address and the connected PinholeServer's should show up "
		"in the host list.") + backendHtml);
	m_novaSite->setToolTip(tr("The Nova site identifier for this server"));
	m_novaSite->setWhatsThis(tr("This is the <b>site</b> portion if the location identifier "
		"for this instance of Pinhole Server.  Pinhole Server will only act upon messages "
		"sent to location identifiers that match its location identifier, wildcards "
		"considered.") + novaHtml);
	m_novaArea->setToolTip(tr("The Nova area identifier for this server"));
	m_novaArea->setWhatsThis(tr("This is the <b>area</b> portion if the location identifier "
		"for this instance of Pinhole Server.  Pinhole Server will only act upon messages "
		"sent to location identifiers that match its location identifier, wildcards "
		"considered.") + novaHtml);
	m_novaDisplay->setToolTip(tr("The Nova display identifier for this server"));
	m_novaDisplay->setWhatsThis(tr("This is the <b>display</b> portion if the location identifier "
		"for this instance of Pinhole Server.  Pinhole Server will only act upon messages "
		"sent to location identifiers that match its location identifier, wildcards "
		"considered.") + novaHtml);
	m_novaTcpEnabled->setToolTip(tr("Connect to a Nova TCP server"));
	m_novaTcpEnabled->setWhatsThis(tr("If checked, Pinhole can accept Nova commands via TCP.  If "
		"an IP address or host name is specified in <b>Nova TCP server</b> Pinhole will "
		"connect to that address, otherwise it will create a TCP server to listen on.") + novaHtml);
	m_novaTcpAddress->setToolTip(tr("The IP address or host name of the Nova TCP server to connect to"));
	m_novaTcpAddress->setWhatsThis(tr("This is where you enter the host name or IP address of "
		"the TCP server to connect to to receive Nova commands.  IP addresses can be in "
		"IPv4 or IPv6 format.<br>If this field is left blank Pinhole will act as a TCP server.") + novaHtml);
	m_novaTcpPort->setToolTip(tr("The TCP pot of the Nova TCP server"));
	m_novaTcpPort->setWhatsThis(tr("This is where you set the TCP port for Pinhole to connect "
		"to or listen on to receive Nova commands.") + novaHtml);
	m_novaUdpEnabled->setToolTip(tr("Accept Nova commands via UDP"));
	m_novaUdpEnabled->setWhatsThis(tr("If checked, Pinhole can receive Nova commands via UDP.") + novaHtml);
	m_novaUdpAddress->setToolTip(tr("The UDP multicast address to join to receive UDP Nova commands"));
	m_novaUdpAddress->setWhatsThis(tr("If this field is blank Pinhole will listen locally for "
		"Nova UDP packets.  If you enter an IPv4 or IPv6 multicast address into this field "
		"Pinhole will join that address to receive packets.") + novaHtml);
	m_novaUdpPort->setToolTip(tr("The UDP port to accept Nova commands on"));
	m_novaUdpPort->setWhatsThis(tr("This is where you set the UDP port that Pinhole will "
		"listen for Nova packets on.") + novaHtml);
	m_alertMemory->setToolTip(tr("Alert on low memory"));
	m_alertMemory->setWhatsThis(tr("If checked, Pinhole will generate an alert if the memory falls below "
		"the specified level.") + resourceHtml);
	m_minMemory->setToolTip(tr("Minimum memory in megabytes"));
	m_minMemory->setWhatsThis(tr("The amount of free memory that must be available to prevent Pinhole "
		"from generating an alert, if this feature is enabled.") + resourceHtml);
	m_alertDisk->setToolTip(tr("Alert on low disk space"));
	m_alertDisk->setWhatsThis(tr("If checked, Pinhole will generate an alert if any of the listed disks "
		"fall below the specified minimum value.") + resourceHtml);
	m_minDisk->setToolTip(tr("Minimum disk space in megabytes"));
	m_minDisk->setWhatsThis(tr("The amount of free disk space that must be available on the listed disks "
		"to prevent Pinhole from generating an alert, if this feature is enabled.") + resourceHtml);
	m_alertDiskList->setToolTip(tr("The list of disks to monitor"));
	m_alertDiskList->setWhatsThis(tr("This list should contain the root path of the disks that Pinhole "
		"will monitor the available space of.  For Windows this should be the drive capital letter and "
		"a backslash such as <b>C:\\</b> and on other operating systems it should be the path to the root "
		"such as <b>/</b>.  Right click on the list to bring up the menu to add a new entry or remove "
		"the selected entries.") + resourceHtml);
}


HostConfigGlobalsWidget::~HostConfigGlobalsWidget()
{
}


void HostConfigGlobalsWidget::resetWidgets()
{
	for (auto widget : m_widgetMap.values())
		ClearWidget(widget);
}


void HostConfigGlobalsWidget::enableWidgets(bool enable)
{
	EnableWidgetMap(m_widgetMap, enable && m_editable);
}


void HostConfigGlobalsWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
}


bool HostConfigGlobalsWidget::event(QEvent* ev)
{
	if (ev->type() == QEvent::WhatsThisClicked)
	{
		ev->accept();
		QWhatsThisClickedEvent* whatsThisEvent = dynamic_cast<QWhatsThisClickedEvent*>(ev);
		assert(whatsThisEvent);
		QUrl url(whatsThisEvent->href(), QUrl::TolerantMode);
		if (!QDesktopServices::openUrl(url))
		{
			QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
				tr("Failed to open URL: %1")
				.arg(url.toString()));
		}
		return true;
	}

	return QWidget::event(ev);
}

