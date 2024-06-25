#pragma once

/* Utility.h - Utility functions */

#include <QMetaEnum>

#include <string>
#include <vector>

template < typename T, size_t N >
inline size_t countof(T(&)[N])
{
	return std::extent< T[N] >::value;
}

class QSslCertificate;
class QSslKey;
class QHostAddress;
class QJsonObject;

// Returns a string representation of a Qt Enum
template<typename QEnum>
QString QtEnumToString(const QEnum value)
{
	return QString(QMetaEnum::fromType<QEnum>().valueToKey(value));
}


struct ProcessInfo
{
	quint32 id = 0;
	QString name;
};

// Converts std::vector<std::string> to QStringList
QStringList StdStringVectorToQStringList(const std::vector<std::string> stringvec);

// Converts QStringList to std::vector<std::string>
std::vector<std::string> QStringListToStdStringVector(const QStringList& stringlist);

// Breaks a string into QStringList with quoted tokens, as on a command line
QStringList SplitCommandLine(const QString& cmdLine);

// Generates an SSL certificate/key pair
QPair<QSslCertificate, QSslKey> GenerateCertKeyPair(
	const QString& country, const QString& organization, const QString& commonName);

// Generates a random string of characters between ascii 35 '#' and 125 '}' or 'A' and 'Z' if fileSafe is true
QString GenerateRandomString(int len = 8, bool fileSafe = false);

// Reads a QVariant if it exists or returns a default
QVariant ReadJsonValueWithDefault(const QJsonObject& jsonObj, const QString& key, const QVariant& value);

// Returns true if another instance of this program is already running
bool IsThisProgramAlreadyRunning();

 // Returns the process id of a process with the given executable path
quint32 findProcessWithPath(const QString& name);

// Returns a list of running processes
QList<ProcessInfo> runningProcesses();

// Kills a process
bool killProcess(quint32 id, int msecs = 30000);

// Returns a string representing the current date-time usable in filenames
QString currentDateTimeFilenameString();

// Returns a string of the IP of a QHostAddress, taking into account IPv6 vIPv4
QString HostAddressToString(const QHostAddress& address, bool* ipv4 = nullptr);

// Registers MsgPack types
void RegisterMsgpackTypes();

// Returns the true base directory of where the application is running from (stupid Apple .app folders)
QString ApplicationBaseDir();

// Returns a string representation of a milliseconds time period
QString MillisecondsToString(qint64 ms);

// Returns a string representing the 'when' of an offset value for a scheduled event
QString EventOffsetToString(const QString& frequency, int offset);

// Splits semicolon separated strings into a StringList removing whitespace 
QStringList SplitSemicolonString(const QString& str);

// Returns a file name safe version of a string
QString FilenameString(const QString& name);

// Returns true if process with pid is valid
bool IsProcessRunning(qint64 pid);

// Modifies a nested JSON value
void modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue);
