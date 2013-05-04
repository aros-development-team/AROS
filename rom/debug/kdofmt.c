/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <proto/debug.h>
#include <proto/exec.h>

VOID KDoFmt(CONST_STRPTR formatString, CONST APTR dataStream, CONST APTR putChProc, APTR putChData)
{
    RawDoFmt(formatString, dataStream, putChProc, putChData);
}
