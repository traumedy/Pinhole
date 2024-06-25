#include "WindowManager.h"

#include <QWidget>
#include <QScreen>
#include <QGuiApplication>
#include <QGridLayout>
#include <QToolBar>
#include <QListWidget>
#include <QListWidgetItem>
#include <QApplication>

#define INTERVAL_LISTUPDATE		200

int WindowManager::m_windowX = 50;
int WindowManager::m_windowY = 50;
const int WindowManager::m_spacing = 30;
const int WindowManager::m_startX = 50;
const int WindowManager::m_startY = 50;
QList<QWidget*> WindowManager::m_windows;

WindowManager::WindowManager(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Pinhole Window Manager"));

	// Use QGridLayout for this widget
	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setMargin(0);

	m_toolBar = new QToolBar(this);
	m_toolBar->addAction(tr("Select all"), [this]() { m_windowList->selectAll(); })->
		setWhatsThis(tr("Selects all the windows in the window list."));
	m_toolBar->addAction(tr("Select none"), [this]() { m_windowList->clearSelection(); })->
		setWhatsThis(tr("Deselects all the windows in the window list."));
	m_toolBar->addAction(tr("Close"), [this]() { closeSelected(); })->
		setWhatsThis(tr("Close the selected windows."));
	m_toolBar->addAction(tr("Cascade"), [this]() { cascadeSelected(); })->
		setWhatsThis(tr("Cascade the selected windows.  Also shows hidden windows."));
	m_toolBar->addAction(tr("Stack"), [this]() { stackSelected();  })->
		setWhatsThis(tr("Stack the selected windows.  Also shows hidden windows."));
	m_toolBar->addAction(tr("Raise"), [this]() { raiseSelected(); })->
		setWhatsThis(tr("Raise the selected windows.  Also shows hidden windows."));
	m_toolBar->addAction(tr("Hide"), [this]() { hideSelected(); })->
		setWhatsThis(tr("Hide the selected windows."));

	gridLayout->addWidget(m_toolBar);

	m_windowList = new QListWidget(this);
	m_windowList->setWhatsThis(tr("This is the list of the title text of open windows.  "
		"Some of these windows may not be visible if they were hidden.  You can select "
		"multiple windows using the control and shift keys."));
	m_windowList->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_windowList->setSelectionMode(QAbstractItemView::ExtendedSelection);
	gridLayout->addWidget(m_windowList);
	connect(m_windowList, &QListWidget::itemDoubleClicked,
		this, [](QListWidgetItem *item)
	{ 
		QWidget* win = item->data(Qt::UserRole).value<QWidget*>();
		win->show();
		win->raise();
	});

	m_updateTimer.setInterval(INTERVAL_LISTUPDATE);
	m_updateTimer.setSingleShot(false);
	connect(&m_updateTimer, &QTimer::timeout,
		this, [this]()
	{
		updateWindowList();
	});
	m_updateTimer.start();
}


WindowManager::~WindowManager()
{
}


void WindowManager::cascadeWindow(QWidget* window)
{
	if (!window->isVisible())
	{
		// HACK: Force window to calculate size
		window->setAttribute(Qt::WA_DontShowOnScreen, true);
		window->show();
		window->hide();
		window->setAttribute(Qt::WA_DontShowOnScreen, false);
	}

	QScreen* screen = QGuiApplication::primaryScreen();

	if (window->frameGeometry().width() + m_windowX >= screen->size().width() - m_startX)
	{
		m_windowX = m_startX;
	}

	if (window->frameGeometry().height() + m_windowY >= screen->size().height() - m_startY)
	{
		m_windowY = m_startY;
	}

	window->move(m_windowX, m_windowY);
	m_windowX += m_spacing;
	m_windowY += m_spacing;
}


void WindowManager::addWindow(QWidget* window)
{
	if (!m_windows.contains(window))
	{
		m_windows.append(window);
	}
}


void WindowManager::removeWindow(QWidget* window)
{
	m_windows.removeAll(window);
}


void WindowManager::updateWindowList()
{
	// Build a map of the widget pointer to the QListWidgetItem that represents them
	QMap<QWidget*, QListWidgetItem*> listWindows;
	for (int x = 0; x < m_windowList->count(); x++)
	{
		QListWidgetItem* item = m_windowList->item(x);
		listWindows[item->data(Qt::UserRole).value<QWidget*>()] = item;
	}

	// Add new items and update existing ones
	for (const auto& win : m_windows)
	{
		QString text = win->windowTitle();
		if (win->isHidden() || !win->isVisible())
			text += tr(" (Hidden)");

		if (!listWindows.contains(win))
		{
			// Add new item if not already in list
			// Set with window title
			QListWidgetItem* newItem = new QListWidgetItem(text, m_windowList);
			newItem->setData(Qt::UserRole, QVariant::fromValue(win));
			// Connect notification of object being destroyed so we can remove from list
			connect(win, &QObject::destroyed,
				this, &WindowManager::windowDestroyed);
		}
		else
		{
			// Update text of item
			listWindows[win]->setText(text);
		}
	}

	// Keep this dialog window on top if no active modal widget
	if (nullptr == QApplication::activeModalWidget())
		raise();
}


void WindowManager::closeSelected()
{
	QList<QListWidgetItem*> selected = m_windowList->selectedItems();
	for (const auto& item : selected)
	{
		QWidget* win = item->data(Qt::UserRole).value<QWidget*>();
		win->close();
		delete item;
	}
}


void WindowManager::cascadeSelected()
{
	QList<QListWidgetItem*> selected = m_windowList->selectedItems();
	for (const auto& item : selected)
	{
		QWidget* win = item->data(Qt::UserRole).value<QWidget*>();
		win->show();
		win->raise();
		cascadeWindow(win);
	}
}


void WindowManager::stackSelected()
{
	QScreen* screen = QGuiApplication::primaryScreen();

	int firstRight = 0;
	int curRight = m_startX;
	int lastBottom = m_startY;
	QList<QListWidgetItem*> selected = m_windowList->selectedItems();
	for (const auto& item : selected)
	{
		bool firstWindow = false;
		QWidget* window = item->data(Qt::UserRole).value<QWidget*>();
		if (window->frameGeometry().height() + lastBottom > screen->size().height() - m_startY)
		{
			lastBottom = m_startY;
			curRight = firstRight;
			firstWindow = true;
		}
		window->move(curRight, lastBottom);
		window->show();
		if (0 == firstRight || firstWindow)
			firstRight = curRight + window->frameGeometry().width();
		lastBottom += window->frameGeometry().height();
	}
}


void WindowManager::raiseSelected()
{
	QList<QListWidgetItem*> selected = m_windowList->selectedItems();
	for (const auto& item : selected)
	{
		QWidget* win = item->data(Qt::UserRole).value<QWidget*>();
		win->show();
		win->raise();
	}
}


void WindowManager::hideSelected()
{
	QList<QListWidgetItem*> selected = m_windowList->selectedItems();
	for (const auto& item : selected)
	{
		QWidget* win = item->data(Qt::UserRole).value<QWidget*>();
		win->hide();
	}
}


void WindowManager::windowDestroyed(QObject* obj)
{
	// Remove window from list
	QWidget* win = reinterpret_cast<QWidget*>(obj);

	for (int x = 0; x < m_windowList->count(); x++)
	{
		if (m_windowList->item(x)->data(Qt::UserRole).value<QWidget*>() == win)
		{
			delete m_windowList->takeItem(x);
			// Window should (better) only be in list once
			return;
		}
	}
}
