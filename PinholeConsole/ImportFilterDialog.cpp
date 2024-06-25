#include "ImportFilterDialog.h"
#include "GuiUtil.h"
#include "Global.h"
#include "../common/PinholeCommon.h"
#include "../common/Utilities.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QLabel>
#include <QTreeWidget>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QEvent>
#include <QWhatsThisClickedEvent>
#include <QDesktopServices>
#include <QMessageBox>
#include <QGuiApplication>


ImportFilterDialog::ImportFilterDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Select portions to import:")));
	m_tree = new QTreeWidget();
	layout->addWidget(m_tree);

	QString addOrUpdateText = tr("Add or update these entries");
	QString setJustTheseEntriesText = tr("Set just these entries");
	QString deleteTheseEntriesText = tr("Delete these entries");

	QGroupBox* appGroupBox = new QGroupBox(tr("Applications"));
	layout->addWidget(appGroupBox);
	QVBoxLayout* appLayout = new QVBoxLayout(appGroupBox);
	m_appAddOrUpdate = new QRadioButton(addOrUpdateText);
	m_appAddOrUpdate->setChecked(true);
	appLayout->addWidget(m_appAddOrUpdate);
	m_appSetTheseEntries = new QRadioButton(setJustTheseEntriesText);
	appLayout->addWidget(m_appSetTheseEntries);
	m_appDeleteTheseEntries = new QRadioButton(deleteTheseEntriesText);
	appLayout->addWidget(m_appDeleteTheseEntries);
	appLayout->addStretch(1);

	QGroupBox* groupGroupBox = new QGroupBox(tr("Groups"));
	layout->addWidget(groupGroupBox);
	QVBoxLayout* groupLayout = new QVBoxLayout(groupGroupBox);
	m_groupAddOrUpdate = new QRadioButton(addOrUpdateText);
	m_groupAddOrUpdate->setChecked(true);
	groupLayout->addWidget(m_groupAddOrUpdate);
	m_groupSetTheseEntries = new QRadioButton(setJustTheseEntriesText);
	groupLayout->addWidget(m_groupSetTheseEntries);
	m_groupDeleteTheseEntries = new QRadioButton(deleteTheseEntriesText);
	groupLayout->addWidget(m_groupDeleteTheseEntries);
	groupLayout->addStretch(1);

	QGroupBox* eventGroupBox = new QGroupBox(tr("Scheduled events"));
	layout->addWidget(eventGroupBox);
	QVBoxLayout* eventLayout = new QVBoxLayout(eventGroupBox);
	eventLayout->addWidget(m_eventSetTheseEntries);
	m_eventAddOrUpdate = new QRadioButton(addOrUpdateText);
	m_eventAddOrUpdate->setChecked(true);
	eventLayout->addWidget(m_eventAddOrUpdate);
	m_eventSetTheseEntries = new QRadioButton(setJustTheseEntriesText);
	eventLayout->addWidget(m_eventSetTheseEntries);
	m_eventDeleteTheseEntries = new QRadioButton(deleteTheseEntriesText);
	eventLayout->addWidget(m_eventDeleteTheseEntries);
	eventLayout->addStretch(1);

	QGroupBox* alertGroupBox = new QGroupBox(tr("Alert slots"));
	layout->addWidget(alertGroupBox);
	QVBoxLayout* alertLayout = new QVBoxLayout(alertGroupBox);
	m_alertAddOrUpdate = new QRadioButton(addOrUpdateText);
	m_alertAddOrUpdate->setChecked(true);
	alertLayout->addWidget(m_alertAddOrUpdate);
	m_alertSetTheseEntries = new QRadioButton(setJustTheseEntriesText);
	alertLayout->addWidget(m_alertSetTheseEntries);
	m_alertDeleteTheseEntries = new QRadioButton(deleteTheseEntriesText);
	alertLayout->addWidget(m_alertDeleteTheseEntries);
	alertLayout->addStretch(1);

	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::rejected,
		this, &ImportFilterDialog::reject);
	connect(buttons, &QDialogButtonBox::accepted,
		this, &ImportFilterDialog::accept);

	m_tree->setColumnCount(1);
	m_tree->setHeaderLabel(tr("Settings"));

	QString settingsHtml = LocalHtmlDoc(DOC_SETTINGS, tr("Details about importing"));

	m_tree->setToolTip(tr("Uncheck portions of settings to not include in import"));
	m_tree->setWhatsThis(tr("This tree contains the different sections of settings as well as "
		"the items within those groups.  Checked items will be included when settings are "
		"imported to the selected hosts.  Unchecked items will be ignored.<p>"
		"Individual items within a group can be selected without selecting the settings for "
		"that group and only the items themselves will be imported, not the global settings "
		"for that group.") + settingsHtml);

	QString addOrUpdateToolTip = tr("Add new %1s and update existing %1s with the same name");
	QString addOrUpdateWhatsThis = tr("If the host being imported to does not contain any %1 with the same name "
		"as those in the list they will be added.  If there is already a %1 with the same name then "
		"it will have the settings overwritten with those in the JSON being imported.  Other %1s not "
		"in the list will be left unchanged.") + settingsHtml;
	QString setJustTheseEntriesToolTip = tr("Set %1 list to just these entries");
	QString setJustTheseEntriesWhatsThis = tr("The %1 entries in the list will be set on the selected "
		"hosts.  Any %1 entries not in the list will be removed from the host.") + settingsHtml;
	QString deleteTheseEntriesToolTip = tr("Delete %1 list with these names");
	QString deleteTheseEntriesWhatsThis = tr("Those %1 entries on the hsot being imported to with "
		"names that match those in the list will be deleted.  Entries that do not match will be left "
		"unchanged.") + settingsHtml;
	QString applicationText = tr("application");
	QString groupText = tr("group");
	QString eventText = tr("event");
	QString alertText = tr("alert slot");

	m_appAddOrUpdate->setToolTip(addOrUpdateToolTip.arg(applicationText));
	m_appAddOrUpdate->setWhatsThis(addOrUpdateWhatsThis.arg(applicationText));
	m_appSetTheseEntries->setToolTip(setJustTheseEntriesToolTip.arg(applicationText));
	m_appSetTheseEntries->setWhatsThis(setJustTheseEntriesWhatsThis.arg(applicationText));
	m_appDeleteTheseEntries->setToolTip(deleteTheseEntriesToolTip.arg(applicationText));
	m_appDeleteTheseEntries->setWhatsThis(deleteTheseEntriesWhatsThis.arg(applicationText));
	m_groupAddOrUpdate->setToolTip(addOrUpdateToolTip.arg(groupText));
	m_groupAddOrUpdate->setWhatsThis(addOrUpdateWhatsThis.arg(groupText));
	m_groupSetTheseEntries->setToolTip(setJustTheseEntriesToolTip.arg(groupText));
	m_groupSetTheseEntries->setWhatsThis(setJustTheseEntriesWhatsThis.arg(groupText));
	m_groupDeleteTheseEntries->setToolTip(deleteTheseEntriesToolTip.arg(groupText));
	m_groupDeleteTheseEntries->setWhatsThis(deleteTheseEntriesWhatsThis.arg(groupText));
	m_eventAddOrUpdate->setToolTip(addOrUpdateToolTip.arg(eventText));
	m_eventAddOrUpdate->setWhatsThis(addOrUpdateWhatsThis.arg(eventText));
	m_eventSetTheseEntries->setToolTip(setJustTheseEntriesToolTip.arg(eventText));
	m_eventSetTheseEntries->setWhatsThis(setJustTheseEntriesWhatsThis.arg(eventText));
	m_eventDeleteTheseEntries->setToolTip(deleteTheseEntriesToolTip.arg(eventText));
	m_eventDeleteTheseEntries->setWhatsThis(deleteTheseEntriesWhatsThis.arg(eventText));
	m_alertAddOrUpdate->setToolTip(addOrUpdateToolTip.arg(alertText));
	m_alertAddOrUpdate->setWhatsThis(addOrUpdateWhatsThis.arg(alertText));
	m_alertSetTheseEntries->setToolTip(setJustTheseEntriesToolTip.arg(alertText));
	m_alertSetTheseEntries->setWhatsThis(setJustTheseEntriesWhatsThis.arg(alertText));
	m_alertDeleteTheseEntries->setToolTip(deleteTheseEntriesToolTip.arg(alertText));
	m_alertDeleteTheseEntries->setWhatsThis(deleteTheseEntriesWhatsThis.arg(alertText));
}


