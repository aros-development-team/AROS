/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: kbd Hidd for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <utility/utility.h>

#include "kbd.h"


#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME		kbdHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct kbdbase
{
    struct Library library;
    struct ExecBase *sysbase;
    BPTR	seglist;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0 
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

#undef SysBase

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct kbd_staticdata *xsd;
    xsd = AllocMem( sizeof (struct kbd_staticdata), MEMF_CLEAR|MEMF_PUBLIC );
    if (xsd)
    {
        xsd->sysbase = SysBase;
	
	InitSemaphore( &xsd->sema );
	
        xsd->oopbase = OpenLibrary(AROSOOP_NAME, 0);
	if (xsd->oopbase)
	{
	    xsd->utilitybase = OpenLibrary(UTILITYNAME, 37);
	    if (xsd->utilitybase)
	    {
	        if (init_kbdclass(xsd))
	        {
		    return TRUE;		
		}
		CloseLibrary(xsd->utilitybase);
	    }
	    CloseLibrary(xsd->oopbase);
	}
	FreeMem(xsd, sizeof (struct kbd_staticdata));
    }
    return FALSE;
}
