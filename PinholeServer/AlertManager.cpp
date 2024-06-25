#include "AlertManager.h"
#include "Settings.h"
#include "Logger.h"
#include "Values.h"
#include "../common/Utilities.h"
#include "smtp.h"

#include <QSettings>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QHostInfo>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QProcess>


AlertManager::AlertManager(Settings* settings, QObject *parent)
	: QObject(parent), m_settings(settings)
{
	m_networkAccessManager = new QNetworkAccessManager(this);

	readAlertSettings();
}


AlertManager::~AlertManager()
{
}


bool AlertManager::readAlertSettings()
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	setSmtpServer(settings->value(PROP_ALERT_SMTPSERVER).toString());
	setSmtpPort(settings->value(PROP_ALERT_SMTPPORT, DEFAULT_SMTPPORT).toInt());
	setSmtpSSL(settings->value(PROP_ALERT_SMTPSSL, false).toBool());
	setSmtpTLS(settings->value(PROP_ALERT_SMTPTLS, false).toBool());
	setSmtpUser(settings->value(PROP_ALERT_SMTPUSER, DEFAULT_SMTPUSER).toString());
	setSmtpPass(settings->value(PROP_ALERT_SMTPPASS).toString());
	setSmtpEmail(settings->value(PROP_ALERT_SMTPEMAIL).toString());
	setSmtpName(settings->value(PROP_ALERT_SMTPNAME).toString());

	QStringList alertSlotList = settings->value(SETTINGS_ALERTSLOTLIST).toStringList();
	for (const auto& alertSlotName : alertSlotList)
	{
		settings->beginGroup(SETTINGS_ALERTSLOTPREFIX + alertSlotName);

		QSharedPointer<AlertSlot> newAlertSlot = QSharedPointer<AlertSlot>::create(alertSlotName);
		newAlertSlot->setEnabled(settings->value(PROP_ALERT_SLOTENABLED).toBool());
		newAlertSlot->setType(settings->value(PROP_ALERT_SLOTTYPE, ALERTSLOT_TYPE_SMPTEMAIL).toString());
		newAlertSlot->setArguments(settings->value(PROP_ALERT_SLOTARG).toString());

		connect(newAlertSlot.data(), &AlertSlot::valueChanged,
			this, &AlertManager::alertSlotValueChanged);

		m_alertSlotList[alertSlotName] = newAlertSlot;

		settings->endGroup();
	}

	return true;
}


bool AlertManager::writeAlertSettings() const
{
	QSharedPointer<QSettings> settings = m_settings->getScopedSettings();

	settings->setValue(PROP_ALERT_SMTPSERVER, getSmtpServer());
	settings->setValue(PROP_ALERT_SMTPPORT, getSmtpPort());
	settings->setValue(PROP_ALERT_SMTPSSL, getSmtpSSL());
	settings->setValue(PROP_ALERT_SMTPTLS, getSmtpTLS());
	settings->setValue(PROP_ALERT_SMTPUSER, getSmtpUser());
	settings->setValue(PROP_ALERT_SMTPPASS, getSmtpPass());
	settings->setValue(PROP_ALERT_SMTPEMAIL, getSmtpEmail());
	settings->setValue(PROP_ALERT_SMTPNAME, getSmtpName());

	// Delete old entries
	QStringList alertSlotList = settings->value(SETTINGS_ALERTSLOTLIST).toStringList();
	for (const auto& alertSlotName : alertSlotList)
	{
		settings->remove(SETTINGS_ALERTSLOTPREFIX + alertSlotName);
	}

	for (const auto& alertSlot : m_alertSlotList)
	{
		QString alertSlotName = alertSlot->getName();
		settings->beginGroup(SETTINGS_ALERTSLOTPREFIX + alertSlotName);
		settings->setValue(PROP_ALERT_SLOTNAME, alertSlotName);
		settings->setValue(PROP_ALERT_SLOTENABLED, alertSlot->getEnabled());
		settings->setValue(PROP_ALERT_SLOTTYPE, alertSlot->getType());
		settings->setValue(PROP_ALERT_SLOTARG, alertSlot->getArguments());
		settings->endGroup();
	}

	settings->setValue(SETTINGS_ALERTSLOTLIST, QStringList(m_alertSlotList.keys()));

	return true;
}


