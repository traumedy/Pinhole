#pragma once

#include <QtWidgets/QMainWindow>
#include <QMap>

class QLabel;
class HostViewWidget;
class HostConfigWidget;
class HostScanDialog;
class WindowManager;
class HostAppWatchDialog;
class HostLogDialog;
class QItemSelection;
class HostFinder;
class QSortFilterProxyModel;

class PinholeConsole : public QMainWindow
{
	Q_OBJECT

public:
	PinholeConsole(QWidget *parent = Q_NULLPTR);
	~PinholeConsole();

public slots:
	void hostListSelectionChanged(const QItemSelection &, const QItemSelection &);
	void scanAction_triggered();
	void loadHostsAction_triggered();
	void saveHostsAction_triggered();
	void clearAction_triggered();
	void monitorAction_triggered();
	void windowManagerAction_triggered();
	void appWatchDialogClose(QString hostAddr);
	void actionWatch_log_triggered();
	void logWatchDialogClose(QString hostAddr);
	void setNoticeText(const QString& text, int id);
	void clearNoticeText(int id);
	void updateNoticeText();

signals:
	void hostChanged(const QString& address, int port, const QString& hostId);

private:
	void setStyle(bool dark);
	bool event(QEvent*) override;

	HostViewWidget* hostViewWidget = nullptr;
	HostConfigWidget* hostConfigWidget = nullptr;
	HostScanDialog* hostScanDialog = nullptr;
	WindowManager* windowManagerDialog = nullptr;

	QLabel* m_noticeLabel = nullptr;

	QSortFilterProxyModel* m_proxyModel = nullptr;
	HostFinder* m_hostFinder = nullptr;

	QMap<QString, QSharedPointer<HostAppWatchDialog>> m_appWatchMap;
	QMap<QString, QSharedPointer<HostLogDialog>> m_logWatchMap;
	QString m_lastHostListDirectoryChosen;
	QMap<int, QString> m_noticeMessages;
};

