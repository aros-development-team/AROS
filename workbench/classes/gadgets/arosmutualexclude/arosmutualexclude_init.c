/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROSMutualExclude initialization code.
    Lang: English.
*/

#include <proto/intuition.h>
#include <stddef.h>
#include <proto/exec.h>
#include "arosmutualexclude_intern.h"


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

#define AROSMutualExcludeBase lh

/* Predeclaration */
struct IClass *InitMutualExcludeClass(struct MXBase_intern *);

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{

    if (!GfxBase)
    	GfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase)
	return FALSE;

    if (!UtilityBase)
	UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
	return FALSE;

    if (!IntuitionBase)
    	IntuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase)
	return FALSE;

    if (!DOSBase)
    	DOSBase = OpenLibrary("dos.library", 37);
    if (!DOSBase)
	return FALSE;

    /* ------------------------- */
    /* Create the class itself */

    if (!lh->classptr)
        lh->classptr = InitMutualExcludeClass(LIBBASE);
    if (!lh->classptr)
    	return FALSE;

    /* ------------------------- */


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

    CloseLibrary(UtilityBase);
    UtilityBase = NULL;

    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;

    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;

    CloseLibrary(DOSBase);
    DOSBase = NULL;

    return;

}

void *const LIBFUNCTABLE[]=
{
    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),
    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),
    (void *)-1L
};
