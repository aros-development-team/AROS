/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/macros.h>

#include "dos_intern.h"
#include "internalloadseg.h"

BPTR InternalLoadSeg_AOS(BPTR fh,
                         BPTR table,
                         SIPTR * funcarray,
                         SIPTR * stack,
                         struct DosLibrary * DOSBase)
{
    /* AOS style HUNK programs are not supported on x86_64 platforms
     */
    return BNULL;
} /* InternalLoadSeg */
