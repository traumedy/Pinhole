#include "HostViewWidget.h"
#include "ScreenviewDialog.h"
#include "TextViewerDialog.h"
#include "DateTimeStartStopDialog.h"
#include "Global.h"
#include "ItemDelegateNoFocusRect.h"
#include "CustomActionDialog.h"
#include "GuiUtil.h"
#include "RemoteExecuteDialog.h"
#include "ImportFilterDialog.h"
#include "StartAppVarsDialog.h"
#include "../common/HostClient.h"
#include "../common/Utilities.h"

#include <QResizeEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMutex>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QDesktopServices>
#include <QScrollArea>
#include <QGuiApplication>
#include <QTableView>
#include <QPushButton>
#include <QApplication>
#include <QHeaderView>
#include <QSettings>
#include <QMenu>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QNetworkDatagram>

#define PREFIX_ACTION			"action_"
#define VALUE_ACTIONLIST		"customActions"
#define VALUE_GROUP				"group"
#define VALUE_COMMAND			"command"
#define VALUE_ARGUMENT			"argument"
#define VALUE_LASTDIRCHOSEN		"lastSettingsDirectoryChosen"


HostViewWidget::HostViewWidget(QWidget *parent)
	: QWidget(parent)
{
	QSettings settings;
	m_lastSettingsDirectoryChosen = settings.value(VALUE_LASTDIRCHOSEN).toString();

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setAlignment(Qt::AlignAbsolute);
	ScrollAreaEx* buttonScrollArea = new ScrollAreaEx;
	buttonScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
	mainLayout->addWidget(buttonScrollArea);

	QWidget* scrollAreaWidgetContents = new QWidget;
	scrollAreaWidgetContents->setProperty("borderless", true);

	m_buttonLayout = new QVBoxLayout(scrollAreaWidgetContents);
	m_buttonLayout->setAlignment(Qt::AlignTop);
	m_buttonLayout->setSizeConstraint(QLayout::SetFixedSize);
	m_startStartupAppsButton = new QPushButton(tr("Start startup apps"));
	connect(m_startStartupAppsButton, &QPushButton::clicked,
		this, [this]() { execServerCommandOnEachSelectedHost(tr("start startup apps"),
			[](HostClient* hostClient) { hostClient->startStartupApps(); }); });
	m_pushButtons.append(m_startStartupAppsButton);
	m_stopAllAppsButton = new QPushButton(tr("Stop all apps"));
	connect(m_stopAllAppsButton, &QPushButton::clicked,
		this, [this]() { execServerCommandOnEachSelectedHost(tr("stop all apps"),
			[](HostClient* hostClient) { hostClient->stopAllApps(); }); });
	m_pushButtons.append(m_stopAllAppsButton);
	m_retrieveLogButton = new QPushButton(tr("Retrieve log"));
	connect(m_retrieveLogButton, &QPushButton::clicked,
		this, &HostViewWidget::retrieveLogButton_clicked);
	m_pushButtons.append(m_retrieveLogButton);
	m_showScreenIdsButton = new QPushButton(tr("Show screen IDs"));
	connect(m_showScreenIdsButton, &QPushButton::clicked,
		this, [this]() { execServerCommandOnEachSelectedHost(tr("show screen IDs"),
			[](HostClient* hostClient) { hostClient->showScreenIds(); }); });
	m_pushButtons.append(m_showScreenIdsButton);
	m_screenshotButton = new QPushButton(tr("View screenshot"));
	connect(m_screenshotButton, &QPushButton::clicked,
		this, [this]() { execServerCommandOnEachSelectedHost(tr("screenshot"),
			[](HostClient* hostClient) { hostClient->requestScreenshot(); }); });
	m_pushButtons.append(m_screenshotButton);
	m_sysinfoButton = new QPushButton(tr("System information"));
	connect(m_sysinfoButton, &QPushButton::clicked,
		this, [this]() { execServerCommandOnEachSelectedHost(tr("view system info"),
			[](HostClient* hostClient) { hostClient->retrieveSystemInfo(); }); });
	m_pushButtons.append(m_sysinfoButton);
	m_executeCommand = new QPushButton(tr("Execute command"));
	connect(m_executeCommand, &QPushButton::clicked,
		this, &HostViewWidget::executeCommand_clicked);
	m_pushButtons.append(m_executeCommand);
	m_startAppVariables = new QPushButton(tr("Start app with variables"));
	connect(m_startAppVariables, &QPushButton::clicked,
		this, &HostViewWidget::startAppVariables_clicked);
	m_pushButtons.append(m_startAppVariables);
	m_shutdownButton = new QPushButton(tr("Shutdown"));
	connect(m_shutdownButton, &QPushButton::clicked,
		this, &HostViewWidget::shutdownButton_clicked);
	m_pushButtons.append(m_shutdownButton);
	m_rebootButton = new QPushButton(tr("Reboot"));
	connect(m_rebootButton, &QPushButton::clicked,
		this, &HostViewWidget::rebootButton_clicked);
	m_pushButtons.append(m_rebootButton);
	m_setHostPassword = new QPushButton(tr("Set password"));
	connect(m_setHostPassword, &QPushButton::clicked,
		this, &HostViewWidget::setHostPassword_clicked);
	m_pushButtons.append(m_setHostPassword);
	m_exportSettingsButton = new QPushButton(tr("Export settings"));
	connect(m_exportSettingsButton, &QPushButton::clicked,
		this, [this]() { execServerCommandOnEachSelectedHost(tr("export settings"),
			[](HostClient* hostClient) { hostClient->requestExportSettings(); }); });
	m_pushButtons.append(m_exportSettingsButton);
	m_importSettingsButton = new QPushButton(tr("Import settings"));
	connect(m_importSettingsButton, &QPushButton::clicked,
		this, &HostViewWidget::importSettingsButton_clicked);
	m_pushButtons.append(m_importSettingsButton);
	m_wakeOnLanButton = new QPushButton(tr("Wake-on-LAN"));
	m_pushButtons.append(m_wakeOnLanButton);
	connect(m_wakeOnLanButton, &QPushButton::clicked,
		this, &HostViewWidget::wakeOnLanButton_clicked);

	for (const auto& button : m_pushButtons)
	{
		button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
		m_buttonLayout->addWidget(button);
	}

	// Add custom buttons added by the user and saved from previous run
	readCustomActions();

	buttonScrollArea->setWidget(scrollAreaWidgetContents);
	buttonScrollArea->show();

	// Buttons initially disabled
	enableButtons(false);

	// Configure host list
	m_hostList = new QTableView(this);
	connect(m_hostList, &QTableView::doubleClicked,
		this, &HostViewWidget::hostList_doubleClicked);
	m_hostList->setSortingEnabled(true);
	m_hostList->setShowGrid(false);
	m_hostList->verticalHeader()->setVisible(false);
	m_hostList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_hostList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_hostList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_hostList->setItemDelegate(new ItemDelegateNoFocusRect());
	mainLayout->addWidget(m_hostList);

	QString settingsHtml = LocalHtmlDoc(DOC_SETTINGS, tr("Details about importing settings JSON"));

	//m_hostList->setToolTip(tr("The list of discovered hosts"));
	m_hostList->setWhatsThis(tr("This list contains all the hosts that have responded to queries.  The list is "
		"saved between executions.  You can sort the list by clicking on the column headers.<br>"
		"Selecting one host in the list will alow you to configure its settings in the lower pane.<br>"
		"You can select multiple hosts using the <b>control</b> and <b>shift</b> keys to perform the "
		"button operations on the left or the toolbar operations on multiple hosts at once.<br>"
		"If the host is password protected you will be prompted when you select a host or attempt an operation.<br>"
		"If a host is available on multiple network interfaces (you will see +# after the address) you can right click "
		"to select which address to connect on.<br>"
		"The <b>Last heard</b> column indicates the last time the remote host responded to a query packet.  Query "
		"packets are regularly sent to all hosts in the host list.<br>"
		"Hosts that have not been seen in the last " QT_STRINGIFY(SECS_MINHIGHLIGHTHOST) " seconds are hashed out."));
	m_startStartupAppsButton->setToolTip(tr("Starts all applications marked to launch at startup on the selected hosts"));
	m_startStartupAppsButton->setWhatsThis(tr("When this button is clicked, any application that has <b>Launch this "
		"application at start</b> will be started on each of the selected hosts."));
	m_stopAllAppsButton->setToolTip(tr("Stops all applictions on the selected hosts"));
	m_stopAllAppsButton->setWhatsThis(tr("When this button is clicked all running applications will be stopped on each "
		"of the selected hosts."));
	m_retrieveLogButton->setToolTip(tr("Retrieve Pinhole log from remote hosts"));
	m_retrieveLogButton->setWhatsThis(tr("This button retrieves the Pinhole log file from the remote hosts and opens them in "
		"a text viewer window."));
	m_showScreenIdsButton->setToolTip(tr("Identify screens onselected  hosts"));
	m_showScreenIdsButton->setWhatsThis(tr("This button will tell the selected hosts to display identifying information on "
		"each display monitor.  The information displayed will include the host name, monitor name, monitor virtual position, "
		"monitor resolution, and the list of curent IP addresses of the host.<p>The informtion will be overlayed on top of the "
		"existing screen and the screens will be automatically restored after 15 seconds."));
	m_screenshotButton->setToolTip(tr("View screenshots of the selected hosts"));
	m_screenshotButton->setWhatsThis(tr("This button will attempt to retrieve an image of the currently selected remote host's "
		"screens.  There will be a delay after the button is clicked while the screenshot is generated and transfered before "
		"the images are displayed.  If an error occurs you will be notified.  Once the image has been displayed you can save "
		"the image to the local computer."));
	m_sysinfoButton->setToolTip(tr("View system information for the selected hosts"));
	m_sysinfoButton->setWhatsThis(tr("This button will retrieve a report with various pieces of system information "
		"from each of the selected hosts."));
	m_executeCommand->setToolTip(tr("Execute a command on selected hosts"));
	m_executeCommand->setWhatsThis(tr("This button allows you to execute a command on remote hosts either by uploading a file "
		"or executing a program that already exists on the remote hosts."));
	m_shutdownButton->setToolTip(tr("Shutdown the selected hosts"));
	m_shutdownButton->setWhatsThis(tr("This button allows you to power down the hosts currently selected in the <b>host list</b>. "
		"You will be prompted with a confirmation dialog with a list of the host addresses before the operation is performed."));
	m_rebootButton->setToolTip(tr("Reboot the selected hosts"));
	m_rebootButton->setWhatsThis(tr("This button allows you to restart the hosts currently selected in the <b>host list</b>. "
		"You will be prompted with a confirmation dialog with a list of the host addresses before the operation is performed."));
	m_setHostPassword->setToolTip(tr("Set or change the password for the selected hosts"));
	m_setHostPassword->setWhatsThis(tr("Hosts initially have no password allowing anyone to connect to them.  Use "
		"this button to add a password or change the password for a host.  You can set the same password for "
		"multiple hosts at once by selecting multiple hosts in the <b>host list</b> before pressing this button."));
	m_exportSettingsButton->setToolTip(tr("Export host settings to a file"));
	m_exportSettingsButton->setWhatsThis(tr("This button will retrieve the settings from the currently selected hosts and "
		"present you with a dialog to save the settings to a file locally.  You can then use the <b>Import settings</b> button "
		"to apply these settings to one or more hosts.") + settingsHtml);
	m_importSettingsButton->setToolTip(tr("Import host settings from a file"));
	m_importSettingsButton->setWhatsThis(tr("This button opens a dialog allowing you to select a file previously saved using "
		"the <b>Export settings</b> button which will be applied to the currently selected hosts.") + settingsHtml);
	m_wakeOnLanButton->setToolTip(tr("Send Wake-on-LAN to selected hosts"));
	m_wakeOnLanButton->setWhatsThis(tr("When clicked, Wake-on-LAN packets are sent for each of the selected hosts.  Packets are "
		"broadcast on 255.255.255.255 on each network interface, sent to the broarcast address of each address on the local host, "
		"and unicast to the last known IP address of the host computer."));
}


