#include "HostScanDialog.h"
#include "HostFinder.h"

#include <QMessageBox>
#include <QHostInfo>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>
#include <QGridLayout>


#define IPV4SEG "(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])"
#define IPV6SEG "[0-9a-fA-F]{1,4}"
#define IPV4REGEX "(" IPV4SEG "\\.){3,3}" IPV4SEG 
#define IPV6REGEX "("\
				"(" IPV6SEG ":){7,7}" IPV6SEG "|"\
				"(" IPV6SEG ":){1,7}:|"\
				"(" IPV6SEG ":){1,6}:" IPV6SEG "|"\
				"(" IPV6SEG ":){1,5}(:" IPV6SEG "){1,2}|"\
				"(" IPV6SEG ":){1,4}(:" IPV6SEG "){1,3}|"\
				"(" IPV6SEG ":){1,3}(:" IPV6SEG "){1,4}|"\
				"(" IPV6SEG ":){1,2}(:" IPV6SEG "){1,5}|"\
				IPV6SEG ":((:" IPV6SEG "){1,6})|"\
				":((:" IPV6SEG "){1,7}|:)|"\
				"fe80:(:" IPV6SEG "){0,4}%[0-9a-zA-Z]{1,}|"\
				"::(ffff(:0{1,4}){0,1}:){0,1}" IPV4REGEX "|"\
				"(" IPV6SEG ":){1,4}:" IPV4REGEX\
				")"


HostScanDialog::HostScanDialog(HostFinder* hostFinder, QWidget *parent)
	: QDialog(parent), m_hostFinder(hostFinder)
{
	QGridLayout* mainLayout = new QGridLayout(this);
	mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

	m_ipv4Button = new QRadioButton("IPv4");
	connect(m_ipv4Button, &QRadioButton::clicked,
		this, &HostScanDialog::ipv4Button_clicked);
	mainLayout->addWidget(m_ipv4Button, 0, 0);
	m_ipv6Button = new QRadioButton("IPv6");
	connect(m_ipv6Button, &QRadioButton::clicked,
		this, &HostScanDialog::ipv6Button_clicked);
	mainLayout->addWidget(m_ipv6Button, 0, 1);
	m_singleAddress = new QLineEdit;
	m_singleAddress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	mainLayout->addWidget(m_singleAddress, 1, 0, 1, 3);
	m_scanSingleButton = new QPushButton(tr("Query single address"));
	connect(m_scanSingleButton, &QPushButton::clicked,
		this, &HostScanDialog::scanSingleButton_clicked);
	mainLayout->addWidget(m_scanSingleButton, 1, 3);
	m_scanSubnetButton = new QPushButton(tr("Scan around address"));
	connect(m_scanSubnetButton, &QPushButton::clicked,
		this, &HostScanDialog::scanSubnetButton_clicked);
	mainLayout->addWidget(m_scanSubnetButton, 1, 4);
	m_hostName = new QLineEdit;
	m_hostName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	mainLayout->addWidget(m_hostName, 2, 0, 1, 3);
	m_lookupHostButton = new QPushButton(tr("Lookup host name"));
	connect(m_lookupHostButton, &QPushButton::clicked,
		this, &HostScanDialog::lookupHostButton_clicked);
	mainLayout->addWidget(m_lookupHostButton, 2, 3);
	m_statusText = new QLabel;
	m_statusText->setAlignment(Qt::AlignHCenter);
	mainLayout->addWidget(m_statusText, 3, 0, 1, 3);

	// Set initial validator to IPv4
	m_ipv4Button->click();

	m_statusText->clear();

	m_singleAddress->setFocus();

	// So we can set text across threads (from the DNS callback)
	connect(this, &HostScanDialog::setSingleAdderessText,
		m_singleAddress, &QLineEdit::setText);
	connect(this, &HostScanDialog::setStatusText,
		m_statusText, &QLabel::setText);

	// Lock the height of the dialog
	setAttribute(Qt::WA_DontShowOnScreen, true);
	show();
	int h = height();
	setMinimumHeight(h);
	setMaximumHeight(h);
	// Make dialog just a bit wider
	setMinimumWidth(width() + 100);
	hide();
	setAttribute(Qt::WA_DontShowOnScreen, false);

	m_ipv4Button->setToolTip(tr("Enter or resolve IPv4 addresses"));
	m_ipv4Button->setWhatsThis(tr("Select this button to enter IP addresses in IPv4 "
		"format.  When resolving host names if both IPv4 and IPv6 are returned the "
		"IPv4 address will be shown."));
	m_ipv6Button->setToolTip(tr("Enter or resolve IPv6 addresses"));
	m_ipv6Button->setWhatsThis(tr("Select this button to enter IP addresses in IPv6 "
		"format.  When resolving host names if both IPv4 and IPv6 are returned the "
		"IPv6 address will be shown."));
	m_singleAddress->setToolTip(tr("Enter an IPv4 or IPv6 IP address to send a query packet to"));
	m_singleAddress->setWhatsThis(tr("This is where you enter the IP address to send "
		"a query packet to.  You can enter a broadcast address in the field.  If "
		"you resolve a host name this is where the address will be put."));
	m_scanSingleButton->setToolTip(tr("Send a query packet to the entered IP address"));
	m_scanSingleButton->setWhatsThis(tr("Click this button to send a query packet to "
		"IP address in the field to the left.  If the address is incomplete you will "
		"get an error."));
	m_scanSubnetButton->setToolTip(tr("Sends query packets to IP addresses around the entered IP address"));
	m_scanSubnetButton->setWhatsThis(tr("Click this button to send query packets to "
		"256 IP addresses around the IP address in the field to the left.<br/>"
		"For IPv4 addresses all the entire class C network of the entered address "
		"will be scanned.<br/>"
		"For IPv6 addresses 128 addresses above and below the entered address will "
		"be scanned (unless the entered address is on the edge of a block)."));
	m_hostName->setToolTip(tr("Enter a host name to resolve to an IP address"));
	m_hostName->setWhatsThis(tr("Enter a host name in this field and click <b>Lookup "
		"host </b> to resolve it to an address.  If the desired IP version is not "
		"returned the other will be filled in."));
	m_lookupHostButton->setToolTip(tr("Lookup the IP address of the entered host name"));
	m_lookupHostButton->setWhatsThis(tr("Click this button to resolve the host name "
		"entered in the field to the left to an IP address."));
}


