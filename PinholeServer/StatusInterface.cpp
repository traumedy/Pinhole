#include "StatusInterface.h"
#include "Settings.h"
#include "AlertManager.h"
#include "AppManager.h"
#include "GlobalManager.h"
#include "Logger.h"
#include "../common/Utilities.h"
#include "../common/PinholeCommon.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QHostInfo>
#include <QCoreApplication>


StatusInterface::StatusInterface(Settings* settings, AlertManager* alertManager, 
	AppManager* appManager, GlobalManager* globalManager, QObject *parent)
	: QObject(parent), m_settings(settings), m_alertManager(alertManager),
	m_appManager(appManager), m_globalManager(globalManager)
{
}


StatusInterface::~StatusInterface()
{
}


QByteArray StatusInterface::processPacket(const QByteArray & packet, const QString& MAC)
{
	// Parse the data as JSON
	QJsonDocument jsonDoc = QJsonDocument::fromJson(packet);
	// Validate the data
	if (jsonDoc.isNull())
	{
		return QByteArray();
	}
	else
	{
		QJsonObject jsonObject = jsonDoc.object();
		QString command = jsonObject[QString(TAG_COMMAND)].toString();
		if (command != UDPCOMMAND_QUERY)
		{
			// Only queries are currently supported
			return QByteArray();
		}
		else
		{
			// Build response json
			QJsonObject jsonResponse;
			jsonResponse[TAG_COMMAND] = UDPCOMMAND_ANNOUNCE;
			jsonResponse[TAG_ID] = m_settings->serverId();
			jsonResponse[TAG_MAC] = MAC;
			jsonResponse[TAG_NAME] = QHostInfo::localHostName();
			jsonResponse[TAG_ROLE] = m_globalManager->getRole();
			jsonResponse[TAG_VERSION] = QCoreApplication::applicationVersion();
			jsonResponse[TAG_OS] = QSysInfo::prettyProductName();

			QString statusText;
			int alertCount = m_alertManager->getAlertCount();
			if (0 != alertCount)
			{
				statusText += tr("%1 alert%2, ")
					.arg(alertCount)
					.arg(alertCount == 1 ? "" : "s");
			}

			int runningCount = m_appManager->runningAppCount();
			if (0 != runningCount)
			{
				statusText += tr("%1 app%2 running")
					.arg(runningCount)
					.arg(runningCount == 1 ? "" : "s");
			}
			else
			{
				statusText += tr("Idle %1").arg(MillisecondsToString(m_settings->idleTime()));
			}

			jsonResponse[TAG_STATUS] = statusText;

#if defined(Q_OS_WIN)
			jsonResponse[TAG_PLATFORM] = "WIN";
#elif defined(Q_OS_MAC)
			jsonResponse[TAG_PLATFORM] = "MAC";
#elif defined(Q_OS_UNIX)
			jsonResponse[TAG_PLATFORM] = "UNI";
#else
			jsonResponse[TAG_PLATFORM] = "UNK";
#endif

			// Return response packet
			return QJsonDocument(jsonResponse).toJson();
		}
	}

	return QByteArray();
}


void StatusInterface::sendStatus(const QString& status)
{
	QJsonObject jsonResponse;
	jsonResponse[TAG_COMMAND] = UDPCOMMAND_STATUS;
	jsonResponse[TAG_ID] = m_settings->serverId();
	jsonResponse[TAG_STATUS] = status;
	emit sendPacket(QJsonDocument(jsonResponse).toJson());
}
