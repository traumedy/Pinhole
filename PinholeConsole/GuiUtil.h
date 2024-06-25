#pragma once

#include <QString>
#include <QMap>
#include <QStandardPaths>

class QWidget;

// Returns a string with HTML text for a link to a local file
QString LocalHtmlDoc(const QString& filePath, const QString& description);

// Clears a QWidget to default state
bool ClearWidget(QWidget* widget);

// Enable the widgets in a map
void EnableWidgetMap(const QMap<QString, QWidget*>& map, bool enable);

// Returns a directory for saving files
QString GetSaveDirectory(const QString& lastDirectory, QList<QStandardPaths::StandardLocation> locations);