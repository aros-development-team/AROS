#ifndef _EXEC_PDEFS_H
#define _EXEC_PDEFS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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
//#   include <dos/dos.h>
#endif

/*
    Defines
*/
#define Dispatch() \
    LP0NR(40, Dispatch, struct ExecBase *, SysBase)

#define Exception() \
    LP0NR(44, Exception, struct ExecBase *, SysBase)

#define PrepareContext(stackPointer, entryPoint, fallBack) \
    LP3(24, APTR, PrepareContext, APTR, stackPointer, APTR, entryPoint, APTR, fallBack, \
	struct ExecBase *, SysBase)

#define RawIOInit() \
    LP0NR(336, RawIOInit, struct ExecBase *, SysBase)

#define RawMayGetChar() \
    LP0(340, LONG, RawMayGetChar, struct ExecBase *, SysBase)

#define Reschedule(task) \
    LP1NR(32, Reschedule, struct Task *, task, struct ExecBase *, SysBase)

#define TaggedOpenLibrary(tag) \
    LP1(540, APTR, TaggedOpenLibrary, Long, tag, struct ExecBase *, SysBase)

#endif /* _EXEC_PDEFS_H */
