#pragma once

#include <QDialog>
#include <QTimer>

class QToolBar;
class QListWidget;


class WindowManager : public QDialog
{
	Q_OBJECT

public:
	WindowManager(QWidget *parent = Q_NULLPTR);
	~WindowManager();

	static void cascadeWindow(QWidget* window);
	static void addWindow(QWidget* window);
	static void removeWindow(QWidget* window);

public slots:
	void windowDestroyed(QObject*);

private:
	void updateWindowList();
	void closeSelected();
	void cascadeSelected();
	void stackSelected();
	void hideSelected();
	void raiseSelected();

	QToolBar* m_toolBar = nullptr;
	QListWidget* m_windowList = nullptr;
	
	QTimer m_updateTimer;

	static int m_windowX;
	static int m_windowY;
	static const int m_spacing;
	static const int m_startX;
	static const int m_startY;
	static QList<QWidget*> m_windows;
};
