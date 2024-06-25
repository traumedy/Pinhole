#include "HostAppWatchDialog.h"
#include "WindowManager.h"
#include "Global.h"
#include "../common/HostClient.h"

#include <QPushButton>
#include <QGridLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QEvent>

HostAppWatchDialog::HostAppWatchDialog(const QString& hostAddr, int port, const QString& hostId, QWidget *parent)
	: QDialog(parent), m_hostAddr(hostAddr), m_hostId(hostId)
{
	m_hostClient = new HostClient(hostAddr, port, hostId, false, false, this);

	setWindowFlag(Qt::WindowMinimizeButtonHint, true);
	setAttribute(Qt::WA_DeleteOnClose, true);

	setWindowTitle(tr("Applications on ") + hostAddr);

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setMargin(0);

	m_appList = new QTableWidget;
	m_appList->setWhatsThis(tr("This is the list of applications for this host.  "
		"You can use the start/stop buttons to control the application."));
	gridLayout->addWidget(m_appList);

	m_appList->setEnabled(false);
	m_appList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_appList->setSelectionMode(QAbstractItemView::NoSelection);

	connect(m_hostClient, &HostClient::valueUpdate,
		this, &HostAppWatchDialog::hostValueChanged);
	connect(m_hostClient, &HostClient::connected,
		this, &HostAppWatchDialog::hostConnected);
	connect(m_hostClient, &HostClient::disconnected,
		this, &HostAppWatchDialog::hostDisconnected);

	m_resizeTimer.setInterval(500);
	m_resizeTimer.setSingleShot(true);
	connect(&m_resizeTimer, &QTimer::timeout,
		this, [this]()
	{
		if (m_appList->rowCount() > 0)
		{
			int xSize = 6 + m_appList->verticalHeader()->width();
			for (int col = 0; col < m_appList->columnCount(); col++)
			{
				xSize += m_appList->columnWidth(col);
			}

			int ySize = 6 + m_appList->horizontalHeader()->height();
			for (int row = 0; row < m_appList->rowCount(); row++)
			{
				ySize += m_appList->rowHeight(row);
			}

			resize(xSize, ySize);
		}
	});
	m_resizeTimer.start();

	WindowManager::addWindow(this);
	WindowManager::cascadeWindow(this);
}


HostAppWatchDialog::~HostAppWatchDialog()
{
	WindowManager::removeWindow(this);
	emit close(m_hostId);
}


void HostAppWatchDialog::hostValueChanged(const QString& group, const QString& item, const QString& property, const QVariant& value)
{
	QStringList appProperties = { PROP_APP_STATE, PROP_APP_LASTSTARTED, PROP_APP_LASTEXITED, PROP_APP_RESTARTS, PROP_APP_RUNNING };
	QStringList appHeaders = { tr("State"), tr("Last started"), tr("Last exited"), tr("Restarts"), tr("Start/Stop") };

	if (GROUP_APP == group)
	{
		if (PROP_APP_LIST == property && item.isEmpty())
		{
			QStringList appNames = value.toStringList();
			m_appList->clear();
			m_appList->setColumnCount(appProperties.length());
			m_appList->setRowCount(appNames.length());
			m_appList->setHorizontalHeaderLabels(appHeaders);
			m_appList->setVerticalHeaderLabels(appNames);

			// Add the start/stop buttons
			for (int row = 0; row < appNames.length(); row++)
			{
				QPushButton* button = new QPushButton("Start/stop", m_appList);
				button->setProperty(PROPERTY_ITEMNAME, appNames[row]);
				connect(button, &QPushButton::clicked,
					this, &HostAppWatchDialog::startStopButtonClicked);
				m_appList->setCellWidget(row, appProperties.lastIndexOf(PROP_APP_RUNNING), button);
			}

			// Request all the properties for all the apps
			for (const auto& app : appNames)
			{
				for (const auto& prop : appProperties)
				{
					m_hostClient->getVariant(GROUP_APP, app, prop);
				}
			}
		}
		else
		{
			if (appProperties.contains(property))
			{
				// Wish there was a more effecient way of finding the item
				for (int row = 0; row < m_appList->rowCount(); row++)
				{
					if (item == m_appList->verticalHeaderItem(row)->text())
					{
						int column = appProperties.indexOf(property);
						if (PROP_APP_RUNNING == property)
						{
							QPushButton* button = qobject_cast<QPushButton*>(m_appList->cellWidget(row, column));
							button->setText(value.toBool() ? "Stop" : "Start");
							button->setProperty(PROPERTY_BUTTONSTATE, value);
						}
						else
						{
							QTableWidgetItem* tableItem = new QTableWidgetItem;
							tableItem->setText(value.toString());
							m_appList->setItem(row, column, tableItem);
						}
						break;
					}
				}
			}
		}
	}
}


void HostAppWatchDialog::hostConnected()
{
	m_hostClient->subscribeToCommand(CMD_VALUE);
	m_hostClient->subscribeToGroup(GROUP_APP);

	m_appList->setEnabled(true);
	m_hostClient->getVariant(GROUP_APP, "", PROP_APP_LIST);
}


void HostAppWatchDialog::hostDisconnected()
{
	m_appList->setEnabled(false);
}


void HostAppWatchDialog::startStopButtonClicked()
{
	QObject* obj = sender();
	QString appName = obj->property(PROPERTY_ITEMNAME).toString();

	if (obj->property(PROPERTY_BUTTONSTATE).toBool())
	{
		m_hostClient->stopApps(QStringList(appName));
	}
	else
	{
		m_hostClient->startApps(QStringList(appName));
	}
}


void HostAppWatchDialog::changeEvent(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::WindowStateChange:
		// Override minimize
		if (windowState() & Qt::WindowMinimized)
		{
			setWindowState(windowState() & ~Qt::WindowMinimized);
			hide();
		}
		break;

	default:
		// Avoid warnings
		break;
	}
}

