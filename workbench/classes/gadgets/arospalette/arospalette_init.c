/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: AROSPalette initialization code.
    Lang: English.
*/

#include <stddef.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include "arospalette_intern.h"


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

/* to avoid removing the gfxhiddclass from memory add #define NOEXPUNGE */

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define SysBase      (LC_SYSBASE_FIELD(lh))

#define AROSPaletteBase lh

/* Predeclaration */
struct IClass *InitPaletteClass(struct PaletteBase_intern *);

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{


    if (!GfxBase)
    	GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return(NULL);

kprintf("apal: gfx inited\n");
    if (!UtilityBase)
	UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
	return(NULL);
kprintf("apal: utility inited\n");
    if (!IntuitionBase)
    	IntuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase)
	return (NULL);
kprintf("apal: intui inited\n");

    /* ------------------------- */
    /* Create the class itself */

    if (!lh->classptr)
    	lh->classptr = InitPaletteClass(lh);
    if (!lh->classptr)
    	return (NULL);
kprintf("apal: class inited\n");
    /* ------------------------- */

    /* You would return NULL if the init failed. */
    return TRUE;

}


VOID SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    if (lh->classptr)
    {
	RemoveClass(lh->classptr);
	FreeClass(lh->classptr);
	lh->classptr = NULL;
    }
    kprintf("apal: class removed\n");

    /* CloseLibrary() checks for NULL-pointers */
    CloseLibrary(UtilityBase);
    UtilityBase = NULL;
    
    kprintf("apal: utility closed\n");

    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;
    
    kprintf("apal: gfx closed\n");
    
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
    kprintf("apal: intui closed\n");
    
    return;

}

