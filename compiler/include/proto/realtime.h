#ifndef PROTO_REALTIME_H
#define PROTO_REALTIME_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/realtime_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/realtime.h>
#else
#include <defines/realtime.h>
#endif

#endif /* PROTO_REALTIME_H */
