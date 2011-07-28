/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Can this disk boot on your architecture?
    Lang: english
*/

#include <aros/debug.h>
#include <proto/dos.h>

#include "dos_intern.h"

BOOL __dos_IsBootable(struct DosLibrary * DOSBase, BPTR lock)
{
    /* For Amiga m68k, if we can mount it, we can boot it. */
    return TRUE;
}


