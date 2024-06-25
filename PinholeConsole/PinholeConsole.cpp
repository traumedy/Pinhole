/* PinholeConsole.cpp - Main application class */

#include "PinholeConsole.h"
#include "HostFinder.h"
#include "HostViewWidget.h"
#include "HostConfigWidget.h"
#include "HostScanDialog.h"
#include "HostAppWatchDialog.h"
#include "HostLogDialog.h"
#include "WindowManager.h"
#include "Settings.h"
#include "Global.h"
#include "GuiUtil.h"
#include "../common/Utilities.h"

#include <QApplication>
#include <QSplitter>
#include <QWhatsThis>
#include <QTableWidget>
#include <QShortcut>
#include <QDesktopServices>
#include <QMessageBox>
#include <QLabel>
#include <QStatusBar>
#include <QGuiApplication>
#include <QToolBar>
#include <QGuiApplication>
#include <QScreen>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QWhatsThisClickedEvent>
#include <QSortFilterProxyModel>
#include <QMenu>


#define VALUE_DARKTHEME			"darkTheme"		// Dark UI theme enabled
#define VALUE_AUTOSEARCH		"autoSearch"	// Auto-search for new hosts enabled
#define VALUE_LASTDIRCHOSEN		"lastHostListDirectoryChosen"	// Last host list directory chosen
#define STYLE_SPLITTERLIGHT		"QSplitter::handle {\
				background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0,\
				stop:0 rgba(255, 255, 255, 0),\
				stop:0.3 rgba(200, 200, 200, 255),\
				stop:0.5 rgba(220, 220, 255, 235),\
				stop:0.7 rgba(200, 200, 200, 255),\
				stop:1 rgba(255, 255, 255, 0));\
				image: url(:/PinholeConsole/Resources/splitter.png);\
			}"
#define STYLE_SPLITTERDARK 		"QSplitter::handle {\
			background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0,\
				stop:0 rgba(255, 255, 255, 0),\
				stop:0.4 rgba(180, 180, 180, 255),\
				stop:0.5 rgba(233, 200, 200, 235),\
				stop:0.6 rgba(180, 180, 180, 255),\
				stop:1 rgba(255, 255, 255, 0));\
				image: url(:/PinholeConsole/Resources/splitter.png);\
			}"


