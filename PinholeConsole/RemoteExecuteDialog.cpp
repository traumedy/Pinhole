#include "RemoteExecuteDialog.h"
#include "GuiUtil.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QSettings>

#define VALUE_LASTDIRCHOSEN		"lastRemoteExecDirectoryChosen"

QString RemoteExecuteDialog::sm_lastDirectoryChosen;

RemoteExecuteDialog::RemoteExecuteDialog(QWidget *parent)
	: QDialog(parent)
{
	setWindowFlag(Qt::MSWindowsFixedSizeDialogHint);
	setWindowTitle(tr("Execute command on remote hosts"));

	QFormLayout* mainLayout = new QFormLayout(this);
	mainLayout->setSizeConstraint(QLayout::SetFixedSize);
	m_localFile = new QRadioButton(tr("Upload file"));
	m_filePath = new QLineEdit;
	m_filePath->setMinimumWidth(200);
	connect(m_filePath, &QLineEdit::textChanged,
		this, [this]() { m_localFile->setChecked(true); });
	m_selectFile = new QPushButton(tr("Choose file..."));
	m_selectFile->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	connect(m_selectFile, &QPushButton::clicked,
		this, &RemoteExecuteDialog::selectFile_clicked);
	QHBoxLayout* buttonRow = new QHBoxLayout;
	buttonRow->addWidget(m_filePath);
	buttonRow->addWidget(m_selectFile);
	mainLayout->addRow(m_localFile, buttonRow);
	m_remoteCommand = new QRadioButton(tr("Remote command"));
	m_remoteCommand->setChecked(true);
	m_commandEdit = new QLineEdit;
	connect(m_commandEdit, &QLineEdit::textChanged,
		this, [this]() { m_remoteCommand->setChecked(true); });
	mainLayout->addRow(m_remoteCommand, m_commandEdit);
	m_commandLineEdit = new QLineEdit;
	mainLayout->addRow(new QLabel(tr("Arguments")), m_commandLineEdit);
	m_directoryEdit = new QLineEdit;
	mainLayout->addRow(new QLabel(tr("Working directory")), m_directoryEdit);
	m_captureOutput = new QCheckBox(tr("Capture console output"));
	mainLayout->addRow(nullptr, m_captureOutput);
	m_elevated = new QCheckBox(tr("Execute elevated"));
	mainLayout->addRow(nullptr, m_elevated);
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	mainLayout->addRow(buttons);

	connect(buttons, &QDialogButtonBox::rejected,
		this, &RemoteExecuteDialog::reject);
	connect(buttons, &QDialogButtonBox::accepted,
		this, [this]()
	{
		if (m_localFile->isChecked())
		{
			if (m_filePath->text().isEmpty())
			{
				QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
					tr("Enter an executable path to execute"));
				return;
			}
			QFile file(m_filePath->text());
			if (!file.open(QFile::ReadOnly))
			{
				QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
					tr("Failed to open file:\n") + file.errorString());
				return;
			}
			m_file = file.readAll();
		}
		else
		{
			if (m_commandEdit->text().isEmpty())
			{
				QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
					tr("Enter an remote executable to execute"));
				return;
			}
		}
		done(QDialog::Accepted);
	});

	m_selectFile->setToolTip(tr("Click to open a choose file dialog"));
	m_selectFile->setWhatsThis(tr("This button will open a dialog allowing you to choose "
		"an executable.  The path to that executable will be entered into the file path "
		"field."));
	m_localFile->setToolTip(tr("Upload a file to execute on remote hosts"));
	m_localFile->setWhatsThis(tr("Select this option and enter the path to a file that will be "
		"uploaded and executed on the selected remote hosts."));
	m_filePath->setToolTip(tr("The path to a local executable to execute on remote hosts"));
	m_filePath->setWhatsThis(tr("Enter the path of a file on the local system that will be "
		"uploaeded to the selected remote hosts and exeucted."));
	m_remoteCommand->setToolTip(tr("Execute a program already on the remote host"));
	m_remoteCommand->setWhatsThis(tr("Select this option to execute a program that already "
		"exists on the remote computers."));
	m_commandEdit->setToolTip(tr("The executable on the remote host to execute"));
	m_commandEdit->setWhatsThis(tr("Enter the executable to execute on the remote host.  This "
		"can be the fully qualified path or if the file is in the path it can be just the "
		"program name."));
	m_commandLineEdit->setToolTip(tr("The arguments to pass to the program"));
	m_commandLineEdit->setWhatsThis(tr("Enter the command line to pass to the program being "
		"executed."));
	m_directoryEdit->setToolTip(tr("The working directory for the program"));
	m_directoryEdit->setWhatsThis(tr("Enter the directory on the remote machine that the "
		"program will be executed in.  (The executable may not actually be located in that "
		"directory."));
	m_captureOutput->setToolTip(tr("Check to have the console output returned"));
	m_captureOutput->setWhatsThis(tr("If this option is checked, stdout and stderr output "
		"from the program will be captured and returned to this machine and displayed in "
		"a new window when the program exits."));
	m_elevated->setToolTip(tr("Execute program with elevated privileges"));
	m_elevated->setWhatsThis(tr("If this option is checked, the program will be executed "
		"as with root or Administrator privileges on the remote machine."));
}


RemoteExecuteDialog::~RemoteExecuteDialog()
{
}


QString RemoteExecuteDialog::filePath() const
{
	return m_filePath->text();
}


QString RemoteExecuteDialog::command() const
{
	return m_commandEdit->text();
}


QString RemoteExecuteDialog::commandLine() const
{
	return m_commandLineEdit->text();
}


QString RemoteExecuteDialog::directory() const
{
	return m_directoryEdit->text();
}


bool RemoteExecuteDialog::localFile() const
{
	return m_localFile->isChecked();
}


bool RemoteExecuteDialog::captureOutput() const
{
	return m_captureOutput->isChecked();
}


bool RemoteExecuteDialog::elevated() const
{
	return m_elevated->isChecked();
}


void RemoteExecuteDialog::selectFile_clicked()
{
	if (sm_lastDirectoryChosen.isEmpty())
	{
		QSettings settings;
		sm_lastDirectoryChosen = settings.value(VALUE_LASTDIRCHOSEN).toString();
	}

	QFileDialog dialog;
	dialog.setDirectory(GetSaveDirectory(sm_lastDirectoryChosen, 
		{ QStandardPaths::DownloadLocation, QStandardPaths::DesktopLocation }));
	dialog.setFileMode(QFileDialog::ExistingFile);
#if defined(Q_OS_WIN)
	dialog.setNameFilters({ "Executable (*.exe)", "Any file (*)" });
	dialog.setDefaultSuffix("exe");
#endif

	if (QDialog::Accepted != dialog.exec())
		return;

	QFileInfo fileInfo(dialog.selectedFiles().first());
	m_filePath->setText(QDir::toNativeSeparators((fileInfo.absoluteFilePath())));
	sm_lastDirectoryChosen = fileInfo.path();
	QSettings settings;
	settings.setValue(VALUE_LASTDIRCHOSEN, sm_lastDirectoryChosen);
}

