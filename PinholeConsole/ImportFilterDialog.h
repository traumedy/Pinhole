#pragma once

#include <QDialog>
#include <QJsonDocument>

class QTreeWidget;
class QRadioButton;

class ImportFilterDialog : public QDialog
{
	Q_OBJECT

public:
	ImportFilterDialog(QWidget *parent);
	~ImportFilterDialog();
	int exec(const QByteArray& jsonData);
	QByteArray jsonData() const;
	bool error() const { return m_error; }


private:
	bool event(QEvent*) override;

	QTreeWidget* m_tree = nullptr;
	QRadioButton* m_appSetTheseEntries = nullptr;
	QRadioButton* m_appAddOrUpdate = nullptr;
	QRadioButton* m_appDeleteTheseEntries = nullptr;
	QRadioButton* m_groupSetTheseEntries = nullptr;
	QRadioButton* m_groupAddOrUpdate = nullptr;
	QRadioButton* m_groupDeleteTheseEntries = nullptr;
	QRadioButton* m_eventSetTheseEntries = nullptr;
	QRadioButton* m_eventAddOrUpdate = nullptr;
	QRadioButton* m_eventDeleteTheseEntries = nullptr;
	QRadioButton* m_alertSetTheseEntries = nullptr;
	QRadioButton* m_alertAddOrUpdate = nullptr;
	QRadioButton* m_alertDeleteTheseEntries = nullptr;

	bool m_error = false;
	QJsonDocument m_jsonDoc;
};
