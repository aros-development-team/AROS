/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_INPUT_H
#define PROTO_INPUT_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/input_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/input.h>
#else
#include <defines/input.h>
#endif

#endif /* PROTO_INPUT_H */
