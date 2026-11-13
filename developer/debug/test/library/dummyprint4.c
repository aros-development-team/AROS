/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dummy.h>

#include LC_LIBDEFS_FILE

/* Member of userel.library */

LONG DummyPrint4(STACKLONG a, STACKLONG b, STACKLONG c, STACKLONG d)
{
    return printx(4, a, b, c, d);
}
