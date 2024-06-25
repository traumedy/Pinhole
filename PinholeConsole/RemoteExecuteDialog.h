#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QCheckBox;
class QPushButton;
class QRadioButton;
class QCheckBox;

class RemoteExecuteDialog : public QDialog
{
	Q_OBJECT

public:
	RemoteExecuteDialog(QWidget *parent);
	~RemoteExecuteDialog();
	QByteArray file() const { return m_file; };
	QString filePath() const;
	QString command() const;
	QString commandLine() const;
	QString directory() const;
	bool localFile() const; 
	bool captureOutput() const;
	bool elevated() const;

private slots:
	void selectFile_clicked();

private:
	QPushButton* m_selectFile = nullptr;
	QRadioButton* m_localFile = nullptr;
	QLineEdit* m_filePath = nullptr;
	QRadioButton* m_remoteCommand = nullptr;
	QLineEdit* m_commandEdit = nullptr;
	QLineEdit* m_commandLineEdit = nullptr;
	QLineEdit* m_directoryEdit = nullptr;
	QCheckBox* m_captureOutput = nullptr;
	QCheckBox* m_elevated = nullptr;

	QByteArray m_file;

	static QString sm_lastDirectoryChosen;
};