HostViewWidget::~HostViewWidget()
{
	QSettings settings;
	settings.setValue(VALUE_LASTDIRCHOSEN, m_lastSettingsDirectoryChosen);

	writeCustomActions();
}


void HostViewWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
}


void HostViewWidget::setHostPassword_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	bool ok;
	QString password = QInputDialog::getText(this, tr("Set password"),
		tr("Enter new password for selected hosts"), QLineEdit::EchoMode::Password,
		QString(), &ok);

	if (!ok)
	{
		return;
	}

	QList<QVariantList> hostList;
	for (const auto& index : indexList)
	{
		if (COL_NAME == index.column())
		{
			QVariantList items;
			items << index.data() << index.data(HOSTROLE_ADDRESS) << index.data(HOSTROLE_PORT) << index.data(HOSTROLE_ID);
			hostList.push_back(items);
		}
	}

	QString titleText;
	QString questionText;
	if (password.isEmpty())
	{
		titleText = tr("Clear password?");
		questionText = tr("Remove the password from the following hosts?");
	}
	else
	{
		titleText = tr("Change password?");
		questionText = tr("Change the password of the following hosts?");
	}
	if (QMessageBox::Yes != QMessageBox::question(this, titleText,
		questionText + "\n" + hostVariantListToString(hostList), QMessageBox::Yes | QMessageBox::No))
		return;

	for (const auto& host : hostList)
	{
		QString hostName = host[0].toString();
		QString addr = host[1].toString();
		int port = host[2].toInt();
		QString id = host[3].toString();
		execServerCommand(addr, port, id, hostName, tr("set password"),
			[password](HostClient* hostClient) { hostClient->setPassword(password); });
	}
}


