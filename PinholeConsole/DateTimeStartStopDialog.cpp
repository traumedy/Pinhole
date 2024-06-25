#include "DateTimeStartStopDialog.h"

#include <QDateTimeEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

DateTimeStartStopDialog::DateTimeStartStopDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Retrieve log entries"));

	QFormLayout* mainLayout = new QFormLayout(this);
	mainLayout->addRow(nullptr, new QLabel(tr("Enter the period from which you want to retrieve log entries")));
	m_startDate = new QDateTimeEdit;
	mainLayout->addRow(new QLabel(tr("Start time")), m_startDate);
	m_endDate = new QDateTimeEdit;
	mainLayout->addRow(new QLabel(tr("End time")), m_endDate);
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	mainLayout->addRow(nullptr, buttons);
	connect(buttons, &QDialogButtonBox::accepted,
		this, &DateTimeStartStopDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected,
		this, &DateTimeStartStopDialog::reject);

	QDateTime current = QDateTime::currentDateTime();
	m_startDate->setDateTime(current.addDays(-7));
	//m_startDate->setMaximumDateTime(maxDateTime);
	m_endDate->setDateTime(current);
	m_endDate->setMinimumDateTime(current.addDays(-7));
	//m_endDate->setMaximumDateTime(maxDateTime);

	// Prevent end date from being less than start date
	connect(m_startDate, &QDateTimeEdit::dateTimeChanged,
		this, [this](const QDateTime& datetime)
	{
		m_endDate->setMinimumDateTime(datetime);
	});

	setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}


DateTimeStartStopDialog::~DateTimeStartStopDialog()
{
}


QDateTime DateTimeStartStopDialog::getStartDate() const
{
	return m_startDate->dateTime();
}


QDateTime DateTimeStartStopDialog::getEndDate() const
{
	return m_endDate->dateTime();
}


