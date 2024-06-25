#include "AspectRatioLabel.h"

AspectRatioLabel::AspectRatioLabel(QWidget* parent, Qt::WindowFlags f) : QLabel(parent, f)
{
}

AspectRatioLabel::~AspectRatioLabel()
{
}

void AspectRatioLabel::setPixmap(const QPixmap& pm)
{
	m_pixmapWidth = pm.width();
	m_pixmapHeight = pm.height();

	updateMargins();
	QLabel::setPixmap(pm);
}

void AspectRatioLabel::resizeEvent(QResizeEvent* event)
{
	updateMargins();
	QLabel::resizeEvent(event);
}

void AspectRatioLabel::updateMargins()
{
	if (m_pixmapWidth <= 0 || m_pixmapHeight <= 0)
		return;

	int w = this->width();
	int h = this->height();

	if (w <= 0 || h <= 0)
		return;

	if (w * m_pixmapHeight > h * m_pixmapWidth)
	{
		int m = (w - (m_pixmapWidth * h / m_pixmapHeight)) / 2;
		setContentsMargins(m, 0, m, 0);
	}
	else
	{
		int m = (h - (m_pixmapHeight * w / m_pixmapWidth)) / 2;
		setContentsMargins(0, m, 0, m);
	}
}

