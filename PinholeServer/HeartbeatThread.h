#pragma once

#include <QObject>
#include <QDateTime>

// The purpose of this class is to listen for heartbeats
// from the main thread and kill the program if it fails to receive
// heartbeats in after reasonable timeout

class QThread;

class HeartbeatThread : public QObject
{
	Q_OBJECT


public slots:
	void heartbeat();
	void start();

public:
	HeartbeatThread(QObject *parent = nullptr);
	~HeartbeatThread();

private slots:
	void checkHeartbeat();

private:
	QDateTime m_lastHeartbeat = QDateTime::currentDateTime();
	QThread* m_thread = nullptr;
};
