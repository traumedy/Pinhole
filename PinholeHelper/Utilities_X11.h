#pragma once

#include <QString>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)

bool ControlX11Window(const QString& displayName, int processId, const QString& command, int timeout = 10);

#endif