bool AlertManager::importSettings(const QJsonObject& root)
{
	setSmtpServer(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPSERVER, getSmtpServer()).toString());
	setSmtpPort(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPPORT, getSmtpPort()).toInt());
	setSmtpPort(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPSSL, getSmtpSSL()).toBool());
	setSmtpPort(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPTLS, getSmtpTLS()).toBool());
	setSmtpUser(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPUSER, getSmtpUser()).toString());
	setSmtpPass(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPPASS, getSmtpPass()).toString());
	setSmtpEmail(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPEMAIL, getSmtpEmail()).toString());
	setSmtpName(ReadJsonValueWithDefault(root, PROP_ALERT_SMTPNAME, getSmtpName()).toString());

	if (!root.contains(JSONTAG_ALERTSLOTS))
	{
		Logger(LOG_WARNING) << tr("JSON import data missing tag %1").arg(JSONTAG_ALERTSLOTS);
		return false;
	}

	QJsonArray alertSlotList = root[JSONTAG_ALERTSLOTS].toArray();

	if (root.contains(STATUSTAG_DELETEENTRIES) && root[STATUSTAG_DELETEENTRIES].toBool() == true)
	{
		// Delete entries
		for (const auto& alertSlot : alertSlotList)
		{
			if (!alertSlot.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON app entry %1 is not object").arg(JSONTAG_ALERTSLOTS);
			}
			else
			{
				QJsonObject jslot = alertSlot.toObject();
				QString name = jslot[PROP_ALERT_SLOTNAME].toString();
				m_alertSlotList.remove(name);
				Logger() << tr("Removing alert slot '%1'").arg(name);
			}
		}
	}
	else
	{
		// Modify and add entries
		QStringList alertSlotNameList;

		for (const auto& alertSlot : alertSlotList)
		{
			if (!alertSlot.isObject())
			{
				Logger(LOG_WARNING) << tr("JSON app entry %1 is not object").arg(JSONTAG_ALERTSLOTS);
			}
			else
			{
				QJsonObject jslot = alertSlot.toObject();
				QString name = jslot[PROP_ALERT_SLOTNAME].toString();
				alertSlotNameList.append(name);
				QSharedPointer<AlertSlot> newAlertSlot;
				if (m_alertSlotList.contains(name))
				{
					newAlertSlot = m_alertSlotList[name];
				}
				else
				{
					newAlertSlot = QSharedPointer<AlertSlot>::create(name);
					connect(newAlertSlot.data(), &AlertSlot::valueChanged,
						this, &AlertManager::alertSlotValueChanged);
				}
				newAlertSlot->setEnabled(ReadJsonValueWithDefault(root, PROP_ALERT_SLOTENABLED, newAlertSlot->getEnabled()).toBool());
				newAlertSlot->setType(ReadJsonValueWithDefault(root, PROP_ALERT_SLOTTYPE, newAlertSlot->getType()).toString());
				newAlertSlot->setArguments(ReadJsonValueWithDefault(root, PROP_ALERT_SLOTARG, newAlertSlot->getArguments()).toString());
			}
		}

		if (!root.contains(STATUSTAG_NODELETE) || root[STATUSTAG_NODELETE].toBool() == false)
		{
			// Remove slots that are no longer in the list
			for (const auto& alertSlotName : m_alertSlotList.keys())
			{
				if (!alertSlotNameList.contains(alertSlotName))
				{
					m_alertSlotList.remove(alertSlotName);
				}
			}
		}
	}

	return true;
}


QJsonObject AlertManager::exportSettings() const
{
	QJsonObject root;
	root[PROP_ALERT_SMTPSERVER] = getSmtpServer();
	root[PROP_ALERT_SMTPPORT] = getSmtpPort();
	root[PROP_ALERT_SMTPSSL] = getSmtpSSL();
	root[PROP_ALERT_SMTPTLS] = getSmtpTLS();
	root[PROP_ALERT_SMTPUSER] = getSmtpUser();
	root[PROP_ALERT_SMTPPASS] = getSmtpPass();
	root[PROP_ALERT_SMTPEMAIL] = getSmtpEmail();
	root[PROP_ALERT_SMTPNAME] = getSmtpName();

	QJsonArray jslotList;

	for (const auto& slot : m_alertSlotList)
	{
		QJsonObject jslot;
		jslot[PROP_ALERT_SLOTNAME] = slot->getName();
		jslot[PROP_ALERT_SLOTENABLED] = slot->getEnabled();
		jslot[PROP_ALERT_SLOTTYPE] = slot->getType();
		jslot[PROP_ALERT_SLOTARG] = slot->getArguments();

		jslotList.append(jslot);
	}

	root[JSONTAG_ALERTSLOTS] = jslotList;
	root[STATUSTAG_NODELETE] = false;
	root[STATUSTAG_DELETEENTRIES] = false;

	return root;
}


