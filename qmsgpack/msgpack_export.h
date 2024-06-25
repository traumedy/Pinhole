#ifndef MSGPACK_EXPORT_H
#define MSGPACK_EXPORT_H

#include <QtCore/qglobal.h>

#if 0
#define MSGPACK_EXPORT 
#else
#if defined(MSGPACK_MAKE_LIB) // building lib
#define MSGPACK_EXPORT Q_DECL_EXPORT
#else // using lib
#define MSGPACK_EXPORT Q_DECL_IMPORT
#endif
#endif

#endif // MSGPACK_EXPORT_H
