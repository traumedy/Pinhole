#pragma once

#include <QWidget>
#include <QDialog>

class RenderArea : public QWidget
{
	Q_OBJECT

public:
	RenderArea(QScreen* screen, int id, QWidget *parent = 0);

	//QSize minimumSizeHint() const override;
	//QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent* event) override;

private:
	QScreen * m_screen = nullptr;
	int m_id;
	QColor m_color;
};


class IdWindow : public QDialog
{
	Q_OBJECT

public:
	IdWindow(QScreen* screen, int id);
	~IdWindow();
	static void showIdWindows();


private:
	QScreen * m_screen = nullptr;
	int m_id;
	RenderArea m_renderArea;
};

