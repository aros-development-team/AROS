/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_CONSOLE_H
#define PROTO_CONSOLE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/console_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/console.h>
#else
#include <defines/console.h>
#endif

#endif /* PROTO_CONSOLE_H */
