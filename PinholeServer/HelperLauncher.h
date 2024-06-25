#pragma once

#include "TimePeriodCount.h"

#include <QObject>
#include <QTimer>
#include <QProcess>

class Settings;
class GlobalManager;
class UserProcess;

class HelperLauncher : public QObject
{
	Q_OBJECT

public:
	HelperLauncher(Settings* settings, GlobalManager* globalManager, QObject *parent = nullptr);
	~HelperLauncher();

public slots:
	void helperStarted();
	void helperFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void helperError(QProcess::ProcessError error);
	void globalValueChanged(const QString& group, const QString& item, const QString& prop, const QVariant& value);
	void start();
	void stop();

signals:
	void finishedStarting();

private slots:
	void startHelper();

private:
	void stopHelper();
	void launchHelperProcess();

	bool m_trayLaunch = false;
	bool started = false;
	UserProcess* m_helperProcess = nullptr;
	bool m_exitExpected = false;
	TimePeriodCount m_restartInPeriod;

	Settings* m_settings = nullptr;
	GlobalManager* m_globalManager = nullptr;
};
