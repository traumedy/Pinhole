#include "FindTextDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QGridLayout>


FindTextDialog::FindTextDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);

	setWindowTitle(tr("Find text"));

	QGridLayout* gridLayout = new QGridLayout(this);

	QLabel* findLabel = new QLabel(tr("Find what:"));
	gridLayout->addWidget(findLabel, 0, 0);
	m_text = new QLineEdit;
	gridLayout->addWidget(m_text, 0, 1, 1, 2);
	m_findButton = new QPushButton(tr("Find next"));
	m_findButton->setDefault(true);
	connect(m_findButton, &QPushButton::clicked,
		this, [this]()
	{
		emit findClicked();
	});
	gridLayout->addWidget(m_findButton, 0, 3);
	m_caseSensitive = new QCheckBox(tr("Case sensitive"));
	gridLayout->addWidget(m_caseSensitive, 1, 0);
	m_wholeWord = new QCheckBox(tr("Whole word only"));
	gridLayout->addWidget(m_wholeWord, 1, 1);
	m_cancelButton = new QPushButton(tr("Cancel"));
	connect(m_cancelButton, &QPushButton::clicked,
		this, &FindTextDialog::hide);
	gridLayout->addWidget(m_cancelButton, 1, 3);
	m_reverseSearch = new QCheckBox(tr("Search backwards"));
	gridLayout->addWidget(m_reverseSearch, 2, 0);
	m_regularExpression = new QCheckBox(tr("Regular expression"));
	gridLayout->addWidget(m_regularExpression, 2, 1);

	layout()->setSizeConstraint(QLayout::SetFixedSize);
	//setFixedSize(size());
}


FindTextDialog::~FindTextDialog()
{
}


QString FindTextDialog::getText() const
{
	return m_text->text();
}


void FindTextDialog::setText(const QString& text)
{
	m_text->setText(text);
}


bool FindTextDialog::getReverseSearch() const
{
	return m_reverseSearch->isChecked();
}


bool FindTextDialog::getCaseSensitive() const
{
	return m_caseSensitive->isChecked();
}


bool FindTextDialog::getWholeWord() const
{
	return m_wholeWord->isChecked();
}


bool FindTextDialog::getRegularExpression() const
{
	return m_regularExpression->isChecked();
}




