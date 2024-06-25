/* Utilities.cpp - Utility functions */

#include "Utilities.h"
#include "PinholeCommon.h"
#include "../qmsgpack/msgpack.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QHostAddress>
#include <QSslCertificate>
#include <QSslKey>
#include <QDir>
#include <QCoreApplication>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/safestack.h>
#include <openssl/pem.h>


// Converts std::vector<std::string> to QStringList
QStringList StdStringVectorToQStringList(const std::vector<std::string> stringvec)
{
	QStringList ret;
	ret.reserve(stringvec.size());
	for (const auto& str : stringvec)
		ret.append(QString::fromStdString(str));
	return ret;
}


#if 0
// Windows style command line splitting
#define NULCHAR    '\0'
#define SPACECHAR  ' '
#define TABCHAR    '\t'
#define DQUOTECHAR '\"'
#define SLASHCHAR  '\\'

QStringList SplitCommandLine(
	const QString& commandline
)
{
	QStringList ret;
	int inquote;                    /* 1 = inside quotes */
	int copychar;                   /* 1 = copy char to *args */
	unsigned numslash;              /* num of backslashes seen */

	/* first scan the program name, copy it, and count the bytes */
	//p = cmdstart;
	int p = 0;
	inquote = 0;

	/* loop on each argument */
	for (;;) {

		QString arg;

		while (p < commandline.length() && 
			(commandline[p] == SPACECHAR || commandline[p] == TABCHAR))
			++p;

		if (p == commandline.length())
			break;              /* end of args */

		/* loop through scanning one argument */
		for (;;) {
			copychar = 1;
			/* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
			   2N+1 backslashes + " ==> N backslashes + literal "
			   N backslashes ==> N backslashes */
			numslash = 0;
			while (p < commandline.length() && commandline[p] == SLASHCHAR) {
				/* count number of backslashes for use below */
				++p;
				++numslash;
			}
			if (p < commandline.length() && commandline[p] == DQUOTECHAR) {
				/* if 2N backslashes before, start/end quote, otherwise
					copy literally */
				if (numslash % 2 == 0) {
					if (inquote && p < commandline.length() - 1 && 
						commandline[p + 1] == DQUOTECHAR) {
						p++;    /* Double quote inside quoted string */
					}
					else {    /* skip first quote char and copy second */
						copychar = 0;       /* don't copy quote */
						inquote = !inquote;
					}
				}
				numslash /= 2;          /* divide numslash by two */
			}

			/* copy slashes */
			while (numslash--) {
				arg += SLASHCHAR;
			}

			/* if at end of arg, break loop */
			if (p == commandline.length() || (!inquote && (commandline[p] == SPACECHAR || commandline[p] == TABCHAR)))
				break;

			/* copy character into argument */
			if (copychar && p < commandline.length()) {
				arg += commandline[p];
			}
			++p;

			if (p == commandline.length())
				break;
		}

		ret.append(arg);
	}

	return ret;
}
#else
// QT style command line splitting
QStringList SplitCommandLine(
	const QString& commandline
)
{
	QStringList args;
	QString tmp;
	int quoteCount = 0;
	bool inQuote = false;

	// handle quoting. tokens can be surrounded by double quotes
	// "hello world". three consecutive double quotes represent
	// the quote character itself.
	for (int i = 0; i < commandline.size(); ++i) {
		if (commandline.at(i) == QLatin1Char('"')) {
			++quoteCount;
			if (quoteCount == 3) {
				// third consecutive quote
				quoteCount = 0;
				tmp += commandline.at(i);
			}
			continue;
		}
		if (quoteCount) {
			if (quoteCount == 1)
				inQuote = !inQuote;
			quoteCount = 0;
		}
		if (!inQuote && commandline.at(i).isSpace()) {
			if (!tmp.isEmpty()) {
				args += tmp;
				tmp.clear();
			}
		}
		else {
			tmp += commandline.at(i);
		}
	}
	if (!tmp.isEmpty())
		args += tmp;

	return args;
}
#endif


std::vector<std::string> QStringListToStdStringVector(const QStringList& stringlist)
{
	std::vector<std::string> ret;
	ret.reserve(stringlist.size());
	for (const auto& str : stringlist)
		ret.push_back(str.toStdString());
	return ret;
}