bool AlertManager::addAlertSlot(const QString& name)
{
	if (m_alertSlotList.contains(name))
		return false;

	m_alertSlotList[name] = QSharedPointer<AlertSlot>::create(name);
	connect(m_alertSlotList[name].data(), &AlertSlot::valueChanged,
		this, &AlertManager::alertSlotValueChanged);

	emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SLOTLIST, QVariant(m_alertSlotList.keys()));
	return true;
}


bool AlertManager::deleteAlertSlot(const QString& name)
{
	if (!m_alertSlotList.contains(name))
		return false;

	m_alertSlotList.remove(name);

	emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SLOTLIST, QVariant(m_alertSlotList.keys()));
	return true;
}


bool AlertManager::renameAlertSlot(const QString& oldName, const QString& newName)
{
	if (!m_alertSlotList.contains(oldName))
		return false;

	// Move pointer to new map key index
	QSharedPointer<AlertSlot> slot = *m_alertSlotList.find(oldName);
	m_alertSlotList.remove(oldName);
	slot->setName(newName);
	m_alertSlotList[newName] = slot;

	emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SLOTLIST, QVariant(m_alertSlotList.keys()));
	return true;
}


int AlertManager::getAlertCount() const
{
	return m_activeAlerts;
}


QString AlertManager::getSmtpServer() const
{
	return m_smtpServer;
}


bool AlertManager::setSmtpServer(const QString& str)
{
	if (m_smtpServer != str)
	{
		m_smtpServer = str;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPSERVER, QVariant(m_smtpServer));
	}
	return true;
}


int AlertManager::getSmtpPort() const
{
	return m_smtpPort;
}


bool AlertManager::setSmtpPort(int val)
{
	if (m_smtpPort != val)
	{
		if (val < 1 || val > MAX_PORT)
		{
			Logger(LOG_EXTRA) << tr("Invalid alert SMTP port value '%1'").arg(val);
			emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPPORT, QVariant(m_smtpPort));
			return false;
		}
		m_smtpPort = val;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPPORT, QVariant(m_smtpPort));
	}
	return true;
}


bool AlertManager::getSmtpSSL() const
{
	return m_smtpSSL;
}


bool AlertManager::setSmtpSSL(bool b)
{
	if (m_smtpSSL != b)
	{
		m_smtpSSL = b;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPSSL, QVariant(m_smtpSSL));
	}
	return true;
}


bool AlertManager::getSmtpTLS() const
{
	return m_smtpTLS;
}


bool AlertManager::setSmtpTLS(bool b)
{
	if (m_smtpTLS != b)
	{
		m_smtpTLS = b;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPTLS, QVariant(m_smtpTLS));
	}
	return true;
}


QString AlertManager::getSmtpUser() const
{
	return m_smtpUser;
}


bool AlertManager::setSmtpUser(const QString& str)
{
	if (m_smtpUser != str)
	{
		m_smtpUser = str;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPUSER, QVariant(m_smtpUser));
	}
	return true;
}


QString AlertManager::getSmtpPass() const
{
	return m_smtpPass;
}


bool AlertManager::setSmtpPass(const QString& str)
{
	if (m_smtpPass != str)
	{
		m_smtpPass = str;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPPASS, QVariant(m_smtpPass));
	}
	return true;
}


QString AlertManager::getSmtpEmail() const
{
	return m_smtpEmail;
}


bool AlertManager::setSmtpEmail(const QString& str)
{
	if (m_smtpEmail != str)
	{
		m_smtpEmail = str;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPEMAIL, QVariant(m_smtpEmail));	
	}
	return true;
}


QString AlertManager::getSmtpName() const
{
	return m_smtpName;
}


bool AlertManager::setSmtpName(const QString& str)
{
	if (m_smtpName != str)
	{
		m_smtpName = str;
		emit valueChanged(GROUP_ALERT, "", PROP_ALERT_SMTPNAME, QVariant(m_smtpName));
	}
	return true;
}


