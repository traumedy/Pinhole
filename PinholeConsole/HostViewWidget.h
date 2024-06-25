#pragma once

#include <QWidget>
#include <QMap>

#include <functional>

class QTableView;
class QPushButton;
class QVBoxLayout;
class HostClient;

class HostViewWidget : public QWidget
{
	Q_OBJECT

public:
	HostViewWidget(QWidget *parent = Q_NULLPTR);
	~HostViewWidget();

public slots:
	void setHostPassword_clicked();
	void shutdownButton_clicked();
	void rebootButton_clicked();
	void importSettingsButton_clicked();
	void wakeOnLanButton_clicked();
	void retrieveLogButton_clicked();
	void hostList_doubleClicked(const QModelIndex&);
	void enableButtons(bool enable);
	void addCustomButtonClicked();
	void executeCommand_clicked();
	void startAppVariables_clicked();

signals:
	void setNoticeText(const QString& text, unsigned int id = 0);
	void clearNoticeText(unsigned int id);

public:
	QTableView * m_hostList;

private:
	void resizeEvent(QResizeEvent* event) override;
	bool event(QEvent* ev) override;
	void execServerCommand(const QString& hostAddress, int port, const QString& hostId,
		const QString& hostName, const QString& failureMessage, std::function<void(HostClient*)> lambda);
	void execServerCommandOnEachSelectedHost(const QString& failureMessage, 
		std::function<void(HostClient*)> lambda);
	void saveSettingsFile(const QString& hostName, const QString& hostAddress, const QByteArray& data);
	QByteArray loadSettingsFile();
	void addCustomActionButton(const QString& name, const QVariantList& vlist);
	void readCustomActions();
	void writeCustomActions();
	void sendWakeOnLan(const QString& IpString, const QString& MacString);
	QString hostPairListToString(const QList<QPair<QString, QString>>& hostlist);
	QString hostVariantListToString(const QList<QVariantList>& hostList);

	QPushButton * m_startStartupAppsButton = nullptr;
	QPushButton * m_stopAllAppsButton = nullptr;
	QPushButton * m_retrieveLogButton = nullptr;
	QPushButton * m_showScreenIdsButton = nullptr;
	QPushButton * m_screenshotButton = nullptr;
	QPushButton * m_sysinfoButton = nullptr;
	QPushButton * m_executeCommand = nullptr;
	QPushButton * m_startAppVariables = nullptr;
	QPushButton * m_shutdownButton = nullptr;
	QPushButton * m_rebootButton = nullptr;
	QPushButton * m_setHostPassword = nullptr;
	QPushButton * m_exportSettingsButton = nullptr;
	QPushButton * m_importSettingsButton = nullptr;
	QPushButton * m_wakeOnLanButton = nullptr;

	QVBoxLayout* m_buttonLayout = nullptr;
	bool m_buttonsEnabled = false;
	QList<QPushButton*> m_pushButtons;
	QMap<QString, QVariantList> m_customActionMap;
	QString m_lastSettingsDirectoryChosen;
	
};
