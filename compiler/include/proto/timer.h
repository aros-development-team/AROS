#ifndef PROTO_TIMER_H
#define PROTO_TIMER_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/timer_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/timer.h>
#else
#include <defines/timer.h>
#endif

#endif /* PROTO_TIMER_H */
