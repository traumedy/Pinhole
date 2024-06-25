#pragma once

#include <QDialog>
#include <QValidator>

class QLineEdit;
class QPushButton;
class QRadioButton;
class QLabel;
class HostFinder;

class HostScanDialog : public QDialog
{
	Q_OBJECT

public:
	HostScanDialog(HostFinder* hostFinder, QWidget *parent = Q_NULLPTR);
	~HostScanDialog();
	void setAddress(const QString& address);

public slots:
	void ipv4Button_clicked();
	void ipv6Button_clicked();
	void scanSingleButton_clicked();
	void scanSubnetButton_clicked();
	void lookupHostButton_clicked();

signals:
	void setStatusText(const QString&);
	void setSingleAdderessText(const QString&);

private:
	bool isSingleAddressValid(QValidator::State state = QValidator::Acceptable);

	HostFinder* m_hostFinder;

	QRadioButton * m_ipv4Button = nullptr;
	QRadioButton * m_ipv6Button = nullptr;
	QLineEdit * m_singleAddress = nullptr;
	QPushButton * m_scanSingleButton = nullptr;
	QPushButton * m_scanSubnetButton = nullptr;
	QLineEdit * m_hostName = nullptr;
	QPushButton * m_lookupHostButton = nullptr;
	QLabel * m_statusText = nullptr;
};
