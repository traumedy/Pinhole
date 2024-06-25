#pragma once

#include <QDialog>
#include <QTimer>

class QTableWidget;
class HostClient;

class HostAppWatchDialog : public QDialog
{
	Q_OBJECT

public:
	HostAppWatchDialog(const QString& hostAddr, int port, const QString& hostId, QWidget *parent = Q_NULLPTR);
	~HostAppWatchDialog();

signals:
	void close(QString addr);

public slots:
	void hostValueChanged(const QString&, const QString&, const QString&, const QVariant&);
	void hostConnected();
	void hostDisconnected();
	void startStopButtonClicked();

private:
	void changeEvent(QEvent* event) override;

	QTableWidget* m_appList = nullptr;
	QTimer m_resizeTimer;
	QString m_hostAddr;
	QString m_hostId;
	HostClient* m_hostClient;
};
