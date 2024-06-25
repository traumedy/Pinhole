#pragma once

#include <QObject>
#include <QMap>
#include <QSharedPointer>

class LogDialog;
class HostClient;
class QMenu;
class QAction;
class QSystemTrayIcon;

class TrayManager : public QObject
{
	Q_OBJECT

public:
	TrayManager(QObject *parent = nullptr);
	~TrayManager();

public slots:
	void hostConnected();
	void hostDisconnected();
	void hostValueChanged(const QString&, const QString&, const QString&, const QVariant&);
	void showLogDialog();

private:
	void buildTrayMenu(bool connected, bool allowControl);
	QByteArray takeScreenshot();

	QSystemTrayIcon* m_trayIcon = nullptr;
	QMenu* m_trayMenu = nullptr;
	QMenu* m_appMenu = nullptr;
	QMenu* m_groupMenu = nullptr;
	QMap<QString, QPair<QAction*, QAction*>> m_appActionMap;

	QSharedPointer<HostClient> m_hostClient;
	QSharedPointer<LogDialog> m_logDialog;
};