PinholeConsole::PinholeConsole(QWidget *parent)
	: QMainWindow(parent)
{
	QToolBar* mainToolBar = new QToolBar(this);
	addToolBar(mainToolBar);

	setWindowTitle(QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion());

	QSplitter* splitter = new QSplitter(Qt::Vertical);

	// Add the top host view widget
	hostViewWidget = new HostViewWidget(splitter);
	splitter->addWidget(hostViewWidget);

	m_hostFinder = new HostFinder(this);

	// Use QSortFilterProxyModel to allow sorting by column
	m_proxyModel = new QSortFilterProxyModel(this);
	m_proxyModel->setSourceModel(m_hostFinder);
	hostViewWidget->m_hostList->setModel(m_proxyModel);
	// Set initial sort
	m_proxyModel->sort(0, Qt::AscendingOrder);

	// Add the bottom view widget
	hostConfigWidget = new HostConfigWidget(splitter);
	splitter->addWidget(hostConfigWidget);

	setCentralWidget(splitter);
	splitter->setSizes(QList<int>{50, 50});

	hostScanDialog = new HostScanDialog(m_hostFinder, this);

	windowManagerDialog = new WindowManager(this);

	// Notify this class when an item in the QTableView selection changes
	connect(hostViewWidget->m_hostList->selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &PinholeConsole::hostListSelectionChanged);

	// Notify host config widget that the selected host changed
	connect(this, &PinholeConsole::hostChanged,
		hostConfigWidget, &HostConfigWidget::hostChanged);

	QAction* windowManagerAction = new QAction(QIcon(":/PinholeConsole/Resources/window.ico"),
		tr("Window manager"));
	windowManagerAction->setToolTip(tr("Window manager"));
	windowManagerAction->setWhatsThis(tr("This button opens a dialog that allows arranging and closing of child windows."));
	connect(windowManagerAction, &QAction::triggered,
		this, &PinholeConsole::windowManagerAction_triggered);
	mainToolBar->addAction(windowManagerAction);

	QAction* clearAction = new QAction(QIcon(":/PinholeConsole/Resources/erase.ico"), 
		tr("Clear hosts"));
	clearAction->setToolTip(tr("Clear host list entries"));
	clearAction->setWhatsThis(tr("If no host list entries are selected this button will clear the host list, otherwise it "
		"will remove the selected entries."));
	connect(clearAction, &QAction::triggered,
		this, &PinholeConsole::clearAction_triggered);
	mainToolBar->addAction(clearAction);

	QAction* loadHostsAction = new QAction(QIcon(":/PinholeConsole/Resources/doc_import.ico"),
		tr("Load host list"));
	loadHostsAction->setToolTip(tr("Load host list"));
	loadHostsAction->setWhatsThis(tr("Load host list from previously saved JSON file"));
	connect(loadHostsAction, &QAction::triggered,
		this, &PinholeConsole::loadHostsAction_triggered);
	mainToolBar->addAction(loadHostsAction);

	QAction* saveHostsAction = new QAction(QIcon(":/PinholeConsole/Resources/doc_export.ico"),
		tr("Save host list"));
	saveHostsAction->setToolTip(tr("Save host list"));
	saveHostsAction->setWhatsThis(tr("Save the current host list to a JSON file"));
	connect(saveHostsAction, &QAction::triggered,
		this, &PinholeConsole::saveHostsAction_triggered);
	mainToolBar->addAction(saveHostsAction);

	QAction* scanAction = new QAction(QIcon(":/PinholeConsole/Resources/Icons8-Windows-8-Security-Iris-Scan.ico"), 
		tr("Scan for hosts"));
	scanAction->setToolTip(tr("Find additional hosts"));
	scanAction->setWhatsThis(tr("This button opens a dialog allowing you to send query packets to IP addresses "
		"that are not in broadcast range of your computer (in a different subnet or across a VPN for instance) "
		"to add them to the host list."));
	connect(scanAction, &QAction::triggered,
		this, &PinholeConsole::scanAction_triggered);
	mainToolBar->addAction(scanAction);

	QAction* autoSearchAction = new QAction(QIcon(":/PinholeConsole/Resources/binocs.ico"),
		tr("Auto-search for hosts"));
	autoSearchAction->setCheckable(true);
	autoSearchAction->setToolTip(tr("Toggle host auto-search"));
	autoSearchAction->setWhatsThis(tr("This button toggles automatically searching for additional hosts.  While "
		"this is checked, PinholeServer hosts within IPv4 broadcast or IPv6 multicast range of any network "
		"interface on the local host should be discovered and will be added to the host list."));
	connect(autoSearchAction, &QAction::triggered,
		this, [this](bool checked)
	{
		m_hostFinder->enableScanning(checked);

		// Immediately save settings
		QSettings settings;
		settings.setValue(VALUE_AUTOSEARCH, checked);
	});
	mainToolBar->addAction(autoSearchAction);

	QAction* monitorAction = new QAction(QIcon(":/PinholeConsole/Resources/Custom-Icon-Design-Mono-General-4-Eye.ico"),
		tr("Monitor hosts"));
	monitorAction->setToolTip(tr("Monitor hosts"));
	monitorAction->setWhatsThis(tr("This button opens a dialog for each selected host containing a table showing "
		"the configured applications and their current state information as well as a button allowing you to start/"
		"stop them.  The table is updated in realtime."));
	connect(monitorAction, &QAction::triggered,
		this, &PinholeConsole::monitorAction_triggered);
	mainToolBar->addAction(monitorAction);

	QAction* watchlogAction = new QAction(QIcon(":/PinholeConsole/Resources/Icons8-Ios7-Household-Log-Cabin.ico"),
		tr("Watch host log"));
	watchlogAction->setToolTip(tr("Monitor the host log in realtime"));
	watchlogAction->setWhatsThis(tr("This button opens a dialog for each selected host that will show log events "
		"generted by the hosts in realtime.  This is useful for debugging purposes.  You can control the level "
		"detail of these log messages using the <b>Remote log level</b> setting in the <b>Global</b> tab."));
	connect(watchlogAction, &QAction::triggered,
		this, &PinholeConsole::actionWatch_log_triggered);
	mainToolBar->addAction(watchlogAction);

	QString customActionHtml = LocalHtmlDoc(DOC_CUSTOMACTIONS, tr("Details on custom action buttons"));
	QAction* addButtonAction = new QAction(QIcon(":/PinholeConsole/Resources/add_button.ico"),
		tr("Add custom action"));
	addButtonAction->setToolTip(tr("Add a custom action button to the host list"));
	addButtonAction->setWhatsThis(tr("This button will open a dialog allowing you to create custom buttons that "
		"act on the currently selected hosts in the host list.") + customActionHtml);
	connect(addButtonAction, &QAction::triggered,
		hostViewWidget, &HostViewWidget::addCustomButtonClicked);
	mainToolBar->addAction(addButtonAction);

	QAction* themeAction = new QAction(QIcon(":/PinholeConsole/Resources/bulb_dark.ico"),
		tr("Enable dark mode"));
	themeAction->setCheckable(true);
	themeAction->setToolTip(tr("Enable dark UI mode"));
	themeAction->setWhatsThis(tr("This button toggles a 'dark' or normal theme for the Pinhole Console interface.  "
		"The state will be saved and restored the next time Pinhole Console is run."));
	connect(themeAction, &QAction::triggered,
		this, [this, themeAction, splitter](bool checked)
	{
		if (checked)
		{
			splitter->setStyleSheet(STYLE_SPLITTERDARK);
			themeAction->setToolTip(tr("Enable normal UI mode"));
			themeAction->setIcon(QIcon(":/PinholeConsole/Resources/bulb_light.ico"));
		}
		else
		{
			splitter->setStyleSheet(STYLE_SPLITTERLIGHT);
			themeAction->setToolTip(tr("Enable dark UI mode"));
			themeAction->setIcon(QIcon(":/PinholeConsole/Resources/bulb_dark.ico"));
		}

		setStyle(checked);

		// Immediately save settings
		QSettings settings;
		settings.setValue(VALUE_DARKTHEME, checked);
	});
	mainToolBar->addAction(themeAction);
	
	QAction* helpAction = new QAction(QIcon(":/PinholeConsole/Resources/help.ico"),
		tr("Help"));
	helpAction->setToolTip(tr("Help"));
	helpAction->setWhatsThis(tr("Opens html help in an external browser"));
	helpAction->setShortcut(QKeySequence::HelpContents);
	connect(helpAction, &QAction::triggered,
		this, [this]()
	{
		QUrl helpUrl = QUrl::fromLocalFile(ApplicationBaseDir() + DOC_PINHOLE);
		if (!QDesktopServices::openUrl(helpUrl))
		{
			QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
				tr("Failed to open URL: %1")
				.arg(helpUrl.toString()));
		}
	});
	mainToolBar->addAction(helpAction);
	mainToolBar->addAction(QWhatsThis::createAction(mainToolBar));

	m_noticeLabel = new QLabel;
	statusBar()->addPermanentWidget(m_noticeLabel);
	connect(hostConfigWidget, &HostConfigWidget::setStatusText,
		this, [this](const QString& text)
	{
		// Checking for isVisible prevents crash on termination for some reason
		if (isVisible())
		{
			statusBar()->showMessage(text);
		}
	}); 
	connect(hostConfigWidget, &HostConfigWidget::setNoticeText,
		this, &PinholeConsole::setNoticeText);
	connect(hostViewWidget, &HostViewWidget::setNoticeText,
		this, &PinholeConsole::setNoticeText);
	connect(hostViewWidget, &HostViewWidget::clearNoticeText,
		this, &PinholeConsole::clearNoticeText);

	// Popup menu for host list items
	hostViewWidget->m_hostList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(hostViewWidget->m_hostList, &QTableView::customContextMenuRequested,
		this, [this](const QPoint& pos)
	{
		auto itemIndex = hostViewWidget->m_hostList->indexAt(pos);
		if (itemIndex.isValid())
		{
			QString id = itemIndex.data(HOSTROLE_ID).toString();
			auto addressList = m_hostFinder->getHostAddressList(id);
			QMenu* menu = new QMenu;
#if defined(QT_DEBUG)
			QMenu* idMenu = menu->addMenu(id);
#endif
			QMenu* preferredMenu = menu->addMenu(tr("Address"));
			for (const auto& address : addressList)
			{
				QMenu* addressMenu = preferredMenu->addMenu(QString("%1 port %2")
					.arg(address.first)
					.arg(address.second));
				addressMenu->addAction(tr("Set preferred address"),
					[this, id, address]()
				{
					m_hostFinder->setHostPreferredAddress(id, address.first, address.second);
				});
				addressMenu->addAction(tr("Remove address"),
					[this, id, address]()
				{
					m_hostFinder->removeHostAddress(id, address.first, address.second);
				});
			}

			menu->addAction(tr("Remove host"),
				[this, id]
			{
				m_hostFinder->eraseEntry(id);
			});

			menu->exec(hostViewWidget->m_hostList->mapToGlobal(pos));
		}
	});

	// Resize main window to be wide enough for tab widgets
	resize(MAINWINDOW_WIDTH, height());
	// Center main window on desktop
	QRect r = geometry();
	r.moveCenter(QGuiApplication::primaryScreen()->availableGeometry().center());
	setGeometry(r);

	// Set initial theme
	QSettings settings;
	bool darkTheme = settings.value(VALUE_DARKTHEME, false).toBool();
	themeAction->setChecked(darkTheme);
	if (darkTheme)
		themeAction->setIcon(QIcon(":/PinholeConsole/Resources/bulb_light.ico"));
	splitter->setStyleSheet(darkTheme ? STYLE_SPLITTERDARK : STYLE_SPLITTERLIGHT);
	setStyle(darkTheme);

	// Set auto-searching
	bool autoSearch = settings.value(VALUE_AUTOSEARCH, false).toBool();
	m_hostFinder->enableScanning(autoSearch);
	autoSearchAction->setChecked(autoSearch);

	m_lastHostListDirectoryChosen = settings.value(VALUE_LASTDIRCHOSEN).toString();
}


