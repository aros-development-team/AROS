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
#undef SysBase
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

#undef GfxBase
#undef UtilityBase

#ifdef CREATE_ROM
#	define SysBase LIBBASE->lb_SysBase
#else
	struct ExecBase * SysBase; /* global variable */
	struct GfxBase * GfxBase; /* unfortunatley need it for AROS to link!!*/
#endif

ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR LIBBASE)
{
  InitSemaphore(&LIBBASE->lb_MemLock);
 
  if (!LIBBASE->lb_ClipRectPool)
     LIBBASE->lb_ClipRectPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, sizeof(struct ClipRect) * 50, sizeof(struct ClipRect) * 50);

  if (NULL == LIBBASE->lb_GfxBase)
    LIBBASE->lb_GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",0);

#ifndef CREATE_ROM
  SysBase = lh->lb_SysBase;
  GfxBase = lh->lb_GfxBase;
#endif

  if (NULL == LIBBASE->lb_UtilityBase)
     LIBBASE->lb_UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library",0);
  
  if (!LIBBASE->lb_GfxBase || !LIBBASE->lb_UtilityBase || !LIBBASE->lb_ClipRectPool)
  {
    if (LIBBASE->lb_GfxBase)
    {
      CloseLibrary((struct Library *)LIBBASE->lb_GfxBase);
      LIBBASE->lb_GfxBase = NULL;
    }
    if (LIBBASE->lb_UtilityBase)
    {
      CloseLibrary((struct Library *)LIBBASE->lb_UtilityBase);
      LIBBASE->lb_UtilityBase = NULL;
    }
    if (LIBBASE->lb_ClipRectPool)
    {
      DeletePool(LIBBASE->lb_ClipRectPool);
      LIBBASE->lb_ClipRectPool = NULL;
    }
    return FALSE;
  }
  
  return TRUE;    
}
