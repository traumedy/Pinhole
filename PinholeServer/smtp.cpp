/*
Copyright (c) 2013 Raivis Strogonovs

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

#include "smtp.h"

#include <QCryptographicHash>

Smtp::Smtp(const QString &user, const QString &pass, const QString &host, bool ssl, bool tls, 
	int port)
{
	socket = new QSslSocket(this);

	connect(socket, &QTcpSocket::readyRead,
		this, &Smtp::readyRead);
	connect(socket, &QTcpSocket::connected,
		this, &Smtp::connected);
	connect(socket, (void (QTcpSocket::*)(QAbstractSocket::SocketError))&QTcpSocket::error,
		this, &Smtp::errorReceived);
	connect(socket, &QTcpSocket::stateChanged,
		this, &Smtp::stateChanged);
	connect(socket, &QTcpSocket::disconnected,
		this, &Smtp::disconnected);
	connect(socket, &QSslSocket::encrypted,
		this, &Smtp::encrypted);

	this->user = user;
	this->pass = pass;
	this->host = host;
	this->port = port;
	this->SSL = ssl;
	this->TLS = tls;
}

void Smtp::sendMail(const QString &from, const QString &to, const QString &subject, const QString &body)
{
	message = "To: " + to + "\n";
	message.append("From: " + from + "\n");
	message.append("Subject: " + subject + "\n\n");		// Need the extra EOL to indicate end of headers?
	message.append(body);
	message.replace(QString::fromLatin1("\n"), QString::fromLatin1("\r\n"));
	message.replace(QString::fromLatin1("\r\n.\r\n"),
		QString::fromLatin1("\r\n..\r\n"));
	this->from = from;
	rcpt = to;
	state = Init;
#if defined(QT_DEBUG)
	qDebug() << "connectToHostEncrypted" << host << port;
#endif

	if (SSL)
		socket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS
	else
		socket->connectToHost(host, port);

	t = new QTextStream(socket);
}

Smtp::~Smtp()
{
	delete t;
	delete socket;
}
void Smtp::stateChanged(QAbstractSocket::SocketState socketState)
{
#ifdef QT_DEBUG
	qDebug() <<"stateChanged " << socketState;
#else
	Q_UNUSED(socketState);
#endif
}

void Smtp::errorReceived(QAbstractSocket::SocketError socketError)
{
#ifdef QT_DEBUG
	qDebug() << "error " << socketError;
#else
	Q_UNUSED(socketError);
#endif
	if (state != Close)
	{
		state = Close;
		QString statusText = tr("Socket error '%1'").arg(socket->errorString());
		if (state == Init)
			statusText = tr("Failed to connect");

		emit status(statusText);
		deleteLater();
	}
}

void Smtp::disconnected()
{
#ifdef QT_DEBUG
	qDebug() << "Disconnected, error" << socket->errorString();
#endif
	if (Close != state)
	{
		state = Close;
		emit status(tr("Disconnected"));
		deleteLater();
	}
}

void Smtp::connected()
{    
#ifdef QT_DEBUG
	qDebug() << "Connected ";
#endif
}

void Smtp::encrypted()
{
#ifdef QT_DEBUG
	qDebug() << "Encrypted ";
#endif

	if (state == StartTls)
	{
		*t << "EHLO localhost\r\n";
		t->flush();
		state = Auth;
	}
}

void Smtp::readyRead()
{
#ifdef QT_DEBUG
	qDebug() << "readyRead, state:" << state;
#endif
	// SMTP is line-oriented

	QString responseLine;
	do
	{
		responseLine = socket->readLine();
		response += responseLine;
	} while (socket->canReadLine() && responseLine[3] != ' ');

	responseLine.truncate(3);

#ifdef QT_DEBUG
	qDebug() << "Server response code:" << responseLine;
	qDebug() << "Server response: " << response;
#endif

	if (state == Init && responseLine == "220")
	{
		// banner was okay, let's go on
		*t << "EHLO localhost\r\n";
		t->flush();

		state = Auth;
	}
	else if (state == Auth && responseLine == "250" && 
		!socket->isEncrypted() && TLS && response.contains("STARTTLS"))
	{
		// Trying AUTH
#ifdef QT_DEBUG
		qDebug() << "Starting TLS";
#endif
		*t << "STARTTLS\r\n";
		t->flush();
		state = StartTls;
	}
	else if (state == StartTls && responseLine == "220")
	{
#ifdef QT_DEBUG
		qDebug() << "Starting encryption...";
#endif
		socket->startClientEncryption();

		// EHLO will be sent again when connection is encrypted in callback
	}
	else if (state == Auth && responseLine == "250")
	{
#ifdef QT_DEBUG
		qDebug() << "Auth";
#endif
		int authPos = response.indexOf("250 AUTH ");
		if (-1 == authPos)
			authPos = response.indexOf("250-AUTH ");	// smtp.gmail.com puts dashes after code

		if (-1 == authPos)
		{
			emit status(tr("No auth header"));
			state = Close;
			deleteLater();
		}
		else
		{
			// Find the best auth method
			authPos += 9;
			int eolPos = response.indexOf("\r", authPos);
			QStringList authMethods = response.mid(authPos, eolPos - authPos).split(" ");
			
			/*
			if (authMethods.contains("XOAUTH2"))
			{

			}
			else */
			if (authMethods.contains("CRAM-MD5"))
			{
				*t << "AUTH CRAM-MD5\r\n";
				t->flush();
				state = CredCRAMMD5;
			}
			else if (authMethods.contains("LOGIN"))
			{
				*t << "AUTH LOGIN\r\n";
				t->flush();
				state = UserLogin;
			}
			else if (authMethods.contains("PLAIN"))
			{
#if defined(QT_DEBUG)
				qDebug() << "AUTH PLAIN" << user << pass;
#endif
				*t << "AUTH PLAIN " + QString("\0" + user + "\0" + pass).toLocal8Bit().toBase64() << "\r\n";
				t->flush();
				state = Mail;
			}
			else
			{
				emit status(tr("No compatible authentication methods"));
				state = Close;
				deleteLater();
			}
		}
	}
	else if (state == CredCRAMMD5 && responseLine == "334")
	{
		// UNTESTED

#if defined(QT_DEBUG)
		qDebug() << "AUTH CRAM-MD5" << user << pass;
#endif

		QByteArray challenge = QByteArray::fromBase64(response.mid(4).trimmed().toLocal8Bit());
		QString response = user + " " + QString(QCryptographicHash::hash(challenge + pass.toLocal8Bit(), QCryptographicHash::Md5)).toLower();
		//qDebug() << response;
		*t << response.toLocal8Bit().toBase64() << "\r\n";
		t->flush();
		state = Mail;
	}
	else if (state == UserLogin && responseLine == "334")
	{
		//Trying User        
#ifdef QT_DEBUG
		qDebug() << "Username" << user;
#endif
		//GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
		//https://developers.google.com/gmail/xoauth2_protocol
		*t << user.toLocal8Bit().toBase64() << "\r\n";
		t->flush();

		state = PassLogin;
	}
	else if (state == PassLogin && responseLine == "334")
	{
		//Trying pass
#ifdef QT_DEBUG
		qDebug() << "Pass" << pass;
#endif
		*t << pass.toLocal8Bit().toBase64() << "\r\n";
		t->flush();

		state = Mail;
	}
	else if (state == Mail && responseLine == "535")
	{
		emit status(tr("Authentication failed"));
		state = Close;
		deleteLater();
	}
	else if (state == Mail && responseLine == "235")
	{
		// HELO response was okay (well, it has to be)

		//Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
#ifdef QT_DEBUG
		qDebug() << "MAIL FROM:<" << from << ">";
#endif
		*t << "MAIL FROM:<" << from << ">\r\n";
		t->flush();
		state = Rcpt;
	}
	else if (state == Rcpt && responseLine == "250")
	{
		//Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
#ifdef QT_DEBUG
		qDebug() << "RCPT TO:<" << rcpt << ">";
#endif
		*t << "RCPT TO:<" << rcpt << ">\r\n"; //r
		t->flush();
		state = Data;
	}
	else if (state == Data && responseLine == "250")
	{
#ifdef QT_DEBUG
		qDebug() << "DATA";
#endif
		*t << "DATA\r\n";
		t->flush();
		state = Body;
	}
	else if (state == Body && responseLine == "354")
	{
#ifdef QT_DEBUG
		qDebug() << "message:" << message;
#endif
		*t << message << "\r\n.\r\n";
		t->flush();
		state = Quit;
	}
	else if (state == Quit && responseLine == "250")
	{
#ifdef QT_DEBUG
		qDebug() << "QUIT";
#endif
		*t << "QUIT\r\n";
		t->flush();
		// here, we just close.
		state = Close;
		// Emtpy message indicates success
		emit status(tr(""));
		deleteLater();
	}
	else if (state == Close)
	{
		deleteLater();
		return;
	}
	else
	{
		// something broke.
		//QMessageBox::warning( 0, tr( "Qt Simple SMTP client" ), tr( "Unexpected reply from SMTP server:\n\n" ) + response );
		state = Close;
		emit status(tr("Failed to send message (response %1, state %2)")
			.arg(responseLine)
			.arg(state));
		deleteLater();
	}

	response = "";
}

