#ifndef PROTO_GRAPHICS_H
#define PROTO_GRAPHICS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/graphics_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/graphics.h>
#else
#include <defines/graphics.h>
#endif

#endif /* PROTO_GRAPHICS_H */
