/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RexxSupport initialization code.
    Lang: English
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include "libdefs.h"

/* #define LC_NO_INITLIB */
/* #define LC_NO_EXPUNGELIB */
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

#undef SysBase

#include <libcore/libheader.c>

struct Library *aroscbase;

#undef SysBase

struct ExecBase *SysBase;

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR Freetype2Base)
{
    D(bug("Inside Init func of regina.library\n"));

    SysBase = LC_SYSBASE_FIELD(Freetype2Base);
    
    if (!(aroscbase = OpenLibrary("arosc.library",41)))
        return FALSE;
  
    return TRUE;
}

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR Freetype2Base)
{
    D(bug("Inside Expunge func of regina.library\n"));

    CloseLibrary(aroscbase);
}
