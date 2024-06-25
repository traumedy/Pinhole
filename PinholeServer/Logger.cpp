#include "Logger.h"
#include "Settings.h"
#include "CommandInterface.h"
#include "Values.h"

#include <QFile>
#include <QDateTime>
#include <QHostInfo>
#include <QDebug>

#include <iostream>

QMutex Logger::s_mutex;
int Logger::s_hostLogLevel = LOG_NORMAL;
int Logger::s_remoteLogLevel = LOG_NORMAL;
Settings* Logger::s_settings = nullptr;
CommandInterface* Logger::s_commandInterface = nullptr;

const QMap<QString, int> Logger::s_logLevelMap =
{
	{ LOG_LEVEL_ERROR, 5 },
	{ LOG_LEVEL_WARNING, 4 },
	{ LOG_LEVEL_NORMAL, 3 },
	{ LOG_LEVEL_EXTRA, 2 },
	{ LOG_LEVEL_DEBUG, 1 }
};

const QStringList Logger::s_LogLevelNames = { "", "Debug", "Extra", "", "Warning", "Error", "" };

QTextStream& qStdOut()
{
	static QTextStream ts(stdout);
	return ts;
}


Logger::Logger(int level, QObject *parent)
	: QObject(parent), m_level(level), m_message(&m_string)
{
}


Logger::~Logger()
{
	if (m_string.isEmpty())
		m_string = tr("<empty log entry>");

	// Terminate the line
	QChar c = m_string[m_string.length() - 1];
	if (c != '\r' && c != '\n')
		m_message << endl;

	// time/date stamp
	QString header = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd: ");
	if (!s_LogLevelNames[m_level].isEmpty())
	{
		header += "[" + s_LogLevelNames[m_level] + "] ";
	}
	
	QMutexLocker locker(&s_mutex);

	// Always output to console
	qStdOut() << header << m_string;
	qStdOut().flush();

	// Output to file
	if (m_level >= s_hostLogLevel)
	{
		QString logFilename(s_settings->dataDir() + FILENAME_LOGFILE);
		QFile logFile(logFilename);
		if (logFile.size() >= MAX_LOG_FILESIZE)
		{
			// Rollover log
			qStdOut() << tr("Rolling over log files...\r\n");
			qStdOut().flush();
			
			// Delete the last log if it exists
			if (QFile::exists(logFilename + "." + QString::number(MAX_LOG_FILES)))
				QFile::remove(logFilename + "." + QString::number(MAX_LOG_FILES));

			for (int n = MAX_LOG_FILES - 1; n > 0; n--)
			{
				// Rename the other numbered logs
				if (QFile::exists(logFilename + "." + QString::number(n)))
					QFile::rename(logFilename + "." + QString::number(n),
						logFilename + "." + QString::number(n + 1));
			}

			// Rename the current log
			QFile::rename(logFilename, logFilename + ".1");
		}
		
		if (!logFile.open(QFile::WriteOnly | QFile::Append))
		{
			qDebug() << tr("Unable to open log file:") << logFilename;
		}
		else
		{
			logFile.write(header.toUtf8());
			logFile.write(m_string.toUtf8());
			logFile.close();
		}
	}

	// Log to network interface
	if (m_level >= s_remoteLogLevel)
	{
		if (nullptr != s_commandInterface)
		{
			s_commandInterface->logMessage(m_level, header + m_string);
		}
	}

#if defined(QT_DEBUG) && defined(Q_OS_WIN)
	//OutputDebugStringW(m_string.toLocal8Bit().data());
#endif
}


QDateTime Logger::getDateTimeFromString(const QByteArray& string)
{
	if (string.length() < 23)
		return QDateTime();

	return QDateTime::fromString(QString::fromUtf8(string.left(19)), DATETIME_STRINGFORMAT);
}


QByteArray Logger::getLogFileData(const QString& startDateStr, const QString& endDateStr)
{
	QDateTime startDate = QDateTime::fromString(startDateStr, DATETIME_STRINGFORMAT);
	QDateTime endDate = QDateTime::fromString(endDateStr, DATETIME_STRINGFORMAT);

	if (startDate.isNull() || endDate.isNull())
		return QByteArray();

	if (startDate > endDate)
		return QByteArray();

	QMutexLocker locker(&s_mutex);

	QString baseLogFilename = s_settings->dataDir() + FILENAME_LOGFILE;
	QStringList logFilenames;
	logFilenames.append(baseLogFilename);
	for (int n = 1; n <= MAX_LOG_FILES; n++)
	{
		if (QFile::exists(baseLogFilename + "." + QString::number(n)))
			logFilenames.append(baseLogFilename + "." + QString::number(n));
	}

	bool logStartsInRange = false;
	bool endDateFound = false;
	QByteArray logData;
	QByteArray line;
	QDateTime lineDate;
	for (const auto& logFilename : logFilenames)
	{
		QFile logFile(logFilename);
		if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qDebug() << tr("Unable to open log file:") << logFilename;
		}
		else
		{
#ifdef QT_DEBUG
			qDebug() << "Opened log file:" << logFilename;
#endif
			do
			{
				line = logFile.readLine();
				lineDate = getDateTimeFromString(line);
			} while (!line.isEmpty() && lineDate.isNull());

			if (line.isEmpty())
			{
#ifdef QT_DEBUG
				qDebug() << "No first line";
#endif
				// No first line?
				continue;
			}

			if (lineDate > endDate)
			{
#ifdef QT_DEBUG
				qDebug() << "File starts after target end date" << lineDate << endDate;
#endif
				// This file begins after the end date we are looking for, skip it
				continue;
			}

			if (endDateFound && lineDate < startDate)
			{
#ifdef QT_DEBUG
				qDebug() << "File starts after end date found, copying entire file" << lineDate << startDate;
#endif
				// We already have the end of data and this file starts after startDate
				logData.prepend(line + logFile.readAll());
				continue;
			}

			logStartsInRange = lineDate >= startDate && lineDate <= endDate;

			QByteArray thisLog;

			// Look for end date
			do
			{
				if (!endDateFound)
				{
					if (lineDate > endDate)
					{
						endDateFound = true;
						break;
					}
				}

				if (lineDate >= startDate)
				{
					thisLog.append(line);
				}

				line = logFile.readLine();
				lineDate = getDateTimeFromString(line);
			} while (!line.isEmpty());

			logData.prepend(thisLog);

			if (!logStartsInRange && endDateFound)
			{
#ifdef QT_DEBUG
				qDebug() << "Log not in range and end date found, done";
#endif

				// All data found
				break;
			}
		}
	}

	if (!logData.isEmpty())
	{
		QString logMsg = tr("(Log entries on %1 in range %2 to %3)\r\n")
			.arg(QHostInfo::localHostName())
			.arg(startDate.toString("yyyy-MM-dd HH:mm"))
			.arg(endDate.toString("yyyy-MM-dd HH:mm"));
		logData.prepend(logMsg.toUtf8());
	}
	else
	{
		QString logMsg = tr("(No log entries on %1 found in range %2 to %3)")
			.arg(QHostInfo::localHostName())
			.arg(startDate.toString("yyyy-MM-dd HH:mm"))
			.arg(endDate.toString("yyyy-MM-dd HH:mm"));
		logData = logMsg.toUtf8();
	}

	return logData;
}

