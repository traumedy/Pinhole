#include "IdWindow.h"
#include "../common/Utilities.h"

#include <QScreen>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#define CLOSEINTERVAL	15000
#define BORDERWIDTH		20


IdWindow::IdWindow(QScreen* screen, int id)
	: m_screen(screen), m_id(id), m_renderArea(screen, id, this)
{
	setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
	setParent(0); // Create TopLevel-Widget
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground, true);
#ifdef Q_OS_MAC
	setWindowFlag(Qt::Tool);
#else
	setWindowFlag(Qt::WindowStaysOnTopHint);
	setWindowState(Qt::WindowFullScreen);
#endif

	QTimer::singleShot(CLOSEINTERVAL, 
		this, &QDialog::close);

	resize(screen->size());
	move(screen->geometry().topLeft());

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setMargin(0);
	setLayout(gridLayout);
	gridLayout->addWidget(&m_renderArea);

	show();
	raise();
}

IdWindow::~IdWindow()
{
}


void IdWindow::showIdWindows()
{
	QList<QScreen*> sysScreens = QGuiApplication::screens();

	int id = 0;
	for (const auto& screen : sysScreens)
	{
		new IdWindow(screen, id++);
	}
}


RenderArea::RenderArea(QScreen* screen, int id, QWidget *parent)
	: QWidget(parent), m_screen(screen), m_id(id)
{
	setBackgroundRole(QPalette::Background);
	setAutoFillBackground(false);

	QColor colors[] = { Qt::red, Qt::green, Qt::blue, Qt::magenta, Qt::cyan, Qt::yellow, Qt::darkCyan };
	m_color = colors[id % countof(colors)];
}


void RenderArea::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);

	// Draw border
	QPen pen(m_color);
	pen.setWidth(BORDERWIDTH);
	pen.setCapStyle(Qt::SquareCap);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(BORDERWIDTH / 2, BORDERWIDTH / 2, width() - BORDERWIDTH, height() - BORDERWIDTH);
	painter.setPen(Qt::white);
	painter.drawRect(0, 0, width() - 1, height() - 1);

	// Draw text
	QStringList text;
	text.append(QHostInfo::localHostName() + " " + m_screen->name());
	QRect screenRect = m_screen->geometry();
	qreal ratio = m_screen->devicePixelRatio();
	text.append(QString::number(screenRect.left() * ratio) + "x" + 
		QString::number(screenRect.top() * ratio) + " ("
		+ QString::number(screenRect.width() * ratio) + "x" + 
		QString::number(screenRect.height() * ratio) + ")");

	QString interfacesString;
	QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
	for (const auto& iface : interfaces)
	{
		auto flags = iface.flags();
		
		if (flags &
			(QNetworkInterface::IsUp |
				QNetworkInterface::IsRunning)
			&& !flags.testFlag(QNetworkInterface::IsLoopBack))
		{
			QList<QNetworkAddressEntry> addresses = iface.addressEntries();
			for (const auto& address : addresses)
			{
				if (!address.ip().isLoopback()/* && !address.ip().isLinkLocal()*/)
				{
					text.append(HostAddressToString(address.ip()));
				}
			}
		}
	}

	QFont font = painter.font();
	// Calculate font size based on screen hieight, this should give 48 for 1080
	font.setPixelSize(height() / 22);

	painter.setPen(Qt::white);
	painter.setBrush(Qt::black);
	QFontMetrics fm(font);

	int textHeight = text.length() * fm.height();
	int fontHeight = fm.height();
	int yPos = (height() - textHeight) / 2;
	if (yPos < fontHeight)
		yPos = fontHeight;

	QPainterPath painterPath;
	for (const auto & str : text)
	{
		int xPos = (width() - fm.horizontalAdvance(str)) / 2;
		painterPath.addText(static_cast<qreal>(xPos), static_cast<qreal>(yPos), font, str);
		yPos += fontHeight;
		painter.setBrush(Qt::white);
		pen.setColor(Qt::black);
		pen.setWidth(2);
		painter.setPen(pen);
		painter.drawPath(painterPath);
	}
}


void RenderArea::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);

	close();
}