void HostViewWidget::shutdownButton_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	QList<QVariantList> hostList;
	for (const auto& index : indexList)
	{
		if (COL_NAME == index.column())
		{
			QVariantList items;
			items << index.data() << index.data(HOSTROLE_ADDRESS) << index.data(HOSTROLE_PORT) << index.data(HOSTROLE_ID);
			hostList.push_back(items);
		}
	}

	if (QMessageBox::Yes != QMessageBox::question(this, tr("Shutdown hosts?"),
		tr("Shutdown the following hosts?\n%1").arg(hostVariantListToString(hostList)), QMessageBox::Yes | QMessageBox::No))
		return;

	for (const auto& host : hostList)
	{
		QString hostName = host[0].toString();
		QString addr = host[1].toString();
		int port = host[2].toInt();
		QString id = host[3].toString();
		execServerCommand(addr, port, id, hostName, tr("shutdown"),
			[](HostClient* hostClient) { hostClient->shutdownHost(); });
	}
}


void HostViewWidget::rebootButton_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	QList<QVariantList> hostList;
	for (const auto& index : indexList)
	{
		if (COL_NAME == index.column())
		{
			QVariantList items;
			items << index.data() << index.data(HOSTROLE_ADDRESS) << index.data(HOSTROLE_PORT) << index.data(HOSTROLE_ID);
			hostList.push_back(items);
		}
	}

	if (QMessageBox::Yes != QMessageBox::question(this, tr("Reboot hosts?"),
		tr("Reboot the following hosts?\n%1")
		.arg(hostVariantListToString(hostList)), QMessageBox::Yes | QMessageBox::No))
		return;

	for (const auto& host : hostList)
	{
		QString hostName = host[0].toString();
		QString addr = host[1].toString();
		int port = host[2].toInt();
		QString id = host[3].toString();
		execServerCommand(addr, port, id, hostName, tr("reboot"),
			[](HostClient* hostClient) { hostClient->rebootHost(); });
	}
}


