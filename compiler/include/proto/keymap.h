/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_KEYMAP_H
#define PROTO_KEYMAP_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/keymap_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/keymap.h>
#else
#include <defines/keymap.h>
#endif

#endif /* PROTO_KEYMAP_H */
