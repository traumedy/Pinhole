#include "HostConfigScheduleWidget.h"
#include "Global.h"

#include <QResizeEvent>
#include <QPushButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>

#include <QListWidget>

HostConfigScheduleWidget::HostConfigScheduleWidget(QWidget *parent)
	: HostConfigWidgetTab(parent)
{
	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	ScrollAreaEx* buttonScrollArea = new ScrollAreaEx;
	buttonScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
	mainLayout->addWidget(buttonScrollArea);
	m_scheduleList = new QTableWidget;
	m_scheduleList->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_scheduleList->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	connect(m_scheduleList, &QTableWidget::itemSelectionChanged,
		this, &HostConfigScheduleWidget::scheduleList_itemSelectionChanged);
	mainLayout->addWidget(m_scheduleList);

	QWidget* buttonScrollAreaWidgetContents = new QWidget;
	buttonScrollAreaWidgetContents->setProperty("borderless", true);
	QVBoxLayout* buttonLayout = new QVBoxLayout(buttonScrollAreaWidgetContents);
	m_addEventButton = new QPushButton(tr("Add event"));
	m_addEventButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	//m_addEventButton->setFixedWidth(BUTTONITEMWIDTH);
	buttonLayout->addWidget(m_addEventButton);
	m_deleteEventButton = new QPushButton(tr("Delete events"));
	m_deleteEventButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	//m_deleteEventButton->setFixedWidth(BUTTONITEMWIDTH);
	buttonLayout->addWidget(m_deleteEventButton);
	m_renameEventButton = new QPushButton(tr("Rename event"));
	m_renameEventButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	buttonLayout->addWidget(m_renameEventButton);
	m_triggerEventButton = new QPushButton(tr("Trigger event"));
	m_triggerEventButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
	//m_triggerEventButton->setFixedWidth(BUTTONITEMWIDTH);
	buttonLayout->addWidget(m_triggerEventButton);

	// For some reason (bug?) Qt does not behave correctly if the QVBoxLayout contains only QPushButtons
	// The buttons aren't spaced correctly and they either don't get wide enough or the QScrollArea is
	// not the correct width or scrolls when it shouldn't.  (Actually this doesn't fix the spacing)
	QListWidget* fakeListwidget = new QListWidget;
	buttonLayout->addWidget(fakeListwidget);
	fakeListwidget->setMaximumWidth(BUTTONITEMWIDTH);
	fakeListwidget->setMaximumHeight(0);

	buttonScrollArea->setWidget(buttonScrollAreaWidgetContents);
	buttonScrollArea->show();

	m_addEventButton->setToolTip(tr("Add a new scheduled event"));
	m_addEventButton->setWhatsThis(tr("Click this button to add a new scheduled event.  "
		"You will be prompted for the name of the event.  If an event with this "
		"name already exists you will get an error.  Event names are case sensitive."));
	m_deleteEventButton->setToolTip(tr("Delete the selected scheduled events"));
	m_deleteEventButton->setWhatsThis(tr("Click this button to delete the selected selected events "
		"from the table on the right."));
	m_renameEventButton->setToolTip(tr("Click this button to rename the selected event."));
	m_renameEventButton->setWhatsThis(tr("Rename the selected scheduled event.  This button only "
		"works if a single event is selected."));
	m_triggerEventButton->setToolTip(tr("Trigger the scheduled event"));
	m_triggerEventButton->setWhatsThis(tr("Click this button to trigger the selected events."));
	m_scheduleList->setToolTip(tr("The list of currently scheduled events"));
	m_scheduleList->setWhatsThis(tr("This is the list of scheduled events.  You can "
		"select multiple entries in this list."));
}


HostConfigScheduleWidget::~HostConfigScheduleWidget()
{
}


void HostConfigScheduleWidget::resizeEvent(QResizeEvent* event)
{
	Q_UNUSED(event);
}


void HostConfigScheduleWidget::enableWidgets(bool enable)
{
	if (enable)
	{
		for (int row = 0; row < m_scheduleList->rowCount(); row++)
		{
			for (int col = 0; col < m_scheduleList->columnCount(); col++)
			{
				QWidget* widget = m_scheduleList->cellWidget(row, col);
				bool enableOverride = false;
				if (m_editable)
				{
					// Some widgets may have been marked as unsupported by a host
					QVariant supported = widget->property(PROPERTY_SUPPORTED);
					if (supported.isValid() && supported.toBool() == false)
						enableOverride = true;
				}
				widget->setEnabled(m_editable && !enableOverride);
			}
		}
	}
	m_scheduleList->setEnabled(enable);
	
	m_addEventButton->setEnabled(enable && m_editable);

	bool itemSelected = !m_scheduleList->selectionModel()->selectedRows().isEmpty();
	bool oneSelected = m_scheduleList->selectionModel()->selectedRows().size() == 1;
	
	m_triggerEventButton->setEnabled(enable && itemSelected);
	m_deleteEventButton->setEnabled(enable && m_editable && itemSelected);
	m_renameEventButton->setEnabled(enable && m_editable && oneSelected);
}


void HostConfigScheduleWidget::resetWidgets()
{
	m_scheduleList->clear();
	m_scheduleList->setRowCount(0);
	m_scheduleList->setColumnCount(0);
}


void HostConfigScheduleWidget::scheduleList_itemSelectionChanged()
{
	enableWidgets(true);
}