void HostViewWidget::importSettingsButton_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	QByteArray jsonData = loadSettingsFile();

	if (jsonData.isEmpty())
	{
		return;
	}

	// Allow the user a chance to modify what is being imported
	ImportFilterDialog dialog(this);
	if (QDialog::Accepted != dialog.exec(jsonData))
	{
		if (dialog.error())
		{
			QMessageBox::warning(this, tr("Error parsing settings JSON"), 
				tr("There was an error parsing the settings JSON file or it contained no import settings."));
		}

		return;
	}
	jsonData = dialog.jsonData();

	QList<QVariantList> hostList;
	for (const auto& index : indexList)
	{
		if (COL_NAME == index.column())
		{
			QVariantList items;
			items << index.data() << index.data(HOSTROLE_ADDRESS) << index.data(HOSTROLE_PORT) << index.data(HOSTROLE_ID);
			hostList.push_back(items);
		}
	}

	if (QMessageBox::Yes != QMessageBox::question(this, tr("Import settings to hosts?"),
		tr("Import settings to the following hosts?\n%1")
		.arg(hostVariantListToString(hostList)), QMessageBox::Yes | QMessageBox::No))
		return;

	for (const auto& host : hostList)
	{
		QString hostName = host[0].toString();
		QString addr = host[1].toString();
		int port = host[2].toInt();
		QString id = host[3].toString();
		execServerCommand(addr, port, id, hostName, tr("import settings"),
			[jsonData](HostClient* hostClient) { hostClient->sendImportData(jsonData); });
	}
}


void HostViewWidget::wakeOnLanButton_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	for (const auto& index : indexList)
	{
		if (COL_ADDRESS == index.column())
		{
			QString addr = index.data().toString();
			QString MAC = index.siblingAtColumn(COL_MAC).data().toString();
			sendWakeOnLan(addr, MAC);
		}
	}
}


void HostViewWidget::retrieveLogButton_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	DateTimeStartStopDialog dialog(this);

	if (QDialog::Accepted != dialog.exec())
	{
		return;
	}

	QDateTime startDate = dialog.getStartDate();
	QDateTime endDate = dialog.getEndDate();

	for (const auto& index : indexList)
	{
		if (COL_NAME == index.column())
		{
			QString hostName = index.data().toString();
			QString addr = index.data(HOSTROLE_ADDRESS).toString();
			int port = index.data(HOSTROLE_PORT).toInt();
			QString id = index.data(HOSTROLE_ID).toString();
			execServerCommand(addr, port, id, hostName, tr("retrieve log"),
				[startDate, endDate](HostClient* hostClient) 
			{ 
				hostClient->retrieveLog(
					startDate.toString(DATETIME_STRINGFORMAT), 
					endDate.toString(DATETIME_STRINGFORMAT));
			});
		}
	}
}


