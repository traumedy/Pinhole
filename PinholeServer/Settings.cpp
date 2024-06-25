#include "Settings.h"
#include "../common/PinholeCommon.h"

#include <QSettings>

QSharedPointer<QSettings> Settings::getScopedSettings() const
{
	QSharedPointer<QSettings> settings;

	if (m_runningAsService)
		settings = QSharedPointer<QSettings>::create(QSettings::SystemScope, PINHOLE_ORG_NAME, PINHOLE_SERVERAPPNAME);
	else
		settings = QSharedPointer<QSettings>::create(QSettings::UserScope, PINHOLE_ORG_NAME, PINHOLE_SERVERAPPNAME);

	settings->setFallbacksEnabled(false);
	return settings;
}

