#include "StartAppVarsDialog.h"
#include "ListWidgetEx.h"

#include <QGuiApplication>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>

StartAppVarsDialog::StartAppVarsDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Start application with variables"));

	QFormLayout* mainLayout = new QFormLayout(this);
	m_appName = new QLineEdit();
	mainLayout->addRow(new QLabel(tr("Application name")), m_appName);
	m_variables = new ListWidgetEx(tr("variable"), "NAME=VALUE");
	mainLayout->addRow(new QLabel(tr("Variables to pass to application")), m_variables);
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	mainLayout->addRow(buttons);

	connect(buttons, &QDialogButtonBox::rejected,
		this, &StartAppVarsDialog::reject);
	connect(buttons, &QDialogButtonBox::accepted,
		[this]()
		{
			if (m_appName->text().isEmpty())
			{
				QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
					tr("Enter the application name to start"));
				return;
			}

			for (int n = 0; n < m_variables->count(); n++)
			{
				QString itemText = m_variables->item(n)->text();
				if (!itemText.contains('='))
				{
					QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
						tr("Variables should be in the form NAME=VALUE"));
					return;
				}
			}

			done(QDialog::Accepted);
		});

	m_appName->setToolTip(tr("Application name to start"));
	m_appName->setWhatsThis(tr("This is the name of the application to start which must exist on all of the "
		"machines the operation is being performed on.  This is case sensitive."));
	m_variables->setToolTip(tr("List of variables to pass to application"));
	m_variables->setWhatsThis(tr("This is the list of variables to pass to the application being started.  "
		"Each variable should be in the form NAME=VALUE such that the command line as configured in "
		"PinholeServer will replace %NAME% with VALUE.  These values are processed before environment "
		"variables so will override environment variables with the same name.\nRight click on this list to "
		"add and remove entries."));
}


StartAppVarsDialog::~StartAppVarsDialog()
{
}


QString StartAppVarsDialog::appName() const
{
	return m_appName->text();
}


QStringList StartAppVarsDialog::variableList() const
{
	return m_variables->getItemsText();
}
