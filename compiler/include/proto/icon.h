#ifndef PROTO_ICON_H
#define PROTO_ICON_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/icon_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/icon.h>
#else
#include <defines/icon.h>
#endif

#endif /* PROTO_ICON_H */