HostScanDialog::~HostScanDialog()
{
}


// Sets the address if it isn't already set
void HostScanDialog::setAddress(const QString & address)
{
	if (m_singleAddress->text().isEmpty())
	{
		m_singleAddress->setText(address);
	}
}


void HostScanDialog::ipv4Button_clicked()
{
	QRegExpValidator *v = new QRegExpValidator(this);
	QRegExp rx(IPV4REGEX);
	v->setRegExp(rx);
	m_singleAddress->setValidator(v);
	if (!isSingleAddressValid(QValidator::Intermediate))
		m_singleAddress->clear();
}


void HostScanDialog::ipv6Button_clicked()
{
	QRegExpValidator *v = new QRegExpValidator(this);
	QRegExp rx(IPV6REGEX);
	v->setRegExp(rx);
	m_singleAddress->setValidator(v);
	if (!isSingleAddressValid(QValidator::Intermediate))
		m_singleAddress->clear();
}


void HostScanDialog::scanSingleButton_clicked()
{
	if (!isSingleAddressValid())
	{
		QMessageBox msgbox(this);
		msgbox.setText(tr("Enter a valid IP address"));
		msgbox.exec();
	}
	else
	{
		QString addressText = m_singleAddress->text();
		QHostAddress addr;
		addr.setAddress(addressText);
		m_hostFinder->queryHostAddress(addr);
		m_statusText->setText(tr("Query sent to ") + addressText);
	}
}