PinholeConsole::~PinholeConsole()
{
	// Clear these before auto-destruct
	m_appWatchMap.clear();
	m_logWatchMap.clear();
}


void PinholeConsole::hostListSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
	Q_UNUSED(selected);
	Q_UNUSED(deselected);

	QModelIndexList selection = hostViewWidget->m_hostList->selectionModel()->selectedRows();
	
	hostViewWidget->enableButtons(!selection.isEmpty());

	int hostsSelected = selection.length();

	QString hostAddress;
	int port = 0;
	QString hostId;
	if (1 == hostsSelected)
	{
		hostAddress = selection[0].data(HOSTROLE_ADDRESS).toString();
		port = selection[0].data(HOSTROLE_PORT).toInt();
		hostId = selection[0].data(HOSTROLE_ID).toString();
	}

	// Assume the possibility that the text fields may not be accurate

	if (hostAddress.isEmpty() || hostId.isEmpty())
	{
		if (hostsSelected > 0)
		{
			statusBar()->showMessage(tr("%1 hosts selected").arg(hostsSelected));
		}
		else
		{
			statusBar()->showMessage(tr("No hosts selected"));
		}
	}
	else
	{
		statusBar()->showMessage(tr("%1 selected").arg(hostAddress));
	}

	emit hostChanged(hostAddress, port, hostId);
}


