#include "Utilities_X11.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)

#include "../common/PinholeCommon.h"

#include <QString>
#include <QElapsedTimer>
#include <QThread>
#include <QDebug>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

//#define DBG

static unsigned char* GetWindowPropertyByAtom(Display* display, Window window, Atom atom,
	long* nitems, Atom* type, int* size)
{
	Atom actual_type;
	int actual_format;
	unsigned long _nitems;
	unsigned long bytes_after; /* unused */
	unsigned char *prop;
	int status;

	status = XGetWindowProperty(display, window, atom, 0, (~0L),
		False, AnyPropertyType, &actual_type,
		&actual_format, &_nitems, &bytes_after,
		&prop);
	if (BadWindow == status)
	{
#ifdef DBG
		//Logger(LOG_WARNING) << QObject::tr("Window id %1 does not exist").arg(window);
		qDebug() << QObject::tr("Window id %1 does not exist").arg(window);
#endif
		return nullptr;
	}
	if (Success != status)
	{
#ifdef DBG
		//Logger(LOG_WARNING) << "XGetWindowProperty failed";
		qDebug() << "XGetWindowProperty failed";
#endif
		return nullptr;
	}

	if (nullptr != nitems)
		*nitems = _nitems;
	if (nullptr != type)
		*type = actual_type;
	if (nullptr != size)
		*size = actual_format;

	return prop;
}

#ifdef DBG
static QPair<QString, QString> GetWindowClass(Display* display, Window window)
{
	XClassHint* hint = XAllocClassHint();
	XGetClassHint(display, window, hint);
	QString name = QString::fromLocal8Bit(hint->res_name);
	QString cls = QString::fromLocal8Bit(hint->res_class);
	XFree(hint);
	return QPair<QString, QString>(name, cls);
}

static QString GetWindowName(Display* display, Window window)
{
	static Atom atom_NET_WM_NAME = (Atom)-1;
	if (atom_NET_WM_NAME == (Atom)-1) 
	{
		atom_NET_WM_NAME = XInternAtom(display, "_NET_WM_NAME", False);
	}
	static Atom atom_WM_NAME = (Atom)-1;
	if (atom_WM_NAME == (Atom)-1) 
	{
		atom_WM_NAME = XInternAtom(display, "WM_NAME", False);
	}

	Atom type;
	int size;
	long nitems;
	unsigned char *name = GetWindowPropertyByAtom(display, window, atom_NET_WM_NAME, &nitems, &type, &size);
	if (nullptr == name)
		name = GetWindowPropertyByAtom(display, window, atom_WM_NAME, &nitems, &type, &size);
	QString ret;

	if (nullptr != name)
	{
		ret = QString::fromLocal8Bit((const char*)name, nitems);
		free(name);
	}

	return ret;
}
#endif

static int GetWindowPid(Display* display, Window window)
{
	static Atom atom_NET_WM_PID = -1;
	if (static_cast<Atom>(-1) == atom_NET_WM_PID)
	{
		atom_NET_WM_PID = XInternAtom(display, "_NET_WM_PID", false);
	}

	Atom type;
	int size;
	long nitems;
	unsigned char *data;
	int windowPid = 0;

	data = GetWindowPropertyByAtom(display, window, atom_NET_WM_PID, &nitems, &type, &size);

	if (nullptr == data)
	{
		return 0;
	}

	windowPid = (int)*((unsigned long*)data);
	free(data);

#ifdef DBG
	qDebug() << "Window:" << window << "Pid:" << windowPid << "Name:" << GetWindowName(display, window) << "Class:" << GetWindowClass(display, window);
#endif
	return windowPid;
}


static Window FindAnyX11WindowWithPid(Display* display, int processId, Window rootWindow = static_cast<Window>(-1), int depth = 0)
{
	if (depth > 3)
		return static_cast<Window>(-1);
	if (rootWindow == static_cast<Window>(-1))
		rootWindow = XDefaultRootWindow(display);
	Window ret = static_cast<Window>(-1);
	Window dummy;
	Window* children = nullptr;
	unsigned int nChildren = 0;
	Status success = XQueryTree(display, rootWindow, &dummy, &dummy, &children, &nChildren);
	if (!success)
	{
#ifdef DBG
		//Logger(LOG_WARNING) << QString("XQueryTree failed");
		qDebug() << QObject::tr("XQueryTree failed");
#endif
	}
	else
	{
		for (unsigned int i = 0; i < nChildren; i++)
		{
			if (processId == GetWindowPid(display, children[i]))
			{
				XWindowAttributes attr;
				XGetWindowAttributes(display, children[i], &attr);
				if (attr.map_state == IsViewable)
				{
					ret = children[i];
					break;
				}
			}

			// Recurisve call
			ret = FindAnyX11WindowWithPid(display, processId, children[i], depth + 1);
			if (static_cast<Window>(-1) != ret)
				break;
		}
	}

	if (nullptr != children)
		XFree(children);

	return ret;
}

