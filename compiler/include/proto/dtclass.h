#ifndef PROTO_DTCLASS_H
#define PROTO_DTCLASS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/dtclass_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/dtclass.h>
#else
#include <defines/dtclass.h>
#endif

#endif /* PROTO_DTCLASS_H */