void PinholeConsole::windowManagerAction_triggered()
{
	windowManagerDialog->show();
	windowManagerDialog->raise();
	windowManagerDialog->activateWindow();
}


void PinholeConsole::scanAction_triggered()
{
	QModelIndexList selection = hostViewWidget->m_hostList->selectionModel()->selectedRows();
	int hostsSelected = selection.length();
	if (1 == hostsSelected)
	{
		QString hostAddress = selection[0].data(HOSTROLE_ADDRESS).toString();
		hostScanDialog->setAddress(hostAddress);
	}

	hostScanDialog->show();
	hostScanDialog->raise();
	hostScanDialog->activateWindow();
}


void PinholeConsole::loadHostsAction_triggered()
{
	QFileDialog dialog(this, tr("Load Host List JSON"));
	dialog.setDirectory(GetSaveDirectory(m_lastHostListDirectoryChosen, { QStandardPaths::DocumentsLocation, QStandardPaths::DesktopLocation }));
	dialog.setNameFilters({ "JSON (*.json)" });
	dialog.setDefaultSuffix("json");
	dialog.setFileMode(QFileDialog::ExistingFile);

	if (QDialog::Accepted != dialog.exec())
		return;

	QString fileName = dialog.selectedFiles().first();
	m_lastHostListDirectoryChosen = QFileInfo(fileName).path();
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
			tr("Cannot open %1: %2")
			.arg(QDir::toNativeSeparators(fileName))
			.arg(file.errorString()));
		return;
	}

	QByteArray data = file.readAll();
	file.close();
	QJsonDocument jdoc = jdoc.fromJson(data);
	if (jdoc.isNull())
	{
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
			tr("Unable to parse JSON data in %1")
			.arg(QDir::toNativeSeparators(fileName)));
		return;
	}
	
	m_hostFinder->importHostList(jdoc.object());
}


