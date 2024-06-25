#pragma once

#include "HostConfigWidgetTab.h"

#include <QWidget>
#include <QMap>

class QPushButton;
class QLabel;
class QCheckBox;
class QSpinBox;
class QListWidget;
class ListWidgetEx;
class QLineEdit;
class QComboBox;

class HostConfigAppsWidget : public HostConfigWidgetTab
{
	Q_OBJECT

public:
	HostConfigAppsWidget(QWidget *parent = Q_NULLPTR);
	~HostConfigAppsWidget();

	void resetAppWidgets();
	void resetWidgets() override;
	void enableWidgets(bool enable) override;

	QPushButton * m_addAppButton = nullptr;
	QPushButton * m_deleteAppButton = nullptr;
	QPushButton * m_renameAppButton = nullptr;
	QPushButton * m_startAppButton = nullptr;
	QPushButton * m_stopAppButton = nullptr;
	QPushButton * m_getConsoleOutput = nullptr;
	QListWidget * m_appList = nullptr;
	QLabel * m_appState = nullptr;
	QLabel * m_lastStarted = nullptr;
	QLabel * m_lastExited = nullptr;
	QLabel * m_restarts = nullptr;
	QLineEdit * m_appExecutable = nullptr;
	QLineEdit * m_appArguments = nullptr;
	QLineEdit * m_appDirectory = nullptr;
	QComboBox * m_launchDisplay = nullptr;
	QSpinBox * m_launchDelay = nullptr;
	QCheckBox * m_launchAtStart = nullptr;
	QCheckBox * m_keepAppRunning = nullptr;
	QCheckBox * m_terminatePrev = nullptr;
	QCheckBox * m_softTerminate = nullptr;
	QCheckBox * m_noCrashThrottle = nullptr;
	QCheckBox * m_lockupScreenshot = nullptr;
	QCheckBox * m_consoleCapture = nullptr;
	QCheckBox * m_appendCapture = nullptr;
	QCheckBox * m_tcpLoopback = nullptr;
	QSpinBox * m_tcpLoopbackPort = nullptr;
	QCheckBox * m_heartbeats = nullptr;
	ListWidgetEx * m_environmentList = nullptr;

signals:
	void widgetValueChanged(QWidget* widget);
	void appChanged(const QString&);

public slots:
	void appList_currentItemChanged();

private:
	void resizeEvent(QResizeEvent* event) override;
	bool event(QEvent*) override;
};
