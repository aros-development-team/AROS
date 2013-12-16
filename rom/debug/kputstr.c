/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <proto/debug.h>
#include <proto/exec.h>

VOID KPutStr(CONST_STRPTR string)
{
    while (*string)
        RawPutChar(*string++);
}
