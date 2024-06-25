#include "LogDialog.h"
#include "../common/HostClient.h"

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

LogDialog::LogDialog(HostClient* hostClient, QWidget *parent)
	: QDialog(parent), m_hostClient(hostClient)
{
	QHBoxLayout* mainLayout = new QHBoxLayout;
	setLayout(mainLayout);

	m_logButton = new QPushButton(tr("Log message"));
	connect(m_logButton, &QPushButton::clicked,
		this, &LogDialog::logButton_clicked);
	mainLayout->addWidget(m_logButton);
	m_logMessage = new QLineEdit;
	mainLayout->addWidget(m_logMessage);
	mainLayout->setStretchFactor(m_logMessage, 3);

	m_logMessage->setFocus();

	// Lock the height of the dialog
	setAttribute(Qt::WA_DontShowOnScreen, true);
	show();
	setMinimumHeight(height());
	setMaximumHeight(height());
	// Lock the height of the dialog
	setAttribute(Qt::WA_DontShowOnScreen, true);
	show();
	setMinimumHeight(height());
	setMaximumHeight(height());
	// Lock the minimum width
	setMinimumWidth(width());
	hide();
	setAttribute(Qt::WA_DontShowOnScreen, false);

	
	m_logButton->setToolTip(tr("Log the text"));
	m_logButton->setWhatsThis(tr("Click this button to log the text in "
		"the field to the right."));
	m_logMessage->setToolTip(tr("The text to log"));
	m_logMessage->setWhatsThis(tr("Enter text in this field be logged for "
		"troubleshooting.  The date and time will be stored with this text."));
}


LogDialog::~LogDialog()
{
}


void LogDialog::logButton_clicked()
{
	QString message = m_logMessage->text();

	if (!message.isEmpty())
	{
		m_hostClient->logMessage(message, LOG_WARNING);
		hide();
	}
}


