/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_EXEC_SYSBASE_H
#define PROTO_EXEC_SYSBASE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/exec_sysbase_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#ifndef NO_INLINE_STDARG
#define NO_INLINE_STDARG
#endif
#include <inline/exec_sysbase.h>
#else
#include <defines/exec_sysbase.h>
#endif

#endif /* PROTO_EXEC_SYSBASE_H */
