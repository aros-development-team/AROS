#ifndef PROTO_TRANSLATOR_H
#define PROTO_TRANSLATOR_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/translator_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/translator.h>
#else
#include <defines/translator.h>
#endif

#endif /* PROTO_TRANSLATOR_H */
