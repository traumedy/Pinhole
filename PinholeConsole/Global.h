#pragma once

#include <QScrollArea>
#include <QStyle>

#define MARGINSIZE				10
#define ITEMLISTMAXHEIGHT		100
#define BUTTONITEMWIDTH			150
#define MINIMUM_DETAILS_WIDTH	400
#define MAINWINDOW_WIDTH		675
#define WAKEONLANPORT			9

#define FREQ_HOSTEXPIRECHECK	2000				// The frequency that hosts' 'last heard' value are checked to 
#define FREQ_LOOPBACKQUERY		1025				// The frequency that local loopback addresses are queried
#define FREQ_HOSTBROADCAST		5025				// The frequency that queries are broadcast at
#define FREQ_HOSTQUERY			3025				// The frequency the host list is queried at
#define FREQ_NOTICEUPDATE		10000				// How soon the notice text is reset after a non-id message

#define SECS_MINHIGHLIGHTHOST	60

#define PROPERTY_GROUPNAME		"groupName"			// The Pinhole group associated with a widget
#define PROPERTY_ITEMNAME		"itemName"			// The Pinhole item associated with a widget
#define PROPERTY_PROPNAME		"propertyName"		// The Pinhole property associated with a widget
#define PROPERTY_BUTTONSTATE	"buttonState"		// Bool button state
#define PROPERTY_SUPPORTED		"propSupported"		// False if a Pinhole property associated with a widget is not supported by the currrent host

#define DOC_PINHOLE				"/doc/Pinhole.html"
#define DOC_ENVIRONMENT			"/doc/Environment.html"
#define DOC_NOVA				"/doc/Nova.html"
#define DOC_SETTINGS			"/doc/Settings.html"
#define DOC_LOGGING				"/doc/Logging.html"
#define DOC_TEXTVIEWER			"/doc/TextViewer.html"
#define DOC_HTTPSERVER			"/doc/HttpServer.html"
#define DOC_LOOPBACK			"/doc/Loopback.html"
#define DOC_RESOURCE			"/doc/Resource.html"
#define DOC_ALERTS				"/doc/Alerts.html"
#define DOC_CUSTOMACTIONS		"/doc/CustomActions.html"

#define COL_ADDRESS		0
#define COL_NAME		1
#define	COL_ROLE		2
#define COL_VERSION		3
#define COL_PLATFORM	4
#define COL_STATUS		5
#define COL_LASTHEARD	6
#define COL_OS			7
#define COL_MAC			8
#define COL_LAST		COL_MAC

#define HOSTROLE_ID			Qt::UserRole + 0
#define HOSTROLE_ADDRESS	Qt::UserRole + 1
#define HOSTROLE_PORT		Qt::UserRole + 2

class ScrollAreaEx : public QScrollArea
{
public:
	ScrollAreaEx(QWidget* parent = nullptr) : QScrollArea(parent)
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

	virtual QSize sizeHint() const override
	{
		auto widgetSize = widget()->sizeHint();
		auto scrollbarSize = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		return QSize(widgetSize.width() + scrollbarSize + 2, widgetSize.height() + scrollbarSize + 2);
	}
};

bool WriteFile(QWidget* parent, const QString& fileName, const QByteArray& data);

// Returns a unique ID
unsigned int NoticeId();