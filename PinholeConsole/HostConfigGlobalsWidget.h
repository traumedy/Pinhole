#pragma once

#include "HostConfigWidgetTab.h"

#include <QWidget>
#include <QMap>
#include <QString>

class QPushButton;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QLineEdit;
class QListWidget;
class ListWidgetEx;

class HostConfigGlobalsWidget : public HostConfigWidgetTab
{
	Q_OBJECT

public:
	HostConfigGlobalsWidget(QWidget *parent = Q_NULLPTR);
	~HostConfigGlobalsWidget();

	void resetWidgets() override;
	void enableWidgets(bool enable) override;

	QLineEdit * m_role = nullptr;
	QComboBox * m_hostLogLevel = nullptr;
	QComboBox * m_remoteLogLevel = nullptr;
	QSpinBox * m_appTerminateTimeout = nullptr;
	QSpinBox * m_appHeartbeatTimeout = nullptr;
	QSpinBox * m_appCrashPeriod = nullptr;
	QSpinBox * m_appCrashCount = nullptr;
	QCheckBox * m_trayLaunch = nullptr;
	QCheckBox * m_trayControl = nullptr;
	QCheckBox * m_httpEnabled = nullptr;
	QSpinBox * m_httpPort = nullptr;
	QLineEdit * m_backendServer = nullptr;
	QLineEdit * m_novaSite = nullptr;
	QLineEdit * m_novaArea = nullptr;
	QLineEdit * m_novaDisplay = nullptr;
	QCheckBox * m_novaTcpEnabled = nullptr;
	QLineEdit * m_novaTcpAddress = nullptr;
	QSpinBox * m_novaTcpPort = nullptr;
	QCheckBox * m_novaUdpEnabled = nullptr;
	QLineEdit * m_novaUdpAddress = nullptr;
	QSpinBox * m_novaUdpPort = nullptr;
	QCheckBox * m_alertMemory = nullptr;
	QSpinBox * m_minMemory = nullptr;
	QCheckBox * m_alertDisk = nullptr;
	QSpinBox * m_minDisk = nullptr;
	ListWidgetEx * m_alertDiskList = nullptr;

signals:
	void widgetValueChanged(QWidget* widget);

private:
	void resizeEvent(QResizeEvent* event) override;
	bool event(QEvent*) override;
};
