/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Layers Resident and initialization.
    Lang: english
*/
#include "layers_intern.h"
#include "libdefs.h"
#include <graphics/gfxbase.h>

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->lb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->lb_SegList)
#define LC_RESIDENTNAME 	layers_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		103
#define LC_LIBBASESIZE		sizeof (struct LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	struct LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct LIBBASETYPEPTR)(lib))->lb_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

#undef ExecBase
struct ExecBase * SysBase; /* global variable */
struct GfxBase * GfxBase;

ULONG SAVEDS L_InitLib (struct LIBBASETYPEPTR lh)
{
  SysBase = lh->lb_SysBase;
  
  if (!GfxBase)
    GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",0);
    
  if (!GfxBase)
    return FALSE;
  
  return TRUE;    
}

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);
} /* L_ExpungeLib */
