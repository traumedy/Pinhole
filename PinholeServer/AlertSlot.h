#pragma once
#include "../common/PinholeCommon.h"

#include <QObject>

class AlertSlot : public QObject
{
	Q_OBJECT

public:
	AlertSlot(const QString& name, QObject *parent = nullptr);
	~AlertSlot();

	QString getName() const;
	void setName(const QString& name);
	bool getEnabled() const;
	bool setEnabled(bool b);
	QString getType() const;
	bool setType(const QString& str);
	QString getArguments() const;
	bool setArguments(const QString& str);

signals:
	void valueChanged(const QString&, const QVariant&);

private:
	const QStringList m_validAlertSlotTypes = 
	{
		ALERTSLOT_TYPE_SMPTEMAIL,
		ALERTSLOT_TYPE_HTTPGET,
		ALERTSLOT_TYPE_HTTPPOST,
		ALERTSLOT_TYPE_SLACK,
		ALERTSLOT_TYPE_EXTERNAL
	};

	QString m_name = "";
	bool m_enabled = false;
	QString m_type = ALERTSLOT_TYPE_SMPTEMAIL;
	QString m_arguments = "";

};
