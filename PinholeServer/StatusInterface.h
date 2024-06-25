#pragma once

#include <QObject>

class Settings;
class AlertManager;
class AppManager;
class GlobalManager;

class StatusInterface : public QObject
{
	Q_OBJECT

public:
	StatusInterface(Settings* settings, AlertManager* alertManager, AppManager* appManager,
		GlobalManager* globalManager, QObject *parent = nullptr);
	~StatusInterface();
	QByteArray processPacket(const QByteArray& packet, const QString& MAC);

public slots:
	void sendStatus(const QString& status);

signals:
	void sendPacket(const QByteArray& packet);

private:
	Settings* m_settings = nullptr;
	AlertManager* m_alertManager = nullptr;
	AppManager* m_appManager = nullptr;
	GlobalManager* m_globalManager = nullptr;

};
