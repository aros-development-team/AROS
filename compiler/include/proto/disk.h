#ifndef PROTO_DISK_H
#define PROTO_DISK_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/disk_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/disk.h>
#else
#include <defines/disk.h>
#endif

#endif /* PROTO_DISK_H */
