/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_LOWLEVEL_H
#define PROTO_LOWLEVEL_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/lowlevel_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/lowlevel.h>
#else
#include <defines/lowlevel.h>
#endif

#endif /* PROTO_LOWLEVEL_H */
