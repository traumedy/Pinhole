
#include "Global.h"
#include "GuiUtil.h"
#include "../common/PinholeCommon.h"

#include "HostConfigGroupsWidget.h"

#include <QResizeEvent>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QCheckBox>


HostConfigGroupsWidget::HostConfigGroupsWidget(QWidget *parent)
	: HostConfigWidgetTab(parent)
{
	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	ScrollAreaEx* buttonScrollArea = new ScrollAreaEx;
	buttonScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
	QScrollArea* detailsScrollArea = new QScrollArea;
	mainLayout->addWidget(buttonScrollArea);
	mainLayout->addWidget(detailsScrollArea);

	QWidget* buttonScrollAreaWidgetContents = new QWidget;
	buttonScrollAreaWidgetContents->setProperty("borderless", true);
	QVBoxLayout* buttonLayout = new QVBoxLayout(buttonScrollAreaWidgetContents);
	m_addGroupButton = new QPushButton(tr("Add group"));
	m_addGroupButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_addGroupButton);
	m_deleteGroupButton = new QPushButton(tr("Delete group"));
	m_deleteGroupButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_deleteGroupButton);
	m_renameGroupButton = new QPushButton(tr("Rename group"));
	m_renameGroupButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_renameGroupButton);
	m_startGroupButton = new QPushButton(tr("Start group"));
	m_startGroupButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_startGroupButton);
	m_stopGroupButton = new QPushButton(tr("Stop group"));
	m_stopGroupButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	m_groupList = new QListWidget;
	m_groupList->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_groupList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_groupList->setSortingEnabled(true);
	m_groupList->setMaximumWidth(BUTTONITEMWIDTH);
	m_groupList->setMaximumHeight(ITEMLISTMAXHEIGHT);
	connect(m_groupList, &QListWidget::itemSelectionChanged,
		this, &HostConfigGroupsWidget::groupList_currentItemChanged);
	buttonLayout->addWidget(m_stopGroupButton);
	buttonLayout->addWidget(m_groupList);
	buttonScrollArea->setWidget(buttonScrollAreaWidgetContents);
	buttonScrollArea->show();

	QWidget* detailsScrollAreaWidgetContents = new QWidget;
	detailsScrollAreaWidgetContents->setProperty("borderless", true);
	detailsScrollAreaWidgetContents->setMinimumWidth(MINIMUM_DETAILS_WIDTH);
	QFormLayout* detailsLayout = new QFormLayout(detailsScrollAreaWidgetContents);
	detailsLayout->setSizeConstraint(QLayout::SetFixedSize);
	m_applicationList = new QListWidget;
	m_applicationList->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	m_applicationList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_applicationList->setMaximumHeight(ITEMLISTMAXHEIGHT);
	connect(m_applicationList, &QListWidget::itemSelectionChanged,
		this, &HostConfigGroupsWidget::applicationList_itemSelectionChanged);
	detailsLayout->addRow(new QLabel(tr("Applications")), m_applicationList);
	QHBoxLayout* appButtonsLayout = new QHBoxLayout;
	appButtonsLayout->setAlignment(Qt::AlignLeft);
	m_deleteAppButton = new QPushButton(tr("Delete app"));
	connect(m_deleteAppButton, &QPushButton::clicked,
		this, &HostConfigGroupsWidget::deleteAppButton_clicked);
	appButtonsLayout->addWidget(m_deleteAppButton);
	m_addAppButton = new QPushButton(tr("Add app"));
	connect(m_addAppButton, &QPushButton::clicked,
		this, &HostConfigGroupsWidget::addAppButton_clicked);
	appButtonsLayout->addWidget(m_addAppButton);
	m_appDropList = new QComboBox;
	appButtonsLayout->addWidget(m_appDropList, 2);
	detailsLayout->addRow(nullptr, appButtonsLayout);
	m_launchAtStart = new QCheckBox(tr("Launch group at start"));
	detailsLayout->addRow(nullptr, m_launchAtStart);
	detailsScrollArea->setWidget(detailsScrollAreaWidgetContents);
	detailsScrollArea->show();

	m_widgetMap =
	{
		{ PROP_GROUP_APPLICATIONS, m_applicationList },
		{ PROP_GROUP_LAUNCHATSTART, m_launchAtStart }
	};

	for (const auto& key : m_widgetMap.keys())
	{
		m_widgetMap[key]->setProperty(PROPERTY_GROUPNAME, GROUP_GROUP);
		m_widgetMap[key]->setProperty(PROPERTY_ITEMNAME, "");
		m_widgetMap[key]->setProperty(PROPERTY_PROPNAME, key);
	}

	m_addGroupButton->setToolTip(tr("Add a new group"));
	m_addGroupButton->setWhatsThis(tr("This button will add a new application group.  "
		"You will be prompted for the group name.  If a group already exists with "
		"that name you will get an error.  Group names are case sensitive."));
	m_deleteGroupButton->setToolTip(tr("Delete the selected group"));
	m_deleteGroupButton->setWhatsThis(tr("This button will delete the currently selected "
		"group."));
	m_renameGroupButton->setToolTip(tr("Rename the selected group"));
	m_renameGroupButton->setWhatsThis(tr("This button allows you to rename the currently "
		"selected group.  You will be prompted for the new group name.  If a "
		"group already exists with that name you will get an error."));
	m_startGroupButton->setToolTip(tr("Start the selected group"));
	m_startGroupButton->setWhatsThis(tr("This button will start the applications in the "
		"currently selected group.  You may get an error if one or more applications "
		"in the group fail to start."));
	m_stopGroupButton->setToolTip(tr("Stop the selected group"));
	m_stopGroupButton->setWhatsThis(tr("This button will stop the applications in the "
		"currently selected group.  You may get an error if one or more applications "
		"in the group fail to stop."));
	m_groupList->setToolTip(tr("The list of groups"));
	m_groupList->setWhatsThis(tr("This is the of application group names.  Click one of "
		"the names to select it."));
	m_applicationList->setToolTip(tr("The list of applications in this group"));
	m_applicationList->setWhatsThis(tr("This is the list of applications in the "
		"currently selected group.  You can select multiple names using the <b>control</b> "
		"and <b>shift</b> keys and the mouse for deleting multiple entries at once."));
	m_deleteAppButton->setToolTip(tr("Delete the selected applciations from the group"));
	m_deleteAppButton->setWhatsThis(tr("Click this button to delete the selected applications "
		"in the application list from the currently selected application group."));
	m_addAppButton->setToolTip(tr("Add the application selected in the drop down load to this group"));
	m_addAppButton->setWhatsThis(tr("Click this button to add the application currently "
		"selected in the dropdown to the right of the button to the application list."));
	m_appDropList->setToolTip(tr("Choose which application to add to this group"));
	m_appDropList->setWhatsThis(tr("This list contains all of the current applications.  "
		"Select one of these applications before clicking the <b>Add app</b> button."));
	m_launchAtStart->setToolTip(tr("Launch applcications in this group at when Pinhole starts"));
	m_launchAtStart->setWhatsThis(tr("If this is checked Pinhole will start the applications "
		"in this group even if the applications themselves are not marked to launch at start."));
}


