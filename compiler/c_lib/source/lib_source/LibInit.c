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

#ifndef _AROS
#   define INTUITIONNAME "intuition.library" /* AROS defines this */
#else
#   define AROS_ALMOST_COMPATIBLE /* INTUITIONNAME */
#   include <intuition/intuitionbase.h> /* INTUITIONNAME */
#endif

 /* Libraries not shareable between Processes or libraries messing
    with RamLib (deadlock and crash) may not be opened here - open/close
    these later locally and or maybe close them fromout L_CloseLibs()
    when expunging !
 */

ULONG SAVEDS STDARGS L_OpenLibs (LIBBASETYPEPTR LIBBASE)
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

void SAVEDS STDARGS L_CloseLibs (LIBBASETYPEPTR exb)
{
    if (GfxBase)       CloseLibrary ((struct Library *) GfxBase);
    if (IntuitionBase) CloseLibrary ((struct Library *) IntuitionBase);
}
