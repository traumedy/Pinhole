#pragma once

#include <QDialog>

class QPushButon;
class QLineEdit;
class HostClient;

class LogDialog : public QDialog
{
	Q_OBJECT

public:
	LogDialog(HostClient* hostClient, QWidget *parent = Q_NULLPTR);
	~LogDialog();

public slots:
	void logButton_clicked();

private:
	QPushButton * m_logButton = nullptr;
	QLineEdit * m_logMessage = nullptr;

	HostClient * m_hostClient = nullptr;
};
