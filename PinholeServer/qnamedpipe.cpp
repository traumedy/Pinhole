#include "qnamedpipe.h"
#include <QtConcurrent>

#ifndef PIPE_BUFFER_LENGTH
#  define PIPE_BUFFER_LENGTH 2048
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
#include <aclapi.h>

class QNamedPipePrivate
{
public:
	QNamedPipePrivate(QNamedPipe* parent, QString path)
		: m_parent(parent)
		, m_handlepipe(NULL)
		, m_event(NULL)
		, m_serverMode(false)
		, m_willBeClose(false)
	{
		// Create a SECURITY_ATTRIBUTES allowing everyone
		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
		PSID everyone_sid = NULL;
		AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
			0, 0, 0, 0, 0, 0, 0, &everyone_sid);

		EXPLICIT_ACCESS ea;
		ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
		ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance = NO_INHERITANCE;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName = (LPWSTR)everyone_sid;

		PACL acl = NULL;
		SetEntriesInAclW(1, &ea, NULL, &acl);

		PSECURITY_DESCRIPTOR sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
			SECURITY_DESCRIPTOR_MIN_LENGTH);
		InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE);

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = sd;
		sa.bInheritHandle = FALSE;

        std::wstring p = path.toStdWString();
        m_handlepipe = ::CreateNamedPipeW(
                    p.data(),
                    PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                    PIPE_TYPE_BYTE,
                    1,
                    PIPE_BUFFER_LENGTH,
                    PIPE_BUFFER_LENGTH,
                    1000,
                    &sa);
        if(m_handlepipe != INVALID_HANDLE_VALUE)
            m_serverMode = true;
        else {
            m_handlepipe = ::CreateFileW(
                        p.data(),
                        GENERIC_READ | GENERIC_WRITE,
                        0, NULL, OPEN_EXISTING, 0, NULL
                        );
        }

		FreeSid(everyone_sid);
		LocalFree(sd);
		LocalFree(acl);
    }
    ~QNamedPipePrivate()
    {
        dispose();
    }
    void dispose()
    {
        if(m_handlepipe) {
            m_willBeClose = true;
            if(m_event) {
                HANDLE event = m_event;
                m_event = NULL;
                ::CloseHandle(event);
            }
            m_handlepipe = NULL;
        }
    }

    void sendMessage(QByteArray bytes)
    {
        DWORD dwResult = 0;
        ::WriteFile(m_handlepipe, bytes.data(), (DWORD)bytes.size(), &dwResult, NULL);
        //qDebug() << dwResult;
    }
    void waitAsync()
    {
        DWORD dwResult;
        m_event = ::CreateEventW(NULL, FALSE, FALSE, NULL);
        HANDLE handlePipe = m_handlepipe;
        do {
            OVERLAPPED ov = {0};
            ov.hEvent = m_event;
            ::ConnectNamedPipe(handlePipe, &ov);
            dwResult = ::WaitForSingleObject(m_event, 100);
            if(dwResult == 0xFFFFFFFF) {
//                qDebug() << "waitAsync" << dwResult;
                ::CloseHandle(handlePipe);
                return;
            }
            DWORD dwLength = 0;
            if(dwResult == WAIT_OBJECT_0) {
                QByteArray bytes(PIPE_BUFFER_LENGTH, 0);
                if (::ReadFile(m_handlepipe, bytes.data(), PIPE_BUFFER_LENGTH, &dwLength, NULL) && dwLength > 0) {
                    bytes.resize(dwLength);
                    emit m_parent->received(bytes);
                }
            }
            ::DisconnectNamedPipe(m_handlepipe);
        } while(1);

    }
    bool isValid() { return m_handlepipe != nullptr; }

    HANDLE m_handlepipe;
    HANDLE m_event;
    bool m_serverMode;
    QNamedPipe* m_parent;
    bool m_willBeClose;
};
#else

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

class QNamedPipePrivate
{
public:
    QNamedPipePrivate(QNamedPipe* parent, QString path)
        : m_fd(0)
        , m_parent(parent)
        , m_serverMode(false)
        , m_willBeClose(false)
        , m_valid(true)
    {
        m_pipepath = path.toUtf8();
        if(::mkfifo(m_pipepath.data(), S_IRWXU) != 0) {
            m_valid = false;
            if((m_fd = ::open(m_pipepath.data(), O_RDWR|O_NONBLOCK)) > 0)
               m_valid = true;
        } else if((m_fd = ::open(m_pipepath.data(), O_RDONLY|O_NONBLOCK)) > 0) {
            m_serverMode = true;
        }
    }
    ~QNamedPipePrivate()
    {
        dispose();
    }

    bool isValid() { return m_fd > 0; }
    void sendMessage(QByteArray bytes)
    {
        if(!m_serverMode) {
            if(::write(m_fd, bytes.data(), bytes.length()) < 0)
                qDebug() << "write() error";
        } else {
            int fd = ::open(m_pipepath.data(), O_WRONLY);
            if(!::write(fd, bytes.data(), bytes.length()))
                qDebug() << "write() error";
            ::close(fd);
        }
    }

    void waitAsync()
    {
		m_waitMutex.lock();
        for(;;) {
            QByteArray bytes(PIPE_BUFFER_LENGTH, 0);
            int fd = ::open(m_pipepath.data(), O_RDONLY); // will be locked
            m_mutex.lock();
            size_t length = ::read(fd, bytes.data(), bytes.size());
            ::close(fd);
            if(length > 0) {
                bytes.resize(length);
                if(length < 3 && bytes[0] == '0')
                    break;
                emit m_parent->received(bytes);
            }
            m_mutex.unlock();
        }
        m_mutex.unlock();
		m_waitMutex.unlock();
    }
    void dispose()
    {
        m_willBeClose = true;
        if(!m_serverMode) {
            ::close(m_fd);
        } else {
            sendMessage("0");
            m_mutex.lock();
            m_mutex.unlock();
            ::close(m_fd);
            m_fd = 0;
            ::unlink(m_pipepath.data());
        }

		m_waitMutex.lock();
		m_waitMutex.unlock();
    }

    int  m_fd;
    QMutex m_mutex;
	QMutex m_waitMutex;
    QByteArray m_pipepath;
    QNamedPipe* m_parent;
    bool m_serverMode;
    bool m_willBeClose;
    bool m_valid;
};
#endif

QNamedPipe::QNamedPipe(QString name, bool valid, QObject *parent)
	: QObject(parent)
	, d(valid ? new QNamedPipePrivate(this, generatePipePath(name)) : nullptr)
{

}

QNamedPipe::~QNamedPipe()
{
    if(d)
        delete d;
}

void QNamedPipe::send(QByteArray bytes)
{
    if(d)
        d->sendMessage(bytes);
}

bool QNamedPipe::isServerMode()
{
    return !d || d->m_serverMode;
}

void QNamedPipe::waitAsync()
{
    if(d)
        QtConcurrent::run(d, &QNamedPipePrivate::waitAsync);
}

bool QNamedPipe::isValid()
{
    return !d || d->isValid();
}

QString QNamedPipe::generatePipePath(QString name)
{
#ifdef Q_OS_WIN
    return QString("\\\\.\\pipe\\%1").arg(name);
#else
    return QString("/tmp/.%1.fifo").arg(name);
#endif
}
