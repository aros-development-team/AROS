#ifndef PROTO_CARDRES_H
#define PROTO_CARDRES_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/cardres_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/cardres.h>
#else
#include <defines/cardres.h>
#endif

#endif /* PROTO_CARDRES_H */
