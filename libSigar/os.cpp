
#include <QObject>

#if defined(Q_OS_WIN)
#include "os/win32/win32_sigar.c"
#include "os/win32/peb.c"
#include "os/win32/wmi.cpp"
#elif defined(Q_OS_MAC)
#include "os/darwin/darwin_sigar.c"
#elif defined(Q_OS_LINUX)
#include "os/linux/linux_sigar.c"
#endif

