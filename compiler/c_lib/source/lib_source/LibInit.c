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
#include "compiler.h"
#include "libdefs.h"
#include "intern.h"

#ifndef _AROS
#   define INTUITIONNAME "intuition.library" /* AROS defines this */
#else
#   define AROS_ALMOST_COMPATIBLE /* INTUITIONNAME */
#   include <intuition/intuitionbase.h> /* INTUITIONNAME */
#endif


struct ExecBase      *SysBase	    = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase	     *GfxBase	    = NULL;

char ALIGNED ExLibName [] = LIBNAME;
char ALIGNED ExLibID   [] = VERSION;
char ALIGNED Copyright [] = "(C)opyright 1996-97 by Andreas R. Kleinert. All rights reserved.";

extern APTR EndResident; /* below */

struct Resident ALIGNED ROMTag =     /* do not change */
{
    RTC_MATCHWORD,
    &ROMTag,
    &EndResident,
    RTF_AUTOINIT,
    LIBVERSION,
    NT_LIBRARY,
    0,
    &ExLibName[0],
    &ExLibID[0],
    (APTR) &InitTab
};

APTR EndResident;

struct MyDataInit		       /* do not change */
{
    UWORD ln_Type_Init;      UWORD ln_Type_Offset;	UWORD ln_Type_Content;
    UBYTE ln_Name_Init;      UBYTE ln_Name_Offset;	ULONG ln_Name_Content;
    UWORD lib_Flags_Init;    UWORD lib_Flags_Offset;	UWORD lib_Flags_Content;
    UWORD lib_Version_Init;  UWORD lib_Version_Offset;	UWORD lib_Version_Content;
    UWORD lib_Revision_Init; UWORD lib_Revision_Offset; UWORD lib_Revision_Content;
    UBYTE lib_IdString_Init; UBYTE lib_IdString_Offset; ULONG lib_IdString_Content;
    ULONG ENDMARK;
}
DataTab =
{
    INITBYTE(OFFSET(Node,         ln_Type),      NT_LIBRARY),
    0x80, (UBYTE) OFFSET(Node,    ln_Name),      (ULONG) &ExLibName[0],
    INITBYTE(OFFSET(Library,      lib_Flags),    LIBF_SUMUSED|LIBF_CHANGED),
    INITWORD(OFFSET(Library,      lib_Version),  LIBVERSION),
    INITWORD(OFFSET(Library,      lib_Revision), LIBREVISION),
    0x80, (UBYTE) OFFSET(Library, lib_IdString), (ULONG) &ExLibID[0],
    (ULONG) 0
};

 /* Libraries not shareable between Processes or libraries messing
    with RamLib (deadlock and crash) may not be opened here - open/close
    these later locally and or maybe close them fromout L_CloseLibs()
    when expunging !
 */

ULONG SAVEDS STDARGS L_OpenLibs(LIBBASETYPEPTR exb)
{
    SysBase = exb->exb_SysBase;

    IntuitionBase = (struct IntuitionBase *) OpenLibrary(INTUITIONNAME, 37);
    if(!IntuitionBase) return(FALSE);

    GfxBase = (struct GfxBase *) OpenLibrary(GRAPHICSNAME, 37);
    if(!GfxBase) return(FALSE);

    LIBBASE->exb_SysBase       = SysBase;

    LIBBASE->exb_IntuitionBase = IntuitionBase;
    LIBBASE->exb_GfxBase       = GfxBase;

    return(TRUE);
}

void SAVEDS STDARGS L_CloseLibs (LIBBASETYPEPTR exb)
{
    if(GfxBase)       CloseLibrary((struct Library *) GfxBase);
    if(IntuitionBase) CloseLibrary((struct Library *) IntuitionBase);
}