QVariant AlertManager::getAlertVariant(const QString& itemName, const QString& propName) const
{
	if (itemName.isEmpty())
	{
		if (PROP_ALERT_SLOTLIST == propName)
		{
			return QVariant(m_alertSlotList.keys());
		}
		else if (PROP_ALERT_ALERTCOUNT == propName)
		{
			return getAlertCount();
		}
		else if (PROP_ALERT_SMTPSERVER == propName)
		{
			return getSmtpServer();
		}
		else if (PROP_ALERT_SMTPPORT == propName)
		{
			return getSmtpPort();
		}
		else if (PROP_ALERT_SMTPSSL == propName)
		{
			return getSmtpSSL();
		}
		else if (PROP_ALERT_SMTPTLS == propName)
		{
			return getSmtpTLS();
		}
		else if (PROP_ALERT_SMTPUSER == propName)
		{
			return getSmtpUser();
		}
		else if (PROP_ALERT_SMTPPASS == propName)
		{
			return getSmtpPass();
		}
		else if (PROP_ALERT_SMTPEMAIL == propName)
		{
			return getSmtpEmail();
		}
		else if (PROP_ALERT_SMTPNAME == propName)
		{
			return getSmtpName();
		}
	}
	else
	{
		if (!m_alertSlotList.contains(itemName))
		{
			Logger(LOG_WARNING) << tr("Missing alert slot item value query: '%1:%2'")
				.arg(itemName)
				.arg(propName);
			return QVariant();
		}

		if (m_alertSlotStringCallMap.find(propName) != m_alertSlotStringCallMap.end())
		{
			return QVariant(std::get<0>(m_alertSlotStringCallMap.at(propName))(m_alertSlotList[itemName].data()));
		}
		else if (m_alertSlotBoolCallMap.find(propName) != m_alertSlotBoolCallMap.end())
		{
			return QVariant(std::get<0>(m_alertSlotBoolCallMap.at(propName))(m_alertSlotList[itemName].data()));
		}
	}

	Logger(LOG_WARNING) << tr("Missing property for alert slot value query: '%1'").arg(propName);
	return QVariant();

}


bool AlertManager::setAlertVariant(const QString& itemName, const QString& propName, const QVariant& value)
{
	if (itemName.isEmpty())
	{
		if (PROP_ALERT_SMTPSERVER == propName)
		{
			return setSmtpServer(value.toString());
		}
		else if (PROP_ALERT_SMTPPORT == propName)
		{
			return setSmtpPort(value.toInt());
		}
		else if (PROP_ALERT_SMTPSSL == propName)
		{
			return setSmtpSSL(value.toBool());
		}
		else if (PROP_ALERT_SMTPTLS == propName)
		{
			return setSmtpTLS(value.toBool());
		}
		else if (PROP_ALERT_SMTPUSER == propName)
		{
			return setSmtpUser(value.toString());
		}
		else if (PROP_ALERT_SMTPPASS == propName)
		{
			return setSmtpPass(value.toString());
		}
		else if (PROP_ALERT_SMTPEMAIL == propName)
		{
			return setSmtpEmail(value.toString());
		}
		else if (PROP_ALERT_SMTPNAME == propName)
		{
			return setSmtpName(value.toString());
		}
		else
		{
			Logger(LOG_WARNING) << tr("Missing property for alert slot value set: '%1'")
				.arg(propName);
			return false;
		}
	}
	else
	{
		if (!m_alertSlotList.contains(itemName))
		{
			Logger(LOG_WARNING) << tr("Missing alert slot item value set: '%1:%2'")
				.arg(itemName)
				.arg(propName);
			return false;
		}
		else
		{
			if (m_alertSlotStringCallMap.find(propName) != m_alertSlotStringCallMap.end())
			{
				if (!value.canConvert(QVariant::String))
				{
					Logger(LOG_WARNING) << tr("Variant for alert slot value set is not string: %1").arg(propName);
					return false;
				}
				return std::get<1>(m_alertSlotStringCallMap.at(propName))(m_alertSlotList[itemName].data(), value.toString());
			}
			else if (m_alertSlotBoolCallMap.find(propName) != m_alertSlotBoolCallMap.end())
			{
				if (!value.canConvert(QVariant::Bool))
				{
					Logger(LOG_WARNING) << tr("Variant for alert slot value set is not bool: %1").arg(propName);
					return false;
				}
				return std::get<1>(m_alertSlotBoolCallMap.at(propName))(m_alertSlotList[itemName].data(), value.toBool());
			}
			else
			{
				Logger(LOG_WARNING) << tr("Missing property for alert slot value set: '%1'").arg(propName);
				return false;
			}
		}
	}
}


