#pragma once

#include <QThread>

class QTimer;

class GuiWaiter : public QThread
{
	Q_OBJECT

public:
	GuiWaiter(QObject *parent = nullptr);
	~GuiWaiter();

signals:
	void finished();

protected:
	void run() override;

private:
	bool checkGUI();
};
