#ifndef PROTO_MISC_H
#define PROTO_MISC_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/misc_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/misc.h>
#else
#include <defines/misc.h>
#endif

#endif /* PROTO_MISC_H */
