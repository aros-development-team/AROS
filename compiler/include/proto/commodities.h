#ifndef PROTO_COMMODITIES_H
#define PROTO_COMMODITIES_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/commodities_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/commodities.h>
#else
#include <defines/commodities.h>
#endif

#endif /* PROTO_COMMODITIES_H */
