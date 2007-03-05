/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
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
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "onbitmap.h"
#include "offbitmap.h"
#include "hardware.h"
#include "vesagfxclass.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 1
#include <aros/debug.h>

static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PCIDevice,	&HiddPCIDeviceAttrBase	},
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

static int PCVesa_Init(LIBBASETYPEPTR LIBBASE)
{
    struct VesaGfx_staticdata *xsd = &LIBBASE->vsd;

    if (!OOP_ObtainAttrBases(abd))
	return FALSE;

#if BUFFERED_VRAM
    InitSemaphore(&xsd->framebufferlock);
#endif
    
    if (initVesaGfxHW(&xsd->data))
    {
	D(bug("[VESA] Init: Everything OK\n"));
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(PCVesa_Init, 0)
