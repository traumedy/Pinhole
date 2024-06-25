#include "TrayManager.h"
#include "IdWindow.h"
#include "LogDialog.h"
#include "../common/HostClient.h"
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include "Utilities_X11.h"
#endif

#include <QDebug>
#include <QPixmap>
#include <QCoreApplication>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QBuffer>


TrayManager::TrayManager(QObject *parent)
	: QObject(parent)
{
	m_hostClient = QSharedPointer<HostClient>::create("127.0.0.1", HOST_TCPPORT, QString(), true, true, this);
	connect(m_hostClient.data(), &HostClient::connected,
		this, &TrayManager::hostConnected);
	connect(m_hostClient.data(), &HostClient::disconnected,
		this, &TrayManager::hostDisconnected);
	connect(m_hostClient.data(), &HostClient::valueUpdate,
		this, &TrayManager::hostValueChanged);
	connect(m_hostClient.data(), &HostClient::commandScreenshot,
		this, [this]()
	{
		m_hostClient->sendScreenshot(takeScreenshot());
	});
	connect(m_hostClient.data(), &HostClient::commandShowScreenIds,
		this, []()
	{
		IdWindow::showIdWindows();
	});
	connect(m_hostClient.data(), &HostClient::commandControlWindow,
		this, [](int pid, const QString& display, const QString& command)
	{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
		if (!ControlX11Window(display, pid, command))
		{
			qDebug() << "ControlX11Window failed" << pid << display << command;
		}
#else
		Q_UNUSED(pid);
		Q_UNUSED(display);
		Q_UNUSED(command);
#endif
	});
	connect(m_hostClient.data(), &HostClient::terminationRequested,
		this, []()
	{
		QCoreApplication::quit();
	});

	if (!QSystemTrayIcon::isSystemTrayAvailable())
	{
		qDebug() << "System tray not available";
	}
	else
	{
		QIcon icon(":/PinholeHelper/Resources/ODLogo.ico");
		if (icon.isNull())
		{
			qDebug() << "Logo is null";
		}

		m_trayIcon = new QSystemTrayIcon(icon);

		m_trayIcon->setToolTip("Pinhole");

		m_trayMenu = new QMenu();
		m_appMenu = new QMenu(tr("Applications"));
		m_appMenu->addAction(tr("(No apps)"))->setEnabled(false);
		m_groupMenu = new QMenu(tr("Groups"));
		m_groupMenu->addAction(tr("(No groups)"))->setEnabled(false);
		buildTrayMenu(false, false);
		m_trayIcon->setContextMenu(m_trayMenu);

		m_trayIcon->show();
	}

	m_logDialog = QSharedPointer<LogDialog>::create(m_hostClient.data());
}


TrayManager::~TrayManager()
{
	if (nullptr != m_trayIcon)
		delete m_trayIcon;
}


void TrayManager::hostConnected()
{
	m_hostClient->subscribeToCommand(CMD_VALUE);
	m_hostClient->subscribeToCommand(CMD_SCREENSHOT);
	m_hostClient->subscribeToCommand(CMD_CONTROLWINDOW);
	m_hostClient->subscribeToGroup(GROUP_APP);
	m_hostClient->subscribeToGroup(GROUP_GROUP);
	m_hostClient->subscribeToGroup(GROUP_GLOBAL);
	if (nullptr != m_trayIcon)
	{
		m_hostClient->getVariant(GROUP_APP, "", PROP_APP_LIST);
		m_hostClient->getVariant(GROUP_GROUP, "", PROP_GROUP_LIST);
		m_hostClient->getVariant(GROUP_GLOBAL, "", PROP_GLOBAL_TRAYCONTROL);
	}
}


void TrayManager::hostDisconnected()
{
	if (nullptr == m_trayIcon)
		return;

	buildTrayMenu(false, false);
}