void HostViewWidget::executeCommand_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	RemoteExecuteDialog remoteExecuteDialog(this);

	if (QDialog::Accepted != remoteExecuteDialog.exec())
		return;

	QByteArray file = remoteExecuteDialog.file();
	QString command = remoteExecuteDialog.command();
	QString args = remoteExecuteDialog.commandLine();
	QString directory = remoteExecuteDialog.directory();
	bool captureOutput = remoteExecuteDialog.captureOutput();
	bool elevated = remoteExecuteDialog.elevated();

	QList<QPair<QString, QString>> hostList;
	for (const auto& index : indexList)
	{
		if (COL_ADDRESS == index.column())
		{
			hostList.append(QPair<QString, QString>(index.data().toString(), index.siblingAtColumn(COL_NAME).data().toString()));
		}
	}

	QString text;
	if (file.isNull() || file.isEmpty())
	{
		text = tr("Execute this command with this command line:\n%1 %2\n")
			.arg(command)
			.arg(args);
	}
	else
	{
		text = tr("Upload this file and execute with this command line:\n%1 %2\n")
			.arg(remoteExecuteDialog.filePath())
			.arg(args);
	}
	text.append(tr(" on the following hosts?\n%1").arg(hostPairListToString(hostList)));

	if (QMessageBox::Yes != QMessageBox::question(this, tr("Execute remote program?"),
		text, QMessageBox::Yes | QMessageBox::No))
		return;

	execServerCommandOnEachSelectedHost(tr("execute remote command"),
		[file, command, args, directory, captureOutput, elevated](HostClient* hostClient)
		{
			hostClient->executeCommand(file, command, args, directory, captureOutput, elevated);
		});
}


void HostViewWidget::startAppVariables_clicked()
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	StartAppVarsDialog dialog(this);

	if (QDialog::Accepted != dialog.exec())
		return;

	QString appName = dialog.appName();
	QStringList variableList = dialog.variableList();

	QList<QPair<QString, QString>> hostList;
	for (const auto& index : indexList)
	{
		if (COL_ADDRESS == index.column())
		{
			hostList.append(QPair<QString, QString>(index.data().toString(), index.siblingAtColumn(COL_NAME).data().toString()));
		}
	}

	QString text = tr("Start application '%1' with %2 variable%3 on the following hosts?\n%4")
		.arg(appName)
		.arg(variableList.length())
		.arg(variableList.length() == 1 ? "" : "s")
		.arg(hostPairListToString(hostList));

	if (QMessageBox::Yes != QMessageBox::question(this, tr("Start app with variables?"),
		text, QMessageBox::Yes | QMessageBox::No))
		return;

	execServerCommandOnEachSelectedHost(tr("start app with variables"),
		[appName, variableList](HostClient* hostClient)
		{
			hostClient->startAppVariables(appName, variableList);
		});
}


// 'Hidden' command to send terminate message to server (ctrl-shift-doubleclick)
void HostViewWidget::hostList_doubleClicked(const QModelIndex& index)
{
	Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

	if (modifiers & (Qt::ShiftModifier | Qt::ControlModifier))
	{
		QString addr = index.data(HOSTROLE_ADDRESS).toString();
		int port = index.data(HOSTROLE_PORT).toInt();
		QString id = index.data(HOSTROLE_ID).toString();
		QString hostName = index.siblingAtColumn(COL_NAME).data().toString();
		execServerCommand(addr, port, id, hostName, tr("terminate"),
			[](HostClient* hostClient) { hostClient->requestTerminate(); });
	}
}


