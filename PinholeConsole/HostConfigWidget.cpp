#include "HostConfigWidget.h"
#include "TextViewerDialog.h"
#include "HostConfigAppsWidget.h"
#include "HostConfigGroupsWidget.h"
#include "HostConfigGlobalsWidget.h"
#include "HostConfigScheduleWidget.h"
#include "HostConfigAlertWidget.h"
#include "HostConfigWidgetTab.h"
#include "ScheduleEventOffsetWidget.h"
#include "Global.h"
#include "../common/HostClient.h"

#include <QTabWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QMap>
#include <QGridLayout>
#include <QTableWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QCoreApplication>


HostConfigWidget::HostConfigWidget(QWidget *parent)
	: QWidget(parent)
{
	QGridLayout* gridLayout = new QGridLayout(this);
	m_tabWidget = new QTabWidget;
	m_makeChangesCheck = new QCheckBox(tr("Make changes"));
	m_tabWidget->setCornerWidget(m_makeChangesCheck, Qt::TopRightCorner);
	gridLayout->addWidget(m_tabWidget);

	int index = 0;
	m_hostConfigAppsWidget = new HostConfigAppsWidget(m_tabWidget);
	m_configWidgets.push_back(m_hostConfigAppsWidget);
	index = m_tabWidget->addTab(m_hostConfigAppsWidget, tr("Applications"));
	m_tabWidget->setTabToolTip(index, tr("Applications to launch"));
	m_tabWidget->setTabWhatsThis(index, tr("The <b>Applications</b> tab allows the configuring of the "
		"applications that Pinhole will launch."));
	m_hostConfigGroupsWidget = new HostConfigGroupsWidget(m_tabWidget);
	m_configWidgets.push_back(m_hostConfigGroupsWidget);
	index = m_tabWidget->addTab(m_hostConfigGroupsWidget, tr("Groups"));
	m_tabWidget->setTabToolTip(index, tr("Groupings of applications"));
	m_tabWidget->setTabWhatsThis(index, tr("The <b>Groups</b> tab allows the configuring of groups "
		"of applications that can be started/stopped together."));
	m_hostConfigGlobalsWidget = new HostConfigGlobalsWidget(m_tabWidget);
	m_configWidgets.push_back(m_hostConfigGlobalsWidget);
	index = m_tabWidget->addTab(m_hostConfigGlobalsWidget, tr("Global"));
	m_tabWidget->setTabToolTip(index, tr("Global settings"));
	m_tabWidget->setTabWhatsThis(index, tr("the <b>Global</b> tab allows the configuring of various "
		"settings used throughout Pinhole."));
	m_hostConfigScheduleWidget = new HostConfigScheduleWidget(m_tabWidget);
	m_configWidgets.push_back(m_hostConfigScheduleWidget);
	index = m_tabWidget->addTab(m_hostConfigScheduleWidget, tr("Schedule"));
	m_tabWidget->setTabToolTip(index, tr("Scheduled events"));
	m_tabWidget->setTabWhatsThis(index, tr("The <b>Scheduled events</b> tab allows the configuring of various "
		"types of events that can be triggered either on regular schedules or as "
		"one time events."));
	m_hostConfigAlertWidget = new HostConfigAlertWidget(m_tabWidget);
	m_configWidgets.push_back(m_hostConfigAlertWidget);
	index = m_tabWidget->addTab(m_hostConfigAlertWidget, tr("Alerts"));
	m_tabWidget->setTabToolTip(index, tr("Triggered alert behavior"));
	m_tabWidget->setTabWhatsThis(index, tr("The <b>Alerts</b> tab allows the configuring of Pinhole "
		"behavior when alerts are triggered.  Multiple recipients can be alerted when "
		"events occur."));

	resetWidgets();
	enableWidgets(false);

	// Connect property widgets to data change slots
	for (auto configWidget : m_configWidgets)
	{
		for (const auto& widget : configWidget->m_widgetMap)
		{
			if (nullptr != qobject_cast<QLineEdit*>(widget))
			{
				connect(qobject_cast<QLineEdit*>(widget), &QLineEdit::editingFinished,
					this, &HostConfigWidget::lineEditValueChanged);
			}
			else if (nullptr != qobject_cast<QCheckBox*>(widget))
			{
				connect(qobject_cast<QCheckBox*>(widget), &QCheckBox::clicked,
					this, &HostConfigWidget::checkValueChanged);
			}
			else if (nullptr != qobject_cast<QListWidget*>(widget))
			{
				connect(qobject_cast<QListWidget*>(widget), &QListWidget::itemChanged,
					this, &HostConfigWidget::listItemChanged);
			}
			else if (nullptr != qobject_cast<QSpinBox*>(widget))
			{
				connect(qobject_cast<QSpinBox*>(widget), (void(QSpinBox::*)())&QSpinBox::editingFinished,
					this, (void(HostConfigWidget::*)())&HostConfigWidget::spinBoxValueChanged);
			}
			else if (nullptr != qobject_cast<QComboBox*>(widget))
			{
				connect(qobject_cast<QComboBox*>(widget), (void (QComboBox::*)(int))&QComboBox::activated,
					this, &HostConfigWidget::comboBoxValueChanged);
			}
		}
	}

	// Get notifications when the application selection changes
	connect(m_hostConfigAppsWidget, &HostConfigAppsWidget::appChanged,
		this, &HostConfigWidget::appChanged);
	// Get notifications when the group selection changes
	connect(m_hostConfigGroupsWidget, &HostConfigGroupsWidget::groupChanged,
		this, &HostConfigWidget::groupChanged);
	// Make changes checkbox clicked
	connect(m_makeChangesCheck, &QCheckBox::clicked,
		this, &HostConfigWidget::makeChanges_clicked);
	// Notifications from HostConfigAppsWidget of widget values changing
	connect(m_hostConfigAppsWidget, &HostConfigAppsWidget::widgetValueChanged,
		this, &HostConfigWidget::setWidgetPropValue);
	// Start app button
	connect(m_hostConfigAppsWidget->m_startAppButton, &QPushButton::clicked,
		this, &HostConfigWidget::startApp);
	// Stop app button
	connect(m_hostConfigAppsWidget->m_stopAppButton, &QPushButton::clicked,
		this, &HostConfigWidget::stopApp);
	// App add button
	connect(m_hostConfigAppsWidget->m_addAppButton, &QPushButton::clicked,
		this, &HostConfigWidget::addApp);
	// App delete button
	connect(m_hostConfigAppsWidget->m_deleteAppButton, &QPushButton::clicked,
		this, &HostConfigWidget::deleteApp);
	// App rename button
	connect(m_hostConfigAppsWidget->m_renameAppButton, &QPushButton::clicked,
		this, &HostConfigWidget::renameApp);
	// Get app console output button
	connect(m_hostConfigAppsWidget->m_getConsoleOutput, &QPushButton::clicked,
		this, &HostConfigWidget::getAppConsoleOutput);
	// Notifications from HostConfigGroupsWidget of widget values changing
	connect(m_hostConfigGroupsWidget, &HostConfigGroupsWidget::widgetValueChanged,
		this, &HostConfigWidget::setWidgetPropValue);
	// Add group button
	connect(m_hostConfigGroupsWidget->m_addGroupButton, &QPushButton::clicked,
		this, &HostConfigWidget::addGroup);
	// Delete group button
	connect(m_hostConfigGroupsWidget->m_deleteGroupButton, &QPushButton::clicked,
		this, &HostConfigWidget::deleteGroup);
	// Rename group button
	connect(m_hostConfigGroupsWidget->m_renameGroupButton, &QPushButton::clicked,
		this, &HostConfigWidget::renameGroup);
	// Notifications from HostConfigGlobalsWidget of widget values changing
	connect(m_hostConfigGlobalsWidget, &HostConfigGlobalsWidget::widgetValueChanged,
		this, &HostConfigWidget::setWidgetPropValue);
	// Add event button
	connect(m_hostConfigScheduleWidget->m_addEventButton, &QPushButton::clicked,
		this, &HostConfigWidget::addEvent);
	// Delete event button
	connect(m_hostConfigScheduleWidget->m_deleteEventButton, &QPushButton::clicked,
		this, &HostConfigWidget::deleteEvent);
	// Rename event button
	connect(m_hostConfigScheduleWidget->m_renameEventButton, &QPushButton::clicked,
		this, &HostConfigWidget::renameEvent);
	// Trigger event button
	connect(m_hostConfigScheduleWidget->m_triggerEventButton, &QPushButton::clicked,
		this, &HostConfigWidget::triggerEvent);
	// Add alert slot button
	connect(m_hostConfigAlertWidget->m_addAlertSlotButton, &QPushButton::clicked,
		this, &HostConfigWidget::addAlertSlot);
	// Delete alert slot button
	connect(m_hostConfigAlertWidget->m_deleteAlertSlotButton, &QPushButton::clicked,
		this, &HostConfigWidget::deleteAlertSlot);
	// Rename alert slot button
	connect(m_hostConfigAlertWidget->m_renameAlertSlotButton, &QPushButton::clicked,
		this, &HostConfigWidget::renameAlertSlot);
	// Reset alert count button
	connect(m_hostConfigAlertWidget->m_resetAlertsButton, &QPushButton::clicked,
		this, &HostConfigWidget::resetAlertCount);
	// Retrieve alerts button
	connect(m_hostConfigAlertWidget->m_retrieveAlertsButton, &QPushButton::clicked,
		this, &HostConfigWidget::retrieveAlerts);

	m_makeChangesCheck->setToolTip(tr("This must be checked to make changes to host configuration"));
	m_makeChangesCheck->setWhatsThis(tr("In order to prevent accidentally making changes to "
		"configuration, you must enable this check box to enable the fields in the "
		"host configuration panel."));
}


