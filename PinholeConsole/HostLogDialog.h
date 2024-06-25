#pragma once

#include <QDialog>

class QToolBar;
class QListWidget;
class QPushButton;
class QLineEdit;
class QComboBox;
class HostClient;

class HostLogDialog : public QDialog
{
	Q_OBJECT

public:
	HostLogDialog(const QString& hostAddr, int port, const QString& hostId, QWidget *parent = Q_NULLPTR);
	~HostLogDialog();

signals:
	void close(QString addr);

public slots:
	void hostLogMessage(int level, QString message);
	void hostConnected();
	void hostDisconnected();
	void hostValueUpdate(const QString& group, const QString& item, const QString& prop, const QVariant& value);
	void comboBoxValueChanged(int index);

private:
	void changeEvent(QEvent* event) override;
	bool event(QEvent*) override;

	QComboBox* m_remoteLogLevel = nullptr;
	QToolBar* m_toolBar = nullptr;
	QListWidget* m_logList = nullptr;
	QPushButton* m_logButton = nullptr;
	QLineEdit* m_logMessage = nullptr;

	QString m_hostAddr;
	QString m_hostId;
	HostClient* m_hostClient = nullptr;
	bool m_autoScroll = true;
	bool m_firstConnect = true;
};
