#pragma once

#include "HostConfigWidgetTab.h"

#include <QWidget>
#include <QMap>

class QPushButton;
class QListWidget;
class QComboBox;
class QCheckBox;

class HostConfigGroupsWidget : public HostConfigWidgetTab
{
	Q_OBJECT

public:
	HostConfigGroupsWidget(QWidget *parent = Q_NULLPTR);
	~HostConfigGroupsWidget();

	void resetGroupWidgets();
	void resetWidgets() override;
	void enableWidgets(bool enable) override;

	QPushButton * m_addGroupButton = nullptr;
	QPushButton * m_deleteGroupButton = nullptr;
	QPushButton * m_renameGroupButton = nullptr;
	QPushButton * m_startGroupButton = nullptr;
	QPushButton * m_stopGroupButton = nullptr;
	QPushButton * m_deleteAppButton = nullptr;
	QPushButton * m_addAppButton = nullptr;
	QListWidget * m_groupList = nullptr;
	QListWidget * m_applicationList = nullptr;
	QComboBox * m_appDropList = nullptr;
	QCheckBox * m_launchAtStart = nullptr;

public slots:
	void addAppButton_clicked();
	void deleteAppButton_clicked();
	void groupList_currentItemChanged();
	void applicationList_itemSelectionChanged();

signals:
	void widgetValueChanged(QWidget* widget);
	void groupChanged(const QString&);

private:
	void resizeEvent(QResizeEvent* event) override;
};
