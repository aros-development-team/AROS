#ifndef PROTO_COLORWHEEL_H
#define PROTO_COLORWHEEL_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/colorwheel_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/colorwheel.h>
#else
#include <defines/colorwheel.h>
#endif

#endif /* PROTO_COLORWHEEL_H */
