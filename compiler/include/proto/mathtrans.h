#ifndef PROTO_MATHTRANS_H
#define PROTO_MATHTRANS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/mathtrans_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/mathtrans.h>
#else
#include <defines/mathtrans.h>
#endif

#endif /* PROTO_MATHTRANS_H */
