/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_RAMDRIVE_H
#define PROTO_RAMDRIVE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/ramdrive_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/ramdrive.h>
#else
#include <defines/ramdrive.h>
#endif

#endif /* PROTO_RAMDRIVE_H */
