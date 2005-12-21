/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vesa gfx Hidd for standalone i386 AROS
    Lang: english
*/

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "onbitmap.h"
#include "offbitmap.h"
#include "hardware.h"
#include "vesagfxclass.h"
#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->SegList)
#define LC_RESIDENTNAME		vesaGfxHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB

#define NOEXPUNGE

struct VesaGfxBase
{
    struct Library library;
    struct ExecBase *sysBase;
    BPTR	SegList;
};

#include <libcore/libheader.c>

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef SysBase
#undef OOPBase

#define SysBase xsd->sysBase
#define OOPBase xsd->oopBase

static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

static VOID freeclasses(struct VesaGfx_staticdata *xsd);

static BOOL initclasses(struct VesaGfx_staticdata *xsd)
{
    /* Get some attrbases */
    
    if (!OOP_ObtainAttrBases(abd))
	goto failure;

    xsd->vesagfxclass = init_vesagfxclass(xsd);
    if (NULL == xsd->vesagfxclass)
	goto failure;

    xsd->onbmclass = init_vesagfxonbmclass(xsd);
    if (NULL == xsd->onbmclass)
	goto failure;

    xsd->offbmclass = init_vesagfxoffbmclass(xsd);
    if (NULL == xsd->offbmclass)
	goto failure;

    return TRUE;

failure:
    freeclasses(xsd);

    return FALSE;
}

static VOID freeclasses(struct VesaGfx_staticdata *xsd)
{
    if (xsd->vesagfxclass)
	free_vesagfxclass(xsd);

    if (xsd->offbmclass)
	free_vesagfxoffbmclass(xsd);

    if (xsd->onbmclass)
	free_vesagfxonbmclass(xsd);

    OOP_ReleaseAttrBases(abd);

    return;
}

#undef SysBase
#define SysBase (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh)
{
    struct VesaGfx_staticdata *xsd;

    xsd = AllocMem(sizeof(struct VesaGfx_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
    if (xsd)
    {
	xsd->sysBase = SysBase;
	
    #if BUFFERED_VRAM
	InitSemaphore(&xsd->framebufferlock);
    #endif
    
	xsd->oopBase = OpenLibrary(AROSOOP_NAME, 0);
	if (xsd->oopBase)
	{
	    xsd->utilityBase = OpenLibrary(UTILITYNAME, 37);
	    if (xsd->utilityBase)
	    {
		if (initVesaGfxHW(&xsd->data))
		{
		    if (initclasses(xsd))
		    {
			D(bug("[VESA] Init: Everything OK\n"));
			return TRUE;
		    }
		}
		CloseLibrary(xsd->utilityBase);
	    }
	    CloseLibrary(xsd->oopBase);
	}
	FreeMem(xsd, sizeof (struct VesaGfx_staticdata));
    }
    return FALSE;
}