QPair<QSslCertificate, QSslKey> GenerateCertKeyPair(
	const QString& country, const QString& organization, const QString& commonName)
{
	EVP_PKEY * pkey = nullptr;
	RSA * rsa = nullptr;
	X509 * x509 = nullptr;
	X509_NAME * name = nullptr;
	BIO * bp_public = nullptr, *bp_private = nullptr;
	const char * buffer = nullptr;
	long size;

	pkey = EVP_PKEY_new();
	q_check_ptr(pkey);
	rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
	q_check_ptr(rsa);
	EVP_PKEY_assign_RSA(pkey, rsa);
	x509 = X509_new();
	q_check_ptr(x509);
	ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
	X509_gmtime_adj(X509_get_notBefore(x509), 0); // not before current time
	X509_gmtime_adj(X509_get_notAfter(x509), 31536000L); // not after a year from this point
	X509_set_pubkey(x509, pkey);
	name = X509_get_subject_name(x509);
	q_check_ptr(name);
	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)country.data(), -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)organization.data(), -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)commonName.data(), -1, -1, 0);
	X509_set_issuer_name(x509, name);
	X509_sign(x509, pkey, EVP_sha1());

	bp_private = BIO_new(BIO_s_mem());
	q_check_ptr(bp_private);
	//if (PEM_write_bio_PrivateKey(bp_private, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1)
	if (PEM_write_bio_PKCS8PrivateKey(bp_private, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1)
	{
		EVP_PKEY_free(pkey);
		X509_free(x509);
		BIO_free_all(bp_private);
		qFatal("PEM_write_bio_PrivateKey");
	}

	bp_public = BIO_new(BIO_s_mem());
	q_check_ptr(bp_public);
	if (PEM_write_bio_X509(bp_public, x509) != 1)
	{
		EVP_PKEY_free(pkey);
		X509_free(x509);
		BIO_free_all(bp_public);
		BIO_free_all(bp_private);
		qFatal("PEM_write_bio_PrivateKey");
	}

	size = BIO_get_mem_data(bp_public, &buffer);
	q_check_ptr(buffer);
	QSslCertificate cert(QByteArray(buffer, size));

	size = BIO_get_mem_data(bp_private, &buffer);
	q_check_ptr(buffer);
	QSslKey key(QByteArray(buffer, size), QSsl::Rsa);

	EVP_PKEY_free(pkey); // this will also free the rsa key
	X509_free(x509);
	BIO_free_all(bp_public);
	BIO_free_all(bp_private);

	return { cert, key };
}


QString GenerateRandomString(int len, bool fileSafe)
{
	// Cheapo
	qsrand(time(nullptr));
	QString ret;
	for (int n = 0; n < len; n++)
	{
		if (fileSafe)
			ret += QChar((qrand() % ('Z' - 'A')) + 'A');
		else
			ret += QChar((qrand() % 90) + '#');
	}
	return ret;
}


QVariant ReadJsonValueWithDefault(const QJsonObject& jsonObj, const QString& key, const QVariant& value)
{
	if (jsonObj.contains(key))
	{
		return jsonObj[key].toVariant();
	}

	return value;
}


class ProcessnameEquals
{
public:
	ProcessnameEquals(const QString &name)
#ifdef Q_OS_WIN
		: m_name(name.toLower())
#else
		: m_name(name)
#endif
	{}

	bool operator()(const ProcessInfo &info)
	{
#ifdef Q_OS_WIN
		const QString infoName = info.name.toLower();
		if (infoName == QDir::toNativeSeparators(m_name))
			return true;
#else
		const QString infoName = info.name;
#endif
		if (infoName == m_name)
			return true;

		const QFileInfo fi(infoName);
		if (fi.fileName() == m_name || fi.baseName() == m_name)
			return true;
		return false;
	}

private:
	QString m_name;
};


bool IsThisProgramAlreadyRunning()
{
	const QList<ProcessInfo> allProcesses = runningProcesses();
	const int count = std::count_if(allProcesses.constBegin(), allProcesses.constEnd(),
		ProcessnameEquals(QCoreApplication::applicationFilePath()));
	return (count > 1);
}


quint32 findProcessWithPath(const QString& name)
{
	for (const auto& proc : runningProcesses())
	{
		ProcessnameEquals p(name);
		if (p(proc))
			return proc.id;
	}

	return 0;
}


QString currentDateTimeFilenameString()
{
	return QDateTime::currentDateTime().toString("yyyy-MM-dd_hh.mm.ss");
}


QString HostAddressToString(const QHostAddress& address, bool* ipv4)
{
	bool conversionOk = false;
	QHostAddress ipv4Addr(address.toIPv4Address(&conversionOk));
	if (conversionOk)
	{
		if (ipv4)
			*ipv4 = true;
		return ipv4Addr.toString();
	}

	if (ipv4)
		*ipv4 = false;
	QString addrStr = address.toString();
	int percentIndex = addrStr.lastIndexOf('%');
	if (-1 != percentIndex)
	{
		addrStr = addrStr.left(percentIndex);
	}
	return addrStr;
}


void RegisterMsgpackTypes()
{
	MsgPack::registerType(QMetaType::QDateTime, 37); // 37 is msgpack user type id
}


QString ApplicationBaseDir()
{
	QString dirPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MAC)
	if (dirPath.endsWith(".app/Contents/MacOS"))
	{
		for (int n = 0; n < 3; n++)
		{
			dirPath = dirPath.left(dirPath.lastIndexOf('/'));
		}
	}
#endif

	return dirPath;
}

#define MS_PER_SEC (1000)
#define MS_PER_MIN (MS_PER_SEC * 60)
#define MS_PER_HOUR (MS_PER_MIN * 60)
#define MS_PER_DAY (MS_PER_HOUR * 24)

