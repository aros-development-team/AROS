/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/exec_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/exec.h>
#ifdef EXEC_PRIVATE_INLINES
#include <inline/exec_private.h>
#endif /* EXEC_PRIVATE_INLINES */
#else
#include <defines/exec.h>
#endif

/* Common support functions */
#ifndef PROTO_ALIB_H
#include <proto/alib.h>
#endif

#endif /* PROTO_EXEC_H */
