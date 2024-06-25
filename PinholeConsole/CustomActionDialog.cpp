#include "CustomActionDialog.h"
#include "Global.h"
#include "../common/PinholeCommon.h"

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QFormLayout>
#include <QStackedLayout>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QGuiApplication>

CustomActionDialog::CustomActionDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);
	setWindowTitle(tr("Add custom action button"));

	QFormLayout* mainLayout = new QFormLayout(this);
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	m_name = new QLineEdit;
	m_name->setToolTip(tr("The name on the custom action button"));
	m_name->setWhatsThis(tr("Enter the name that will apear on the new button of above the "
		"standard buttons to the left of the host list."));
	mainLayout->addRow(new QLabel(tr("Button name")), m_name);
	m_command = new QComboBox;
	m_command->setToolTip(tr("The action or command"));
	m_command->setWhatsThis(tr("Select the action or command that will be performed when "
		"the custom button is clicked."));
	mainLayout->addRow(new QLabel(tr("Command")), m_command);
	QStackedLayout* stackedLayout = new QStackedLayout;
	m_singleArgument = new QLineEdit;
	m_singleArgument->setToolTip(tr("The target of the action/command"));
	m_singleArgument->setWhatsThis(tr("Enter the name of the object that the action or "
		"command will be performed on."));
	m_multiArgument = new QListWidget;
	m_multiArgument->setToolTip(tr("The list of targets of the action/command"));
	m_multiArgument->setWhatsThis(tr("Create a list of object names that the action or "
		"command will be performed on (such as the application names to start/stop).  "
		"Use the <b>Add</b> button to create a new entry and double click on entries "
		"to edit them.  Use the <b>Del</b> button to delete the selected items in the list."));
	m_multiArgument->setMaximumHeight(ITEMLISTMAXHEIGHT);
	m_multiArgument->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	m_multiArgument->setEditTriggers(QAbstractItemView::DoubleClicked);
	QWidget* singleArgLayoutWidget = new QWidget;
	QGridLayout* singleArgLayout = new QGridLayout(singleArgLayoutWidget);
	singleArgLayout->setContentsMargins(0, 0, 0, 0);
	singleArgLayout->setAlignment(Qt::AlignTop);
	singleArgLayout->addWidget(m_singleArgument);
	stackedLayout->addWidget(singleArgLayoutWidget);
	QWidget* multiArgLayoutWidget = new QWidget;
	QGridLayout* multiArgLayout = new QGridLayout(multiArgLayoutWidget);
	multiArgLayout->setContentsMargins(0, 0, 0, 0);
	multiArgLayout->addWidget(m_multiArgument, 0, 0, 2, 2);
	QPushButton* multiAddItem = new QPushButton(tr("Add"));
	multiAddItem->setToolTip(tr("Add new item to argument list"));
	multiAddItem->setWhatsThis(tr("Click this button to add a new item to the argument list, "
		"then double click the item to edit the text."));
	multiArgLayout->addWidget(multiAddItem, 2, 0);
	QPushButton* multiDelItem = new QPushButton(tr("Del"));
	multiDelItem->setToolTip(tr("Delete selected items"));
	multiDelItem->setWhatsThis(tr("Click this button to delete the currently selected items "
		"from the arguments list.  You can select multiple items in the list using the "
		"<b>control</b> and <b>shit</b> keys while clicking."));
	multiArgLayout->addWidget(multiDelItem, 2, 1);
	stackedLayout->addWidget(multiArgLayoutWidget);
	mainLayout->addRow(new QLabel(tr("Argument")), stackedLayout);
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	mainLayout->addRow(nullptr, buttons);

	m_command->addItem(tr("Start applications"), CMD_APP_STARTAPPS);
	m_command->addItem(tr("Stop applications"), CMD_APP_STOPAPPS);
	m_command->addItem(tr("Restart applications"), CMD_APP_RESTARTAPPS);
	m_command->addItem(tr("Start group"), CMD_GROUP_STARTGROUP);
	m_command->addItem(tr("Stop group"), CMD_GROUP_STOPGROUP);
	m_command->addItem(tr("Trigger events"), CMD_SCHED_TRIGGEREVENTS);
	stackedLayout->setCurrentIndex(1);

	m_layoutIndexMap[CMD_APP_STARTAPPS] = 1;
	m_layoutIndexMap[CMD_APP_STOPAPPS] = 1;
	m_layoutIndexMap[CMD_APP_RESTARTAPPS] = 1;
	m_layoutIndexMap[CMD_GROUP_STARTGROUP] = 0;
	m_layoutIndexMap[CMD_GROUP_STOPGROUP] = 0;
	m_layoutIndexMap[CMD_SCHED_TRIGGEREVENTS] = 1;

	m_groupMap[CMD_APP_STARTAPPS] = GROUP_APP;
	m_groupMap[CMD_APP_STOPAPPS] = GROUP_APP;
	m_groupMap[CMD_APP_RESTARTAPPS] = GROUP_APP;
	m_groupMap[CMD_GROUP_STARTGROUP] = GROUP_GROUP;
	m_groupMap[CMD_GROUP_STOPGROUP] = GROUP_GROUP;
	m_groupMap[CMD_SCHED_TRIGGEREVENTS] = GROUP_SCHEDULE;

	connect(m_command, QOverload<int>::of(&QComboBox::currentIndexChanged),
		[this, stackedLayout](int index)
	{
		QString command = m_command->itemData(index).toString();
		stackedLayout->setCurrentIndex(m_layoutIndexMap[command]);
	});

	connect(multiAddItem, &QPushButton::clicked,
		this, [this]()
	{
		// Add item this way to make it editable
		QListWidgetItem* item = new QListWidgetItem(tr("[item]"), m_multiArgument);
		item->setFlags(item->flags().setFlag(Qt::ItemIsEditable, true));
	});

	connect(multiDelItem, &QPushButton::clicked,
		this, [this]()
	{
		QList<QListWidgetItem*> itemList = m_multiArgument->selectedItems();

		for (const auto& item : itemList)
		{
			delete item;
		}
	});

	connect(buttons, &QDialogButtonBox::rejected,
		this, &CustomActionDialog::reject);
	connect(buttons, &QDialogButtonBox::accepted,
		this, [this]()
	{
		int layoutIndex = m_layoutIndexMap[m_command->currentData().toString()];
		if (m_name->text().isEmpty())
		{
			QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
				tr("Enter a name for this button"));
		}
		else if (m_usedNames.contains(m_name->text()))
		{
			QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
				tr("Enter a name that has not already been used"));
		}
		else if ((0 == layoutIndex && m_singleArgument->text().isEmpty()) ||
			(1 == layoutIndex && m_multiArgument->count() == 0))
		{
			QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
				tr("Enter %1argument%2 for this command")
				.arg(0 == layoutIndex ? "an " : "")
				.arg(0 == layoutIndex ? "" : "s"));
		}
		else
		{
			done(QDialog::Accepted);
		}
	});
}


CustomActionDialog::~CustomActionDialog()
{
}


QString CustomActionDialog::getCommandName()
{
	return m_name->text();
}


QVariantList CustomActionDialog::getCommandVariantList()
{
	QString command = m_command->currentData().toString();
	QVariantList vlist;
	vlist << CMD_COMMAND << m_groupMap[command] << command;
	if (0 == m_layoutIndexMap[command])
	{
		vlist << m_singleArgument->text();
	}
	else
	{
		QStringList argList;
		for (int x = 0; x < m_multiArgument->count(); x++)
		{
			argList.append(m_multiArgument->item(x)->text());
		}
		vlist << argList;
	}
	return vlist;
}


void CustomActionDialog::setUsedNames(const QStringList& names)
{
	m_usedNames = names;
}

