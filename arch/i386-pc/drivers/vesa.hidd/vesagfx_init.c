/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

#include "hardware.h"
#include "vesagfxclass.h"

#include LC_LIBDEFS_FILE

#include <aros/debug.h>

OOP_AttrBase HiddPCIDeviceAttrBase;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PCIDevice,	&HiddPCIDeviceAttrBase	},
	{ NULL, NULL }
};

static int PCVesa_Init(LIBBASETYPEPTR LIBBASE)
{
    struct VesaGfx_staticdata *xsd = &LIBBASE->vsd;

    if (!OOP_ObtainAttrBases(abd))
	return FALSE;

    InitSemaphore(&xsd->framebufferlock);
    InitSemaphore(&xsd->HW_acc);

    if (initVesaGfxHW(&xsd->data))
    {
	D(bug("[VESA] Init: Everything OK\n"));
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(PCVesa_Init, 0)
