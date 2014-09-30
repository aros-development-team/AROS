/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include "debug.h"
#include "error.h"
#include "hash.h"

void Purify_CheckRead (char * addr, long size)
{
#if DEBUG
    printf ("Purify_CheckRead (addr=%p, size=%ld)\n", addr, size);
#endif

    if (!Purify_CheckMemoryAccess (addr, size, PURIFY_MemAccess_Read))
	Purify_PrintAccessError ("Read", addr, size);
}

void Purify_CheckWrite (char * addr, long size)
{
#if DEBUG
    printf ("Purify_CheckWrite (addr=%p, size=%ld)\n", addr, size);
#endif

    if (!Purify_CheckMemoryAccess (addr, size, PURIFY_MemAccess_Write))
	Purify_PrintAccessError ("Write", addr, size);

#if DEBUG
    Purify_PrintMemory ();
#endif
}


