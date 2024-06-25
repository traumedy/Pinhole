#pragma once

#include "../common/PinholeCommon.h"

#include <QObject>
#include <QMap>
#include <QTextStream>
#include <QMutex>

class Settings;
class CommandInterface;

class Logger : public QObject
{
	Q_OBJECT

public:
	Logger(int level = LOG_NORMAL, QObject *parent = nullptr);
	~Logger();

	static void setSettings(Settings* settings)
	{
		s_settings = settings;
	}

	static void setCommandInterface(CommandInterface* commandInterface)
	{
		s_commandInterface = commandInterface;
	}

	static void setHostLogLevel(const QString& logLevel)
	{
		s_hostLogLevel = s_logLevelMap[logLevel];
	}

	static void setRemoteLogLevel(const QString& logLevel)
	{
		s_remoteLogLevel = s_logLevelMap[logLevel];
	}

	template <class T>
	Logger& operator<<(const T& value) {
		m_message << value;
		return *this;
	}

	static QByteArray getLogFileData(const QString& startDate, const QString& endDate);

	const static QMap<QString, int> s_logLevelMap;
	const static QStringList s_LogLevelNames;

private:
	static QDateTime getDateTimeFromString(const QByteArray& string);

	int m_level = LOG_NORMAL;
	QString m_string;
	QTextStream m_message;

	static QMutex s_mutex;
	static int s_hostLogLevel;
	static int s_remoteLogLevel;
	static Settings* s_settings;
	static CommandInterface* s_commandInterface;
};