HostConfigGroupsWidget::~HostConfigGroupsWidget()
{
}


void HostConfigGroupsWidget::resetWidgets()
{
	m_groupList->clear();
	m_appDropList->clear();
	resetGroupWidgets();
}


void HostConfigGroupsWidget::resetGroupWidgets()
{
	for (auto widget : m_widgetMap.values())
		ClearWidget(widget);
}


void HostConfigGroupsWidget::enableWidgets(bool enable)
{
	m_groupList->setEnabled(enable);
	m_addGroupButton->setEnabled(enable && m_editable);

	bool itemSelected = m_groupList->currentItem() != nullptr;

	m_startGroupButton->setEnabled(enable && itemSelected);
	m_stopGroupButton->setEnabled(enable && itemSelected);
	m_deleteGroupButton->setEnabled(enable && itemSelected && m_editable);
	m_renameGroupButton->setEnabled(enable && itemSelected && m_editable);

	EnableWidgetMap(m_widgetMap, enable && itemSelected && m_editable);

	m_appDropList->setEnabled(enable && itemSelected && m_editable);
	m_addAppButton->setEnabled(enable && itemSelected && m_editable);
	m_deleteAppButton->setEnabled(enable && itemSelected && m_editable && 
		!m_applicationList->selectedItems().isEmpty());
}


void HostConfigGroupsWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
}


void HostConfigGroupsWidget::addAppButton_clicked()
{
	QString appName = m_appDropList->currentText();
	if (m_applicationList->findItems(appName, Qt::MatchExactly).isEmpty())
	{
		m_applicationList->addItem(appName);
		emit widgetValueChanged(m_applicationList);
	}
}


void HostConfigGroupsWidget::deleteAppButton_clicked()
{
	QList<QListWidgetItem*> itemList = m_applicationList->selectedItems();

	for (const auto& item : itemList)
	{
		delete item;
	}

	emit widgetValueChanged(m_applicationList);
}


void HostConfigGroupsWidget::groupList_currentItemChanged()
{
	resetGroupWidgets();

	QString newGroupName;
	QListWidgetItem* currentItem = m_groupList->currentItem();
	if (currentItem)
		newGroupName = currentItem->text();

	enableWidgets(true);

	emit groupChanged(newGroupName);
}


void HostConfigGroupsWidget::applicationList_itemSelectionChanged()
{
	m_deleteAppButton->setEnabled(!m_applicationList->selectedItems().isEmpty());
}

