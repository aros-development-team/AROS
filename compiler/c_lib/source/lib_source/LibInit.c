/*
**	$VER: LibInit.c 37.14 (13.8.97)
**
**	Library initializers and functions to be called by StartUp.c
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

#define __USE_SYSBASE	     // perhaps only recognized by SAS/C

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <graphics/gfxbase.h>

#ifdef __MAXON__
#include <clib/exec_protos.h>
#else
#include <proto/exec.h>
#endif
#include <libcore/compiler.h>
#include "libdefs.h"
#include "intern.h"

struct ExecBase      * SysBase	     = NULL;
struct IntuitionBase * IntuitionBase = NULL;
struct GfxBase	     * GfxBase	     = NULL;

#ifndef __AROS__
#   define INTUITIONNAME "intuition.library" /* AROS defines this */
#else
#   include <intuition/intuitionbase.h> /* INTUITIONNAME */
#endif

/* -----------------------------------------------------------------------
    Libraries not shareable between Processes or libraries messing with
    RamLib (deadlock and crash) may not be opened here - open/close these
    later locally and or maybe close them fromout L_CloseLibs() when
    expunging !

    You may bypass this by opening such libs in L_OpenLib(), but then you
    will have to a) protect it by a semaphore and b) make sure, that
    libraries are opened only once (when using global library bases).

    This function is called exactly once during the life cycle of the
    library.
----------------------------------------------------------------------- */
ULONG SAVEDS STDARGS L_InitLib (LIBBASETYPEPTR LIBBASE)
{
    SysBase = LIBBASE->exb_LibHeader.lh_SysBase;

    IntuitionBase = (struct IntuitionBase *) OpenLibrary (INTUITIONNAME, 37);
    if (!IntuitionBase)
	return (FALSE);

    GfxBase = (struct GfxBase *) OpenLibrary (GRAPHICSNAME, 37);
    if (!GfxBase)
	return (FALSE);

    LIBBASE->exb_IntuitionBase = IntuitionBase;
    LIBBASE->exb_GfxBase       = GfxBase;

    return (TRUE);
}

#if 0

/* -----------------------------------------------------------------------
    L_OpenLib:

    This one is called by OpenLib every time someone opens the library.
    We don't use this in our example, and therefore it's commented out.
----------------------------------------------------------------------- */
ULONG SAVEDS STDARGS L_InitLib (LIBBASETYPEPTR LIBBASE)
{
    return TRUE;
}

/* -----------------------------------------------------------------------
    L_CloseLib:

    This one is called by CloseLib every time someone closes the library.
    We don't use this in our example, and therefore it's commented out.

    The function is only called if the library has been opened at least
    once.
----------------------------------------------------------------------- */
void SAVEDS STDARGS L_CloseLib (LIBBASETYPEPTR exb)
{
}
#endif

/* -----------------------------------------------------------------------
    L_ExpungeLib:

    This one by default is called by ExpungeLib, which only can take place
    once and thus per definition is single-threaded.

    When calling this fromout LibClose instead, you will have to protect it
    by a semaphore, since you don't know whether a given
    CloseLibrary(foobase) may cause a Wait(). Additionally, there should be
    protection, that a library won't be closed twice.

    This function is also called if L_InitLib() fails, so you can cleanup
    here.
----------------------------------------------------------------------- */
void SAVEDS STDARGS L_ExpungeLib (LIBBASETYPEPTR exb)
{
    if (GfxBase)
	CloseLibrary ((struct Library *) GfxBase);

    if (IntuitionBase)
	CloseLibrary ((struct Library *) IntuitionBase);
}
