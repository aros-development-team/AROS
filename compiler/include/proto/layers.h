/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_LAYERS_H
#define PROTO_LAYERS_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/layers_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/layers.h>
#else
#include <defines/layers.h>
#endif

#endif /* PROTO_LAYERS_H */
