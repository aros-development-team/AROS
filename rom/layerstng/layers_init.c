/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Layers Resident and initialization.
    Lang: english
*/
#include "layers_intern.h"
#include "libdefs.h"
#include <graphics/gfxbase.h>

#ifdef SysBase
#   undef SysBase
#endif
#ifdef ExecBase
#   undef ExecBase
#endif


/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->lb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR)(lib))->lb_SegList)
#define LC_RESIDENTNAME 	Layers_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		60
#define LC_LIBBASESIZE		sizeof (LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->lb_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

struct ExecBase *SysBase; /* global variable */
struct GfxBase *GfxBase;
struct Library *UtilityBase;

ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->lb_SysBase;

    if (!GfxBase) GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0);
    if (!GfxBase) return FALSE;

    if (!UtilityBase) UtilityBase = OpenLibrary("utility.library", 0);
    if (!UtilityBase) return FALSE;
    
    return TRUE;    
}

