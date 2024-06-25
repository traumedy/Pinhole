#include "HeartbeatThread.h"
#include "Logger.h"
#include "Values.h"

#include <QTimer>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>


HeartbeatThread::HeartbeatThread(QObject *parent)
	: QObject(parent)
{
}


HeartbeatThread::~HeartbeatThread()
{
	if (m_thread)
	{
		m_thread->quit();
		m_thread->wait();
	}
}


void HeartbeatThread::start()
{
#if defined(QT_DEBUG)
	Logger() << tr("RUNNING IN DEBUG, NOT STARTING HEARTBEAT THREAD");
	return;
#endif
	Logger(LOG_DEBUG) << tr("Starting lockup heartbeat thread");

	// Heartbeast checker runs on secondary thread	
	m_thread = new QThread(this);
	QTimer* checkTimer = new QTimer(nullptr);	// Must be nullptr
	checkTimer->setSingleShot(false);
	checkTimer->setInterval(INTERVAL_APPHEARTBEAT);
	checkTimer->moveToThread(m_thread);
	// Must specify Qt::DirectConnection or it gets queued to the main thread
	connect(checkTimer, &QTimer::timeout,
		this, &HeartbeatThread::checkHeartbeat, Qt::DirectConnection);	
	connect(m_thread, &QThread::started,
		checkTimer, QOverload<>::of(&QTimer::start));
	connect(m_thread, &QThread::finished,
		checkTimer, &QTimer::deleteLater);
	m_thread->start();

	// Heartbeat sender runs on main thread
	QTimer* heartbeatSender = new QTimer(this);
	heartbeatSender->setSingleShot(false);
	heartbeatSender->setInterval(INTERVAL_APPHEARTBEAT);
	connect(heartbeatSender, &QTimer::timeout,
		this, &HeartbeatThread::heartbeat);
	heartbeatSender->start();
}


void HeartbeatThread::heartbeat()
{
	//qDebug() << "heartbeat" << QThread::currentThreadId();

	m_lastHeartbeat = QDateTime::currentDateTime();
}


void HeartbeatThread::checkHeartbeat()
{
	//qDebug() << "checkHeartbeat" << QThread::currentThreadId();

	if (m_lastHeartbeat.msecsTo(QDateTime::currentDateTime()) > INTERVAL_APPTIMEOUT)
	{
		Logger(LOG_ERROR) << tr("!!! INTERNAL HEARTBEAT TIMEOUT %1 SECS, MAIN THREAD LOCKUP?  TERMINATING!")
			.arg(INTERVAL_APPTIMEOUT / 1000);
		QCoreApplication::exit(EXIT_LOCKUP);
		QThread::sleep(5000);
		abort();
	}
}