void PinholeConsole::saveHostsAction_triggered()
{
	QJsonObject hostList = m_hostFinder->exportHostList();
	QFileDialog dialog(this, tr("Save Host List JSON as"));
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDirectory(GetSaveDirectory(m_lastHostListDirectoryChosen, { QStandardPaths::DocumentsLocation, QStandardPaths::DesktopLocation }));
	dialog.setNameFilters({ "JSON (*.json)" });
	dialog.setDefaultSuffix("json");

	QByteArray data = QJsonDocument(hostList).toJson();

	QString defaultFilename = tr("Pinhole-HostList-%1.json").arg(currentDateTimeFilenameString());
	dialog.selectFile(defaultFilename);
	int result;
	while ((result = dialog.exec()) == QDialog::Accepted && !WriteFile(this, dialog.selectedFiles().first(), data)) 
	{
		m_lastHostListDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
		dialog.setDirectory(m_lastHostListDirectoryChosen);
	}

	if (QDialog::Accepted == result)
		m_lastHostListDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
}


void PinholeConsole::clearAction_triggered()
{
	QModelIndexList indexList = hostViewWidget->m_hostList->selectionModel()->selectedIndexes();

	if (indexList.isEmpty())
	{
		m_hostFinder->clear();
		hostViewWidget->enableButtons(false);
	}
	else
	{
		QStringList idsToRemove;
		for (const auto& index : indexList)
		{
			if (index.column() == 0)
			{
				idsToRemove.push_back(index.data(HOSTROLE_ID).toString());
			}
		}

		for (const auto& id : idsToRemove)
			m_hostFinder->eraseEntry(id);

		hostViewWidget->m_hostList->selectionModel()->clear();
	}
}