void AlertManager::generateAlert(const QString& text)
{
	Logger(LOG_WARNING) << tr("ALERT: ") << text;

	emit sendStatus(tr("ALERT GENERATED"));

	// Reset global idle time
	m_settings->resetIdle();

	m_activeAlerts++;
	m_alertList.append(QSharedPointer<Alert>::create(text, QDateTime::currentDateTime()));
	emit valueChanged(GROUP_ALERT, "", PROP_ALERT_ALERTCOUNT, QVariant(m_activeAlerts));

	// Write alert to a log file
	QString logFilename = m_settings->dataDir() + FILENAME_ALERTLOG;
	QFile alertLog(logFilename);
	if (!alertLog.open(QFile::WriteOnly | QFile::Append))
	{
		Logger(LOG_ERROR) << tr("Error '%1' opening alert log file for write: '%2'")
			.arg(alertLog.errorString())
			.arg(logFilename);
	}
	else
	{
		QString header = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss ddd: ");
		alertLog.write(header.toLocal8Bit());
		alertLog.write(text.toLocal8Bit());
		alertLog.write(QByteArray("\r\n"));
		alertLog.close();
	}

	QString fullText = QString("[%1] %2").arg(QHostInfo::localHostName()).arg(text);

	for (const auto& alertSlot : m_alertSlotList)
	{
		if (alertSlot->getEnabled())
		{
			QString type = alertSlot->getType();

			if (ALERTSLOT_TYPE_SMPTEMAIL == type)
			{
				QString destEmail = alertSlot->getArguments();
				if (destEmail.isEmpty())
				{
					Logger(LOG_ERROR) << tr("Alert %1: Empty destination email argument").arg(alertSlot->getName());
				}
				else
				{
					sendSmtpEmailAlert(fullText, destEmail);
				}
			}
			else if (ALERTSLOT_TYPE_HTTPGET == type)
			{
				QString url = alertSlot->getArguments();
				if (url.isEmpty())
				{
					Logger(LOG_ERROR) << tr("Alert %1: Empty URL argument").arg(alertSlot->getName());
				}
				else
				{
					sendHttpGetAlert(fullText, url);
				}
			}
			else if (ALERTSLOT_TYPE_HTTPPOST == type)
			{
				QString url = alertSlot->getArguments();
				if (url.isEmpty())
				{
					Logger(LOG_ERROR) << tr("Alert %1: Empty URL argument").arg(alertSlot->getName());
				}
				else
				{
					sendHttpPostAlert(fullText, url);
				}
			}
			else if (ALERTSLOT_TYPE_SLACK == type)
			{
				QString url = alertSlot->getArguments();
				if (url.isEmpty())
				{
					Logger(LOG_ERROR) << tr("Alert %1: Empty Webhook URL").arg(alertSlot->getName());
				}
				else
				{
					sendSlackAlert(fullText, url);
				}
			}
			else if (ALERTSLOT_TYPE_EXTERNAL == type)
			{
				QString commandLine = alertSlot->getArguments();
				if (commandLine.isEmpty())
				{
					Logger(LOG_ERROR) << tr("Alert %1: Empty command line").arg(alertSlot->getName());
				}
				else
				{
					executeExternalAlert(fullText, commandLine);
				}
			}
		}
	}
}


bool AlertManager::resetActiveAlertCount()
{
	m_activeAlerts = 0;
	emit valueChanged(GROUP_ALERT, "", PROP_ALERT_ALERTCOUNT, QVariant(m_activeAlerts));
	return true;
}


// Called by AlertSlot objects when a value changes
void AlertManager::alertSlotValueChanged(const QString& property, const QVariant& value) const
{
	QString alertSlotName = sender()->property(PROP_ALERT_SLOTNAME).toString();
	emit valueChanged(GROUP_ALERT, alertSlotName, property, value);
}


void AlertManager::sendSmtpEmailAlert(const QString& text, const QString& arg) const
{
	Logger() << tr("Sending alert smtp email to '%1'").arg(arg);
	Smtp* smtp = new Smtp(m_smtpUser, m_smtpPass, m_smtpServer, m_smtpSSL, m_smtpTLS, m_smtpPort);
	connect(smtp, &Smtp::status,
		this, [smtp, arg](QString status)
	{
		if (status.isEmpty())
		{
			Logger() << tr("SMTP alert email sent to %1").arg(arg);
		}
		else
		{
			Logger(LOG_ERROR) << tr("SMTP alert email failure: ") << status;
		}

		// object SHOULD delete itself but just in case
		smtp->deleteLater();
	});

	QString subject = tr("Pinhole alert from %1").arg(QHostInfo::localHostName());
	smtp->sendMail(m_smtpEmail, arg, subject, text);
}


