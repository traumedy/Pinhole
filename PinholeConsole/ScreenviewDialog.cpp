#include "ScreenviewDialog.h"
#include "AspectRatioLabel.h"
#include "WindowManager.h"
#include "GuiUtil.h"
#include "../common/Utilities.h"

#include <QGuiApplication>
#include <QScreen>
#include <QMenu>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QStandardPaths>
#include <QClipboard>
#include <QScrollArea>
#include <QScrollBar>
#include <QMenuBar>
#include <QGuiApplication>


ScreenviewDialog::ScreenviewDialog(const QByteArray& imageArray, const QString& clientName,
	const QString& clientAddress, QWidget *parent)
	: QDialog(parent), m_clientName(clientName), m_clientAddress(clientAddress)
{
	setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
	setAttribute(Qt::WA_DeleteOnClose, true);

	m_creationDateTime = QDateTime::currentDateTime();
	m_imageLabel = new AspectRatioLabel;
	m_scrollArea = new QScrollArea; 

	setWindowFlag(Qt::WindowMaximizeButtonHint, true);

	m_imageLabel->setBackgroundRole(QPalette::Base);
	m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	m_imageLabel->setScaledContents(true);

	m_scrollArea->setBackgroundRole(QPalette::Dark);
	m_scrollArea->setWidget(m_imageLabel);

	// Decode the image from PNG and load it into the QLabel
	m_pixmap.loadFromData(imageArray, "PNG");
	m_pixmap.setDevicePixelRatio(QGuiApplication::primaryScreen()->devicePixelRatio());
	m_imageLabel->setPixmap(m_pixmap);
	m_imageLabel->adjustSize();

	setWindowTitle(tr("Screenshot %1x%2 of %3 (%4) at %5")
		.arg(m_pixmap.width())
		.arg(m_pixmap.height())
		.arg(m_clientName)
		.arg(m_clientAddress)
		.arg(m_creationDateTime.toString()));

	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->addWidget(m_scrollArea);
	layout()->setMargin(0);

	m_menuBar = new QMenuBar(this);
	createActions();
	layout()->setMenuBar(m_menuBar);

	resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);

	// Start in 'fit to window'
	m_fitToWindowAct->setChecked(true);
	fitToWindow();

	WindowManager::addWindow(this);
	WindowManager::cascadeWindow(this);
}


ScreenviewDialog::~ScreenviewDialog()
{
	WindowManager::removeWindow(this);
}


QString g_lastDirectoryChosen;

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
	dialog.setDirectory(GetSaveDirectory(g_lastDirectoryChosen,
		{ QStandardPaths::PicturesLocation, QStandardPaths::DesktopLocation }));

	QStringList mimeTypeFilters;
	const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
		? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
	foreach(const QByteArray &mimeTypeName, supportedMimeTypes)
		mimeTypeFilters.append(mimeTypeName);
	mimeTypeFilters.sort();
	dialog.setMimeTypeFilters(mimeTypeFilters);
	dialog.selectMimeTypeFilter("image/png");
	if (acceptMode == QFileDialog::AcceptSave)
		dialog.setDefaultSuffix("png");
}


void ScreenviewDialog::createActions()
{
	QMenu *fileMenu = m_menuBar->addMenu(tr("&File"));
	fileMenu->addAction(tr("&Save As..."), this, &ScreenviewDialog::saveAs)->setShortcut(QKeySequence::Save);
	fileMenu->addAction(tr("&Close"), this, &ScreenviewDialog::close)->setShortcut(QKeySequence::Close);

	QMenu *editMenu = m_menuBar->addMenu(tr("&Edit"));
	editMenu->addAction(tr("&Copy"), this, &ScreenviewDialog::copy);

	QMenu *viewMenu = m_menuBar->addMenu(tr("&View"));

	m_zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ScreenviewDialog::zoomIn);
	m_zoomInAct->setShortcut(QKeySequence::ZoomIn);

	m_zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ScreenviewDialog::zoomOut);
	m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);

	m_normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &ScreenviewDialog::normalSize);
	m_normalSizeAct->setShortcut(tr("Ctrl+S"));

	viewMenu->addSeparator();

	m_fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ScreenviewDialog::fitToWindow);
	m_fitToWindowAct->setCheckable(true);
	m_fitToWindowAct->setShortcut(tr("Ctrl+F"));
}


void ScreenviewDialog::saveAs()
{
	QFileDialog dialog(this, tr("Save Screenshot As"));
	initializeImageFileDialog(dialog, QFileDialog::AcceptSave);
	dialog.setAcceptMode(QFileDialog::AcceptSave);

	QString defaultFilename = tr("Screenshot-%1-%2-%3.png")
		.arg(m_clientName).
		arg(m_clientAddress).
		arg(m_creationDateTime.toString("yyyy-MM-dd_hh.mm.ss"));
	dialog.selectFile(FilenameString(defaultFilename));

	int result;
	while ((result = dialog.exec()) == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) 
	{
		g_lastDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
		dialog.setDirectory(g_lastDirectoryChosen);
	}

	if (QDialog::Accepted == result)
	{
		g_lastDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
	}
}


bool ScreenviewDialog::saveFile(const QString &fileName)
{
	QImageWriter writer(fileName);

	if (!writer.write(m_pixmap.toImage())) {
		QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
			tr("Cannot write %1: %2")
			.arg(QDir::toNativeSeparators(fileName))
			.arg(writer.errorString()));
		return false;
	}
	return true;
}


void ScreenviewDialog::copy()
{
#ifndef QT_NO_CLIPBOARD
	QGuiApplication::clipboard()->setImage(m_pixmap.toImage());
#endif // !QT_NO_CLIPBOARD
}


void ScreenviewDialog::zoomIn()
{
	scaleImage(1.25);
}


void ScreenviewDialog::zoomOut()
{
	scaleImage(0.8);
}


void ScreenviewDialog::normalSize()
{
	m_imageLabel->adjustSize();
	m_scaleFactor = 1.0;
}


void ScreenviewDialog::fitToWindow()
{
	bool fitToWindow = m_fitToWindowAct->isChecked();
	m_scrollArea->setWidgetResizable(fitToWindow);
	if (!fitToWindow)
		normalSize();
	updateActions();
}


void ScreenviewDialog::scaleImage(double factor)
{
	m_scaleFactor *= factor;
	m_imageLabel->resize(m_scaleFactor * m_imageLabel->pixmap()->size());

	adjustScrollBar(m_scrollArea->horizontalScrollBar(), factor);
	adjustScrollBar(m_scrollArea->verticalScrollBar(), factor);

	m_zoomInAct->setEnabled(m_scaleFactor < 3.0);
	m_zoomOutAct->setEnabled(m_scaleFactor > 0.333);
}


void ScreenviewDialog::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
	scrollBar->setValue(int(factor * scrollBar->value()
		+ ((factor - 1) * scrollBar->pageStep() / 2)));
}


void ScreenviewDialog::updateActions()
{
	m_zoomInAct->setEnabled(!m_fitToWindowAct->isChecked());
	m_zoomOutAct->setEnabled(!m_fitToWindowAct->isChecked());
	m_normalSizeAct->setEnabled(!m_fitToWindowAct->isChecked());
}


void ScreenviewDialog::changeEvent(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::WindowStateChange:
		// Override minimize
		if (windowState() & Qt::WindowMinimized)
		{
			windowState().setFlag(Qt::WindowMinimized, false);
			hide();
		}
		break;
	
	default:
		// Avoid warnings
		break;
	}
}