ImportFilterDialog::~ImportFilterDialog()
{
}


int ImportFilterDialog::exec(const QByteArray & jsonData)
{
	m_jsonDoc = QJsonDocument::fromJson(jsonData);
	if (m_jsonDoc.isNull())
	{
		m_error = true;
	}
	else
	{
		bool anyGroupFound = false;

		QString settingsHtml = LocalHtmlDoc(DOC_SETTINGS, tr("Details about importing"));

		QString groupToolTip = tr("Uncheck to prevent importing of %1 settings");
		QString groupWhatsThis = tr("Unchecking this item will prevent %1 settings "
			"from being imported.  All other checked settings, including individual "
			"%2 will still be imported.") + settingsHtml;
		QString itemToolTip = tr("Uncheck to prevent importing of the %1 named '%2'");
		QString itemWhatsThis = tr("Unchecking this item will prevent the %1 named "
			"'%2' from being imported.  All other checked items will still be imported.")
			+ settingsHtml;

		if (!m_jsonDoc[JSONTAG_GLOBALSETTINGS].isUndefined())
		{
			anyGroupFound = true;
			QTreeWidgetItem* item = new QTreeWidgetItem({ tr("Global settings") });
			item->setData(0, Qt::UserRole, JSONTAG_GLOBALSETTINGS);
			item->setCheckState(0, Qt::Checked);
			item->setToolTip(0, groupToolTip.arg(tr("global")));
			item->setWhatsThis(0, tr("Unchecking this item will prevent global settings "
				"from being imported.  All other checked settings will still be imported."));
			m_tree->addTopLevelItem(item);
		}

		if (!m_jsonDoc[JSONTAG_APPSETTINGS].isUndefined())
		{
			anyGroupFound = true;
			QTreeWidgetItem* item = new QTreeWidgetItem({ tr("Application settings") });
			item->setData(0, Qt::UserRole, JSONTAG_APPSETTINGS);
			item->setCheckState(0, Qt::Checked);
			item->setToolTip(0, groupToolTip.arg(tr("application")));
			item->setWhatsThis(0, groupWhatsThis.arg(tr("application")).arg(tr("applications")));
			m_tree->addTopLevelItem(item);

			if (!m_jsonDoc[JSONTAG_APPSETTINGS][JSONTAG_APPLICATIONS].isUndefined())
			{
				QJsonArray appList = m_jsonDoc[JSONTAG_APPSETTINGS][JSONTAG_APPLICATIONS].toArray();
				for (const auto& app : appList)
				{
					if (app.isObject())
					{
						QJsonObject japp = app.toObject();
						QString name = japp[PROP_APP_NAME].toString();
						QTreeWidgetItem* appItem = new QTreeWidgetItem({ name });
						appItem->setData(0, Qt::UserRole, name);
						appItem->setCheckState(0, Qt::Checked);
						appItem->setToolTip(0, itemToolTip.arg(tr("application")).arg(name));
						appItem->setWhatsThis(0, itemWhatsThis.arg(tr("application")).arg(name));
						item->addChild(appItem);
					}
				}

				m_tree->expandItem(item);
			}
		}
		else
		{
			m_appSetTheseEntries->setEnabled(false);
			m_appAddOrUpdate->setEnabled(false);
			m_appDeleteTheseEntries->setEnabled(false);
		}

		if (!m_jsonDoc[JSONTAG_GROUPSETTINGS].isUndefined())
		{
			anyGroupFound = true;
			QTreeWidgetItem* item = new QTreeWidgetItem({ tr("Group settings") });
			item->setData(0, Qt::UserRole, JSONTAG_GROUPSETTINGS);
			item->setCheckState(0, Qt::Checked);
			item->setToolTip(0, groupToolTip.arg(tr("group")));
			item->setWhatsThis(0, groupWhatsThis.arg(tr("group")).arg(tr("application groups")));
			m_tree->addTopLevelItem(item);

			if (!m_jsonDoc[JSONTAG_GROUPSETTINGS][JSONTAG_GROUPS].isUndefined())
			{
				QJsonArray groupList = m_jsonDoc[JSONTAG_GROUPSETTINGS][JSONTAG_GROUPS].toArray();
				for (const auto& group : groupList)
				{
					if (group.isObject())
					{
						QJsonObject jgroup = group.toObject();
						QString name = jgroup[PROP_GROUP_NAME].toString();
						QTreeWidgetItem* groupItem = new QTreeWidgetItem({ name });
						groupItem->setData(0, Qt::UserRole, name);
						groupItem->setCheckState(0, Qt::Checked);
						groupItem->setToolTip(0, itemToolTip.arg(tr("group")).arg(name));
						groupItem->setWhatsThis(0, itemWhatsThis.arg(tr("group")).arg(name));
						item->addChild(groupItem);
					}
				}

				m_tree->expandItem(item);
			}
		}
		else
		{
			m_groupSetTheseEntries->setEnabled(false);
			m_groupAddOrUpdate->setEnabled(false);
			m_groupDeleteTheseEntries->setEnabled(false);
		}

		if (!m_jsonDoc[JSONTAG_SCHEDETTINGS].isUndefined())
		{
			anyGroupFound = true;
			QTreeWidgetItem* item = new QTreeWidgetItem({ tr("Schedule settings") });
			item->setData(0, Qt::UserRole, JSONTAG_SCHEDETTINGS);
			item->setCheckState(0, Qt::Checked);
			item->setToolTip(0, groupToolTip.arg(tr("schedule")));
			item->setWhatsThis(0, groupWhatsThis.arg(tr("scedule")).arg(tr("scheduled events")));
			m_tree->addTopLevelItem(item);

			if (!m_jsonDoc[JSONTAG_SCHEDETTINGS][JSONTAG_EVENTS].isUndefined())
			{
				QJsonArray eventList = m_jsonDoc[JSONTAG_SCHEDETTINGS][JSONTAG_EVENTS].toArray();
				for (const auto& event : eventList)
				{
					if (event.isObject())
					{
						QJsonObject jevent = event.toObject();
						QString name = jevent[PROP_SCHED_NAME].toString();
						QTreeWidgetItem* eventItem = new QTreeWidgetItem({ name });
						eventItem->setData(0, Qt::UserRole, name);
						eventItem->setCheckState(0, Qt::Checked);
						eventItem->setToolTip(0, itemToolTip.arg(tr("scheduled event")).arg(name));
						eventItem->setWhatsThis(0, itemWhatsThis.arg(tr("scheduled event")).arg(name));
						item->addChild(eventItem);
					}
				}

				m_tree->expandItem(item);
			}
		}
		else
		{
			m_eventSetTheseEntries->setEnabled(false);
			m_eventAddOrUpdate->setEnabled(false);
			m_eventDeleteTheseEntries->setEnabled(false);
		}

		if (!m_jsonDoc[JSONTAG_ALERTSETTINGS].isUndefined())
		{
			anyGroupFound = true;
			QTreeWidgetItem* item = new QTreeWidgetItem({ tr("Alert settings") });
			item->setData(0, Qt::UserRole, JSONTAG_ALERTSETTINGS);
			item->setCheckState(0, Qt::Checked);
			item->setToolTip(0, groupToolTip.arg(tr("alert")));
			item->setWhatsThis(0, groupWhatsThis.arg(tr("alert")).arg(tr("alert slots")));
			m_tree->addTopLevelItem(item);

			if (!m_jsonDoc[JSONTAG_SCHEDETTINGS][JSONTAG_ALERTSLOTS].isUndefined())
			{
				QJsonArray alertSlotList = m_jsonDoc[JSONTAG_SCHEDETTINGS][JSONTAG_ALERTSLOTS].toArray();
				for (const auto& alertSlot : alertSlotList)
				{
					QJsonObject jslot = alertSlot.toObject();
					QString name = jslot[PROP_ALERT_SLOTNAME].toString();
					QTreeWidgetItem* alertSlotItem = new QTreeWidgetItem({ name });
					alertSlotItem->setData(0, Qt::UserRole, name);
					alertSlotItem->setCheckState(0, Qt::Checked);
					alertSlotItem->setToolTip(0, itemToolTip.arg(tr("alert slot")).arg(name));
					alertSlotItem->setWhatsThis(0, itemWhatsThis.arg(tr("alert slot")).arg(name));
					item->addChild(alertSlotItem);
				}

				m_tree->expandItem(item);
			}
		}
		else
		{
			m_alertSetTheseEntries->setEnabled(false);
			m_alertAddOrUpdate->setEnabled(false);
			m_alertDeleteTheseEntries->setEnabled(false);
		}

		if (!anyGroupFound)
			m_error = true;
	}

	if (m_error)
		return QDialog::Rejected;

	return QDialog::exec();
}


