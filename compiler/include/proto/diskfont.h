#ifndef PROTO_DISKFONT_H
#define PROTO_DISKFONT_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/diskfont_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/diskfont.h>
#else
#include <defines/diskfont.h>
#endif

#endif /* PROTO_DISKFONT_H */
