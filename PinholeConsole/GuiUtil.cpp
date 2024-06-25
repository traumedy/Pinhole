#include "GuiUtil.h"
#include "Global.h"
#include "../common/Utilities.h"

#include <QUrl>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QListWidget>
#include <QStandardPaths>
#include <QDir>


QString LocalHtmlDoc(const QString& filePath, const QString& description)
{
	QUrl url = QUrl::fromLocalFile(ApplicationBaseDir() + filePath);
	return "<p><a href=\"" + url.toString() + "\">" + description + "</a></p>";
}


bool ClearWidget(QWidget* widget)
{
	if (nullptr != qobject_cast<QLabel*>(widget))
	{
		qobject_cast<QLabel*>(widget)->clear();
	}
	else if (nullptr != qobject_cast<QCheckBox*>(widget))
	{
		qobject_cast<QCheckBox*>(widget)->setChecked(false);
	}
	else if (nullptr != qobject_cast<QLineEdit*>(widget))
	{
		qobject_cast<QLineEdit*>(widget)->clear();
	}
	else if (nullptr != qobject_cast<QComboBox*>(widget))
	{
		qobject_cast<QComboBox*>(widget)->setCurrentIndex(-1);
	}
	else if (nullptr != qobject_cast<QSpinBox*>(widget))
	{
		qobject_cast<QSpinBox*>(widget)->clear();
	}
	else if (nullptr != qobject_cast<QListWidget*>(widget))
	{
		qobject_cast<QListWidget*>(widget)->clear();
	}
	else
	{
		return false;
	}

	return true;
}


void EnableWidgetMap(const QMap<QString, QWidget*>& map, bool enable)
{
	for (const auto& widget : map)
	{
		bool enableOverride = false;
		if (enable)
		{
			// Some widgets may have been marked as unsupported by a host
			QVariant supported = widget->property(PROPERTY_SUPPORTED);
			if (supported.isValid() && supported.toBool() == false)
				enableOverride = true;
		}
		widget->setEnabled(enable && !enableOverride);
	}
}


QString GetSaveDirectory(const QString& lastDirectory, QList<QStandardPaths::StandardLocation> locations)
{
	if (!lastDirectory.isEmpty())
		return lastDirectory;

	for (const auto& locId : locations)
	{
		const QStringList locList = QStandardPaths::standardLocations(locId);
		if (!locList.isEmpty())
			return locList.first();
	}

	return QDir::homePath();
}