void TrayManager::hostValueChanged(const QString& group, const QString& item, const QString& property, const QVariant& value)
{
	if (nullptr == m_trayIcon)
		return;

	if (GROUP_APP == group)
	{
		if (PROP_APP_LIST == property)
		{
			m_appMenu->clear();

			QStringList appList = value.toStringList();

			if (appList.isEmpty())
			{
				m_appMenu->addAction(tr("(No applications"))->setEnabled(false);
			}
			else
			{
				for (const auto& appName : appList)
				{
					m_hostClient->getVariant(GROUP_APP, appName, PROP_APP_RUNNING);

					QMenu* appMenu = m_appMenu->addMenu(appName);
					QAction* startAction = appMenu->addAction(tr("Start app"));
					connect(startAction, &QAction::triggered,
						this, [this, appName]()
					{
						m_hostClient->startApps(QStringList(appName));
					});
					QAction* stopAction = appMenu->addAction(tr("Stop app"));
					connect(stopAction, &QAction::triggered,
						this, [this, appName]()
					{
						m_hostClient->stopApps(QStringList(appName));
					});

					m_appActionMap[appName] = QPair<QAction*, QAction*>(startAction, stopAction);
				}
			}
		}
		else if (PROP_APP_RUNNING == property)
		{
			if (m_appActionMap.contains(item))
			{
				bool running = value.toBool();
				auto actions = m_appActionMap[item];
				actions.first->setEnabled(!running);
				actions.second->setEnabled(running);
			}
		}
	}
	else if (GROUP_GROUP == group)
	{
		if (PROP_GROUP_LIST == property)
		{
			m_groupMenu->clear();

			QStringList groupList = value.toStringList();

			if (groupList.isEmpty())
			{
				m_groupMenu->addAction(tr("(No groups)"))->setEnabled(false);
			}
			else
			{
				for (const auto& groupName : groupList)
				{
					QMenu* groupMenu = m_groupMenu->addMenu(groupName);
					QAction* startAction = groupMenu->addAction(tr("Start group"));
					connect(startAction, &QAction::triggered,
						this, [this, groupName]()
					{
						m_hostClient->startGroup(groupName);
					});
					QAction* stopAction = groupMenu->addAction(tr("Stop group"));
					connect(stopAction, &QAction::triggered,
						this, [this, groupName]()
					{
						m_hostClient->stopGroup(groupName);
					});
				}
			}
		}
	}
	else if (GROUP_GLOBAL == group)
	{
		if (PROP_GLOBAL_TRAYCONTROL == property)
		{
			bool showIcon = value.toBool();
			buildTrayMenu(true, showIcon);
		}
	}
}


void TrayManager::buildTrayMenu(bool connected, bool allowControl)
{
	if (nullptr == m_trayIcon)
		return;
	
	m_trayMenu->clear();
	m_trayMenu->addAction(connected ? tr("(Connected)") : tr("(Not connected)"))->setEnabled(false);

	// Tooltips for these menu actions seem to not work

	if (connected)
	{
		if (allowControl)
		{
			m_trayMenu->addMenu(m_appMenu);
			m_trayMenu->addMenu(m_groupMenu);
			QAction* startupAppsAction = m_trayMenu->addAction(tr("Start startup apps"));
			startupAppsAction->setToolTip(tr("Starts all applications marked to start at startup"));
			connect(startupAppsAction, &QAction::triggered,
				this, [this]()
			{
				m_hostClient->startStartupApps();
			});
			QAction* stopAppsAction = m_trayMenu->addAction(tr("Stop all apps"));
			stopAppsAction->setToolTip(tr("Stops all running applications"));
			connect(stopAppsAction, &QAction::triggered,
				this, [this]()
			{
				m_hostClient->stopAllApps();
			});
		}

		QAction* logAction = m_trayMenu->addAction(tr("Log message"));
		logAction->setToolTip(tr("Opens the log message dialog. Ctrl-Shift-F2 will also open this dialog."));
		connect(logAction, &QAction::triggered,
			this, &TrayManager::showLogDialog);
	}

#ifdef QT_DEBUG
	QAction* exitAction = m_trayMenu->addAction(tr("Exit"));
	exitAction->setToolTip(tr("Exit Pinhole Helper.  If Pinhole Server is running it will be restarted."));
	connect(exitAction, &QAction::triggered,
		this, []()
	{
		QCoreApplication::quit();
	});
#endif
}


QByteArray TrayManager::takeScreenshot()
{
	QList<QScreen*> sysScreens = QGuiApplication::screens();

	// Get the overall max dimensions
	int left = 0;
	int right = 0;
	int top = 0;
	int bottom = 0;
	for (const auto& screen : sysScreens)
	{
		qreal ratio = screen->devicePixelRatio();
		QRect rect = screen->geometry();
		left = qMin(static_cast<int>(rect.left() * ratio), left);
		top = qMin(static_cast<int>(rect.top() * ratio), top);
		right = qMax(static_cast<int>((rect.right() + 1) * ratio), right);
		bottom = qMax(static_cast<int>((rect.bottom() + 1) * ratio), bottom);
	}

	// Allocate a pixmap for the total combination of screens
	QPixmap totalPixmap((right - left), bottom - top);
	QPainter painter(&totalPixmap);

	// Grab each screen and copy into the total pixmap
	for (const auto& screen : sysScreens)
	{
		QPixmap screenPixmap = screen->grabWindow(0);
		qreal ratio = screen->devicePixelRatio();
		QRect rect = screen->geometry();
		painter.drawPixmap(static_cast<int>(rect.left() * ratio) - left, 
				static_cast<int>(rect.top() * ratio) - top, screenPixmap);
	}

	// Convert pixmap to PNG byte array
	QByteArray byteArray;
	QBuffer buff(&byteArray);
	buff.open(QIODevice::WriteOnly);
	totalPixmap.save(&buff, "PNG");
	return byteArray;
}


void TrayManager::showLogDialog()
{
	if (m_hostClient->isConnected())
	{
		m_logDialog->setVisible(true);
		m_logDialog->activateWindow();
		m_logDialog->raise();
	}
	else
	{
		QMessageBox::information(nullptr, QGuiApplication::applicationDisplayName(),
			tr("Not connected to server"));
	}
}

