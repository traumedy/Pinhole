#pragma once

#include <QWidget>
#include <QMap>


class HostConfigWidgetTab : public QWidget
{
public:
	HostConfigWidgetTab(QWidget* parent) : QWidget(parent) {  }
	void setEditable(bool editable) { m_editable = editable; }
	virtual void enableWidgets(bool enable) = 0;
	virtual void resetWidgets() = 0;

	QMap<QString, QWidget*> m_widgetMap;

protected:
	bool m_editable = false;
};