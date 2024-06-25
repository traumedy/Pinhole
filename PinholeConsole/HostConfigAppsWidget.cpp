#include "HostConfigAppsWidget.h"
#include "Global.h"
#include "ListWidgetEx.h"
#include "GuiUtil.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QResizeEvent>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QMessageBox>


HostConfigAppsWidget::HostConfigAppsWidget(QWidget *parent)
	: HostConfigWidgetTab(parent)
{
	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	ScrollAreaEx* buttonScrollArea = new ScrollAreaEx;
	buttonScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
	QScrollArea* detailsScrollArea = new QScrollArea;
	detailsScrollArea->setWidgetResizable(true);
	mainLayout->addWidget(buttonScrollArea);
	mainLayout->addWidget(detailsScrollArea);

	QWidget* buttonScrollAreaWidgetContents = new QWidget;
	buttonScrollAreaWidgetContents->setProperty("borderless", true);
	QVBoxLayout* buttonLayout = new QVBoxLayout(buttonScrollAreaWidgetContents);
	m_addAppButton = new QPushButton(tr("Add application"));
	m_addAppButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_addAppButton);
	m_deleteAppButton = new QPushButton(tr("Delete application"));
	m_deleteAppButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_deleteAppButton);
	m_renameAppButton = new QPushButton(tr("Rename application"));
	m_renameAppButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_renameAppButton);
	m_startAppButton = new QPushButton(tr("Start application"));
	m_startAppButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_startAppButton);
	m_stopAppButton = new QPushButton(tr("Stop application"));
	m_stopAppButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_stopAppButton);
	m_getConsoleOutput = new QPushButton(tr("Get console output"));
	m_getConsoleOutput->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_getConsoleOutput);
	m_appList = new QListWidget;
	m_appList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_appList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_appList->setSortingEnabled(true);
	m_appList->setMaximumWidth(BUTTONITEMWIDTH);
	m_appList->setMaximumHeight(ITEMLISTMAXHEIGHT);
	connect(m_appList, &QListWidget::currentItemChanged,
		this, &HostConfigAppsWidget::appList_currentItemChanged);
	buttonLayout->addWidget(m_appList);
	buttonScrollArea->setWidget(buttonScrollAreaWidgetContents);
	buttonScrollArea->show();

	QWidget* detailsScrollAreaWidgetContents = new QWidget;
	detailsScrollAreaWidgetContents->setProperty("borderless", true);
	detailsScrollAreaWidgetContents->setMinimumWidth(MINIMUM_DETAILS_WIDTH);
	QFormLayout* detailsLayout = new QFormLayout(detailsScrollAreaWidgetContents);
	detailsLayout->setSizeConstraint(QLayout::SetFixedSize);
	m_appState = new QLabel;
	detailsLayout->addRow(new QLabel(tr("App state")), m_appState);
	m_lastStarted = new QLabel;
	detailsLayout->addRow(new QLabel(tr("Last started")), m_lastStarted);
	m_lastExited = new QLabel;
	detailsLayout->addRow(new QLabel(tr("Last exited")), m_lastExited);
	m_restarts = new QLabel;
	detailsLayout->addRow(new QLabel(tr("Restarts")), m_restarts);
	m_appExecutable = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("Executable")), m_appExecutable);
	m_appArguments = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("Arguments")), m_appArguments);
	m_appDirectory = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("Working directory")), m_appDirectory);
	m_launchDisplay = new QComboBox;
	m_launchDisplay->addItem(tr("Normal"), DISPLAY_NORMAL);
	m_launchDisplay->addItem(tr("Hidden"), DISPLAY_HIDDEN);
	m_launchDisplay->addItem(tr("Minimized"), DISPLAY_MINIMIZE);
	m_launchDisplay->addItem(tr("Maximized"), DISPLAY_MAXIMIZE);
	m_launchDisplay->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	detailsLayout->addRow(tr("Launch displayed"), m_launchDisplay);
	m_launchDelay = new QSpinBox;
	m_launchDelay->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_launchDelay->setMinimum(0);
	m_launchDelay->setMaximum(99999);
	detailsLayout->addRow(tr("Launch delay"), m_launchDelay);
	m_launchAtStart = new QCheckBox(tr("Launch application at start"));
	detailsLayout->addRow(nullptr, m_launchAtStart);
	m_keepAppRunning = new QCheckBox(tr("Keep application running"));
	detailsLayout->addRow(nullptr, m_keepAppRunning);
	m_terminatePrev = new QCheckBox(tr("Terminate existing instances of executable"));
	detailsLayout->addRow(nullptr, m_terminatePrev);
	m_softTerminate = new QCheckBox(tr("Soft terminate application"));
	detailsLayout->addRow(nullptr, m_softTerminate);
	m_noCrashThrottle = new QCheckBox(tr("Do not throttle restarts because of crashes"));
	detailsLayout->addRow(nullptr, m_noCrashThrottle);
	m_lockupScreenshot = new QCheckBox(tr("Take screenshot on application lockup"));
	detailsLayout->addRow(nullptr, m_lockupScreenshot);
	m_consoleCapture = new QCheckBox(tr("Capture console output to file"));
	detailsLayout->addRow(nullptr, m_consoleCapture);
	m_appendCapture = new QCheckBox(tr("Append console output"));
	detailsLayout->addRow(nullptr, m_appendCapture);
	m_heartbeats = new QCheckBox(tr("Enforce timeout heartbeats"));
	detailsLayout->addRow(nullptr, m_heartbeats);
	m_tcpLoopback = new QCheckBox(tr("Enable application TCP loopback"));
	detailsLayout->addRow(nullptr, m_tcpLoopback);
	m_tcpLoopbackPort = new QSpinBox;
	m_tcpLoopbackPort->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_tcpLoopbackPort->setMinimum(MIN_LISTENINGPORT);
	m_tcpLoopbackPort->setMaximum(MAX_PORT);
	detailsLayout->addRow(new QLabel(tr("TCP loopback port")), m_tcpLoopbackPort);
	m_environmentList = new ListWidgetEx(tr("environment variable"), "NAME=VALUE");
	connect(m_environmentList, &ListWidgetEx::valueChanged,
		this, [this]() { emit widgetValueChanged(m_environmentList); });
	detailsLayout->addRow(new QLabel(tr("Environment")), m_environmentList);

	detailsScrollArea->setWidget(detailsScrollAreaWidgetContents);
	detailsScrollArea->show();

	m_widgetMap =
	{
		{ PROP_APP_STATE, m_appState },
		{ PROP_APP_LASTSTARTED, m_lastStarted },
		{ PROP_APP_LASTEXITED, m_lastExited },
		{ PROP_APP_RESTARTS, m_restarts },
		{ PROP_APP_EXECUTABLE, m_appExecutable },
		{ PROP_APP_ARGUMENTS, m_appArguments },
		{ PROP_APP_DIRECTORY, m_appDirectory },
		{ PROP_APP_LAUNCHDISPLAY, m_launchDisplay },
		{ PROP_APP_LAUNCHDELAY, m_launchDelay },
		{ PROP_APP_LAUNCHATSTART, m_launchAtStart },
		{ PROP_APP_KEEPAPPRUNNING, m_keepAppRunning },
		{ PROP_APP_TERMINATEPREV, m_terminatePrev },
		{ PROP_APP_SOFTTERMINATE, m_softTerminate },
		{ PROP_APP_NOCRASHTHROTTLE, m_noCrashThrottle }, 
		{ PROP_APP_LOCKUPSCREENSHOT, m_lockupScreenshot },
		{ PROP_APP_CONSOLECAPTURE, m_consoleCapture },
		{ PROP_APP_APPENDCAPTURE, m_appendCapture },
		{ PROP_APP_TCPLOOPBACK, m_tcpLoopback },
		{ PROP_APP_TCPLOOPBACKPORT, m_tcpLoopbackPort },
		{ PROP_APP_HEARTBEATS, m_heartbeats },
		{ PROP_APP_ENVIRONMENT, m_environmentList}
	};

	m_appList->setProperty(PROPERTY_GROUPNAME, GROUP_APP);
	m_appList->setProperty(PROPERTY_PROPNAME, PROP_APP_LIST);

	// Set the property names for the widgets
	for (const auto& key : m_widgetMap.keys())
	{
		m_widgetMap[key]->setProperty(PROPERTY_GROUPNAME, GROUP_APP);
		m_widgetMap[key]->setProperty(PROPERTY_PROPNAME, key);
	}

	QString environmentHtml = LocalHtmlDoc(DOC_ENVIRONMENT, tr("Details about application environment and variables"));
	QString httpHtml = LocalHtmlDoc(DOC_HTTPSERVER, tr("Details about HTTP server"));
	QString loggingHtml = LocalHtmlDoc(DOC_LOGGING, tr("More information about logging"));
	QString loopbackHtml = LocalHtmlDoc(DOC_LOOPBACK, tr("More information about the TCP loopback"));
	
	m_addAppButton->setToolTip(tr("Add a new application"));
	m_addAppButton->setWhatsThis(tr("This button will add a new application to the host.  You will be prompted for "
		"the name of the new application.  The only field required for an application is the path to the "
		"executable.  If you enter a duplicate application name you will get an error.  Application names "
		"<b>are</b> case sensitive."));
	m_deleteAppButton->setToolTip(tr("Delete the selected application"));
	m_deleteAppButton->setWhatsThis(tr("This button will delete the currently selected application.  You will "
		"be prompted with a confirmation dialog before the application is deleted."));
	m_renameAppButton->setToolTip(tr("Rename the selected application"));
	m_renameAppButton->setWhatsThis(tr("This button allows you to rename the currently selected application.  You "
		"will be prompted for the new name.  If the new name already exists you will get an error."));
	m_startAppButton->setToolTip(tr("Start the selected application"));
	m_startAppButton->setWhatsThis(tr("This button will attempt to start the currently selected application."));
	m_stopAppButton->setToolTip(tr("Stop the selected application"));
	m_stopAppButton->setWhatsThis(tr("This button will attempt to stop the currently selected application."));
	m_getConsoleOutput->setToolTip(tr("Get the console output from the last run of this application"));
	m_getConsoleOutput->setWhatsThis(tr("Retrieves the console output log from the last time the application was"
		"run.  If the program is not a console application or the program was not run (the file doesn't exist)"
		"this operation will fail."));
	m_appList->setToolTip(tr("The list of applications"));
	m_appList->setWhatsThis(tr("This is the current list of applications.  Click one of these to select it."));
	m_appState->setToolTip(tr("The running state of the application"));
	m_appState->setWhatsThis(tr("This field shows the current state of the application, typically "
		"<i>Running</i> or <i>Not running</i>."));
	m_lastStarted->setToolTip(tr("The time/date the last time this application was started"));
	m_lastStarted->setWhatsThis(tr("This field shows the date and time that the application last entered "
		"the <i>Running</i> state."));
	m_lastExited->setToolTip(tr("The time/date the last time this application exited"));
	m_lastExited->setWhatsThis(tr("This field shows the date and time that the application last entered "
		"the <i>Not running</i> state."));
	m_restarts->setToolTip(tr("The number of times the app has been restarted"));
	m_restarts->setWhatsThis(tr("This is the number of times that Pinhole has restarted this application "
		"because it is configured to keep this app running and either it terminated unexpectedly or it "
		"is configured to enforce timeout heartbeats and did not receive a heartbeat within the "
		"timeout period."));
	m_appExecutable->setToolTip(tr("The name of the program to execute"));
	m_appExecutable->setWhatsThis(tr("This is where you enter the the program to run.  If it does not contain the "
		"path the location of the execuable the path will be used to find it.  If a path is included, it "
		"should be the full qualified path without quotes and may include spaces and the file extension."));
	m_appArguments->setToolTip(tr("The arguments passed to the application"));
	m_appArguments->setWhatsThis(tr("This is where you enter the arguments that are passed to the program.  "
		"Arguments with spaces should be enclosed in double quotes (<b>\"</b>) and for empty arguments use "
		"double double quotes (<b>\"\"</b>).  To pass a quote use tripple quotes(<b>\"\"\"</b>).  "
		/*To pass a quote in an argument escape the quote using a backslash.*/
		"<br>You can include environment variables in the arguments using the format %VARIABLENAME%.  "
		"Use %% to pass a % sign."));
	m_appDirectory->setToolTip(tr("The initial working directory used to launch the application"));
	m_appDirectory->setWhatsThis(tr("This is where you enter the working directory for the application to "
		"be run in.  This is usually same directory the program file is located but in some cases (such as "
		"node.js) it needs to be the directory containing the project or data files."));
	m_launchDisplay->setToolTip(tr("How the application window show initially be shown (if supported)"));
	m_launchDisplay->setWhatsThis(tr("This sets the initial size or visibility of the window of the application.  "
		"  Unless it is hidden (in which case it will never be visible on the desktop) the widow can be "
		"repositioned after it is running.  GUI applications may not be written correctly to actually behave as "
		"instructed by this option.<p>This will probably only work on Windows."));
	m_launchDelay->setToolTip(tr("Application launch delay in milliseconds"));
	m_launchDelay->setWhatsThis(tr("When this application is requested to start (or at startup) this delay "
		"will occur before the application is started.  This can be useful to give other applications a "
		"chance to start first."));
	m_launchAtStart->setToolTip(tr("Launch this application when Pinhole Server starts"));
	m_launchAtStart->setWhatsThis(tr("Checking this tells Pinhole to launch this application when Pinhole "
		"starts (usually with the system).  If this is not checked the application must be started by a "
		"scheduled event or manually."));
	m_keepAppRunning->setToolTip(tr("Restart this application if it terminates"));
	m_keepAppRunning->setWhatsThis(tr("Checking this tells Pinhole to re-launch this application if it "
		"terminates, such as a crash.  On Windows for this to be effective you must disable the JIT "
		"debugger."));
	m_terminatePrev->setToolTip(tr("Terminate existing processing of this executable when app starts"));
	m_terminatePrev->setWhatsThis(tr("If checked, when the Pinhole application is started any existing"
		"instances of the executable will be terminated."));
	m_softTerminate->setToolTip(tr("Attempt to allow process to exit on its own before forcing termination"));
	m_softTerminate->setWhatsThis(tr("When checked Pinhole will terminate the application by first requesting "
		"the application to exit by sending it a message; SIGTERM on Unix/Linux/Mac and WM_CLOSE on Windows.  If the "
		"program does not exit on its own in the time specified in <b>App terminate timeout</b> on the "
		"<b>Global</b> tab it will be forcibly terminated (SIGKILL on Unix/Linux/Mac, TerminateProcess on Windows).  "
		"This should NOT be checked for Windows console applications running in the background."));
	m_noCrashThrottle->setToolTip(tr("Do not stop restarting this app because of too many crashes"));
	m_noCrashThrottle->setWhatsThis(tr("If this is enabled Pinhole will ignore the <b>Crash throttle period</b> "
		"and <b>Crash throttle max</b> rule for this application."));
	m_lockupScreenshot->setToolTip(tr("Save a screenshot when an application locks up"));
	m_lockupScreenshot->setWhatsThis(tr("When a program has <b>Enforce timeout heartbeats</b> enabled and "
		"fails to send heartbeats in the allowed time, this option tells Pinhole to save an image of the "
		"screen to disk before stopping or restarting the application."));
	m_consoleCapture->setToolTip(tr("Writes the output of console applications to a file on the server"));
	m_consoleCapture->setWhatsThis(tr("When this is checked for console applications, stdout and stderr "
		"are redirected to a file stored in the data directory on the server.  This file can be useful "
		"when remotely debugging console applications.  The file is overwritten every time the application "
		"is restarted unless <b>append capture</b> is enabled.  If this is checked console applications"
		"will be run hidden.<br>"
		"While this option can be toggled while the application is running, on Windows it will only have an "
		"effect if the program was set to running as 'hidden' when it was started.  Toggling this option while "
		"an application is running will clear the contents of the log if <b>append capture</b> is not enabled."));
	m_appendCapture->setToolTip(tr("Append console output"));
	m_appendCapture->setWhatsThis(tr("When console output is started, either when an application is started "
		"or if capturing is toggled while an application is running, the contents of the previous capture log "
		"is cleared unless this settings is enabled."));
	m_tcpLoopback->setToolTip(tr("Listen for TCP loopback connections from the application"));
	m_tcpLoopback->setWhatsThis(tr("Checking this will cause Pinhole to create a listening TCP port for the "
		"application to connect to.  This connection can be used for sending heartbeats and reporting errors.") + loopbackHtml);
	m_tcpLoopbackPort->setToolTip(tr("The TCP port to listen for connections from the application on"));
	m_tcpLoopbackPort->setWhatsThis(tr("This is where you set the port for the TCP loopback for this application.  "
		"Each application that has TCP loopback enabled must use a unique port.  This value is exported to the "
		"application in an environment variable named <b>" APPENVVAR_APPTCPPORT "</b>.") + loopbackHtml);
	m_heartbeats->setToolTip(tr("Restart the app if heartbeats are not received within a timeout period"));
	m_heartbeats->setWhatsThis(tr("Checking this tells Pinhole to enforce heartbeat timeout detection.  If a "
		"heartbeats are not received from the application within the timeout defined in <b>App heartbeat "
		"timeout</b> field on the <b>Global</b> tab the application is assumed to have locked up and will "
		"be terminated and restarted.  Heartbeats can be sent via the TCP loopback, the logging named pipe "
		"or via the HTTP interface.") + loopbackHtml + httpHtml + loggingHtml);
	m_environmentList->setToolTip(tr("Environment variables passed to the application"));
	m_environmentList->setWhatsThis(tr("This is a list of environment variables that will be passed to the "
		"application.  You can edit the values by double clicking on them.  The should be in the form "
		"<b>NAME=VALUE</b>.  You can select multiple entries using the <b>shift</b> and <b>control</b> keys "
		"and mouse in order to delete multiple entries at once.  Right click anywhere on the list to "
		"bring up the menu to add a new entry or remove the selected entries.") + environmentHtml);
}


