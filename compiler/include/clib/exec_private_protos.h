#ifndef CLIB_EXEC_PRIVATE_PROTOS_H
#define CLIB_EXEC_PRIVATE_PROTOS_H

/*
    Copyright © 1995-1997, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private prototypes for exec.library
    Lang: english
*/

AROS_LP0(void, RawIOInit,
    struct ExecBase *, SysBase, 84, Exec)

AROS_LP0(LONG, RawMayGetChar,
    struct ExecBase *, SysBase, 85, Exec)

AROS_LP1(void, RawPutChar,
    AROS_LPA(UBYTE, chr, D0),
    struct ExecBase *, SysBase, 86, Exec)

#endif /* CLIB_EXEC_PRIVATE_PROTOS_H */
