#ifndef _EXEC_PRIVATE_H
#define _EXEC_PRIVATE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private prototypes for exec.library
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__)
#   ifndef NO_INLINE_STDARG
#	define NO_INLINE_STDARG
#   endif
#   include "exec_pinline.h"
#else
#   include "exec_pdefs.h"
#endif

/*
    Prototypes
*/
AROS_LP0(void, Dispatch,
    struct ExecBase *, SysBase, 10, Exec)

AROS_LP0(void, Exception,
    struct ExecBase *, SysBase, 11, Exec)

AROS_LP3(APTR, PrepareContext,
    AROS_LPA(APTR, stackPointer,    A0),
    AROS_LPA(APTR, entryPoint,      A1),
    AROS_LPA(APTR, fallBack,        A2),
    struct ExecBase *, SysBase, 6, Exec)

AROS_LP0(void, RawIOInit,
    struct ExecBase *, SysBase, 84, Exec)

AROS_LP0(LONG, RawMayGetChar,
    struct ExecBase *, SysBase, 85, Exec)

AROS_LP1(void, RawPutChar,
    AROS_LPA(UBYTE, chr, D0),
    struct ExecBase *, SysBase, 86, Exec)

AROS_LP1(void, Reschedule,
    AROS_LPA(struct Task *, task, A0),
    struct ExecBase *, SysBase, 8, Exec)

AROS_LP1(APTR, TaggedOpenLibrary,
    AROS_LPA(LONG, tag, D0),
    struct ExecBase *, SysBase, 135, Exec)


#endif /* _EXEC_PRIVATE_H */
