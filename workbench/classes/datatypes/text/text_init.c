/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Text.datatype initialization code.
    Lang: English.
*/

#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>

#include "text_intern.h"
#include LC_LIBDEFS_FILE

/***************************************************************************************************/

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (lib->sysbase)
#define LC_SEGLIST_FIELD(lib)   (lib->seglist)
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (lib->library)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

#include <libcore/libheader.c>

#undef SysBase

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef	register
#define register

#undef __a6
#define __a6

/***************************************************************************************************/

extern ASM SAVEDS int __UserLibInit( register __a6 struct Library *libbase );
extern ASM SAVEDS void __UserLibCleanup( register __a6 struct Library *libbase );

/***************************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    ULONG result;
    
    //SysBase = sysbase;
    
    D(bug("Inside initfunc of text.datatype\n"));

    if (__UserLibInit((struct Library *)lh) == 0)
    {
        result = TRUE;
    } else {
        result = FALSE;
    }
    
    D(bug("Leaving initfunc of text.datatype. result = %d\n", result));
    
    return result;
}

/***************************************************************************************************/

void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    __UserLibCleanup((struct Library *)lh);
}

/***************************************************************************************************/
