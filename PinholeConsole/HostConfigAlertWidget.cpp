#include "HostConfigAlertWidget.h"
#include "Global.h"
#include "GuiUtil.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QUrl>
#include <QEvent>
#include <QWhatsThisClickedEvent>
#include <QMessageBox>
#include <QDesktopServices>
#include <QGuiApplication>


HostConfigAlertWidget::HostConfigAlertWidget(QWidget *parent)
	: HostConfigWidgetTab(parent)
{
	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	ScrollAreaEx* buttonScrollArea = new ScrollAreaEx;
	buttonScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
	QScrollArea* detailsScrollArea = new QScrollArea;
	mainLayout->addWidget(buttonScrollArea);
	mainLayout->addWidget(detailsScrollArea);

	QWidget* buttonScrollAreaWidgetContents = new QWidget;
	buttonScrollAreaWidgetContents->setProperty("borderless", true);
	QVBoxLayout* buttonLayout = new QVBoxLayout(buttonScrollAreaWidgetContents);
	m_addAlertSlotButton = new QPushButton(tr("Add alert slot"));
	buttonLayout->addWidget(m_addAlertSlotButton);
	m_deleteAlertSlotButton = new QPushButton(tr("Delete alert slots"));
	buttonLayout->addWidget(m_deleteAlertSlotButton);
	m_renameAlertSlotButton = new QPushButton(tr("Rename alert slot"));
	buttonLayout->addWidget(m_renameAlertSlotButton);
	m_resetAlertsButton = new QPushButton(tr("Reset active alerts"));
	buttonLayout->addWidget(m_resetAlertsButton);
	m_retrieveAlertsButton = new QPushButton(tr("Retrieve alert list"));
	buttonLayout->addWidget(m_retrieveAlertsButton);
	buttonScrollArea->setWidget(buttonScrollAreaWidgetContents);
	buttonScrollArea->show();

	QWidget* detailsScrollAreaWidgetContents = new QWidget;
	detailsScrollAreaWidgetContents->setProperty("borderless", true);
	detailsScrollAreaWidgetContents->setMinimumWidth(MINIMUM_DETAILS_WIDTH);
	QVBoxLayout* vboxLayout = new QVBoxLayout(detailsScrollAreaWidgetContents);
	m_alertSlotList = new QTableWidget;
	m_alertSlotList->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	m_alertSlotList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_alertSlotList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_alertSlotList->setMinimumWidth(MINIMUM_DETAILS_WIDTH);
	connect(m_alertSlotList, &QTableWidget::itemSelectionChanged,
		this, &HostConfigAlertWidget::alertSlotList_itemSelectionChanged);
	vboxLayout->addWidget(m_alertSlotList);

	QFormLayout* detailsLayout = new QFormLayout;
	m_alertCount = new QLabel;
	detailsLayout->addRow(new QLabel(tr("Active alert count")), m_alertCount);
	m_smtpServer = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("SMTP server")), m_smtpServer);
	m_smtpPort = new QSpinBox;
	m_smtpPort->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_smtpPort->setMinimum(1);
	m_smtpPort->setMaximum(MAX_PORT);
	detailsLayout->addRow(new QLabel(tr("SMTP server port")), m_smtpPort);
	m_smtpSSL = new QCheckBox(tr("SSL"));
	detailsLayout->addRow(nullptr, m_smtpSSL);
	m_smtpTLS = new QCheckBox(tr("TLS"));;
	detailsLayout->addRow(nullptr, m_smtpTLS);
	m_smtpUser = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("SMTP server user")), m_smtpUser);
	m_smtpPass = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("SMTP server password")), m_smtpPass);
	m_smtpEmail = new QLineEdit;
	detailsLayout->addRow(new QLabel(tr("SMTP sender email")), m_smtpEmail);
	m_smtpName = new QLineEdit;
	//detailsLayout->addRow(new QLabel(tr("SMTP sender name")), m_smtpName);

	vboxLayout->addLayout(detailsLayout);
	detailsScrollArea->setWidget(detailsScrollAreaWidgetContents);
	detailsScrollArea->show();

	m_widgetMap =
	{
		{ PROP_ALERT_ALERTCOUNT, m_alertCount },
		{ PROP_ALERT_SMTPSERVER, m_smtpServer },
		{ PROP_ALERT_SMTPPORT, m_smtpPort },
		{ PROP_ALERT_SMTPSSL, m_smtpSSL},
		{ PROP_ALERT_SMTPTLS, m_smtpTLS},
		{ PROP_ALERT_SMTPUSER, m_smtpUser },
		{ PROP_ALERT_SMTPPASS, m_smtpPass },
		{ PROP_ALERT_SMTPEMAIL, m_smtpEmail },
		{ PROP_ALERT_SMTPNAME, m_smtpName },
	};

	// Set the property names for the widgets
	for (const auto& key : m_widgetMap.keys())
	{
		m_widgetMap[key]->setProperty(PROPERTY_GROUPNAME, GROUP_ALERT);
		m_widgetMap[key]->setProperty(PROPERTY_PROPNAME, key);
	}

	QString alertsHtml = LocalHtmlDoc(DOC_ALERTS, tr("Details about alerts"));

	m_addAlertSlotButton->setToolTip(tr("Add a new alert destination slot"));
	m_addAlertSlotButton->setWhatsThis(tr("This button will open a dialog asking for the name "
		"of a new alert slot that will be added to the table to the right.") + alertsHtml);
	m_deleteAlertSlotButton->setToolTip(tr("Delet the selected alert slots"));
	m_deleteAlertSlotButton->setWhatsThis(tr("This button will delete the selected alert slots "
		"from the table to the right."));
	m_renameAlertSlotButton->setToolTip(tr("Rename the selected alert slot"));
	m_renameAlertSlotButton->setWhatsThis(tr("This button will allow you to rename the selected "
		"alert slot.  Only ONE alert slot must be selected for this button to work."));
	m_resetAlertsButton->setToolTip(tr("Reset the active alert count"));
	m_resetAlertsButton->setWhatsThis(tr("Resets the count of alerts back to zero."));
	m_retrieveAlertsButton->setToolTip(tr("Retrieves the list of alerts"));
	m_retrieveAlertsButton->setWhatsThis(tr("This button will retrieve the list of alerts that have "
		"occurred on the remote host and open them in a text viewer window locally."));
	m_alertSlotList->setToolTip(tr("The list of alert destination slots"));
	m_alertSlotList->setWhatsThis(tr("When alerts are generated they will be delivered to each "
		"destination slot in this list."));
	m_smtpServer->setToolTip(tr("The SMTP server used to send alert emails"));
	m_smtpServer->setWhatsThis(tr("The IP or host name of the SMTP server used to send emails.  "
		"Pinhole supports SSL/TLS (STARTTLS) encryption and authentication.") + alertsHtml);
	m_smtpPort->setToolTip(tr("The SMTP server port to connect to"));
	m_smtpPort->setWhatsThis(tr("The traditional SMTP port is 25 but more modern servers use "
		"ESMTPS on port 587.  Some may also use port 465 or some non-standard port.") + alertsHtml);
	m_smtpSSL->setToolTip(tr("Use SSL for SMTP connections"));
	m_smtpSSL->setWhatsThis(tr("Use SSL encryption when connecting to SMTP servers.  This overrides "
		"the TLS option.  Usually port 465."));
	m_smtpTLS->setToolTip(tr("Use STARTTLS encryption late encryption"));
	m_smtpTLS->setWhatsThis(tr("Use late encryption using STARTTLS mechanism.  Does not work if SSL "
		"is checked as the connection starts encrypted.  Usually port 487."));
	m_smtpUser->setToolTip(tr("The username used for SMTP authentication"));
	m_smtpUser->setWhatsThis(tr("If the server supports authentication, this username will be used "
		"during authentication.") + alertsHtml);
	m_smtpPass->setToolTip(tr("The password used to SMTP authentication"));
	m_smtpPass->setWhatsThis(tr("If the server supports authentication, this password will be used "
		"during authentication.") + alertsHtml);
	m_smtpEmail->setToolTip(tr("The return email address used to in emails"));
	m_smtpEmail->setToolTip(tr("This string will show in the 'From' field of sent alert emails.") + alertsHtml);
	m_smtpName->setToolTip(tr("Unused"));
}


