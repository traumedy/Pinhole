#pragma once

#include "HostConfigWidgetTab.h"

#include <QWidget>
#include <QMap>

class QLineEdit;
class QSpinBox;
class QPushButton;
class QCheckBox;
class QTableWidget;
class QLabel;

class HostConfigAlertWidget : public HostConfigWidgetTab
{
	Q_OBJECT

public:
	HostConfigAlertWidget(QWidget *parent);
	~HostConfigAlertWidget();

	void resetWidgets() override;
	void enableWidgets(bool enable) override;

	QPushButton * m_addAlertSlotButton = nullptr;
	QPushButton * m_deleteAlertSlotButton = nullptr;
	QPushButton * m_renameAlertSlotButton = nullptr;
	QPushButton * m_resetAlertsButton = nullptr;
	QPushButton * m_retrieveAlertsButton = nullptr;
	QTableWidget * m_alertSlotList = nullptr;
	QLabel * m_alertCount = nullptr;
	QLineEdit * m_smtpServer = nullptr;
	QSpinBox * m_smtpPort = nullptr;
	QCheckBox * m_smtpSSL = nullptr;
	QCheckBox * m_smtpTLS = nullptr;
	QLineEdit * m_smtpUser = nullptr;
	QLineEdit * m_smtpPass = nullptr;
	QLineEdit * m_smtpEmail = nullptr;
	QLineEdit * m_smtpName = nullptr;

private slots:
	void alertSlotList_itemSelectionChanged();

private:
	bool event(QEvent*) override;

};
