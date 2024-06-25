#pragma once

#include <QPixmap>
#include <QDateTime>
#include <QDialog>

class QScrollBar;
class QScrollArea;
class QLabel;
class QMenuBar;
class AspectRatioLabel;

class ScreenviewDialog : public QDialog
{
	Q_OBJECT

public:
	ScreenviewDialog(const QByteArray& imageArray, const QString& clientname,
		const QString& clientAddress, QWidget *parent = Q_NULLPTR);
	~ScreenviewDialog();

private slots:
	void saveAs();
	void copy();
	void zoomIn();
	void zoomOut();
	void normalSize();
	void fitToWindow();

private:
	void changeEvent(QEvent* event) override;
	void createActions();
	bool saveFile(const QString &fileName);
	void scaleImage(double factor);
	void adjustScrollBar(QScrollBar *scrollBar, double factor);
	void updateActions();

	QDateTime m_creationDateTime;
	QString m_clientName;
	QString m_clientAddress;
	QPixmap m_pixmap;
	AspectRatioLabel* m_imageLabel = nullptr;
	QScrollArea* m_scrollArea = nullptr;
	double m_scaleFactor = 1.0;

	QMenuBar* m_menuBar = nullptr;
	QAction* m_zoomInAct = nullptr;
	QAction* m_zoomOutAct = nullptr;
	QAction* m_normalSizeAct = nullptr;
	QAction* m_fitToWindowAct = nullptr;
};
