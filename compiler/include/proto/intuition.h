#ifndef PROTO_INTUITION_H
#define PROTO_INTUITION_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/intuition_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/intuition.h>
#else
#include <defines/intuition.h>
#endif

#endif /* PROTO_INTUITION_H */