HostConfigWidget::~HostConfigWidget()
{
}


void HostConfigWidget::hostChanged(const QString& newHost, int port, const QString& hostId)
{
	m_currentHost = newHost;
	resetWidgets();
	enableWidgets(false);
	m_hostClient.clear();
	m_currentApp.clear();
	m_currentGroup.clear();

	if (!newHost.isEmpty())
	{
		emit setStatusText(tr("Trying to connect to %1 port %2...").arg(newHost).arg(port));

		m_hostClient = QSharedPointer<HostClient>::create(newHost, port, hostId, false, false, this);
		connect(m_hostClient.data(), &HostClient::valueUpdate,
			this, &HostConfigWidget::hostValueChanged);
		connect(m_hostClient.data(), &HostClient::missingValue,
			this, &HostConfigWidget::HostConfigWidget::hostMissingValue);
		connect(m_hostClient.data(), &HostClient::commandError,
			this, &HostConfigWidget::HostConfigWidget::hostCommandError);
		connect(m_hostClient.data(), &HostClient::commandMissing,
			this, &HostConfigWidget::HostConfigWidget::hostCommandMissing);
		connect(m_hostClient.data(), &HostClient::commandData,
			this, &HostConfigWidget::hostCommandData);
		connect(m_hostClient.data(), &HostClient::connected,
			this, &HostConfigWidget::hostConnected);
		connect(m_hostClient.data(), &HostClient::disconnected,
			this, &HostConfigWidget::hostDisconnected);
	}
}


void HostConfigWidget::resetWidgets()
{
	for (auto widget : m_configWidgets)
	{
		widget->resetWidgets();
	}
}


void HostConfigWidget::enableWidgets(bool enable)
{
	for (auto widget : m_configWidgets)
	{
		widget->enableWidgets(enable);
	}
}


void HostConfigWidget::setEditable(bool editable)
{
	for (auto widget : m_configWidgets)
	{
		widget->setEditable(editable);
	}

	enableWidgets(!m_hostClient.isNull() && m_hostClient->isConnected());
}


void HostConfigWidget::fillAppValues()
{
	if (m_currentApp.isEmpty())
		return;

	for (const auto& widget : m_hostConfigAppsWidget->m_widgetMap)
	{
		widget->setProperty(PROPERTY_ITEMNAME, m_currentApp);
		QString groupName = widget->property(PROPERTY_GROUPNAME).toString();
		QString propName = widget->property(PROPERTY_PROPNAME).toString();
		m_hostClient->getVariant(groupName, m_currentApp, propName);
	}

	m_hostClient->getVariant(GROUP_APP, m_currentApp, PROP_APP_RUNNING);
}


void HostConfigWidget::fillGroupValues()
{
	if (m_currentGroup.isEmpty())
		return;

	for (const auto& widget : m_hostConfigGroupsWidget->m_widgetMap)
	{
		widget->setProperty(PROPERTY_ITEMNAME, m_currentGroup);
		QString groupName = widget->property(PROPERTY_GROUPNAME).toString();
		QString propName = widget->property(PROPERTY_PROPNAME).toString();
		m_hostClient->getVariant(groupName, m_currentGroup, propName);
	}
}


void HostConfigWidget::startApp()
{
	m_hostClient->startApps(QStringList(m_currentApp));
}


void HostConfigWidget::stopApp()
{
	m_hostClient->stopApps(QStringList(m_currentApp));
}


void HostConfigWidget::addApp()
{
	bool ok;
	QString appName = QInputDialog::getText(this, tr("New application"),
		tr("Enter name for new application"), QLineEdit::Normal,
		"", &ok);

	if (ok && !appName.isEmpty())
	{
		m_hostClient->addApplication(appName);
	}
}


void HostConfigWidget::deleteApp()
{
	QModelIndexList selection = m_hostConfigAppsWidget->m_appList->selectionModel()->selectedRows();

	if (1 == selection.size())
	{
		QString appName = selection[0].data().toString();
		QString message = tr("Delete application '%1'?").arg(appName);
		QMessageBox msgBox(this);
		msgBox.setText(message);
		msgBox.setInformativeText(tr("The application will be permanently removed."));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		if (QMessageBox::Yes == msgBox.exec())
		{
			m_hostClient->deleteApplication(appName);
		}
	}
}


