#ifndef PROTO_LOCALE_H
#define PROTO_LOCALE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/locale_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/locale.h>
#else
#include <defines/locale.h>
#endif

#endif /* PROTO_LOCALE_H */