void AlertManager::sendHttpGetAlert(const QString& text, const QString& arg) const
{
	// Encode the alert text into the URL
	QUrl url(arg.toLocal8Bit().replace(ALERT_REPLACE_TEXT, QUrl::toPercentEncoding(text)));
	if (!url.isValid())
	{
		Logger(LOG_ERROR) << tr("Invalid HTTP GET URL: '%1'").arg(arg);
		return;
	}

	QNetworkRequest request(url);
	QNetworkReply* reply = m_networkAccessManager->get(request);

	connect(reply, &QNetworkReply::finished,
		this, [reply, arg]()
	{
		if (QNetworkReply::NoError != reply->error())
		{
			Logger(LOG_ERROR) << tr("Error '%1' sending HTTP GET alert to '%2'")
				.arg(reply->errorString())
				.arg(arg);
		}
		else
		{
			Logger() << tr("HTTP GET alert sent to '%1'").arg(arg);
		}

		reply->deleteLater();
	});

}


void AlertManager::sendHttpPostAlert(const QString& text, const QString& arg) const
{
	QUrl url(arg);
	if (!url.isValid())
	{
		Logger(LOG_ERROR) << tr("Invalid HTTP POST URL: '%1'").arg(arg);
		return;
	}
	
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	QNetworkReply* reply = m_networkAccessManager->post(request, text.toLocal8Bit().data());

	connect(reply, &QNetworkReply::finished, 
		this, [reply, arg]()
	{
		if (QNetworkReply::NoError != reply->error())
		{
			Logger(LOG_ERROR) << tr("Error '%1' sending HTTP POST alert to '%2'")
				.arg(reply->errorString())
				.arg(arg);
		}
		else
		{
			Logger() << tr("HTTP POST alert sent to '%1'").arg(arg);
		}

		reply->deleteLater();
	});
}


void AlertManager::sendSlackAlert(const QString& text, const QString& arg) const
{
	QUrl url(arg);
	if (!url.isValid())
	{
		Logger(LOG_ERROR) << tr("Invalid Slack Webhook URL: '%1'").arg(arg);
		return;
	}

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	QJsonObject jsonObject;
	jsonObject["text"] = text;
	QJsonDocument jsonDoc;
	jsonDoc.setObject(jsonObject);
	QNetworkReply* reply = m_networkAccessManager->post(request, jsonDoc.toJson());

	connect(reply, &QNetworkReply::finished,
		this, [reply, arg]()
	{
		if (QNetworkReply::NoError != reply->error())
		{
			Logger(LOG_ERROR) << tr("Error '%1' sending Slack alert to '%2'")
				.arg(reply->errorString())
				.arg(arg);
		}
		else
		{
			Logger() << tr("Slack alert sent to '%1'").arg(arg);
		}

		reply->deleteLater();
	});
}


void AlertManager::executeExternalAlert(const QString& text, const QString& arg) const
{
	// Replace the alert text in the command line
	QString commandLine(arg); 
	QString escapedText(text);
	escapedText.replace("\"", "\"\"\"");	// Replace quotes with tripple quotes
	commandLine.replace(QString(ALERT_REPLACE_TEXT), escapedText);

	if (!QProcess::startDetached(commandLine))
	{
		Logger(LOG_ERROR) << tr("Error executing exeternal command '%1'").arg(arg);
	}
	else
	{
		Logger() << tr("Executed external alert command '%1'").arg(arg);
	}
}


QByteArray AlertManager::retrieveAlertList() const
{
	// Reads the alert log
	QString logFilename = m_settings->dataDir() + FILENAME_ALERTLOG;
	QFile alertLog(logFilename);
	if (!alertLog.exists())
	{
		return tr("(No alerts logged)").toUtf8();
	}
	else if (!alertLog.open(QFile::ReadOnly))
	{
		Logger(LOG_ERROR) << tr("Error '%1' opening alert log file for read: '%2'")
			.arg(alertLog.errorString())
			.arg(logFilename);
		return tr("(An error occured reading the alert list)").toUtf8();
	}

	return alertLog.readAll();
}

