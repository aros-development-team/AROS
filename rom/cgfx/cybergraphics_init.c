/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CGFX Library
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE

#include <proto/exec.h>
#include <proto/oop.h>
#include <utility/utility.h>

#include "cybergraphics_intern.h"
#include "libdefs.h"



#ifdef SysBase
#   undef SysBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct IntCGFXBase *)(lib))->sysbase)
#define LC_SEGLIST_FIELD(lib)   (((struct IntCGFXBase *)(lib))->seglist)
#define LC_RESIDENTNAME 	Cybergraphics_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		8
#define LC_LIBBASESIZE		sizeof(struct IntCGFXBase)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct IntCGFXBase *)(lib))->libnode)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>


#define SysBase (GetCGFXBase(CyberGfxBase)->sysbase)


static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{

    UtilityBase = OpenLibrary (UTILITYNAME, 0);
    if (UtilityBase)
    {
    	OOPBase = OpenLibrary("oop.library", 0);
	if (OOPBase)
	{
#undef GfxBase
	    GetCGFXBase(LIBBASE)->gfxbase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	    if (GetCGFXBase(LIBBASE)->gfxbase)
	    {
		return TRUE;
	    }
	    CloseLibrary(OOPBase);
	}
	CloseLibrary(UtilityBase);
    }
    
    return (FALSE);
}
