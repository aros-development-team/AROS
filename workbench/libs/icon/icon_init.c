/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Init of icon.library
    Lang: english
*/

/****************************************************************************************/

#define SAVEDS
#include <utility/utility.h> /* this must be before icon_intern.h */

#include "icon_intern.h"
#include "libdefs.h"

#define LC_SYSBASE_FIELD(lib)	(((LIBBASETYPEPTR       )(lib))->ib_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->ib_SegList)
#define LC_RESIDENTNAME		Icon_resident
#ifdef __MORPHOS__
#define LC_RESIDENTFLAGS	RTF_AUTOINIT | RTF_PPC
#else
#define LC_RESIDENTFLAGS	RTF_AUTOINIT
#endif
#define LC_RESIDENTPRI		0
#define LC_LIBBASESIZE		sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_CLOSELIB
#define LC_NO_OPENLIB

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

#undef DOSBase
#undef SysBase

#ifdef __MORPHOS__
    unsigned long __amigappc__ = 1;
#endif

/****************************************************************************************/

struct ExecBase   * SysBase; /* global variable */
struct DosLibrary * DOSBase;

/****************************************************************************************/

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    LONG i;
    
    SysBase = lh->ib_SysBase;

    DOSBase = (struct DosLibrary *) OpenLibrary (DOSNAME, 39);
    if (!DOSBase)
	return FALSE;

    LB(lh)->utilitybase = OpenLibrary (UTILITYNAME, 39);
    if (!LB(lh)->utilitybase)
	return NULL;

    LB(lh)->intuitionbase = (IntuitionBase_T *)OpenLibrary("intuition.library", 39);
    if (!LB(lh)->intuitionbase)
	return NULL;

    /* Following libraries needed only for 3.5 style icons. If
       the libraries cannot be opened, we simply don't support
       3.5 icons */
       
    LB(lh)->iffparsebase = OpenLibrary("iffparse.library", 39);
    LB(lh)->gfxbase      = (GfxBase_T *)OpenLibrary("graphics.library", 39);
    LB(lh)->cybergfxbase = OpenLibrary("cybergraphics.library", 39);

    LB(lh)->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
    LB(lh)->dsh.h_Data  = lh;

    InitSemaphore(&LB(lh)->iconlistlock);
    for(i = 0; i < ICONLIST_HASHSIZE; i++)
    {
    	NewList((struct List *)&LB(lh)->iconlists[i]);
    }
    
    return TRUE;
}

/****************************************************************************************/

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (DOSBase)
	CloseLibrary ((struct Library *)DOSBase);

    if (LB(lh)->gfxbase)
    	CloseLibrary ((struct Library *)LB(lh)->gfxbase);
	
    if (LB(lh)->iffparsebase)
    	CloseLibrary (LB(lh)->iffparsebase);
	
    if (LB(lh)->cybergfxbase)
    	CloseLibrary (LB(lh)->cybergfxbase);
	
    if (LB(lh)->intuitionbase)
	CloseLibrary ((struct Library *)LB(lh)->intuitionbase);

    if (LB(lh)->utilitybase)
	CloseLibrary (LB(lh)->utilitybase);

}

/****************************************************************************************/