QString MillisecondsToString(qint64 ms)
{
	QString ret;

	if (ms < 1000)
	{
		ret = QString("%1ms").arg(ms);
	}
	else
	{
		if (ms > MS_PER_DAY)
		{
			ret += QString("%1D:").arg(ms / MS_PER_DAY);
		}
		if (ms > MS_PER_HOUR)
		{
			ret += QString("%1H:").arg((ms % MS_PER_DAY) / MS_PER_HOUR);
		}
		if (ms > MS_PER_MIN)
		{
			ret += QString("%1M:").arg((ms % MS_PER_HOUR) / MS_PER_MIN);
		}
		ret += QString("%1S").arg((ms % MS_PER_MIN) / MS_PER_SEC);
	}
	return ret;
}


QString EventOffsetToString(const QString& frequency, int offset)
{
	if (SCHED_FREQ_DISABLED == frequency)
	{
		return "Never";
	}
	else if (SCHED_FREQ_WEEKLY == frequency)
	{
		QDateTime eventTime;
		eventTime.setSecsSinceEpoch(static_cast<qint64>(offset) * 60);
		return eventTime.toString("ddd HH:mm A");
	}
	else if (SCHED_FREQ_DAILY == frequency)
	{
		QTime time;
		time.setHMS((offset % 1440) / 60, offset % 60, 0);
		return time.toString("hh:mm A");
	}
	else if (SCHED_FREQ_HOURLY == frequency)
	{
		return "+" + QString::number(offset % 60);
	}
	else if (SCHED_FREQ_ONCE == frequency)
	{
		QDateTime eventTime;
		eventTime.setSecsSinceEpoch(static_cast<qint64>(offset) * 60);
		return eventTime.toString("yyyy-MM-dd HH:mm ddd");
	}

	return "";
}


QStringList SplitSemicolonString(const QString& str)
{
	QStringList ret;
	if (str.isEmpty())
		return ret;
	ret = str.split(';', QString::SkipEmptyParts);
	for (auto s : ret)
	{
		s = s.trimmed();
	}

	return ret;
}


QString FilenameString(const QString& name)
{
	QString ret(name);
	QString illegal("$%<>=:/\\\"|?*");
	for (auto& c : ret)
	{
		if (illegal.contains(c))
			c = '_';
	}
	return ret;
}


void modifyJsonValue(QJsonValue& destValue, const QString& path, const QJsonValue& newValue)
{
	const int indexOfDot = path.indexOf('.');
	const QString dotPropertyName = path.left(indexOfDot);
	const QString dotSubPath = indexOfDot > 0 ? path.mid(indexOfDot + 1) : QString();

	const int indexOfSquareBracketOpen = path.indexOf('[');
	const int indexOfSquareBracketClose = path.indexOf(']');

	const int arrayIndex = path.mid(indexOfSquareBracketOpen + 1, indexOfSquareBracketClose - indexOfSquareBracketOpen - 1).toInt();

	const QString squareBracketPropertyName = path.left(indexOfSquareBracketOpen);
	const QString squareBracketSubPath = indexOfSquareBracketClose > 0 ? (path.mid(indexOfSquareBracketClose + 1)[0] == '.' ? path.mid(indexOfSquareBracketClose + 2) : path.mid(indexOfSquareBracketClose + 1)) : QString();

	// determine what is first in path. dot or bracket
	bool useDot = true;
	if (indexOfDot >= 0) // there is a dot in path
	{
		if (indexOfSquareBracketOpen >= 0) // there is squarebracket in path
		{
			if (indexOfDot > indexOfSquareBracketOpen)
				useDot = false;
			else
				useDot = true;
		}
		else
			useDot = true;
	}
	else
	{
		if (indexOfSquareBracketOpen >= 0)
			useDot = false;
		else
			useDot = true; // acutally, id doesn't matter, both dot and square bracket don't exist
	}

	QString usedPropertyName = useDot ? dotPropertyName : squareBracketPropertyName;
	QString usedSubPath = useDot ? dotSubPath : squareBracketSubPath;

	QJsonValue subValue;
	if (destValue.isArray())
		subValue = destValue.toArray()[usedPropertyName.toInt()];
	else if (destValue.isObject())
		subValue = destValue.toObject()[usedPropertyName];
	else
		qDebug() << "oh, what should i do now with the following value?! " << destValue;

	if (usedSubPath.isEmpty())
	{
		subValue = newValue;
	}
	else
	{
		if (subValue.isArray())
		{
			QJsonArray arr = subValue.toArray();
			QJsonValue arrEntry = arr[arrayIndex];
			modifyJsonValue(arrEntry, usedSubPath, newValue);
			arr[arrayIndex] = arrEntry;
			subValue = arr;
		}
		else if (subValue.isObject())
			modifyJsonValue(subValue, usedSubPath, newValue);
		else
			subValue = newValue;
	}

	if (destValue.isArray())
	{
		QJsonArray arr = destValue.toArray();
		if (subValue.isNull())
			arr.removeAt(arrayIndex);
		else
			arr[arrayIndex] = subValue;
		destValue = arr;
	}
	else if (destValue.isObject())
	{
		QJsonObject obj = destValue.toObject();
		if (subValue.isNull())
			obj.remove(usedPropertyName);
		else
			obj[usedPropertyName] = subValue;
		destValue = obj;
	}
	else
		destValue = newValue;
}

