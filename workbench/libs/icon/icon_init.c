/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

*/

#define SAVEDS
#include <utility/utility.h> /* this must be before icon_intern.h */

#include "icon_intern.h"
#include "identify.h"

#include LC_LIBDEFS_FILE

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

#   define DEBUG 0
#   include <aros/debug.h>

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
    // FIXME: doesn't free resources if init fails...
    
    if (!(DOSBase = (struct DosLibrary *) OpenLibrary (DOSNAME, 39)))
    {
	return FALSE;
    }
    
    if (!(LB(lh)->ib_UtilityBase = OpenLibrary (UTILITYNAME, 39)))
    {
        return FALSE;
    }
    
    if (!(LB(lh)->ib_IntuitionBase = OpenLibrary("intuition.library", 39)))
    {
        return FALSE;
    }
    
    if (!(LB(lh)->ib_DataTypesBase = OpenLibrary("datatypes.library", 41)))
    {
        return FALSE;
    }
    
    /* Initialize memory pool ----------------------------------------------*/
    if (!(LB(lh)->ib_MemoryPool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 8194, 8194)))
    {
        return FALSE;
    }
    
    /* 
        Following libraries needed only for 3.5 style icons. If the libraries
        cannot be opened, we simply don't support 3.5 icons.
    */
       
    LB(lh)->ib_IFFParseBase = OpenLibrary("iffparse.library", 39);
    LB(lh)->ib_GfxBase      = OpenLibrary("graphics.library", 39);
    LB(lh)->ib_CyberGfxBase = OpenLibrary("cybergraphics.library", 39);

    LB(lh)->dsh.h_Entry = (void *)AROS_ASMSYMNAME(dosstreamhook);
    LB(lh)->dsh.h_Data  = lh;

    InitSemaphore(&LB(lh)->iconlistlock);
    for(i = 0; i < ICONLIST_HASHSIZE; i++)
    {
    	NewList((struct List *)&LB(lh)->iconlists[i]);
    }

    /* Setup default identify hook -----------------------------------------*/
    LB(lh)->ib_DefaultIdentifyHook.h_Entry = (HOOKFUNC) FindDefaultIcon;
    LB(lh)->ib_DefaultIdentifyHook.h_Data  = LB(lh)->ib_DataTypesBase;
    
    /* Setup default global settings ---------------------------------------*/
    LB(lh)->ib_Screen               = NULL; // FIXME: better default
    LB(lh)->ib_Precision            = PRECISION_ICON;
    LB(lh)->ib_EmbossRectangle.MinX = 0; // FIXME: better default
    LB(lh)->ib_EmbossRectangle.MaxX = 0; 
    LB(lh)->ib_EmbossRectangle.MinY = 0; 
    LB(lh)->ib_EmbossRectangle.MaxY = 0; 
    LB(lh)->ib_Frameless            = TRUE;
    LB(lh)->ib_IdentifyHook         = NULL; // FIXME: better default
    LB(lh)->ib_MaxNameLength        = 25;
    LB(lh)->ib_NewIconsSupport      = TRUE;
    LB(lh)->ib_ColorIconSupport     = TRUE;
    
    return TRUE;
}

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (DOSBase)
	CloseLibrary((struct Library *) DOSBase);

    if (LB(lh)->ib_GfxBase)       CloseLibrary(LB(lh)->ib_GfxBase);
    if (LB(lh)->ib_IFFParseBase)  CloseLibrary(LB(lh)->ib_IFFParseBase);
    if (LB(lh)->ib_CyberGfxBase)  CloseLibrary(LB(lh)->ib_CyberGfxBase);
    if (LB(lh)->ib_IntuitionBase) CloseLibrary(LB(lh)->ib_IntuitionBase);
    if (LB(lh)->ib_UtilityBase)   CloseLibrary(LB(lh)->ib_UtilityBase);
}
