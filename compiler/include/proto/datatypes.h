/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_DATATYPES_H
#define PROTO_DATATYPES_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/datatypes_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/datatypes.h>
#else
#include <defines/datatypes.h>
#endif

#endif /* PROTO_DATATYPES_H */
