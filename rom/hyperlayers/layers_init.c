/*
    (C) 1995-97 AROS - The Amiga Research OS
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
#define LC_SYSBASE_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->lb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->lb_SegList)
#define LC_RESIDENTNAME 	Layers_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		60
#define LC_LIBBASESIZE		sizeof (struct LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	struct LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct LIBBASETYPEPTR)(lib))->lb_LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

struct ExecBase * SysBase; /* global variable */
struct GfxBase * GfxBase;

#ifdef UtilityBase
#undef UtilityBase
#endif

ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
  SysBase = lh->lb_SysBase;
  
  InitSemaphore(&lh->lb_MemLock);
 
  if (!lh->lb_ClipRectPool)
     lh->lb_ClipRectPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC, sizeof(struct ClipRect) * 50, sizeof(struct ClipRect) * 50);

  if (!GfxBase)
    GfxBase = (struct GfxBase *) OpenLibrary("graphics.library",0);

  if (NULL == lh->lb_UtilityBase)
     lh->lb_UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library",0);
  
  if (!GfxBase || !lh->lb_UtilityBase || !lh->lb_ClipRectPool)
  {
    if (GfxBase)
    {
      CloseLibrary((struct Library *)GfxBase);
      GfxBase = NULL;
    }
    if (lh->lb_UtilityBase)
    {
      CloseLibrary((struct Library *)lh->lb_UtilityBase);
      lh->lb_UtilityBase = NULL;
    }
    if (lh->lb_ClipRectPool)
    {
      DeletePool(lh->lb_ClipRectPool);
      lh->lb_ClipRectPool = NULL;
    }
    return FALSE;
  }
  
  return TRUE;    
}

