#pragma once

#include <QListWidget>

class ListWidgetEx : public QListWidget
{
	Q_OBJECT

public:
	ListWidgetEx(const QString& objectName, const QString& defaultItem, QWidget *parent = nullptr);
	~ListWidgetEx();
	QStringList getItemsText() const;

signals:
	void valueChanged();

private slots:
	void customContextMenuRequested(const QPoint&);

private:
	QString m_objectName;
	QString m_defaultItem;
};