void PinholeConsole::monitorAction_triggered()
{
	QModelIndexList indexList = hostViewWidget->m_hostList->selectionModel()->selectedIndexes();

	for (const auto& index : indexList)
	{
		if (index.column() == 0)
		{
			QString id = index.data(HOSTROLE_ID).toString();
			QString addr = index.data(HOSTROLE_ADDRESS).toString();
			int port = index.data(HOSTROLE_PORT).toInt();

			if (!m_appWatchMap.contains(id))
			{
				m_appWatchMap[id] = QSharedPointer<HostAppWatchDialog>(new HostAppWatchDialog(addr, port, id, this), &QObject::deleteLater);
				connect(m_appWatchMap[id].data(), &HostAppWatchDialog::close,
					this, &PinholeConsole::appWatchDialogClose);
			}

			m_appWatchMap[id]->show();
			m_appWatchMap[id]->raise();
			m_appWatchMap[id]->activateWindow();
		}
	}
}


void PinholeConsole::appWatchDialogClose(QString hostId)
{
	if (m_appWatchMap.contains(hostId))
	{
		m_appWatchMap.remove(hostId);
	}
	else
	{
		setNoticeText(tr("Internal error: Missing appWatchDialog id %1")
			.arg(hostId), 0);
	}
}


void PinholeConsole::actionWatch_log_triggered()
{
	QModelIndexList indexList = hostViewWidget->m_hostList->selectionModel()->selectedIndexes();

	for (const auto& index : indexList)
	{
		if (index.column() == 0)
		{
			QString id = index.data(HOSTROLE_ID).toString();
			QString addr = index.data(HOSTROLE_ADDRESS).toString();
			int port = index.data(HOSTROLE_PORT).toInt();

			if (!m_logWatchMap.contains(id))
			{
				m_logWatchMap[id] = QSharedPointer<HostLogDialog>(new HostLogDialog(addr, port, id, this), &QObject::deleteLater);
				connect(m_logWatchMap[id].data(), &HostLogDialog::close,
					this, &PinholeConsole::logWatchDialogClose);
			}

			m_logWatchMap[id]->show();
			m_logWatchMap[id]->raise();
			m_logWatchMap[id]->activateWindow();
		}
	}
}


void PinholeConsole::logWatchDialogClose(QString hostId)
{
	if (m_logWatchMap.contains(hostId))
	{
		m_logWatchMap.remove(hostId);
	}
	else
	{
		setNoticeText(tr("Internal error: Missing logWatchDialog id %1")
			.arg(hostId), 0);
	}
}


void PinholeConsole::setNoticeText(const QString& text, int id)
{
	if (id == 0)
	{
		m_noticeLabel->setText(text);
		if (!m_noticeMessages.isEmpty())
		{
			QTimer::singleShot(FREQ_NOTICEUPDATE, this, &PinholeConsole::updateNoticeText);
		}
	}
	else
	{
		m_noticeMessages[id] = text;
		updateNoticeText();
	}
}


void PinholeConsole::clearNoticeText(int id)
{
	if (m_noticeMessages.contains(id))
	{
		m_noticeMessages.remove(id);
	}

	updateNoticeText();
}


void PinholeConsole::updateNoticeText()
{
	if (m_noticeMessages.empty())
		m_noticeLabel->clear();
	else if (1 == m_noticeMessages.size())
		m_noticeLabel->setText(m_noticeMessages.begin().value());
	else
		m_noticeLabel->setText(QString("%1 (+%2)").arg(m_noticeMessages.begin().value()).arg(m_noticeMessages.size() - 1));

}


