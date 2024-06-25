#pragma once

#include <QObject>

class Settings : public QObject
{
	Q_OBJECT

public:
	QString dataDir() const { return m_dataDir; }
	void setDataDir(const QString& dir) { m_dataDir = dir; }

private:

	QString m_dataDir;
};
