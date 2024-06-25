#include "TextViewerDialog.h"
#include "FindTextDialog.h"
#include "Global.h"
#include "WindowManager.h"
#include "GuiUtil.h"
#include "../common/Utilities.h"

#include <QMenuBar>
#include <QMenu>
#include <QPlainTextEdit>
#include <QGridLayout>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QGuiApplication>
#include <QInputDialog>
#include <QShortcut>
#include <QDesktopServices>
#include <QSettings>

#define VALUE_LASTDIRCHOSEN		"lastTextViewerDirectoryChosen"


TextViewerDialog::TextViewerDialog(const QString& description, const QString& hostAddress, 
	const QString& hostName, const QByteArray& compressedText, QWidget *parent)
	: QDialog(parent), m_description(description), m_hostAddress(hostAddress), 
	m_hostName(hostName), m_text(qUncompress(compressedText))
{
	QSettings settings;
	g_lastDirectoryChosen = settings.value(VALUE_LASTDIRCHOSEN).toString();

	setWindowFlag(Qt::WindowContextHelpButtonHint, false);
	setWindowFlag(Qt::WindowMinMaxButtonsHint, true);
	setAttribute(Qt::WA_DeleteOnClose, true);

	setWindowTitle(tr("%1 from %2 (%3)")
		.arg(description)
		.arg(hostName)
		.arg(hostAddress));

	QGridLayout* mainLayout = new QGridLayout(this);
	mainLayout->setMargin(0);

	m_textEdit = new QPlainTextEdit(QString::fromUtf8(m_text));
	m_textEdit->setReadOnly(true);
	m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
	mainLayout->addWidget(m_textEdit);

	m_findDialog = new FindTextDialog(this);
	connect(m_findDialog, &FindTextDialog::findClicked,
		this, &TextViewerDialog::findText);

	QMenuBar* menuBar = new QMenuBar(this);

	QMenu* fileMenu = menuBar->addMenu(tr("&File"));
	fileMenu->addAction(tr("&Save As..."), this, 
		&TextViewerDialog::saveAction_triggered)->setShortcut(QKeySequence::Save);
	fileMenu->addAction(tr("&Close"), this, 
		&TextViewerDialog::close)->setShortcut(QKeySequence::Close);

	QMenu* editMenu = menuBar->addMenu(tr("&Edit"));
	editMenu->addAction(tr("&Copy"), m_textEdit, 
		&QPlainTextEdit::copy)->setShortcut(QKeySequence::Copy);
	editMenu->addAction(tr("&Find"), m_findDialog, 
		&FindTextDialog::show)->setShortcut(QKeySequence::Find);
	editMenu->addAction(tr("Find &Next"), this, 
		&TextViewerDialog::findText)->setShortcut(QKeySequence::FindNext);
	editMenu->addAction(tr("Select &All"), m_textEdit, 
		&QPlainTextEdit::selectAll)->setShortcut(QKeySequence::SelectAll);

	QMenu* viewMenu = menuBar->addMenu(tr("&View"));
	QAction* lineWrapAction = viewMenu->addAction(tr("&Wrap"));
	lineWrapAction->setShortcut(QKeySequence(tr("Ctrl+W")));
	lineWrapAction->setCheckable(true);
	lineWrapAction->setChecked(false);
	connect(lineWrapAction, &QAction::triggered,
		this, [this](bool checked)
	{
		m_textEdit->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
	});
	viewMenu->addAction(tr("Zoom &In"), [this] 
		{ m_textEdit->zoomIn(); })->setShortcut(QKeySequence::ZoomIn);
	viewMenu->addAction(tr("Zoom &Out"), [this] 
		{ m_textEdit->zoomOut(); })->setShortcut(QKeySequence::ZoomOut);

	QMenu* helpMenu = menuBar->addMenu(tr("&Help"));
	helpMenu->addAction(tr("&Keyboard help"), [this]
	{
		QUrl helpUrl = QUrl::fromLocalFile(ApplicationBaseDir() + DOC_TEXTVIEWER);
		if (!QDesktopServices::openUrl(helpUrl))
		{
			QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
				tr("Failed to open URL: %1")
				.arg(helpUrl.toString()));
		}
	})->setShortcut(QKeySequence::HelpContents);

	layout()->setMenuBar(menuBar);

	// Set font to mono spaced
	QFont font("monospace");
	font.setStyleHint(QFont::Monospace);
	m_textEdit->setFont(font);

	WindowManager::addWindow(this);
	WindowManager::cascadeWindow(this);
}


TextViewerDialog::~TextViewerDialog()
{
	WindowManager::removeWindow(this);
}


void TextViewerDialog::saveAction_triggered()
{
	QFileDialog dialog(this, tr("Save %1 file from %2 As")
		.arg(m_description)
		.arg(m_hostAddress));
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDirectory(GetSaveDirectory(g_lastDirectoryChosen,
		{ QStandardPaths::DocumentsLocation, QStandardPaths::DesktopLocation }));
	dialog.setNameFilters({ "Text (*.txt)", "Any files (*)" });
	dialog.setDefaultSuffix("txt");

	QString defaultFilename = tr("Pinhole%1-%2.txt")
		.arg(m_description)
		.arg(m_hostName);
	dialog.selectFile(FilenameString(defaultFilename));

	int result;
	while ((result = dialog.exec()) == QDialog::Accepted && 
		!WriteFile(this, dialog.selectedFiles().first(), m_text)) 
	{
		g_lastDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
		dialog.setDirectory(g_lastDirectoryChosen);
	}

	if (QDialog::Accepted == result)
	{
		g_lastDirectoryChosen = QFileInfo(dialog.selectedFiles().first()).path();
		QSettings settings;
		settings.setValue(VALUE_LASTDIRCHOSEN, g_lastDirectoryChosen);
	}
}


void TextViewerDialog::findText()
{
	QString text = m_findDialog->getText();
	if (text.isEmpty())
		return;

	if (m_findDialog->getRegularExpression())
	{
		QRegExp regExp(text,
			m_findDialog->getCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive);
		QTextDocument::FindFlags findFlags;
		if (m_findDialog->getReverseSearch())
			findFlags.setFlag(QTextDocument::FindBackward);
		if (m_findDialog->getWholeWord())
			findFlags.setFlag(QTextDocument::FindWholeWords);
		if (!m_textEdit->find(regExp, findFlags))
		{
			QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
				tr("Regular expression not found"));
		}
	}
	else
	{
		QTextDocument::FindFlags findFlags;
		if (m_findDialog->getCaseSensitive())
			findFlags.setFlag(QTextDocument::FindCaseSensitively);
		if (m_findDialog->getReverseSearch())
			findFlags.setFlag(QTextDocument::FindBackward);
		if (m_findDialog->getWholeWord())
			findFlags.setFlag(QTextDocument::FindWholeWords);
		if (!m_textEdit->find(text, findFlags))
		{
			QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
				tr("Text not found"));
		}
	}
}


void TextViewerDialog::changeEvent(QEvent* event)
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

