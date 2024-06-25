#pragma once

#include <QDialog>
#include <QMap>

class QLineEdit;
class QComboBox;
class QListWidget;

class CustomActionDialog : public QDialog
{
	Q_OBJECT

public:
	CustomActionDialog(QWidget *parent);
	~CustomActionDialog();
	QString getCommandName();
	QVariantList getCommandVariantList();
	void setUsedNames(const QStringList& names);

private:
	QLineEdit* m_name = nullptr;
	QComboBox* m_command = nullptr;
	QLineEdit* m_singleArgument = nullptr;
	QListWidget* m_multiArgument = nullptr;

	QMap<QString, int> m_layoutIndexMap;
	QMap<QString, QString> m_groupMap;
	QStringList m_usedNames;
};
