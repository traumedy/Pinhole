#pragma once

#include <QDialog>
#include <QDateTime>

class QDateTimeEdit;

class DateTimeStartStopDialog : public QDialog
{
	Q_OBJECT

public:
	DateTimeStartStopDialog(QWidget *parent);
	~DateTimeStartStopDialog();
	QDateTime getStartDate() const;
	QDateTime getEndDate() const;


private:
	QDateTimeEdit * m_startDate = nullptr;
	QDateTimeEdit * m_endDate = nullptr;
};
