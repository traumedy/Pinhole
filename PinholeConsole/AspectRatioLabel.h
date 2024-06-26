#pragma once

#include <QLabel>

class AspectRatioLabel : public QLabel
{
public:
	explicit AspectRatioLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~AspectRatioLabel();

public slots:
	void setPixmap(const QPixmap& pm);

protected:
	void resizeEvent(QResizeEvent* event) override;

private:
	void updateMargins();

	int m_pixmapWidth = 0;
	int m_pixmapHeight = 0;
};

