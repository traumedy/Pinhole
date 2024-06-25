#pragma once

#include <QObject>

class Group : public QObject
{
	Q_OBJECT

public:
	Group(QString name, QObject *parent = nullptr);
	~Group();

	QString getName() const;
	void setName(const QString& str);
	QStringList getApplications() const;
	bool setApplications(const QStringList& list);
	bool getLaunchAtStart() const;
	bool setLaunchAtStart(bool set);

signals:
	void valueChanged(const QString&, const QVariant&);

private:
	QString m_name = "";
	QStringList m_applications;
	bool m_launchAtStart = false;
};
