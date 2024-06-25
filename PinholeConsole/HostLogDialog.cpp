#include "HostLogDialog.h"
#include "Global.h"
#include "WindowManager.h"
#include "GuiUtil.h"
#include "../common/HostClient.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QLayout>
#include <QAbstractItemView>
#include <QToolBar>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDesktopServices>
#include <QMessageBox>
#include <QGuiApplication>
#include <QWhatsThisClickedEvent>


HostLogDialog::HostLogDialog(const QString& hostAddr, int port, const QString& hostId, QWidget *parent)
	: QDialog(parent), m_hostAddr(hostAddr), m_hostId(hostId)
{
	m_hostClient = new HostClient(hostAddr, port, hostId, false, false, this);

	setWindowFlag(Qt::WindowMinimizeButtonHint, true);
	setAttribute(Qt::WA_DeleteOnClose, true);

	setWindowTitle(tr("Trying to connect to %1").arg(hostAddr));

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setMargin(0);

	m_toolBar = new QToolBar(this);
	m_toolBar->addAction("Clear", [this]() { m_logList->clear(); })->
		setWhatsThis(tr("This button will clear the log list."));
	QAction* scrollAction = m_toolBar->addAction("Auto scroll", [this]() 
	{ 
		m_autoScroll = !m_autoScroll; 
		if (m_autoScroll)
			m_logList->scrollToBottom();
	});
	scrollAction->setCheckable(true);
	scrollAction->setChecked(true);
	scrollAction->setWhatsThis(tr("This button enables/disables the automatic "
		"scrolling of the log list.  When this is enabled the list will "
		"automatically move to the bottom when a new message is received."));

	m_remoteLogLevel = new QComboBox;
	m_remoteLogLevel->setEditable(false);
	m_remoteLogLevel->setEnabled(false);
	m_remoteLogLevel->addItem(tr("Errors"), LOG_LEVEL_ERROR);
	m_remoteLogLevel->addItem(tr("Warnings"), LOG_LEVEL_WARNING);
	m_remoteLogLevel->addItem(tr("Normal"), LOG_LEVEL_NORMAL);
	m_remoteLogLevel->addItem(tr("Extra"), LOG_LEVEL_EXTRA);
	m_remoteLogLevel->addItem(tr("Debug"), LOG_LEVEL_DEBUG);
	m_remoteLogLevel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	m_remoteLogLevel->setToolTip(tr("The log level to display"));
	m_remoteLogLevel->setWhatsThis(tr("This controls the remote logging level.  It can also be controled "
		"from the Globals configuration tab."));
	m_toolBar->addWidget(m_remoteLogLevel);

	gridLayout->addWidget(m_toolBar);

	m_logList = new QListWidget(this);
	m_logList->setWhatsThis(tr("This is the list of log messages sent from the host.  "
		"The messages are color coded by severity."));
	m_logList->setEnabled(false);
	m_logList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_logList->setSelectionMode(QAbstractItemView::NoSelection);
	// Override style sheet so it is them same even in case of dark theme
	m_logList->setStyleSheet("QListWidget { color: black; background: #808080; }");
	gridLayout->addWidget(m_logList);

	QString loggingHtml = LocalHtmlDoc(DOC_LOGGING, tr("More information about logging"));

	QHBoxLayout* hLayout = new QHBoxLayout;
	m_logButton = new QPushButton(tr("Log message"), this);
	m_logButton->setToolTip(tr("Log the text"));
	m_logButton->setWhatsThis(tr("Click this button to add the text in the field to the "
		"right to the log.  This can be used to make a note in the log for analyzing "
		"the logs later."));
	m_logMessage = new QLineEdit(this);
	m_logMessage->setToolTip(tr("Text to log"));
	m_logMessage->setWhatsThis(tr("This is where you enter the text to add to the log.") + loggingHtml);
	hLayout->addWidget(m_logButton);
	hLayout->addWidget(m_logMessage);
	// addItem() in QGridLayout is protected but not in QLayout??
	layout()->addItem(hLayout);

	connect(m_hostClient, &HostClient::hostLog,
		this, &HostLogDialog::hostLogMessage);
	connect(m_hostClient, &HostClient::connected,
		this, &HostLogDialog::hostConnected);
	connect(m_hostClient, &HostClient::disconnected,
		this, &HostLogDialog::hostDisconnected);
	connect(m_hostClient, &HostClient::valueUpdate,
		this, &HostLogDialog::hostValueUpdate);
	connect(m_logButton, &QPushButton::clicked,
		this, [this]()
		{
			QString message = m_logMessage->text();
			if (!message.isEmpty())
			{
				m_hostClient->logMessage(message, LOG_WARNING);
				m_logMessage->clear();
			}
		});
	connect(m_remoteLogLevel, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged,
		this, &HostLogDialog::comboBoxValueChanged);

	WindowManager::addWindow(this);
	WindowManager::cascadeWindow(this);
	m_logMessage->setFocus();
}


