#pragma once

#include "../common/PinholeCommon.h"

#include <QWidget>
#include <QTimer>

class QCheckBox;
class QListWidget;
class QTabWidget;
class QListWidgetItem;
class HostConfigAppsWidget;
class HostConfigGroupsWidget;
class HostConfigGlobalsWidget;
class HostConfigScheduleWidget;
class HostConfigAlertWidget;
class HostConfigWidgetTab;
class HostClient;

class HostConfigWidget : public QWidget
{
	Q_OBJECT

public:
	HostConfigWidget(QWidget *parent = Q_NULLPTR);
	~HostConfigWidget();

public slots:
	void hostChanged(const QString& address, int port, const QString& hostId);
	void startApp();
	void stopApp();
	void addApp();
	void deleteApp();
	void renameApp();
	void getAppConsoleOutput();
	void addGroup();
	void deleteGroup();
	void renameGroup();
	void addEvent();
	void deleteEvent();
	void renameEvent();
	void triggerEvent();
	void addAlertSlot();
	void deleteAlertSlot();
	void renameAlertSlot();
	void resetAlertCount();
	void retrieveAlerts();
	void lineEditValueChanged();
	void checkValueChanged(bool);
	void comboBoxValueChanged(int);
	void spinBoxValueChanged();
	void offsetValueChanged(int);
	void listItemChanged(QListWidgetItem *item);
	void hostValueChanged(const QString&, const QString&, const QString&, const QVariant&);
	void hostMissingValue(const QString&, const QString&, const QString&);
	void hostCommandError(const QString&, const QString&);
	void hostCommandMissing(const QString&, const QString&);
	void hostCommandData(const QString&, const QString&, const QVariant&);
	void hostConnected();
	void hostDisconnected();
	void setWidgetPropValue(QWidget* widget);
	void enableWidgets(bool enable);
	void appChanged(const QString& app);
	void groupChanged(const QString& group);
	void setEditable(bool editable);
	void makeChanges_clicked(bool checked);

signals:
	void setStatusText(const QString& text);
	void setNoticeText(const QString& text, unsigned int id = 0);
	void clearNoticeText(unsigned int id);

private:
	QStringList scheduleProperties =
	{
		PROP_SCHED_TYPE,
		PROP_SCHED_FREQUENCY,
		PROP_SCHED_ARGUMENTS,
		PROP_SCHED_OFFSET,
		PROP_SCHED_LASTTRIGGERED
	};
	QStringList scheduleHeaders =
	{
		tr("Type"),
		tr("Frequency"),
		tr("Arguments"),
		tr("When"),
		tr("Last Triggered")
	};

	QStringList alertSlotProperties = 
	{
		PROP_ALERT_SLOTENABLED,
		PROP_ALERT_SLOTTYPE,
		PROP_ALERT_SLOTARG
	};
	QStringList alertSlotHeaders =
	{
		tr("Enabled"),
		tr("Type"),
		tr("Argument")
	};

	void resetWidgets();
	void fillAppValues();
	void fillGroupValues();
	void setWidgetMapPropValue(const QMap<QString, QWidget*>& widgetMap, const QString& property, const QVariant& value);
	void setQListWidgetPropValue(QListWidget* listWidget, const QString& groupName,
		const QString& itemName, const QString& propName);
	void setApplicationValue(const QString& item, const QString& property, const QVariant& value);
	void setGroupValue(const QString& item, const QString& property, const QVariant& value);
	void setScheduleValue(const QString& item, const QString& property, const QVariant& value);
	void setAlertValue(const QString& item, const QString& property, const QVariant& value);

	bool m_selfupdatingWidget = false;

	QList<HostConfigWidgetTab*> m_configWidgets;
	QString m_currentHost;
	QString m_currentApp;
	QString m_currentGroup;
	QSharedPointer<HostClient> m_hostClient;

	QTabWidget* m_tabWidget = nullptr;
	QCheckBox* m_makeChangesCheck = nullptr;

	HostConfigAppsWidget * m_hostConfigAppsWidget = nullptr;
	HostConfigGroupsWidget * m_hostConfigGroupsWidget = nullptr;
	HostConfigGlobalsWidget * m_hostConfigGlobalsWidget = nullptr;
	HostConfigScheduleWidget * m_hostConfigScheduleWidget = nullptr;
	HostConfigAlertWidget * m_hostConfigAlertWidget = nullptr;
};