QByteArray ImportFilterDialog::jsonData() const
{
	QJsonObject jobject;

	QTreeWidgetItem* root = m_tree->invisibleRootItem();
	for (int n = 0; n < root->childCount(); n++)
	{
		QTreeWidgetItem* group = root->child(n);
		QString groupTag = group->data(0, Qt::UserRole).toString();
		if (group->checkState(0) == Qt::Checked)
		{
			jobject[groupTag] = m_jsonDoc[groupTag];
		}

		QString groupObjectTag;
		QString itemNameProp;
		bool deleteEntries = false;
		bool noDelete = false;
		if (JSONTAG_APPSETTINGS == groupTag)
		{
			groupObjectTag = JSONTAG_APPLICATIONS;
			itemNameProp = PROP_APP_NAME;
			if (m_appAddOrUpdate->isChecked())
			{
				noDelete = true;
			}
			else if (m_appDeleteTheseEntries->isChecked())
			{
				deleteEntries = true;
			}
		}
		else if (JSONTAG_GROUPSETTINGS == groupTag)
		{
			groupObjectTag = JSONTAG_GROUPS;
			itemNameProp = PROP_GROUP_NAME;
			if (m_groupAddOrUpdate->isChecked())
			{
				noDelete = true;
			}
			else if (m_groupDeleteTheseEntries->isChecked())
			{
				deleteEntries = true;
			}
		}
		else if (JSONTAG_SCHEDETTINGS == groupTag)
		{
			groupObjectTag = JSONTAG_EVENTS;
			itemNameProp = PROP_SCHED_NAME;
			if (m_eventAddOrUpdate->isChecked())
			{
				noDelete = true;
			}
			else if (m_eventDeleteTheseEntries->isChecked())
			{
				deleteEntries = true;
			}
		}
		else if (JSONTAG_ALERTSETTINGS == groupTag)
		{
			groupObjectTag = JSONTAG_ALERTSLOTS;
			itemNameProp = PROP_ALERT_SLOTNAME;
			if (m_alertAddOrUpdate->isChecked())
			{
				noDelete = true;
			}
			else if (m_alertDeleteTheseEntries->isChecked())
			{
				deleteEntries = true;
			}
		}
		else
		{
			continue;
		}

		QJsonArray oldItems = m_jsonDoc[groupTag][groupObjectTag].toArray();
		QJsonArray newItems;

		for (int i = 0; i < group->childCount(); i++)
		{
			QTreeWidgetItem* item = group->child(i);
			QString itemName = item->data(0, Qt::UserRole).toString();
			if (item->checkState(0) == Qt::Checked)
			{
				for (const auto& i : oldItems)
				{
					if (i.isObject())
					{
						QJsonObject jobj = i.toObject();
						if (jobj[itemNameProp] == itemName)
						{
							newItems.append(i);
							break;
						}
					}
				}
			}
		}

		if (group->checkState(0) == Qt::Unchecked)
		{
			jobject[groupTag] = QJsonObject();
		}

		QJsonValue jval(jobject);

		modifyJsonValue(jval, groupTag + "." + groupObjectTag, newItems);
		modifyJsonValue(jval, groupTag + "." + STATUSTAG_DELETEENTRIES, deleteEntries);
		modifyJsonValue(jval, groupTag + "." + STATUSTAG_NODELETE, noDelete);

		jobject = jval.toObject();
	}

	return QJsonDocument(jobject).toJson();
}


bool ImportFilterDialog::event(QEvent* ev)
{
	if (ev->type() == QEvent::WhatsThisClicked)
	{
		ev->accept();
		QWhatsThisClickedEvent* whatsThisEvent = dynamic_cast<QWhatsThisClickedEvent*>(ev);
		assert(whatsThisEvent);
		QUrl url(whatsThisEvent->href(), QUrl::TolerantMode);
		if (!QDesktopServices::openUrl(url))
		{
			QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
				tr("Failed to open URL: %1")
				.arg(url.toString()));
		}
		return true;
	}

	return QWidget::event(ev);
}
