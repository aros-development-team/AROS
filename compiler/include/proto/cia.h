/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_CIA_H
#define PROTO_CIA_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/cia_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/cia.h>
#else
#include <defines/cia.h>
#endif

#endif /* PROTO_CIA_H */
