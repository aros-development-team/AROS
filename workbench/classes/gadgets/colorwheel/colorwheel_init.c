/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ColorWheel initialization code.
    Lang: English.
*/

#define AROS_ALMOST_COMPATIBLE
#include <stddef.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>

#include "colorwheel_intern.h"
#include "libdefs.h"

/***************************************************************************************************/

/* Global IntuitionBase */
#ifdef GLOBAL_INTUIBASE
struct IntuitionBase *IntuitionBase;
#endif

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))


#define ColorWheelBase ((LIBBASETYPEPTR) lh)

/***************************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    D(bug("Inside initfunc of colorwheel.gadget\n"));

    if (!GfxBase)
    	GfxBase = (GraphicsBase *) OpenLibrary ("graphics.library", 37);
    if (!GfxBase)
	return NULL;

    if (!UtilityBase)
	UtilityBase = OpenLibrary ("utility.library", 37);
    if (!UtilityBase)
	return NULL;

    if (!IntuitionBase)
    	IntuitionBase = (IntuiBase *) OpenLibrary ("intuition.library", 37);
    if (!IntuitionBase)
	return NULL;

    if (!CyberGfxBase)
        CyberGfxBase = OpenLibrary ("cybergraphics.library", 0);
    /* we can live even without CyberGfxBase */

    if (!ColorWheelBase->classptr)
    	ColorWheelBase->classptr = InitColorWheelClass (ColorWheelBase);
    if (!ColorWheelBase->classptr)
    	return NULL;

    return TRUE;
}

/***************************************************************************************************/

void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    if (ColorWheelBase->classptr)
    {
	RemoveClass (ColorWheelBase->classptr);
	FreeClass (ColorWheelBase->classptr);
	ColorWheelBase->classptr = NULL;
    }

    /* CloseLibrary() checks for NULL-pointers */
    CloseLibrary (UtilityBase);
    UtilityBase = NULL;
    
    CloseLibrary ((struct Library *) GfxBase);
    GfxBase = NULL;
    
    CloseLibrary ((struct Library *) IntuitionBase);
    IntuitionBase = NULL;

    CloseLibrary(CyberGfxBase);
    CyberGfxBase = NULL;

}

/***************************************************************************************************/
