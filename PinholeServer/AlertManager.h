#pragma once

#include "AlertSlot.h"
#include "../common/PinholeCommon.h"

#include <QObject>
#include <QDateTime>
#include <QMap>

#include <functional>

class QNetworkAccessManager;
class Settings;

class Alert
{
public:
	Alert(const QString& text, const QDateTime& time) 
		: m_text(text), m_time(time)
	{
	}

	QString GetText() const
	{
		return m_text;
	}

	QDateTime GetTime() const
	{
		return m_time;
	}

private:
	QString m_text;
	QDateTime m_time;
};



class AlertManager : public QObject
{
	Q_OBJECT

public:
	AlertManager(Settings* settings, QObject *parent = nullptr);
	~AlertManager();

	bool readAlertSettings();
	bool writeAlertSettings() const;
	bool importSettings(const QJsonObject& root);
	QJsonObject exportSettings() const;
	bool addAlertSlot(const QString& name);
	bool deleteAlertSlot(const QString& name);
	bool renameAlertSlot(const QString& oldName, const QString& newName);
	int getAlertCount() const;
	QString getSmtpServer() const;
	bool setSmtpServer(const QString& str);
	int getSmtpPort() const;
	bool setSmtpPort(int val);
	bool getSmtpSSL() const;
	bool setSmtpSSL(bool b);
	bool getSmtpTLS() const;
	bool setSmtpTLS(bool b);
	QString getSmtpUser() const;
	bool setSmtpUser(const QString& str);
	QString getSmtpPass() const;
	bool setSmtpPass(const QString& str);
	QString getSmtpEmail() const;
	bool setSmtpEmail(const QString& str);
	QString getSmtpName() const;
	bool setSmtpName(const QString& str);

	QVariant getAlertVariant(const QString& itemName, const QString& propName) const;
	bool setAlertVariant(const QString& itemName, const QString& propName, const QVariant& value);

	bool resetActiveAlertCount();
	void sendSmtpEmailAlert(const QString& text, const QString& arg) const;
	void sendHttpGetAlert(const QString& text, const QString& arg) const;
	void sendHttpPostAlert(const QString& text, const QString& arg) const;
	void sendSlackAlert(const QString& text, const QString& arg) const;
	void executeExternalAlert(const QString& text, const QString& arg) const;
	QByteArray retrieveAlertList() const;

signals:
	void valueChanged(const QString&, const QString&, const QString&, const QVariant&) const;
	void sendStatus(const QString&);

public slots:
	void alertSlotValueChanged(const QString&, const QVariant&) const;
	void generateAlert(const QString& text);

private:

	const std::map<QString, std::pair<std::function<QString(AlertSlot*)>, std::function<bool(AlertSlot*, const QString&)>>> m_alertSlotStringCallMap =
	{
		{ PROP_ALERT_SLOTTYPE, { &AlertSlot::getType, &AlertSlot::setType } },
		{ PROP_ALERT_SLOTARG, { &AlertSlot::getArguments, &AlertSlot::setArguments } },
	};

	const std::map<QString, std::pair<std::function<bool(AlertSlot*)>, std::function<bool(AlertSlot*, bool)>>> m_alertSlotBoolCallMap =
	{
		{ PROP_ALERT_SLOTENABLED, { &AlertSlot::getEnabled, &AlertSlot::setEnabled } },
	};

	QString m_smtpServer = "";
	int m_smtpPort = DEFAULT_SMTPPORT;
	bool m_smtpSSL = false;
	bool m_smtpTLS = false;
	QString m_smtpUser = DEFAULT_SMTPUSER;
	QString m_smtpPass = "";
	QString m_smtpEmail = "";
	QString m_smtpName = "";

	int m_activeAlerts = 0;
	QList<QSharedPointer<Alert>> m_alertList;
	QMap<QString, QSharedPointer<AlertSlot>> m_alertSlotList;
	QNetworkAccessManager* m_networkAccessManager = nullptr;
	Settings* m_settings = nullptr;
};
