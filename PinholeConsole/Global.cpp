#include "Global.h"

#include <QWidget>
#include <QString>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QGuiApplication>


bool WriteFile(QWidget* parent, const QString& fileName, const QByteArray& data)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(parent, QGuiApplication::applicationDisplayName(),
			QObject::tr("Cannot create file %1: %2")
			.arg(QDir::toNativeSeparators(fileName))
			.arg(file.errorString()));
		return false;
	}

	if (file.write(data) != data.length())
	{
		QMessageBox::warning(parent, QGuiApplication::applicationDisplayName(),
			QObject::tr("Cannot write file %1: %2")
			.arg(QDir::toNativeSeparators(fileName))
			.arg(file.errorString()));
		return false;
	}

	file.close();
	return true;
}


unsigned int NoticeId()
{
	static unsigned int ret = 0;
	return ++ret;
}
