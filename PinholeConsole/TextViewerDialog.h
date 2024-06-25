#pragma once

#include <QDialog>

class FindTextDialog;
class QPlainTextEdit;

class TextViewerDialog : public QDialog
{
	Q_OBJECT

public:
	TextViewerDialog(const QString& description, const QString& hostAddress, 
		const QString& hostName, const QByteArray& compressedText, QWidget *parent);
	~TextViewerDialog();

public slots:
	void saveAction_triggered();
	void findText();

private:
	void changeEvent(QEvent* event) override;
	QString m_description;
	QString m_hostAddress;
	QString m_hostName;
	QByteArray m_text;
	QPlainTextEdit * m_textEdit = nullptr;
	FindTextDialog * m_findDialog = nullptr;
	QString g_lastDirectoryChosen;
};

