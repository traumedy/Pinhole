#pragma once


#include <QObject>

#if defined(Q_OS_WIN)
#include "os/win32/sigar_os.h"
#elif defined(Q_OS_MAC)
#include "os/darwin/sigar_os.h"
#elif defined(Q_OS_LINUX)
#include "os/linux/sigar_os.h"
#endif
