/*
    (C) 2000 AROS - The Amiga Research OS
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

#include "colorwheel_intern.h"
#include "libdefs.h"


#undef SysBase

/* Customize libheader.c */
#define LC_LIBBASESIZE  sizeof(LIBBASETYPE)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

#include <libcore/libheader.c>


/* Global Data */
/* struct ExecBase *SysBase; */

#define ColorWheelBase ((LIBBASETYPEPTR) lh)


ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    D(bug("Inside initfunc of colorwheel.gadget\n"));

#if 0
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

    /* ------------------------- */
    /* Create the class itself */

    if (!ColorWheelBase->classptr)
    	ColorWheelBase->classptr = InitColorWheelClass (ColorWheelBase);
    if (!ColorWheelBase->classptr)
    	return NULL;

    /* ------------------------- */
#endif
    /* You would return NULL if the init failed. */
    return TRUE;
}

#if 0
ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
}

void SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR lh)
{
}
#endif

void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
#if 0
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
#endif
}