void HostConfigWidget::renameApp()
{
	QModelIndexList selection = m_hostConfigAppsWidget->m_appList->selectionModel()->selectedRows();

	if (1 == selection.size())
	{
		QString appName = selection[0].data().toString();
		bool ok;
		QString appNewName = QInputDialog::getText(this, tr("Rename application"),
			tr("Enter new name for application '%1'").arg(appName), QLineEdit::Normal,
			appName, &ok);

		if (ok && !appName.isEmpty() && appName != appNewName)
		{
			m_hostClient->renameApplication(appName, appNewName);
		}
	}
}


void HostConfigWidget::getAppConsoleOutput()
{
	QModelIndexList selection = m_hostConfigAppsWidget->m_appList->selectionModel()->selectedRows();

	if (1 == selection.size())
	{
		QString appName = selection[0].data().toString();
		m_hostClient->getApplicationConsoleOutput(appName);
	}
}


void HostConfigWidget::addGroup()
{
	bool ok;
	QString groupName = QInputDialog::getText(this, tr("New group"),
		tr("Enter name for new group"), QLineEdit::Normal,
		"", &ok);

	if (ok && !groupName.isEmpty())
	{
		m_hostClient->addGroup(groupName);
	}
}


void HostConfigWidget::deleteGroup()
{
	QModelIndexList selection = m_hostConfigGroupsWidget->m_groupList->selectionModel()->selectedRows();

	if (1 == selection.size())
	{
		QString groupName = selection[0].data().toString();
		QString message = tr("Delete group '%1'?").arg(groupName);
		QMessageBox msgBox(this);
		msgBox.setText(message);
		msgBox.setInformativeText(tr("The group will be permanently removed."));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		if (QMessageBox::Yes == msgBox.exec())
		{
			m_hostClient->deleteGroup(groupName);
		}
	}
}


void HostConfigWidget::renameGroup()
{
	QModelIndexList selection = m_hostConfigGroupsWidget->m_groupList->selectionModel()->selectedRows();

	if (1 == selection.size())
	{
		QString groupName = selection[0].data().toString();
		bool ok;
		QString groupNewName = QInputDialog::getText(this, tr("Rename group"),
			tr("Enter new name for group"), QLineEdit::Normal,
			groupName, &ok);

		if (ok && !groupName.isEmpty() && groupName != groupNewName)
		{
			m_hostClient->renameGroup(groupName, groupNewName);
		}
	}
}


void HostConfigWidget::addEvent()
{
	bool ok;
	QString eventName = QInputDialog::getText(this, tr("New event"),
		tr("Enter name for new event"), QLineEdit::Normal,
		"", &ok);

	if (ok && !eventName.isEmpty())
	{
		m_hostClient->addScheduleEvent(eventName);
	}
}


