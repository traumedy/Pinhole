#pragma once

#include <QObject>

class DummyWindow : public QObject
{
	Q_OBJECT

public:
	DummyWindow(QObject *parent = nullptr);
	~DummyWindow();
};
