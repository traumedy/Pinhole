#pragma once

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QPushButton;

class FindTextDialog : public QDialog
{
	Q_OBJECT

public:
	FindTextDialog(QWidget *parent);
	~FindTextDialog();
	QString getText() const;
	void setText(const QString& text);
	bool getReverseSearch() const;
	bool getCaseSensitive() const;
	bool getWholeWord() const;
	bool getRegularExpression() const;

signals:
	void findClicked();

private:
	QLineEdit * m_text = nullptr;
	QCheckBox * m_caseSensitive = nullptr;
	QCheckBox * m_reverseSearch = nullptr;
	QCheckBox * m_wholeWord = nullptr;
	QCheckBox * m_regularExpression = nullptr;
	QPushButton * m_findButton = nullptr;
	QPushButton * m_cancelButton = nullptr;
};
