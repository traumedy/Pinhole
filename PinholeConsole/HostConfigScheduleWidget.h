#pragma once

#include "HostConfigWidgetTab.h"

#include <QWidget>

class QPushButton;
class QTableWidget;

class HostConfigScheduleWidget : public HostConfigWidgetTab
{
	Q_OBJECT

public:
	HostConfigScheduleWidget(QWidget *parent = Q_NULLPTR);
	~HostConfigScheduleWidget();

	void resetWidgets() override;
	void enableWidgets(bool enable) override;

	QPushButton * m_addEventButton = nullptr;
	QPushButton * m_deleteEventButton = nullptr;
	QPushButton * m_renameEventButton = nullptr;
	QPushButton * m_triggerEventButton = nullptr;
	QTableWidget * m_scheduleList = nullptr;

private slots:
	void scheduleList_itemSelectionChanged();

private:
	void resizeEvent(QResizeEvent* event) override;
};