HostConfigAlertWidget::~HostConfigAlertWidget()
{
}


void HostConfigAlertWidget::resetWidgets()
{
	m_alertSlotList->clear();
	m_alertSlotList->setRowCount(0);
	m_alertSlotList->setColumnCount(0);

	for (auto widget : m_widgetMap.values())
		ClearWidget(widget);
}


void HostConfigAlertWidget::enableWidgets(bool enable)
{
	if (enable)
	{
		for (int row = 0; row < m_alertSlotList->rowCount(); row++)
		{
			for (int col = 0; col < m_alertSlotList->columnCount(); col++)
			{
				QWidget* widget = m_alertSlotList->cellWidget(row, col);
				bool enableOverride = false;
				if (m_editable)
				{
					// Some widgets may have been marked as unsupported by a host
					QVariant supported = widget->property(PROPERTY_SUPPORTED);
					if (supported.isValid() && supported.toBool() == false)
						enableOverride = true;
				}
				widget->setEnabled(m_editable && !enableOverride);
			}
		}
	}
	m_alertSlotList->setEnabled(enable);

	m_addAlertSlotButton->setEnabled(enable && m_editable);
	m_resetAlertsButton->setEnabled(enable);
	m_retrieveAlertsButton->setEnabled(enable);

	m_smtpServer->setEnabled(enable && m_editable);
	m_smtpPort->setEnabled(enable && m_editable);
	m_smtpSSL->setEnabled(enable && m_editable);
	m_smtpTLS->setEnabled(enable && m_editable);
	m_smtpUser->setEnabled(enable && m_editable);
	m_smtpPass->setEnabled(enable && m_editable);
	m_smtpEmail->setEnabled(enable && m_editable);
	m_smtpName->setEnabled(enable && m_editable);

	bool itemSelected = !m_alertSlotList->selectionModel()->selectedRows().isEmpty();
	bool oneSelected = m_alertSlotList->selectionModel()->selectedRows().size() == 1;

	m_deleteAlertSlotButton->setEnabled(enable && m_editable && itemSelected);
	m_renameAlertSlotButton->setEnabled(enable && m_editable && oneSelected);
}


void HostConfigAlertWidget::alertSlotList_itemSelectionChanged()
{
	enableWidgets(true);
}


bool HostConfigAlertWidget::event(QEvent* ev)
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
