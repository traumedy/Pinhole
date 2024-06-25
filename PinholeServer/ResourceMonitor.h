#pragma once

#include <QObject>
#include <QTimer>

class Settings;
class GlobalManager;

class ResourceMonitor : public QObject
{
	Q_OBJECT

public:
	ResourceMonitor(Settings* settings, GlobalManager* globalManager, QObject *parent = nullptr);
	~ResourceMonitor();

private slots:
	void checkResources();
	void globalValueChanged(const QString&, const QString&, const QString&, const QVariant&);

signals:
	void generateAlert(const QString& text);

private:
	QTimer m_checkTimer;
	bool m_memoryAlertSent = false;
	QStringList m_diskAlertsSent;

	bool m_alertMemory = false;
	int m_minMemory = 0;
	bool m_alertDisk = false;
	int m_minDisk = 0;
	QStringList m_alertDiskList;

	Settings* m_settings = nullptr;
	GlobalManager* m_globalManager = nullptr;
};
