#ifndef PROTO_NONVOLATILE_H
#define PROTO_NONVOLATILE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/nonvolatile_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/nonvolatile.h>
#else
#include <defines/nonvolatile.h>
#endif

#endif /* PROTO_NONVOLATILE_H */