// Executes a command on a server, sets notification message and clears it when response is received
void HostViewWidget::execServerCommand(const QString& hostAddress, int port, const QString& hostId,
	const QString& hostName, const QString& commandDescription, std::function<void(HostClient*)> func)
{
	unsigned int nid = NoticeId();
	emit setNoticeText(tr("Waiting for %1 from %2 (%3)...")
		.arg(commandDescription)
		.arg(hostName)
		.arg(hostAddress),
		nid);

	HostClient* hostClient = new HostClient(hostAddress, port, hostId, false, false, this);

	connect(hostClient, &HostClient::connected,
		this, [hostClient, func]()
		{
			func(hostClient);
		});

	connect(hostClient, &HostClient::disconnected,
		this, [this, hostClient, nid]()
		{
			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});

	connect(hostClient, &HostClient::connectFailed,
		this, [this, hostClient, commandDescription, nid](const QString& reason)
		{
			QMessageBox* messageBox = new QMessageBox(this);
			messageBox->setAttribute(Qt::WA_DeleteOnClose, true);
			messageBox->setText(tr("Failed to connect to host %1 to %2: %3")
				.arg(hostClient->getHostAddress())
				.arg(commandDescription)
				.arg(reason));
			messageBox->setWindowModality(Qt::NonModal);
			messageBox->show();
			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});

	connect(hostClient, &HostClient::commandSuccess,
		this, [this, hostClient, nid]()
		{
			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});

	connect(hostClient, &HostClient::commandError,
		this, [this, hostClient, commandDescription, nid]()
		{
			QMessageBox* messageBox = new QMessageBox(this);
			messageBox->setAttribute(Qt::WA_DeleteOnClose, true);
			messageBox->setText(tr("An error occured while attempting %1 on host %2 (%3)")
				.arg(commandDescription)
				.arg(hostClient->getHostName())
				.arg(hostClient->getHostAddress()));
			messageBox->setWindowModality(Qt::NonModal);
			messageBox->show();
			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});

	connect(hostClient, &HostClient::commandUnknown,
		this, [this, hostClient, commandDescription, nid]()
		{
			QMessageBox* messageBox = new QMessageBox(this);
			messageBox->setAttribute(Qt::WA_DeleteOnClose, true);
			messageBox->setText(tr("The host %1 (%2) is unable to %3 because it doesn't know that command (host version %4)")
				.arg(hostClient->getHostName())
				.arg(hostClient->getHostAddress())
				.arg(commandDescription)
				.arg(hostClient->getHostVersion()));
			messageBox->setWindowModality(Qt::NonModal);
			messageBox->show();
			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});

	connect(hostClient, &HostClient::commandMissing,
		this, [this, hostClient, commandDescription, nid]()
		{
			QMessageBox* messageBox = new QMessageBox(this);
			messageBox->setAttribute(Qt::WA_DeleteOnClose, true);
			messageBox->setText(tr("The host %1 (%2) is unable to %3 because it doesn't know that command (host version %4)")
				.arg(hostClient->getHostName())
				.arg(hostClient->getHostAddress())
				.arg(commandDescription)
				.arg(hostClient->getHostVersion()));
			messageBox->setWindowModality(Qt::NonModal);
			messageBox->show();
			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});

	connect(hostClient, &HostClient::commandData,
		this, [this, hostClient, nid](const QString& group, const QString& subCommand, const QVariant& data)
		{
			if (GROUP_NONE == group)
			{
				if (CMD_NONE_GETSCREENSHOT == subCommand)
				{
					ScreenviewDialog* screenviewWidget = new ScreenviewDialog(data.toByteArray(), 
						hostClient->getHostName(), hostClient->getHostAddress(), this);
					screenviewWidget->setAttribute(Qt::WA_DeleteOnClose, true);
					screenviewWidget->setWindowModality(Qt::NonModal);
					screenviewWidget->setVisible(true);
				}
				else if (CMD_NONE_EXPORTSETTINGS == subCommand)
				{
					saveSettingsFile(hostClient->getHostName(), hostClient->getHostAddress(), data.toByteArray());
				}
				else if (CMD_NONE_RETRIEVELOG == subCommand)
				{
					TextViewerDialog* textViewer = new TextViewerDialog(tr("Log"), hostClient->getHostAddress(),
						hostClient->getHostName(), data.toByteArray(), this);
					textViewer->show();
				}
			}
			else if (GROUP_APP == group)
			{
				if (CMD_APP_EXECUTE == subCommand)
				{
					TextViewerDialog* textViewer = new TextViewerDialog(tr("ExeOutput"), hostClient->getHostAddress(),
						hostClient->getHostName(), data.toByteArray(), this);
					textViewer->show();
				}
			}
			else if (GROUP_GLOBAL == group)
			{
				if (CMD_GLBOAL_SYSINFO == subCommand)
				{
					TextViewerDialog* textViewer = new TextViewerDialog(tr("Sysinfo"), hostClient->getHostAddress(),
						hostClient->getHostName(), data.toByteArray(), this);
					textViewer->show();
				}
			}

			hostClient->deleteLater();
			emit clearNoticeText(nid);
		});
}


void HostViewWidget::execServerCommandOnEachSelectedHost(const QString& failureMessage, std::function<void(HostClient*)> lambda)
{
	QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		return;
	}

	for (const auto& index : indexList)
	{
		if (COL_NAME == index.column())
		{
			QString hostName = index.data().toString();
			QString addr = index.data(HOSTROLE_ADDRESS).toString();
			int port = index.data(HOSTROLE_PORT).toInt();
			QString id = index.data(HOSTROLE_ID).toString();
			execServerCommand(addr, port, id, hostName, failureMessage, lambda);
		}
	}
}


void HostViewWidget::saveSettingsFile(const QString& hostName, const QString& hostAddress, const QByteArray& data)
{
	QFileDialog dialog(this, tr("Save Settings from %1 (%2) As")
		.arg(hostName)
		.arg(hostAddress));
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDirectory(GetSaveDirectory(m_lastSettingsDirectoryChosen, { QStandardPaths::DocumentsLocation, QStandardPaths::DesktopLocation }));
	dialog.setNameFilters({ "JSON (*.json)" });
	dialog.setDefaultSuffix("json");

	QString defaultFilename = tr("PinholeSettings-%1.json").arg(hostName);
	dialog.selectFile(FilenameString(defaultFilename));
	int result;
	while ((result = dialog.exec()) == QDialog::Accepted && !WriteFile(this, dialog.selectedFiles().first(), data)) 
	{
		m_lastSettingsDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
		dialog.setDirectory(m_lastSettingsDirectoryChosen);
	}

	if (QDialog::Accepted == result)
	{
		m_lastSettingsDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
	}
}