HostLogDialog::~HostLogDialog()
{
	WindowManager::removeWindow(this);
	emit close(m_hostId);
}


void HostLogDialog::hostLogMessage(int level, QString message)
{
	QListWidgetItem* item = new QListWidgetItem(message.trimmed(), m_logList);
	item->setFlags(Qt::NoItemFlags);
	switch (level)
	{
	case LOG_ERROR:
		item->setBackground(Qt::red);
		break;
	case LOG_WARNING:
		item->setBackground(QColor("pink"));
		break;
	case LOG_EXTRA:
		item->setBackground(Qt::cyan);
		break;
	case LOG_DEBUG:
		item->setBackground(Qt::yellow);
		break;
	}
	
	if (m_autoScroll)
		m_logList->scrollToBottom();
}


void HostLogDialog::hostConnected()
{
	if (m_firstConnect)
	{
		m_firstConnect = false;
	}
	else
	{
		hostLogMessage(LOG_DEBUG, tr("(Reconnected to host, log entries might have been missed)"));
	}

	setWindowTitle(tr("Log of %1 %2")
		.arg(m_hostClient->getHostName())
		.arg(m_hostClient->getHostAddress()));

	m_hostClient->subscribeToCommand(CMD_LOG);
	m_hostClient->subscribeToCommand(CMD_VALUE);
	m_hostClient->subscribeToGroup(GROUP_GLOBAL);
	m_hostClient->getVariant(GROUP_GLOBAL, QString(), PROP_GLOBAL_REMOTELOGLEVEL);

	m_remoteLogLevel->setEnabled(true);
	m_logList->setEnabled(true);
}


void HostLogDialog::hostDisconnected()
{
	setWindowTitle(tr("Disconnected, trying to reconnect to %1")
		.arg(m_hostClient->getHostAddress()));

	hostLogMessage(LOG_DEBUG, tr("(Disconnected from host, trying to reconnect)"));

	m_remoteLogLevel->setEnabled(false);
	m_logList->setEnabled(false);
}


void HostLogDialog::hostValueUpdate(const QString& group, const QString& item, const QString& prop, const QVariant& value)
{
	Q_UNUSED(item);

	if (GROUP_GLOBAL == group)
	{
		if (PROP_GLOBAL_REMOTELOGLEVEL == prop)
		{
			m_remoteLogLevel->setCurrentIndex(m_remoteLogLevel->findData(value));
		}
	}
}


void HostLogDialog::comboBoxValueChanged(int index)
{
	QObject* obj = sender();

	m_hostClient->setVariant(GROUP_GLOBAL, QString(), PROP_GLOBAL_REMOTELOGLEVEL, qobject_cast<QComboBox*>(obj)->itemData(index));
}


bool HostLogDialog::event(QEvent* ev)
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


void HostLogDialog::changeEvent(QEvent* event)
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