HostConfigAppsWidget::~HostConfigAppsWidget()
{
}


void HostConfigAppsWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
}


void HostConfigAppsWidget::resetWidgets()
{
	m_appList->clear();
	resetAppWidgets();
}

void HostConfigAppsWidget::resetAppWidgets()
{
	for (auto widget : m_widgetMap.values())
		ClearWidget(widget);
}


void HostConfigAppsWidget::enableWidgets(bool enable)
{
	m_appList->setEnabled(enable);
	m_addAppButton->setEnabled(enable && m_editable);

	bool itemSelected = m_appList->currentItem() != nullptr;

	m_startAppButton->setEnabled(enable && itemSelected);
	m_stopAppButton->setEnabled(enable && itemSelected);
	m_getConsoleOutput->setEnabled(enable && itemSelected);
	m_deleteAppButton->setEnabled(enable && itemSelected && m_editable);
	m_renameAppButton->setEnabled(enable && itemSelected && m_editable);

	EnableWidgetMap(m_widgetMap, enable && itemSelected && m_editable);
}


void HostConfigAppsWidget::appList_currentItemChanged()
{
	resetAppWidgets();

	QString newAppName;
	QListWidgetItem* currentItem = m_appList->currentItem();
	if (currentItem)
		newAppName = currentItem->text();

	enableWidgets(true);

	emit appChanged(newAppName);
}


bool HostConfigAppsWidget::event(QEvent* ev)
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


