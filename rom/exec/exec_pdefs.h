#ifndef _EXEC_PDEFS_H
#define _EXEC_PDEFS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private defines for exec.library
    Lang: english
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

/*
    Defines
*/
#define Dispatch() \
    AROS_LC0(void, Dispatch, \
    struct ExecBase *, SysBase, 10, Exec)

#define Exception() \
    AROS_LC0(void, Exception, \
    struct ExecBase *, SysBase, 11, Exec)

#define PrepareContext(stackPointer, entryPoint, fallBack) \
    AROS_LC3(APTR, PrepareContext, \
    AROS_LCA(APTR, stackPointer,    A0), \
    AROS_LCA(APTR, entryPoint,      A1), \
    AROS_LCA(APTR, fallBack,        A2), \
    struct ExecBase *, SysBase, 6, Exec)

#define RawIOInit() \
    AROS_LC0(void, RawIOInit, \
    struct ExecBase *, SysBase, 84, Exec)

#define RawMayGetChar() \
    AROS_LC0(LONG, RawMayGetChar, \
    struct ExecBase *, SysBase, 85, Exec)

#define RawPutChar(chr) \
    AROS_LC1(void, RawPutChar, \
    AROS_LCA(UBYTE, chr, D0), \
    struct ExecBase *, SysBase, 86, Exec)

#define Reschedule(task) \
    AROS_LC1(void, Reschedule, \
    AROS_LCA(struct Task *, task, A0), \
    struct ExecBase *, SysBase, 8, Exec)

#define TaggedOpenLibrary(tag) \
    AROS_LC1(APTR, TaggedOpenLibrary, \
    AROS_LCA(LONG, tag, D0), \
    struct ExecBase *, SysBase, 135, Exec)


#endif /* _EXEC_PDEFS_H */