QByteArray HostViewWidget::loadSettingsFile()
{
	QFileDialog dialog(this, tr("Import Settings File"));

	dialog.setDirectory(GetSaveDirectory(m_lastSettingsDirectoryChosen, { QStandardPaths::DocumentsLocation, QStandardPaths::DesktopLocation }));
	dialog.setNameFilters({ "JSON (*.json)" });
	dialog.setDefaultSuffix("json");
	dialog.setFileMode(QFileDialog::ExistingFile);

	if (QDialog::Accepted != dialog.exec())
		return QByteArray();

	QString fileName = dialog.selectedFiles().first();
	m_lastSettingsDirectoryChosen = QFileInfo(fileName).path();
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
			tr("Cannot open %1: %2")
			.arg(QDir::toNativeSeparators(fileName))
			.arg(file.errorString()));
		return QByteArray();
	}

	QByteArray data = file.readAll();
	file.close();
	return data;
}


bool HostViewWidget::event(QEvent* ev)
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


void HostViewWidget::enableButtons(bool enable)
{
	m_buttonsEnabled = enable;

	for (const auto& button : m_pushButtons)
	{
		button->setEnabled(enable);
	}
}


void HostViewWidget::addCustomButtonClicked()
{
	CustomActionDialog actionDialog(this);

	// Set the list of previously used names so they aren't reused
	actionDialog.setUsedNames(m_customActionMap.keys());

	if (QDialog::Accepted == actionDialog.exec())
	{
		QString actionName = actionDialog.getCommandName();
		QVariantList actionVlist = actionDialog.getCommandVariantList();

		addCustomActionButton(actionName, actionVlist);
		m_customActionMap[actionName] = actionVlist;
	}
}


void HostViewWidget::addCustomActionButton(const QString& name, const QVariantList& vlist)
{
	QPushButton* button = new QPushButton(name);
	button->setToolTip(tr("Custom action"));

	QString actionString;
	actionString = vlist[2].toString() + "(";
	if (vlist[3].canConvert<QStringList>())
		actionString += vlist[3].toStringList().join(";");
	else
		actionString += vlist[3].toString();
	actionString += ")";

	button->setWhatsThis(tr("Custom action:<pre>%1</pre>Right click to rename or remove.")
		.arg(actionString));

	// Add to list of push buttons so it will get enabled/disabled
	m_pushButtons.append(button);
	// Set initial enabled/disalbed state
	button->setEnabled(m_buttonsEnabled);

	connect(button, &QPushButton::clicked,
		this, [this, name, vlist]()
	{
		QModelIndexList indexList = m_hostList->selectionModel()->selectedIndexes();

		if (indexList.isEmpty())
		{
			return;
		}

		for (const auto& index : indexList)
		{
			if (COL_NAME == index.column())
			{
				QString hostName = index.data().toString();
				QString addr = index.data(HOSTROLE_ADDRESS).toString();
				int port = index.data(HOSTROLE_PORT).toInt();
				QString id = index.data(HOSTROLE_ID).toString();
				execServerCommand(addr, port, id, hostName, tr("custom action '%1'").arg(name),
					[vlist](HostClient* hostClient) { hostClient->sendVariantList(vlist); });
			}
		}
	});

	button->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(button, &QPushButton::customContextMenuRequested,
		this, [this, name, vlist, button](const QPoint& pos)
	{
		QMenu* menu = new QMenu;
		menu->addAction(tr("Remove custom action button"), [this, name, button]()
		{
			m_pushButtons.removeAll(button);
			m_customActionMap.remove(name);
			delete button;
		});
		menu->addAction(tr("Rename custom action button"), [this, name, vlist, button]()
		{
			bool ok = false;
			QString newName = QInputDialog::getText(this, tr("Rename custom action button"),
				tr("Enter name for new custom action button"), QLineEdit::Normal,
				"", &ok);

			if (ok)
			{
				if (m_customActionMap.contains(newName))
				{
					QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
						tr("A custom action button with that name already exists"));
				}
				else if (newName.isEmpty())
				{
					QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
						tr("Name cannot be blank"));
				}
				else
				{
					button->setText(newName);
					m_customActionMap.remove(name);
					m_customActionMap[newName] = vlist;
				}
			}
		});
		menu->addAction(tr("More info on custom actions"), [this]()
		{
			QUrl url = QUrl::fromLocalFile(ApplicationBaseDir() + DOC_CUSTOMACTIONS);
			if (!QDesktopServices::openUrl(url))
			{
				QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
					tr("Failed to open URL: %1")
					.arg(url.toString()));
			}
		});

		menu->exec(button->mapToGlobal(pos));
	});

	// So custom style will be applied
	button->setProperty("custom", true);

	button->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	m_buttonLayout->insertWidget(0, button);
	m_buttonLayout->update();
}


