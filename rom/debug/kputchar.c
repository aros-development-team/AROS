/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

#include <proto/debug.h>
#include <proto/exec.h>

VOID KPutChar(LONG ch)
{
    RawPutChar((UBYTE)ch);
}