void HostConfigWidget::deleteEvent()
{
	QModelIndexList selectedRows = m_hostConfigScheduleWidget->m_scheduleList->selectionModel()->selectedRows();

	QString message = tr("Delete %1 scheduled event%2?").arg(selectedRows.size()).arg(1 == selectedRows.size() ? "" : "s");
	QMessageBox msgBox(this);
	msgBox.setText(message);
	msgBox.setInformativeText(tr("The scheduled event%1 will be permanently removed.").arg(1 == selectedRows.size() ? "" : "s"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	if (QMessageBox::Yes != msgBox.exec())
		return;

	for (const auto& index : selectedRows)
	{
		QString name = m_hostConfigScheduleWidget->m_scheduleList->verticalHeaderItem(index.row())->text();
		m_hostClient->deleteScheduleEvent(name);
	}
}


void HostConfigWidget::renameEvent()
{
	QModelIndexList selectedRows = m_hostConfigScheduleWidget->m_scheduleList->selectionModel()->selectedRows();

	if (selectedRows.size() != 1)
		return;

	QString oldName = m_hostConfigScheduleWidget->m_scheduleList->verticalHeaderItem(selectedRows[0].row())->text();
	bool ok;
	QString newName = QInputDialog::getText(this, tr("Rename scheduled event"),
		tr("Enter new name for scheduled event '%1'").arg(oldName), QLineEdit::Normal,
		oldName, &ok);
	
	if (ok && !newName.isEmpty())
	{
		m_hostClient->renameScheduleEvent(oldName, newName);
	}
}


void HostConfigWidget::triggerEvent()
{
	QModelIndexList selectedRows = m_hostConfigScheduleWidget->m_scheduleList->selectionModel()->selectedRows();

	QStringList names;
	for (const auto& index : selectedRows)
	{
		QString name = m_hostConfigScheduleWidget->m_scheduleList->verticalHeaderItem(index.row())->text();
		names.append(name);
	}

	if (!names.isEmpty())
	{
		m_hostClient->triggerScheduleEvents(names);
	}
}


void HostConfigWidget::addAlertSlot()
{
	bool ok;
	QString slotName = QInputDialog::getText(this, tr("New alert slot"),
		tr("Enter name for alert slot"), QLineEdit::Normal,
		"", &ok);

	if (ok && !slotName.isEmpty())
	{
		m_hostClient->addAlertSlot(slotName);
	}
}


void HostConfigWidget::deleteAlertSlot()
{
	QModelIndexList selectedRows = m_hostConfigAlertWidget->m_alertSlotList->selectionModel()->selectedRows();

	if (selectedRows.isEmpty())
		return;

	QString message = tr("Delete %1 alert slot%1?").arg(selectedRows.size()).arg(1 == selectedRows.size() ? "" : "s");
	QMessageBox msgBox(this);
	msgBox.setText(message);
	msgBox.setInformativeText(tr("The alert slot%1 will be permanently removed.").arg(1 == selectedRows.size() ? "" : "s"));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	if (QMessageBox::Yes != msgBox.exec())
		return;

	for (const auto& index : selectedRows)
	{
		QString name = m_hostConfigAlertWidget->m_alertSlotList->verticalHeaderItem(index.row())->text();
		m_hostClient->deleteAlertSlot(name);
	}
}


void HostConfigWidget::renameAlertSlot()
{
	QModelIndexList selectedRows = m_hostConfigAlertWidget->m_alertSlotList->selectionModel()->selectedRows();

	if (selectedRows.size() != 1)
		return;

	QString oldName = m_hostConfigAlertWidget->m_alertSlotList->verticalHeaderItem(selectedRows[0].row())->text();
	bool ok;
	QString newName = QInputDialog::getText(this, tr("Rename alert slot"),
		tr("Enter new name for alert slot '%1'").arg(oldName), QLineEdit::Normal,
		oldName, &ok);

	if (ok && !newName.isEmpty())
	{
		m_hostClient->renameAlertSlot(oldName, newName);
	}
}


void HostConfigWidget::resetAlertCount()
{
	m_hostClient->resetAlertCount();
}


void HostConfigWidget::retrieveAlerts()
{
	m_hostClient->retrieveAlertList();
}


void HostConfigWidget::lineEditValueChanged()
{
	if (m_selfupdatingWidget)
		return;

	// Get the property name
	QObject* obj = sender();
	QString groupName = obj->property(PROPERTY_GROUPNAME).toString();
	QString itemName = obj->property(PROPERTY_ITEMNAME).toString();
	QString propName = obj->property(PROPERTY_PROPNAME).toString();

	if (groupName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QLineEdit group name"));
		return;
	}

	if (propName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QLineEdit property name"));
		return;
	}

	m_hostClient->setVariant(groupName, itemName, propName, QVariant(qobject_cast<QLineEdit*>(obj)->text()));
}


void HostConfigWidget::checkValueChanged(bool checked)
{
	if (m_selfupdatingWidget)
		return;

	// Get the property name
	QObject* obj = sender();
	QString groupName = obj->property(PROPERTY_GROUPNAME).toString();
	QString itemName = obj->property(PROPERTY_ITEMNAME).toString();
	QString propName = obj->property(PROPERTY_PROPNAME).toString();

	if (groupName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QCheckBox group name"));
		return;
	}

	if (propName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QCheckBox property name"));
		return;
	}

	m_hostClient->setVariant(groupName, itemName, propName, QVariant(checked));
}


void HostConfigWidget::comboBoxValueChanged(int index)
{
	if (m_selfupdatingWidget)
		return;

	// Get the property name
	QObject* obj = sender();
	QString groupName = obj->property(PROPERTY_GROUPNAME).toString();
	QString itemName = obj->property(PROPERTY_ITEMNAME).toString();
	QString propName = obj->property(PROPERTY_PROPNAME).toString();

	if (groupName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QComboBox group name"));
		return;
	}

	if (propName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QComboBox property name"));
		return;
	}

	QVariant value = qobject_cast<QComboBox*>(obj)->itemData(index);

	m_hostClient->setVariant(groupName, itemName, propName, value);
}


void HostConfigWidget::spinBoxValueChanged()
{
	if (m_selfupdatingWidget)
		return;

	// Get the property name
	QObject* obj = sender();
	QString groupName = obj->property(PROPERTY_GROUPNAME).toString();
	QString itemName = obj->property(PROPERTY_ITEMNAME).toString();
	QString propName = obj->property(PROPERTY_PROPNAME).toString();

	if (groupName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QSpinBox group name"));
		return;
	}

	if (propName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QSpinBox property name"));
		return;
	}

	m_hostClient->setVariant(groupName, itemName, propName, QVariant(qobject_cast<QSpinBox*>(obj)->value()));
}


void HostConfigWidget::offsetValueChanged(int offset)
{
	if (m_selfupdatingWidget)
		return;

	// Get the property name
	QObject* obj = sender();
	QString groupName = obj->property(PROPERTY_GROUPNAME).toString();
	QString itemName = obj->property(PROPERTY_ITEMNAME).toString();
	QString propName = obj->property(PROPERTY_PROPNAME).toString();

	if (groupName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty offset group name"));
		return;
	}

	if (propName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty offset property name"));
		return;
	}

	m_hostClient->setVariant(groupName, itemName, propName, offset);
}


void HostConfigWidget::listItemChanged(QListWidgetItem *item)
{
	Q_UNUSED(item);

	if (m_selfupdatingWidget)
		return;

	// Get the property name
	QObject* obj = sender();
	QString groupName = obj->property(PROPERTY_GROUPNAME).toString();
	QString itemName = obj->property(PROPERTY_ITEMNAME).toString();
	QString propName = obj->property(PROPERTY_PROPNAME).toString();

	if (groupName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QListWidget group name"));
		return;
	}

	if (propName.isEmpty())
	{
		emit setNoticeText(tr("Internal error: empty QListWidget property name"));
		return;
	}

	setQListWidgetPropValue(qobject_cast<QListWidget*>(obj), groupName, itemName, propName);
}


void HostConfigWidget::hostValueChanged(const QString& group, const QString& item, const QString& property, const QVariant& value)
{
	// Set this flag so we don't trigger when we modify the value ourself
	m_selfupdatingWidget = true;

	if (GROUP_APP == group)
	{
		setApplicationValue(item, property, value);
	}
	else if (GROUP_GROUP == group)
	{
		setGroupValue(item, property, value);
	}
	else if (GROUP_GLOBAL == group)
	{
		setWidgetMapPropValue(m_hostConfigGlobalsWidget->m_widgetMap, property, value);
	}
	else if (GROUP_SCHEDULE == group)
	{
		setScheduleValue(item, property, value);
	}
	else if (GROUP_ALERT == group)
	{
		setAlertValue(item, property, value);
	}

	m_selfupdatingWidget = false;
}


void HostConfigWidget::hostMissingValue(const QString& group, const QString& item, const QString& property)
{
	if (GROUP_APP == group)
	{
		if (!m_hostConfigAppsWidget->m_widgetMap.contains(property))
		{
			emit setNoticeText(tr("Internal error: Missing application widget property '%1'").arg(property));
		}
		else
		{
			m_hostConfigAppsWidget->m_widgetMap[property]->setEnabled(false);
			m_hostConfigAppsWidget->m_widgetMap[property]->setProperty(PROPERTY_SUPPORTED, QVariant(false));
		}
	}
	else if (GROUP_GROUP == group)
	{
		if (!m_hostConfigGroupsWidget->m_widgetMap.contains(property))
		{
			emit setNoticeText(tr("Internal error: Missing group widget property '%1'").arg(property));
		}
		else
		{
			m_hostConfigGroupsWidget->m_widgetMap[property]->setEnabled(false);
			m_hostConfigGroupsWidget->m_widgetMap[property]->setProperty(PROPERTY_SUPPORTED, QVariant(false));
		}
	}
	else if (GROUP_GLOBAL == group)
	{
		if (!m_hostConfigGlobalsWidget->m_widgetMap.contains(property))
		{
			emit setNoticeText(tr("Internal error: Missing global widget property '%1'").arg(property));
		}
		else
		{
			m_hostConfigGlobalsWidget->m_widgetMap[property]->setEnabled(false);
			m_hostConfigGlobalsWidget->m_widgetMap[property]->setProperty(PROPERTY_SUPPORTED, QVariant(false));
		}
	}
	else if (GROUP_SCHEDULE == group)
	{
		if (!scheduleProperties.contains(property))
		{
			emit setNoticeText(tr("Internal error: Missing schedule widget property '%1'").arg(property));
		}
		else
		{
			for (int row = 0; row < m_hostConfigScheduleWidget->m_scheduleList->rowCount(); row++)
			{
				if (item == m_hostConfigScheduleWidget->m_scheduleList->verticalHeaderItem(row)->text())
				{
					int column = scheduleProperties.indexOf(property);
					m_hostConfigScheduleWidget->m_scheduleList->cellWidget(row, column)->setEnabled(false);
					m_hostConfigScheduleWidget->m_scheduleList->cellWidget(row, column)->setProperty(PROPERTY_SUPPORTED, QVariant(false));
					break;
				}
			}
		}
	}
	else if (GROUP_ALERT == group)
	{
		if (!alertSlotProperties.contains(property))
		{
			emit setNoticeText(tr("Internal error: Missing alert widget property '%1'").arg(property));
		}
		else
		{
			for (int row = 0; row < m_hostConfigAlertWidget->m_alertSlotList->rowCount(); row++)
			{
				if (item == m_hostConfigAlertWidget->m_alertSlotList->verticalHeaderItem(row)->text())
				{
					int column = alertSlotProperties.indexOf(property);
					m_hostConfigAlertWidget->m_alertSlotList->cellWidget(row, column)->setEnabled(false);
					m_hostConfigAlertWidget->m_alertSlotList->cellWidget(row, column)->setProperty(PROPERTY_SUPPORTED, QVariant(false));
				}
				break;
			}
		}
	}
}


void HostConfigWidget::hostCommandError(const QString& group, const QString& command)
{
	QMessageBox messageBox(this);
	messageBox.setText(tr("A host error occured while executing the command '%1:%2'")
		.arg(group)
		.arg(command));
	messageBox.exec();
}


void HostConfigWidget::hostCommandMissing(const QString& group, const QString& command)
{
	QMessageBox messageBox(this);
	messageBox.setText(tr("The host does not recognize the command '%1:%2'")
		.arg(group)
		.arg(command));
	messageBox.setInformativeText(tr("You are probably running a newer of the software than the host.\n"
		"Client version: %1\nServer version: %2\nTry upgrading the host server?")
		.arg(QCoreApplication::applicationVersion())
		.arg(m_hostClient->getHostVersion()));
	messageBox.exec();
}


void HostConfigWidget::hostCommandData(const QString& group, const QString& command, const QVariant& data)
{
	if (GROUP_APP == group)
	{
		if (CMD_APP_GETCONSOLE == command)
		{
			TextViewerDialog* textViewer = new TextViewerDialog(tr("ConsoleOutput-%1").arg(m_currentApp),
				m_hostClient->getHostAddress(), m_hostClient->getHostName(), data.toByteArray(), this);
			textViewer->show();
		}
	}
	else if (GROUP_ALERT == group)
	{
		if (CMD_ALERT_RETRIEVELIST == command)
		{
			TextViewerDialog* textViewer = new TextViewerDialog(tr("Alerts"),
				m_hostClient->getHostAddress(), m_hostClient->getHostName(), data.toByteArray(), this);
			textViewer->show();
		}
	}
}


void HostConfigWidget::hostConnected()
{
	emit setStatusText(tr("Connected to %1 (%2) v%3")
		.arg(m_hostClient->getHostName())
		.arg(m_hostClient->getHostAddress())
		.arg(m_hostClient->getHostVersion()));

	m_hostClient->subscribeToCommand(CMD_VALUE);
	m_hostClient->subscribeToGroup(GROUP_APP);
	m_hostClient->subscribeToGroup(GROUP_GROUP);
	m_hostClient->subscribeToGroup(GROUP_GLOBAL);
	m_hostClient->subscribeToGroup(GROUP_SCHEDULE);
	m_hostClient->subscribeToGroup(GROUP_ALERT);

	m_hostClient->getVariant(GROUP_APP, QString(), PROP_APP_LIST);
	m_hostClient->getVariant(GROUP_GROUP, QString(), PROP_GROUP_LIST);
	m_hostClient->getVariant(GROUP_SCHEDULE, QString(), PROP_SCHED_LIST);
	m_hostClient->getVariant(GROUP_ALERT, QString(), PROP_ALERT_SLOTLIST);

	for (const auto& widget : m_hostConfigGlobalsWidget->m_widgetMap)
	{
		QString group = widget->property(PROPERTY_GROUPNAME).toString();
		QString item = widget->property(PROPERTY_ITEMNAME).toString();
		QString prop = widget->property(PROPERTY_PROPNAME).toString();
		m_hostClient->getVariant(group, item, prop);
	}

	for (const auto& widget : m_hostConfigAlertWidget->m_widgetMap)
	{
		QString group = widget->property(PROPERTY_GROUPNAME).toString();
		QString item = widget->property(PROPERTY_ITEMNAME).toString();
		QString prop = widget->property(PROPERTY_PROPNAME).toString();
		m_hostClient->getVariant(group, item, prop);
	}

	enableWidgets(true);
}


void HostConfigWidget::hostDisconnected()
{
	if (!m_hostClient.isNull())
	{
		emit setStatusText(tr("Disconnected from %1 (%2), trying to reconnect...")
			.arg(m_hostClient->getHostName())
			.arg(m_hostClient->getHostAddress()));
	}
	resetWidgets();
	enableWidgets(false);
}


void HostConfigWidget::setWidgetPropValue(QWidget* widget)
{
	QString groupName = widget->property(PROPERTY_GROUPNAME).toString();
	QString itemName = widget->property(PROPERTY_ITEMNAME).toString();
	QString propName = widget->property(PROPERTY_PROPNAME).toString();

	if (nullptr != qobject_cast<QListWidget*>(widget))
	{
		setQListWidgetPropValue(qobject_cast<QListWidget*>(widget), groupName, itemName, propName);
	}
}


void HostConfigWidget::setWidgetMapPropValue(const QMap<QString, QWidget*>& widgetMap, const QString& property, const QVariant& value)
{
	if (!widgetMap.contains(property))
	{
		emit setNoticeText(tr("Internal error: Missing widget property '%1'. Client may be out of date.").arg(property));
		return;
	}

	QWidget* widget = widgetMap[property];

#ifdef QT_DEBUG
	//qDebug() << __FUNCTION__ << widget->metaObject()->className() << property << value;
#endif

	// Mark this control as supported by this host
	widget->setProperty(PROPERTY_SUPPORTED, QVariant(true));
	if (m_makeChangesCheck->isChecked())
	{
		widget->setEnabled(true);
	}

	if (nullptr != qobject_cast<QLabel*>(widget))
	{
		qobject_cast<QLabel*>(widget)->setText(value.toString());
	}
	else if (nullptr != qobject_cast<QLineEdit*>(widget))
	{
		qobject_cast<QLineEdit*>(widget)->setText(value.toString());
	}
	else if (nullptr != qobject_cast<QCheckBox*>(widget))
	{
		qobject_cast<QCheckBox*>(widget)->setChecked(value.toBool());
	}
	else if (nullptr != qobject_cast<QSpinBox*>(widget))
	{
		qobject_cast<QSpinBox*>(widget)->setValue(value.toInt());
	}
	else if (nullptr != qobject_cast<QComboBox*>(widget))
	{
		QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
		comboBox->setCurrentIndex(comboBox->findData(value));
	}
	else if (nullptr != qobject_cast<QListWidget*>(widget))
	{
		QListWidget* object = qobject_cast<QListWidget*>(widget);
		object->clear();
		// Add items this way to make them editable
		QStringList stringList = value.toStringList();
		for (const auto& string : stringList)
		{
			QListWidgetItem* item = new QListWidgetItem(string, object);
			item->setFlags(item->flags().setFlag(Qt::ItemIsEditable, true));
		}
	}
}


void HostConfigWidget::setQListWidgetPropValue(QListWidget* listWidget, const QString& groupName,
	const QString& itemName, const QString& propName)
{
	QStringList stringList;
	for (int n = 0; n < listWidget->count(); n++)
	{
		stringList.append(listWidget->item(n)->text());
	}

	m_hostClient->setVariant(groupName, itemName, propName, QVariant(stringList));
}


void HostConfigWidget::appChanged(const QString& app)
{
	m_currentApp = app;
	if (!m_currentApp.isEmpty())
		fillAppValues();
}


void HostConfigWidget::groupChanged(const QString& group)
{
	m_currentGroup = group;
	if (!m_currentGroup.isEmpty())
		fillGroupValues();
}


void HostConfigWidget::makeChanges_clicked(bool checked)
{
	setEditable(checked);

	if (!m_hostClient.isNull() && m_hostClient->isConnected() && !m_currentApp.isEmpty())
	{
		// Update the start/stop app buttons
		m_hostClient->getVariant(GROUP_APP, m_currentApp, PROP_APP_RUNNING);
	}
}


void HostConfigWidget::setApplicationValue(const QString& item, const QString& property, const QVariant& value)
{
	if (item.isEmpty())
	{
		if (PROP_APP_LIST == property)
		{
			QString selectedApp = m_currentApp;
			m_currentApp.clear();	// Clear this so it doesn't try and update the fields
			QStringList appList = value.toStringList();
			m_hostConfigAppsWidget->m_appList->clear();
			m_hostConfigAppsWidget->m_appList->addItems(appList);

			// The application list is also used in the group properties page
			m_hostConfigGroupsWidget->m_appDropList->clear();
			m_hostConfigGroupsWidget->m_appDropList->addItems(appList);

			if (!selectedApp.isEmpty())
			{
				m_currentApp = selectedApp;

				// If an app was selected check if it is still in the list to reselect it
				if (appList.contains(m_currentApp))
				{
					// If the app is still in the list, reselect it
					m_hostConfigAppsWidget->m_appList->setCurrentRow(appList.indexOf(m_currentApp));
				}
				else
				{
					// If the app is no longer in the list, deselect app
					m_currentApp.clear();
					m_hostConfigAppsWidget->resetAppWidgets();
					m_hostConfigAppsWidget->enableWidgets(false);
					m_hostConfigAppsWidget->m_appList->setEnabled(true);
					m_hostConfigAppsWidget->m_addAppButton->setEnabled(true);
				}
			}
		}
	}
	else if (item == m_currentApp)
	{
		if (PROP_APP_NAME == property)
		{
			// App being renamed
			m_currentApp = value.toString();
		}
		else if (PROP_APP_RUNNING == property)
		{
			bool running = value.toBool();
			m_hostConfigAppsWidget->m_startAppButton->setEnabled(!running);
			m_hostConfigAppsWidget->m_stopAppButton->setEnabled(running);
		}
		else
		{
			// If this item is in our currently selected app, update the widget with this property
			setWidgetMapPropValue(m_hostConfigAppsWidget->m_widgetMap, property, value);
		}
	}
}


void HostConfigWidget::setGroupValue(const QString& item, const QString& property, const QVariant& value)
{
	if (item.isEmpty())
	{
		if (PROP_GROUP_LIST == property)
		{
			QString selectedGroup = m_currentGroup;
			m_currentGroup.clear();		// Clear this so it doesn't try and update the fields
			QStringList groupList = value.toStringList();
			m_hostConfigGroupsWidget->m_groupList->clear();
			m_hostConfigGroupsWidget->m_groupList->addItems(groupList);

			if (!selectedGroup.isEmpty())
			{
				m_currentGroup = selectedGroup;

				// If an app was selected check if it is still in the list to reselect it
				if (groupList.contains(m_currentGroup))
				{
					// If group is still in the list, reselect it
					m_hostConfigGroupsWidget->m_groupList->setCurrentRow(groupList.indexOf(m_currentGroup));
				}
				else
				{
					// If group is no longer in list, deselect it
					m_currentGroup.clear();
					m_hostConfigGroupsWidget->resetGroupWidgets();
					m_hostConfigGroupsWidget->enableWidgets(false);
					m_hostConfigGroupsWidget->m_groupList->setEnabled(true);
					m_hostConfigGroupsWidget->m_addGroupButton->setEnabled(true);
				}
			}
		}
	}
	else if (item == m_currentGroup)
	{
		if (PROP_GROUP_NAME == property)
		{
			// Group being renamed
			m_currentGroup = value.toString();
		}
		else
		{
			// If this item is in our currently selected group, update the widget with this property
			setWidgetMapPropValue(m_hostConfigGroupsWidget->m_widgetMap, property, value);
		}
	}
}


void HostConfigWidget::setScheduleValue(const QString& item, const QString& property, const QVariant& value)
{
	if (item.isEmpty())
	{
		if (PROP_SCHED_LIST == property)
		{
			m_hostConfigScheduleWidget->m_deleteEventButton->setEnabled(false);
			m_hostConfigScheduleWidget->m_triggerEventButton->setEnabled(false);

			QStringList eventNames = value.toStringList();
			m_hostConfigScheduleWidget->m_scheduleList->clear();
			m_hostConfigScheduleWidget->m_scheduleList->setColumnCount(scheduleProperties.length());
			m_hostConfigScheduleWidget->m_scheduleList->setRowCount(eventNames.length());
			m_hostConfigScheduleWidget->m_scheduleList->setHorizontalHeaderLabels(scheduleHeaders);
			m_hostConfigScheduleWidget->m_scheduleList->setVerticalHeaderLabels(eventNames);
			for (int row = 0; row < eventNames.length(); row++)
			{
				for (int column = 0; column < scheduleProperties.length(); column++)
				{
					if (PROP_SCHED_LASTTRIGGERED == scheduleProperties[column])
					{
						QLabel* label = new QLabel(m_hostConfigScheduleWidget->m_scheduleList);
						label->setToolTip(tr("When this event was last triggered"));
						m_hostConfigScheduleWidget->m_scheduleList->setCellWidget(row, column, label);
					}
					else if (PROP_SCHED_TYPE == scheduleProperties[column])
					{
						QComboBox* comboBox = new QComboBox(m_hostConfigScheduleWidget->m_scheduleList);
						comboBox->setFrame(false);
						comboBox->setEditable(false);
						comboBox->addItem(tr("Start apps"), SCHED_TYPE_STARTAPPS);
						comboBox->addItem(tr("Stop apps"), SCHED_TYPE_STOPAPPS);
						comboBox->addItem(tr("Restart apps"), SCHED_TYPE_RESTARTAPPS);
						comboBox->addItem(tr("Start group"), SCHED_TYPE_STARTGROUP);
						comboBox->addItem(tr("Stop group"), SCHED_TYPE_STOPGROUP);
						comboBox->addItem(tr("Shutdown"), SCHED_TYPE_SHUTDOWN);
						comboBox->addItem(tr("Reboot"), SCHED_TYPE_REBOOT);
						comboBox->addItem(tr("Screenshot"), SCHED_TYPE_SCREENSHOT);
						comboBox->addItem(tr("Trigger events"), SCHED_TYPE_TRIGGEREVENTS);
						comboBox->addItem(tr("Generate alert"), SCHED_TYPE_ALERT);
						comboBox->setToolTip(tr("The type of event"));
						m_hostConfigScheduleWidget->m_scheduleList->setCellWidget(row, column, comboBox);
						connect(comboBox, (void (QComboBox::*)(int))&QComboBox::activated,
							this, &HostConfigWidget::comboBoxValueChanged);
					}
					else if (PROP_SCHED_FREQUENCY == scheduleProperties[column])
					{
						QComboBox* comboBox = new QComboBox(m_hostConfigScheduleWidget->m_scheduleList);
						comboBox->setFrame(false);
						comboBox->setEditable(false);
						comboBox->addItem(tr("Disabled"), SCHED_FREQ_DISABLED);
						comboBox->addItem(tr("Weekly"), SCHED_FREQ_WEEKLY);
						comboBox->addItem(tr("Daily"), SCHED_FREQ_DAILY);
						comboBox->addItem(tr("Hourly"), SCHED_FREQ_HOURLY);
						comboBox->addItem(tr("Once"), SCHED_FREQ_ONCE);
						comboBox->setToolTip(tr("The frequency at which the event will occur"));
						m_hostConfigScheduleWidget->m_scheduleList->setCellWidget(row, column, comboBox);
						connect(comboBox, (void (QComboBox::*)(int))&QComboBox::activated,
							this, &HostConfigWidget::comboBoxValueChanged);
					}
					else if (PROP_SCHED_OFFSET == scheduleProperties[column])
					{
						ScheduleEventOffsetWidget* offsetWidget = new ScheduleEventOffsetWidget(m_hostConfigScheduleWidget->m_scheduleList);
						connect(offsetWidget, &ScheduleEventOffsetWidget::offsetChanged,
							this, &HostConfigWidget::offsetValueChanged);
						offsetWidget->setToolTip(tr("The time at which the event will be triggered"));
						m_hostConfigScheduleWidget->m_scheduleList->setCellWidget(row, column, offsetWidget);
					}
					else
					{
						QLineEdit* lineEdit = new QLineEdit(m_hostConfigScheduleWidget->m_scheduleList);
						lineEdit->setFrame(false);
						connect(lineEdit, &QLineEdit::editingFinished,
							this, &HostConfigWidget::lineEditValueChanged);
						m_hostConfigScheduleWidget->m_scheduleList->setCellWidget(row, column, lineEdit);
					}

					QWidget* widget = m_hostConfigScheduleWidget->m_scheduleList->cellWidget(row, column);
					widget->setProperty(PROPERTY_GROUPNAME, GROUP_SCHEDULE);
					widget->setProperty(PROPERTY_ITEMNAME, eventNames[row]);
					widget->setProperty(PROPERTY_PROPNAME, scheduleProperties[column]);
					widget->setProperty(PROPERTY_SUPPORTED, QVariant(true));
					if (m_makeChangesCheck->isChecked())
						widget->setEnabled(true);
				}
			}

			// Request all the properties for all the schedule events
			for (const auto& event : eventNames)
			{
				for (const auto& prop : scheduleProperties)
				{
					m_hostClient->getVariant(GROUP_SCHEDULE, event, prop);
				}
			}
		}
	}
	else
	{
		// Wish there was a more effecient way of finding the item
		for (int row = 0; row < m_hostConfigScheduleWidget->m_scheduleList->rowCount(); row++)
		{
			if (item == m_hostConfigScheduleWidget->m_scheduleList->verticalHeaderItem(row)->text())
			{
				if (!scheduleProperties.contains(property))
				{
					emit setNoticeText(tr("Internal error: Missing schedule property '%1'. Client may be out of date.").arg(property));
				}
				else
				{
					int column = scheduleProperties.indexOf(property);
					QWidget* widget = m_hostConfigScheduleWidget->m_scheduleList->cellWidget(row, column);
					if (nullptr != qobject_cast<QLabel*>(widget))
					{
						qobject_cast<QLabel*>(widget)->setText(value.toString());
					}
					else if (nullptr != qobject_cast<QComboBox*>(widget))
					{
						QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
						comboBox->setCurrentIndex(comboBox->findData(value));

						if (PROP_SCHED_FREQUENCY == property)
						{
							ScheduleEventOffsetWidget* offsetWidget = qobject_cast<ScheduleEventOffsetWidget*>
								(m_hostConfigScheduleWidget->m_scheduleList->cellWidget(row, scheduleProperties.indexOf(PROP_SCHED_OFFSET)));
							// hack to get schedule list to update size properly, hide then show
							m_hostConfigScheduleWidget->m_scheduleList->hide();
							offsetWidget->setType(value.toString());
							m_hostConfigScheduleWidget->m_scheduleList->show();
						}
						else if (PROP_SCHED_TYPE == property)
						{
							// Set the arguments tooltip depending on the type of the roperty
							QLineEdit* argumentsWidget = qobject_cast<QLineEdit*>
								(m_hostConfigScheduleWidget->m_scheduleList->cellWidget(row, scheduleProperties.indexOf(PROP_SCHED_ARGUMENTS)));

							QString tooltipText = tr("The arguments for this trigger");
							if (SCHED_TYPE_STARTAPPS == value)
							{
								tooltipText = tr("One or more application names to start.\n"
									"Separate application names with a semicolon, do not include spaces.");
							}
							else if (SCHED_TYPE_STOPAPPS == value)
							{
								tooltipText = tr("One or more application names to stop.\n"
									"Separate application names with a semicolon, do not include spaces.");
							}
							else if (SCHED_TYPE_RESTARTAPPS == value)
							{
								tooltipText = tr("One or more application names to restart.\n"
									"Separate application names with a semicolon, do not include spaces.");
							}
							else if (SCHED_TYPE_STARTGROUP == value)
							{
								tooltipText = tr("The name of a group to start");
							}
							else if (SCHED_TYPE_STOPGROUP == value)
							{
								tooltipText = tr("The name of a group to stop");
							}
							else if (SCHED_TYPE_SHUTDOWN == value)
							{
								tooltipText = tr("The message to log with this event");
							}
							else if (SCHED_TYPE_REBOOT == value)
							{
								tooltipText = tr("The message to log with this event");
							}
							else if (SCHED_TYPE_SCREENSHOT == value)
							{
								tooltipText = tr("The path to the filename to save the screenshot in PNG format, "
									"%DATE% will be replaced with time/date stamp");
							}
							else if (SCHED_TYPE_TRIGGEREVENTS == value)
							{
								tooltipText = tr("The name of another scheduled event to trigger");
							}
							else if (SCHED_TYPE_ALERT == value)
							{
								tooltipText = tr("The text of the alert");
							}
							else
							{
								tooltipText = tr("UNKNOWN TYPE");
							}

							argumentsWidget->setToolTip(tooltipText);
						}
					}
					else if (nullptr != qobject_cast<ScheduleEventOffsetWidget*>(widget))
					{
						ScheduleEventOffsetWidget* offsetWidget = qobject_cast<ScheduleEventOffsetWidget*>(widget);
						offsetWidget->setOffset(value.toInt());
					}
					else if (nullptr != qobject_cast<QLineEdit*>(widget))
					{
						qobject_cast<QLineEdit*>(widget)->setText(value.toString());
					}
				}

				break;
			}
		}
	}
}


void HostConfigWidget::setAlertValue(const QString& item, const QString& property, const QVariant& value)
{
	if (item.isEmpty())
	{
		if (PROP_ALERT_SLOTLIST == property)
		{
			m_hostConfigAlertWidget->m_deleteAlertSlotButton->setEnabled(false);

			QStringList alertSlotNames = value.toStringList();
			m_hostConfigAlertWidget->m_alertSlotList->clear();
			m_hostConfigAlertWidget->m_alertSlotList->setColumnCount(alertSlotProperties.length());
			m_hostConfigAlertWidget->m_alertSlotList->setRowCount(alertSlotNames.length());
			m_hostConfigAlertWidget->m_alertSlotList->setHorizontalHeaderLabels(alertSlotHeaders);
			m_hostConfigAlertWidget->m_alertSlotList->setVerticalHeaderLabels(alertSlotNames);
			for (int row = 0; row < alertSlotNames.length(); row++)
			{
				for (int column = 0; column < alertSlotProperties.length(); column++)
				{
					if (PROP_ALERT_SLOTENABLED == alertSlotProperties[column])
					{
						QCheckBox* checkBox = new QCheckBox(tr("Enabled"), m_hostConfigAlertWidget->m_alertSlotList);
						connect(checkBox, &QCheckBox::clicked,
							this, &HostConfigWidget::checkValueChanged);
						checkBox->setToolTip(tr("Is this alert slot enabled"));
						m_hostConfigAlertWidget->m_alertSlotList->setCellWidget(row, column, checkBox);
					}
					else if (PROP_ALERT_SLOTTYPE == alertSlotProperties[column])
					{
						QComboBox* comboBox = new QComboBox(m_hostConfigAlertWidget->m_alertSlotList);
						comboBox->setFrame(false);
						comboBox->setEditable(false);
						comboBox->addItem(tr("SMTP email"), ALERTSLOT_TYPE_SMPTEMAIL);
						comboBox->addItem(tr("HTTP GET"), ALERTSLOT_TYPE_HTTPGET);
						comboBox->addItem(tr("HTTP POST"), ALERTSLOT_TYPE_HTTPPOST);
						comboBox->addItem(tr("Slack WebHook"), ALERTSLOT_TYPE_SLACK);
						comboBox->addItem(tr("External command"), ALERTSLOT_TYPE_EXTERNAL);
						comboBox->setToolTip(tr("The type of alert, how to deliver the message"));
						connect(comboBox, (void (QComboBox::*)(int))&QComboBox::activated,
							this, &HostConfigWidget::comboBoxValueChanged);
						m_hostConfigAlertWidget->m_alertSlotList->setCellWidget(row, column, comboBox);
					}
					else if (PROP_ALERT_SLOTARG == alertSlotProperties[column])
					{
						QLineEdit* lineEdit = new QLineEdit(m_hostConfigAlertWidget->m_alertSlotList);
						lineEdit->setFrame(false);
						connect(lineEdit, &QLineEdit::editingFinished,
							this, &HostConfigWidget::lineEditValueChanged);
						m_hostConfigAlertWidget->m_alertSlotList->setCellWidget(row, column, lineEdit);
					}

					QWidget* widget = m_hostConfigAlertWidget->m_alertSlotList->cellWidget(row, column);
					widget->setProperty(PROPERTY_GROUPNAME, GROUP_ALERT);
					widget->setProperty(PROPERTY_ITEMNAME, alertSlotNames[row]);
					widget->setProperty(PROPERTY_PROPNAME, alertSlotProperties[column]);
					widget->setProperty(PROPERTY_SUPPORTED, QVariant(true));
					if (m_makeChangesCheck->isChecked())
						widget->setEnabled(true);
				}
			}

			// Request all the properties for all the alert slot item
			for (const auto& alertSlot : alertSlotNames)
			{
				for (const auto& prop : alertSlotProperties)
				{
					m_hostClient->getVariant(GROUP_ALERT, alertSlot, prop);
				}
			}
		}
		else
		{
			// Non table item
			setWidgetMapPropValue(m_hostConfigAlertWidget->m_widgetMap, property, value);
		}
	}
	else
	{
		// Wish there was a more effecient way of finding the item
		for (int row = 0; row < m_hostConfigAlertWidget->m_alertSlotList->rowCount(); row++)
		{
			if (item == m_hostConfigAlertWidget->m_alertSlotList->verticalHeaderItem(row)->text())
			{
				if (!alertSlotProperties.contains(property))
				{
					emit setNoticeText(tr("Internal error: Missing alert slot property '%1'. Client may be out of date.").arg(property));
				}
				else
				{
					int column = alertSlotProperties.indexOf(property);
					QWidget* widget = m_hostConfigAlertWidget->m_alertSlotList->cellWidget(row, column);
					if (nullptr != qobject_cast<QCheckBox*>(widget))
					{
						qobject_cast<QCheckBox*>(widget)->setChecked(value.toBool());
					}
					else if (nullptr != qobject_cast<QComboBox*>(widget))
					{
						QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
						comboBox->setCurrentIndex(comboBox->findData(value));
						if (PROP_ALERT_SLOTTYPE == property)
						{
							// Set the arguments tooltip depending on the type of the roperty
							QLineEdit* argumentsWidget = qobject_cast<QLineEdit*>
								(m_hostConfigAlertWidget->m_alertSlotList->cellWidget(row, alertSlotProperties.indexOf(PROP_ALERT_SLOTARG)));

							QString tooltipText = tr("The arguments for this trigger");
							if (ALERTSLOT_TYPE_SMPTEMAIL == value)
							{
								tooltipText = tr("The destination email address");
							}
							else if (ALERTSLOT_TYPE_HTTPGET == value)
							{
								tooltipText = tr("The URL to get via HTTP, %1 will be replaced with the percent encoded alert text")
									.arg(ALERT_REPLACE_TEXT);
							}
							else if (ALERTSLOT_TYPE_HTTPPOST == value)
							{
								tooltipText = tr("The URL to post the alert data to via HTTP");
							}
							else if (ALERTSLOT_TYPE_SLACK == value)
							{
								tooltipText = tr("The Slack Webhook URL for Your Workspace");
							}
							else if (ALERTSLOT_TYPE_EXTERNAL == value)
							{
								tooltipText = tr("The command line to run.  %1 will be replaced with the alert text.")
									.arg(ALERT_REPLACE_TEXT);
							}
							else
							{
								tooltipText = tr("UNKNOWN TYPE");
							}

							argumentsWidget->setToolTip(tooltipText);
						}
					}
					else if (nullptr != qobject_cast<QLineEdit*>(widget))
					{
						qobject_cast<QLineEdit*>(widget)->setText(value.toString());
					}
				}
			}
		}
	}
}


