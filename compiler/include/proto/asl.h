/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_ASL_H
#define PROTO_ASL_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/asl_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/asl.h>
#else
#include <defines/asl.h>
#endif

#endif /* PROTO_ASL_H */
