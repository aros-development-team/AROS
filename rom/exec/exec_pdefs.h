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
#   include <dos/dos.h>
#endif

/*
    Defines
*/
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

#define TaggedOpenLibrary(tag) \
    AROS_LC1(APTR, TaggedOpenLibrary, \
    AROS_LCA(LONG, tag, D0), \
    struct ExecBase *, SysBase, 135, Exec)


#endif /* _EXEC_PDEFS_H */
