#ifndef _EXEC_PDEFS_H
#define _EXEC_PDEFS_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Private defines for exec.library
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#ifndef Dispatch
#define Dispatch() \
    AROS_LC0(void, Dispatch, \
    struct ExecBase *, SysBase, 10, Exec)
#endif

#ifndef Exception
#define Exception() \
    AROS_LC0(void, Exception, \
    struct ExecBase *, SysBase, 11, Exec)
#endif

#ifndef PrepareContext
#define PrepareContext(stackPointer, entryPoint, fallBack) \
    AROS_LC3(APTR, PrepareContext, \
    AROS_LCA(APTR, stackPointer,    A0), \
    AROS_LCA(APTR, entryPoint,      A1), \
    AROS_LCA(APTR, fallBack,        A2), \
    struct ExecBase *, SysBase, 6, Exec)
#endif

#ifndef RawIOInit
#define RawIOInit() \
    AROS_LC0(void, RawIOInit, \
    struct ExecBase *, SysBase, 84, Exec)
#endif

#ifndef RawMayGetChar
#define RawMayGetChar() \
    AROS_LC0(LONG, RawMayGetChar, \
    struct ExecBase *, SysBase, 85, Exec)
#endif

#ifndef RawPutChar
#define RawPutChar(chr) \
    AROS_LC1(void, RawPutChar, \
    AROS_LCA(UBYTE, chr, D0), \
    struct ExecBase *, SysBase, 86, Exec)
#endif

#ifndef Reschedule
#define Reschedule(task) \
    AROS_LC1(void, Reschedule, \
    AROS_LCA(struct Task *, task, A0), \
    struct ExecBase *, SysBase, 8, Exec)
#endif

#ifndef TaggedOpenLibrary
#define TaggedOpenLibrary(tag) \
    AROS_LC1(APTR, TaggedOpenLibrary, \
    AROS_LCA(LONG, tag, D0), \
    struct ExecBase *, SysBase, 135, Exec)
#endif

#endif /* _EXEC_PDEFS_H */
