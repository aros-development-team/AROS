/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>

#include <proto/intuition.h>

#include "coolimages_intern.h"
#include "libdefs.h"

/****************************************************************************************/

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (CIB(lib)->sysbase)
#define LC_SEGLIST_FIELD(lib)   (CIB(lib)->seglist)
#define LC_LIBBASESIZE		sizeof(struct CoolImagesBase_intern)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)	(CIB(lib)->library)

/* #define LC_NO_INITLIB    */
/* #define LC_NO_OPENLIB    */
/* #define LC_NO_CLOSELIB   */
/* #define LC_NO_EXPUNGELIB */

#include <libcore/libheader.c>

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

/* Global libbase vars */
#undef IntuitionBase

struct ExecBase      *SysBase;
struct IntuitionBase *IntuitionBase;
struct GfxBase	     *GfxBase;
struct Library	     *CyberGfxBase;
struct UtilityBase   *UtilityBase;

struct ExecBase **SysBasePtr = &SysBase;

#define SysBase			(LC_SYSBASE_FIELD(CoolImagesBase))

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR CoolImagesBase)
{
    D(bug("Inside Init func of coolimages.library\n"));

    *SysBasePtr = SysBase;

    if (!UtilityBase)
        (struct Library *)UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
        return FALSE;

    if (!IntuitionBase)
    	(struct Library *)IntuitionBase = OpenLibrary("intuition.library", 37);
    if (!IntuitionBase)
    	return FALSE;

    if (!GfxBase)
    	(struct Library *)GfxBase = OpenLibrary("graphics.library", 37);
    if (!GfxBase)
    	return FALSE;

    if (!CyberGfxBase)
    	CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
    /* may fail */

    if (!cool_buttonclass)
    {
    	if (!InitCoolButtonClass(CyberGfxBase)) return FALSE;
    	
	cool_buttonclass->cl_ID = COOLBUTTONGCLASS;
	AddClass(cool_buttonclass);
    }
    
    if (!cool_imageclass)
    {
    	if (!InitCoolImageClass(CyberGfxBase)) return FALSE;
	
	cool_imageclass->cl_ID = COOLIMAGECLASS;
	AddClass(cool_imageclass);
    }
    
    return TRUE;
}

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR CoolImagesBase)
{
    D(bug("Inside Open func of coolimages.library\n"));

    return TRUE;
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR CoolImagesBase)
{
    D(bug("Inside Close func of coolimages.library\n"));
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR CoolImagesBase)
{
    D(bug("Inside Expunge func of coolimages.library\n"));

    CleanupCoolImageClass();
    CleanupCoolButtonClass();
    
    /* CloseLibrary() checks for NULL-pointers */
    
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;

    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
    
    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;

    CloseLibrary(CyberGfxBase);
    CyberGfxBase = NULL;
}

/****************************************************************************************/
