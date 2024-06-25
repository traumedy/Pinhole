#pragma once

#include "../common/PinholeCommon.h"

#include <QWidget>


class QHBoxLayout;
class QDateTimeEdit;
class QTimeEdit;
class QComboBox;
class QSpinBox;

class ScheduleEventOffsetWidget : public QWidget
{
	Q_OBJECT

public:
	ScheduleEventOffsetWidget(QWidget *parent);
	~ScheduleEventOffsetWidget();

	void setType(const QString& type);
	void setOffset(int offset);
	int getOffset();
	
signals:
	void offsetChanged(int);

public slots:
	void timeChanged(const QTime&);
	void comboIndexChanged(int);
	void spinboxValueChanged();
	void dateTimeChanged();

private:
	void updateOffsetValue();
	void updateWidgetOffset();

	QHBoxLayout* m_layout = nullptr;
	QTimeEdit * m_timeEdit = nullptr;
	QComboBox * m_comboBox = nullptr;
	QSpinBox * m_spinBox = nullptr;
	QDateTimeEdit * m_dateTimeEdit = nullptr;

	QString m_type = SCHED_FREQ_DISABLED;
	int m_offset = 0;
};
