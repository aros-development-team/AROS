/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <aros/libcall.h>
#include <aros/debug.h>
#include <libcore/base.h>
#include <libcore/compiler.h>

#include <proto/exec.h>
#include <proto/alib.h>

struct Library *aroscbase;
struct ExecBase *SysBase;

ULONG SAVEDS STDARGS FreeType2_L_InitLib (struct LibHeader *Freetype2Base)
{
    D(bug("Inside Init func of freetype2.library\n"));

    SysBase = Freetype2Base->lh_SysBase;
    
    if (!(aroscbase = OpenLibrary("arosc.library",41)))
        return FALSE;
  
    return TRUE;
}

void  SAVEDS STDARGS FreeType2_L_ExpungeLib(struct LibHeader *Freetype2Base)
{
    D(bug("Inside Expunge func of freetype2.library\n"));

    CloseLibrary(aroscbase);
}