void HostViewWidget::readCustomActions()
{
	QSettings settings;

	m_customActionMap.clear();

	QStringList keys = settings.value(VALUE_ACTIONLIST).toStringList();

	for (const auto& key : keys)
	{
		settings.beginGroup(PREFIX_ACTION + key);

		QVariant group = settings.value(VALUE_GROUP);
		QVariant command = settings.value(VALUE_COMMAND);
		QVariant argument = settings.value(VALUE_ARGUMENT);
		QVariantList vlist;
		vlist << CMD_COMMAND << group << command << argument;

		addCustomActionButton(key, vlist);
		m_customActionMap[key] = vlist;

		settings.endGroup();
	}
}


void HostViewWidget::writeCustomActions()
{
	QSettings settings;

	// Remove existing actions
	QStringList keys = settings.value(VALUE_ACTIONLIST).toStringList();
	for (const auto& key : keys)
	{
		settings.remove(PREFIX_ACTION + key);
	}

	for (const auto& key : m_customActionMap.keys())
	{
		settings.beginGroup(PREFIX_ACTION + key);
		QVariantList vlist = m_customActionMap[key];

		settings.setValue(VALUE_GROUP, vlist[1]);
		settings.setValue(VALUE_COMMAND, vlist[2]);
		settings.setValue(VALUE_ARGUMENT, vlist[3]);

		settings.endGroup();
	}

	settings.setValue(VALUE_ACTIONLIST, QStringList(m_customActionMap.keys()));
}


// Takes a MAC address string, sends a WOL packet
// If a machine has more than one network adapter, it is necessary to enumerate through them, 
//  bind to the local address on each one, and send the broadcast packet.  (I think, doesn't hurt certainly...)
void HostViewWidget::sendWakeOnLan(const QString& IpString, const QString& MacString)
{
	if (MacString.length() != 17)
	{
		return;
	}

	// Wake-on-LAN packet is 0xFF 6 times followed by MAC address 16 times

	// Split colon separated string into string list
	QStringList byteList = MacString.split(':');
	if (byteList.size() != 6)
	{
		return;
	}

	// Convert list of string numbers to binary QByteArray
	QByteArray MAC;
	for (const auto& byt : byteList)
	{
		bool ok;
		// QString doesn't have a toUChar()
		MAC += static_cast<unsigned char>(byt.toUShort(&ok, 16));
	}

	// Create magic packet
	QByteArray magicPacket(6, (char)0xff); // header is 0xFF 6 times
	for (int x = 0; x < 16; x++)	// followed by MAC address 16 times
		magicPacket += MAC;

	// IPv4 Broadcast on each interface and address
	for (const auto& iface : QNetworkInterface::allInterfaces())
	{
		if (iface.isValid() &&
			iface.flags().testFlag(QNetworkInterface::IsRunning) &&
			iface.flags().testFlag(QNetworkInterface::CanBroadcast) &&
			!iface.flags().testFlag(QNetworkInterface::IsLoopBack))
		{
			// Send to broadcast (255.255.255.255) on each interface
			QNetworkDatagram datagram;
			datagram.setData(magicPacket);
			datagram.setInterfaceIndex(iface.index());
			datagram.setDestination(QHostAddress::Broadcast, WAKEONLANPORT);
			{
				QUdpSocket socket;
				socket.writeDatagram(datagram);
			}

			// Send to address specific broadcast address on each address bound to interface 
			for (const auto& address : iface.addressEntries())
			{
				QUdpSocket socket;
				if (address.ip().protocol() == QAbstractSocket::IPv6Protocol)
				{
					// Send to all nodes in the interface-local and link-local multicast address
					// Not sure if this does anything but can't hurt
					socket.writeDatagram(magicPacket, QHostAddress("FF01:0:0:0:0:0:0:1"), WAKEONLANPORT);
					socket.writeDatagram(magicPacket, QHostAddress("FF02:0:0:0:0:0:0:1"), WAKEONLANPORT);
				}
				else
				{
					socket.writeDatagram(magicPacket, address.broadcast(), WAKEONLANPORT);
				}
			}
		}
	}

	// Unicast to IP address
	QUdpSocket socket;
	socket.writeDatagram(magicPacket, QHostAddress(IpString), WAKEONLANPORT);

	emit setNoticeText(tr("Wake-on-LAN sent to %1...").arg(IpString));
}


// Returns a formatted string from host address/name pair list
QString HostViewWidget::hostPairListToString(const QList<QPair<QString, QString>>& hostList)
{
	QString ret;

	for (const auto& host : hostList)
	{
		ret += QString("  %1 (%2)")
			.arg(host.second)
			.arg(host.first);
	}
	return ret;
}

// Returns a formatted string from host name/address/port variant list
QString HostViewWidget::hostVariantListToString(const QList<QVariantList>& hostList)
{
	QString ret;

	for (const auto& host : hostList)
	{
		ret += QString("  %1 (%2)")
			.arg(host[0].toString())
			.arg(host[1].toString());
	}
	return ret;
}

