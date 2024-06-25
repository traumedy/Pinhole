#include "ScheduleEventOffsetWidget.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QDateTimeEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QSpinBox>


ScheduleEventOffsetWidget::ScheduleEventOffsetWidget(QWidget *parent)
	: QWidget(parent)
{
	m_layout = new QHBoxLayout(this);
	m_layout->setAlignment(Qt::AlignLeft);
	m_layout->setSizeConstraint(QLayout::SetMinimumSize);
	m_layout->setMargin(0);
	m_layout->setContentsMargins(0, 0, 0, 0);

	m_timeEdit = new QTimeEdit;
	//m_timeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	m_timeEdit->setFrame(false);
	m_timeEdit->hide();
	connect(m_timeEdit, &QDateTimeEdit::timeChanged,
		this, &ScheduleEventOffsetWidget::timeChanged);

	m_comboBox = new QComboBox;
	//m_timeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	m_comboBox->setFrame(false);
	m_comboBox->addItems({ tr("Monday"), tr("Tuesday"), 
		tr("Wednesday"), tr("Thursday"), tr("Friday"), tr("Saturday"), tr("Sunday") });
	m_comboBox->hide();
	connect(m_comboBox, (void (QComboBox::*)(int))&QComboBox::currentIndexChanged,
		this, &ScheduleEventOffsetWidget::comboIndexChanged);

	m_spinBox = new QSpinBox;
	//m_timeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	m_spinBox->setFrame(false);
	m_spinBox->setMinimum(0);
	m_spinBox->setMaximum(59);
	m_spinBox->hide();
	connect(m_spinBox, (void (QSpinBox::*)(int))&QSpinBox::editingFinished,
		this, &ScheduleEventOffsetWidget::spinboxValueChanged);

	m_dateTimeEdit = new QDateTimeEdit;
	//m_timeEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
	m_dateTimeEdit->setFrame(false);
	m_dateTimeEdit->hide();
	connect(m_dateTimeEdit, &QDateTimeEdit::editingFinished,
		this, &ScheduleEventOffsetWidget::dateTimeChanged);

	m_layout->addWidget(m_comboBox);
	m_layout->addWidget(m_timeEdit);
	m_layout->addWidget(m_spinBox);
	m_layout->addWidget(m_dateTimeEdit);
}


ScheduleEventOffsetWidget::~ScheduleEventOffsetWidget()
{
	delete m_timeEdit;
	delete m_comboBox;
	delete m_spinBox;
	delete m_dateTimeEdit;
}


void ScheduleEventOffsetWidget::setType(const QString& type)
{
	updateOffsetValue();

	m_type = type;

	if (SCHED_FREQ_DISABLED == m_type)
	{
		m_comboBox->hide();
		m_timeEdit->hide();
		m_spinBox->hide();
		m_dateTimeEdit->hide();
	}
	else if (SCHED_FREQ_WEEKLY == m_type)
	{
		m_comboBox->show();
		m_timeEdit->show();
		m_spinBox->hide();
		m_dateTimeEdit->hide();
	}
	else if (SCHED_FREQ_DAILY == m_type)
	{
		m_comboBox->hide();
		m_timeEdit->show();
		m_spinBox->hide();
		m_dateTimeEdit->hide();
	}
	else if (SCHED_FREQ_HOURLY == m_type)
	{
		m_comboBox->hide();
		m_timeEdit->hide();
		m_spinBox->show();
		m_dateTimeEdit->hide();
	}
	else if (SCHED_FREQ_ONCE == m_type)
	{
		m_comboBox->hide();
		m_timeEdit->hide();
		m_spinBox->hide();
		m_dateTimeEdit->show();
	}

	updateWidgetOffset();
}


void ScheduleEventOffsetWidget::setOffset(int offset)
{
	m_offset = offset;
	updateWidgetOffset();
}


int ScheduleEventOffsetWidget::getOffset()
{
	updateOffsetValue();
	return m_offset;
}


void ScheduleEventOffsetWidget::updateOffsetValue()
{
	if (SCHED_FREQ_WEEKLY == m_type)
	{
		QTime time = m_timeEdit->time();
		m_offset = (m_offset / 10080 * 10080) + 
			m_comboBox->currentIndex() * 1440 + time.hour() * 60 + time.minute();
	}
	else if (SCHED_FREQ_DAILY == m_type)
	{
		QTime time = m_timeEdit->time();
		m_offset = (m_offset / 1440 * 1440) + time.hour() * 60 + time.minute();
	}
	else if (SCHED_FREQ_HOURLY == m_type)
	{
		m_offset = (m_offset / 60 * 60) + m_spinBox->value();
	}
	else if (SCHED_FREQ_ONCE == m_type)
	{
		QDateTime dateTime = m_dateTimeEdit->dateTime();
		m_offset = dateTime.toSecsSinceEpoch() / 60;
	}
}


void ScheduleEventOffsetWidget::updateWidgetOffset()
{
	if (SCHED_FREQ_WEEKLY == m_type)
	{
		int index = (m_offset % 10080) / 1440;
		m_comboBox->setCurrentIndex(index);

		QTime time;
		time.setHMS((m_offset % 1440) / 60, m_offset % 60, 0);
		m_timeEdit->setTime(time);
	}
	else if (SCHED_FREQ_DAILY == m_type)
	{
		QTime time;
		time.setHMS((m_offset % 1440) / 60, m_offset % 60, 0);
		m_timeEdit->setTime(time);
	}
	else if (SCHED_FREQ_HOURLY == m_type)
	{
		m_spinBox->setValue(m_offset % 60);
	}
	else if (SCHED_FREQ_ONCE == m_type)
	{
		QDateTime dateTime;
		dateTime.setSecsSinceEpoch(m_offset * 60);
		m_dateTimeEdit->setDateTime(dateTime);
	}
}


void ScheduleEventOffsetWidget::timeChanged(const QTime&)
{
	updateOffsetValue();
	emit offsetChanged(m_offset);
}

void ScheduleEventOffsetWidget::comboIndexChanged(int)
{
	updateOffsetValue();
	emit offsetChanged(m_offset);
}


void ScheduleEventOffsetWidget::spinboxValueChanged()
{
	updateOffsetValue();
	emit offsetChanged(m_offset);
}


void ScheduleEventOffsetWidget::dateTimeChanged()
{
	updateOffsetValue();
	emit offsetChanged(m_offset);
}