static Window FindX11TopWindowWithPid(Display* display, int processId)
{
	Window ret = static_cast<Window>(-1);
	Window rootWindow = XDefaultRootWindow(display);
	Window dummy;
	Window* children = nullptr;
	unsigned int nChildren = 0;
	Status success = XQueryTree(display, rootWindow, &dummy, &dummy, &children, &nChildren);
	if (!success)
	{
#ifdef DBG
		//Logger(LOG_WARNING) << QString("XQueryTree failed");
		qDebug() << QObject::tr("XQueryTree failed");
#endif
	}
	else
	{
		for (unsigned int i = 0; i < nChildren; i++)
		{
			if (processId == GetWindowPid(display, children[i]))
			{
				XWindowAttributes attr;
				XGetWindowAttributes(display, children[i], &attr);
				if (attr.map_state == IsViewable)
				{
					ret = children[i];
					break;
				}
			}
		}
	}

	if (nullptr != children)
		XFree(children);

	return ret;
}

#ifdef DBG
static const char* StatusToString(int status)
{
	switch (status)
	{
	case BadRequest:
		return "BadRequest";
	case BadValue:
		return "BadValue";
	case BadWindow:
		return "BadWindow";
	case BadPixmap:
		return "BadPixmap";
	case BadAtom:
		return "BadAtom";
	case BadCursor:
		return "BadCursor";
	case BadFont:
		return "BadFont";
	case BadMatch:
		return "BadMatch";
	case BadDrawable:
		return "BadDrawable";
	case BadAccess:
		return "BadAccess";
	case BadAlloc:
		return "BadAlloc";
	case BadColor:
		return "BadColor";
	case BadGC:
		return "BadGC";
	case BadIDChoice:
		return "BadIDChoice";
	case BadName:
		return "BadName";
	case BadLength:
		return "BadLength";
	case BadImplementation:
		return "BadImplementation";
	}

	return "Unknown";
}
#endif

static bool StatusSuccess(const char *function, int status)
{
#ifndef DBG
	Q_UNUSED(function);
#endif

	// For some reason the X11 functions seem to return BadRequest even on success
	if (0 == status || BadRequest == status)
		return true;

#ifdef DBG
	//Logger(LOG_WARNING) << QObject::tr("%1 failed with %2: %3")
	qDebug() << QObject::tr("%1 failed with %2: %3")
		.arg(function)
		.arg(status)
		.arg(StatusToString(status));
#endif
	return false;
}

static bool MinimizeX11Window(Display* display, Window window)
{
	XWindowAttributes attr;
	int ret = XGetWindowAttributes(display, window, &attr);
	StatusSuccess("XGetWindowAttributes", ret);
	int screen = XScreenNumberOfScreen(attr.screen);
	ret = XIconifyWindow(display, window, screen);
	return StatusSuccess("XIconifyWindow", ret);
}

static bool MaximizeX11Window(Display* display, Window window)
{
	XWindowAttributes attr;
	int ret = XGetWindowAttributes(display, window, &attr);
	StatusSuccess("XGetWindowAttributes", ret);
	ret = XRaiseWindow(display, window);
	StatusSuccess("XRaiseWindow", ret);
	Window rootWindow = XDefaultRootWindow(display);
	XEvent xev;
	memset(&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = true;
	xev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", false);
	xev.xclient.window = window;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
	xev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
	ret = XSendEvent(display, rootWindow, false,
		SubstructureNotifyMask | SubstructureRedirectMask, &xev);
	xev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	ret = XSendEvent(display, rootWindow, false,
		SubstructureNotifyMask | SubstructureRedirectMask, &xev);
	return StatusSuccess("XSendEvent", ret);
}

static bool HideX11Window(Display* display, Window window)
{
	int ret = XUnmapWindow(display, window);
	XFlush(display);
	return StatusSuccess("XUnmapWindow", ret);
}

bool ControlX11Window(const QString& displayName, int processId, const QString& command, int timeout)
{
	// Open display
	Display* display = XOpenDisplay(displayName.toLocal8Bit().data());
	if (nullptr == display)
	{
#ifdef DBG
		//Logger(LOG_WARNING) << qObject::tr("XOpenDisplay(%1) failed").arg(displayName);
		qDebug() << QObject::tr("XOpenDisplay(%1) failed").arg(displayName);
#endif
		return false;
	}

	QElapsedTimer timer;
	timer.start();
	Window target;
	do
	{
		// First try to find top level windows
		target = FindX11TopWindowWithPid(display, processId);
		// If no top level windows try any window
		if (static_cast<Window>(-1) == target)
			target = FindAnyX11WindowWithPid(display, processId);
		QThread::msleep(100);
	} while (static_cast<Window>(-1) == target && timer.elapsed() < timeout * 1000);

	bool ret = false;
	if (static_cast<Window>(-1) == target)
	{
#ifdef DBG
		//Logger(LOG_WARNING) << QObject::tr("Did not find window with pid %1").arg(processId);
		qDebug() << QObject::tr("Did not find window with pid %1").arg(processId);
#endif
	}
	else
	{
#ifdef DBG
		//Logger(LOG_DEBUG) << QObject::tr("X11 window found pid %1 window %2")
		qDebug() << QObject::tr("X11 window found pid %1 window %2")
			.arg(processId)
			.arg(target);
#endif

		if (DISPLAY_MINIMIZE == command)
		{
			ret = MinimizeX11Window(display, target);
		}
		else if (DISPLAY_MAXIMIZE == command)
		{
			ret = MaximizeX11Window(display, target);
		}
		else if (DISPLAY_HIDDEN == command)
		{
			ret = HideX11Window(display, target);
		}
	}

	XCloseDisplay(display);

	return ret;
}

#endif

