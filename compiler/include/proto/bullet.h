#ifndef PROTO_BULLET_H
#define PROTO_BULLET_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/bullet_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/bullet.h>
#else
#include <defines/bullet.h>
#endif

#endif /* PROTO_BULLET_H */