void HostScanDialog::scanSubnetButton_clicked()
{
	if (!isSingleAddressValid())
	{
		QMessageBox msgbox(this);
		msgbox.setText(tr("Enter a valid IP address"));
		msgbox.exec();
	}
	else
	{
		if (m_ipv4Button->isChecked())
		{
			QString baseAddress = m_singleAddress->text().left(m_singleAddress->text().lastIndexOf('.') + 1);
			QHostAddress addr;
			for (unsigned int x = 0; x <= 255; x++)
			{
				addr.setAddress(baseAddress + QString::number(x));
				m_hostFinder->queryHostAddress(addr);
			}
			m_statusText->setText(tr("Scanned %010 to .255")
				.arg(baseAddress));
		}
		else
		{
			int lastColonIndex = m_singleAddress->text().lastIndexOf(':') + 1;
			if (-1 != lastColonIndex)
			{
				QString baseAddress = m_singleAddress->text().left(lastColonIndex);
				QString lastAddress = m_singleAddress->text().mid(lastColonIndex);
				bool ok = false;
				int startAddress = lastAddress.toInt(&ok, 16);
				if (ok)
				{
					// Just scan 128 addresses before and after IP, unless 
					// address is at the edge of block
					if (startAddress < 128)
						startAddress = 0;
					else if (startAddress >= 65407)
						startAddress = 65279;
					else
						startAddress -= 128;

					QHostAddress addr;
					for (int x = startAddress; x <= startAddress + 256; x++)
					{
						addr.setAddress(baseAddress + QString::number(x, 16).rightJustified(4, '0'));
						m_hostFinder->queryHostAddress(addr);
					}
					m_statusText->setText(tr("Scanned %1%2 to :%3")
						.arg(baseAddress)
						.arg(QString::number(startAddress, 16).rightJustified(4, '0'))
						.arg(QString::number(startAddress + 256, 16).rightJustified(4, '0')));
				}
			}
		}
	}
}


void HostScanDialog::lookupHostButton_clicked()
{
	QString name = m_hostName->text();

	if (!name.isEmpty())
	{
		emit setStatusText(tr("Looking up host '%1'...").arg(name));

		QHostInfo::lookupHost(name, 
			[this](QHostInfo hostInfo)
		{
			QString errorStr;
			switch (hostInfo.error())
			{
			case QHostInfo::HostNotFound:
				errorStr = tr("Host not found: ") + hostInfo.hostName();
				break;
			case QHostInfo::UnknownError:
				errorStr = hostInfo.hostName() + ": " + hostInfo.errorString();
				break;
			case QHostInfo::NoError:
				break;
			}

			if (!errorStr.isEmpty())
			{
				emit setStatusText(errorStr);
			}
			else
			{
				m_statusText->clear();

				if (m_ipv6Button->isChecked())
				{
					bool found = false;
					for (const auto& addr : hostInfo.addresses())
					{
						if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv6Protocol)
						{
							found = true;
							emit setSingleAdderessText(addr.toString());
							break;
						}
					}
					
					if (!found)
					{
						emit setStatusText(tr("No IPv6 address found"));
						if (!hostInfo.addresses().isEmpty())
						{
							m_ipv4Button->click();
							emit setSingleAdderessText(hostInfo.addresses()[0].toString());
						}
					}
				}
				else
				{
					bool found = false;
					for (const auto& addr : hostInfo.addresses())
					{
						if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol)
						{
							found = true;
							emit setSingleAdderessText(addr.toString());
							break;
						}
					}

					if (!found)
					{
						emit setStatusText(tr("No IPv4 address found"));
						if (!hostInfo.addresses().isEmpty())
						{
							m_ipv6Button->click();
							emit setSingleAdderessText(hostInfo.addresses()[0].toString());
						}
					}
				}
			}
		});
	}
}


bool HostScanDialog::isSingleAddressValid(QValidator::State state)
{
	const QValidator* validator = m_singleAddress->validator();

	int pos = m_singleAddress->cursorPosition();
	QString text = m_singleAddress->text();
	return state == validator->validate(text, pos);
}

