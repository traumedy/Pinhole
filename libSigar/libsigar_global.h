#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(LIBSIGAR_LIB)
#  define LIBSIGAR_EXPORT Q_DECL_EXPORT
# else
#  define LIBSIGAR_EXPORT Q_DECL_IMPORT
# endif
#else
# define LIBSIGAR_EXPORT
#endif
