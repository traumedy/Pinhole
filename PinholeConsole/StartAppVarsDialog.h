#pragma once

#include <QDialog>

class QLineEdit;
class ListWidgetEx;

class StartAppVarsDialog : public QDialog
{
	Q_OBJECT

public:
	StartAppVarsDialog(QWidget *parent);
	~StartAppVarsDialog();
	QString appName() const;
	QStringList variableList() const;

private:
	QLineEdit* m_appName = nullptr;
	ListWidgetEx* m_variables = nullptr;


};