void PinholeConsole::setStyle(bool dark)
{
	if (!dark)
	{
		// Regular theme
		qApp->setStyleSheet("\
			QPushButton[custom=\"true\"] {\
				border: 1px solid #8f8f91;\
				min-height: 20px;\
				border-radius: 4px;\
				background-color: qlineargradient(x1 : 0, y1 : 0, x2 : 0, y2 : 1,\
					stop: 0 #a0a0a0, stop: 1 #d0d0d0);\
				min-width: 80px;\
			}\
			QPushButton[custom=\"true\"]:hover {\
				background-color: qlineargradient(x1 : 0, y1 : 0, x2 : 0, y2 : 1,\
					stop: 0 #a0a0d0, stop: 1 #a0daf0);\
			}\
			QPushButton[custom=\"true\"]:pressed {\
				background-color: qlineargradient(x1 : 0, y1 : 0, x2 : 0, y2 : 1,\
					stop: 0 #707070, stop: 1 #909090);\
			}\
			QTabWidget::pane { \
				border-width: 0px; \
			} \
			");
	}
	else
	{
		qApp->setStyleSheet("\
			* { \
				color: white; \
				background-color: black; \
				border: 1px solid grey; \
			} \
			*:disabled { \
				color: grey; \
			} \
			*[borderless=\"true\"] { \
				border-color: transparent; \
			} \
			QToolTip { \
				color: #aabbee; \
				border: 1px dot-dash #eeaaaa; \
				background-color: #333333; \
				opacity: 200; \
			} \
			QToolButton { \
				border: 2px solid #3f3f31; \
				border-radius: 4px; \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #76777a, stop: 1 #4a4b4e); \
			} \
			QToolButton:pressed { \
				background-color: qlineargradient(x1: 0, y1 : 0, x2: 0, y2: 1, \
					stop: 0 #dadbde, stop: 1 #f6f7fa); \
			} \
			QToolButton:checked { \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #909090, stop: 1 #606060); \
			} \
			QPushButton { \
				min-height: 20px; \
				border-radius: 4px; \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #26272a, stop: 1 #4a4b4e); \
				min-width: 80px; \
				padding-left: 3px; \
				padding-right: 3px; \
			} \
			QPushButton:hover { \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #404040, stop: 1 #606060); \
			} \
			QPushButton:pressed { \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #000000, stop: 1 #202020); \
			} \
			QPushButton:default { \
				border-color: #ee9999; \
			} \
			QPushButton[custom=\"true\"] { \
				border-style: outset; \
				border-width: 1px; \
				border-color: grey; \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #606060, stop: 1 #404040); \
			} \
			QPushButton[custom=\"true\"]:hover { \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #505050, stop: 1 #707a70); \
			} \
			QPushButton[custom=\"true\"]:pressed { \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
					stop: 0 #000000, stop: 1 #202020); \
			} \
			QHeaderView::section { \
				background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, \
				stop: 0 #616161, stop: 0.5 #505050, \
				stop: 0.6 #434343, stop:1 #656565); \
				color: white; \
				padding-left: 4px; \
				border: 1px solid #6c6c6c; \
			} \
			QHeaderView::section:selected, QHeaderView::section:checked { \
				background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, \
				stop: 0 #6161FF, stop: 0.5 #5050FF, \
				stop: 0.6 #434377, stop:1 #656599); \
			} \
			QTableCornerButton::section { \
				background-color: black; \
			} \
			QTabBar::tab { \
				background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, \
				stop:0 #616161, stop: 0.5 #505050, \
				stop: 0.6 #434343, stop:1 #656565); \
				color: white; \
				border: 2px solid #C4C4C3; \
				padding: 2px; \
				min-width: 18ex; \
			} \
			QTabBar::tab:selected, QTabBar::tab:hover { \
				background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, \
				stop : 0 #0a0a0a, stop: 0.4 #040404, \
				stop: 0.5 #171717, stop: 1.0 #0a0a0a); \
			} \
			QTabBar::tab:!selected { \
				margin-top: 2px; \
			} \
			QListView::item:selected:!active { \
				color: white; \
				background-color: #002030; \
			} \
			QLabel { \
				border-color: transparent; \
			} \
			QCheckBox { \
				border-color: transparent; \
			} \
			QRadioButton { \
				border-color: transparent; \
			} \
			QTableView { \
				background-color: black; \
			} \
			QTabWidget::pane { \
				border-width: 0px; \
			} \
			QTabBar { \
				border-top-color: transparent; \
				border-left-color: transparent; \
				border-right-color: transparent; \
			} \
			QSizeGrip { \
				border-left-color: transparent; \
				border-top-color: transparent; \
				width: 8px; \
				height: 8px; \
				image: url(:/PinholeConsole/Resources/sizegrip.png) \
			} \
			");
	}
}


bool PinholeConsole::event(QEvent* ev)
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
