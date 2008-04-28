/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vmware gfx Hidd for standalone i386 AROS
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
#include "vmwaregfxclass.h"
#include "svga_reg.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;
static OOP_AttrBase HiddPCIDeviceAttrBase;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

AROS_UFH3(void, VMEnumerator,
    AROS_UFHA(struct Hook *,    hook,           A0),
    AROS_UFHA(OOP_Object *,     pciDevice,      A2),
    AROS_UFHA(APTR,             message,        A1))
{
    AROS_USERFUNC_INIT
    struct VMWareGfx_staticdata *xsd = (struct VMWareGfx_staticdata *)hook->h_Data;
    
    IPTR ProductID, VendorID, SubClass;
    
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_SubClass, &SubClass);

    bug("[VMWare] vmwareSVGA %04x device\n", ProductID);
    
    if (ProductID == DEVICE_VMWARE0710)
    {
        xsd->data.indexReg = SVGA_LEGACY_BASE_PORT + SVGA_INDEX_PORT*sizeof(ULONG);
        xsd->data.valueReg = SVGA_LEGACY_BASE_PORT + SVGA_VALUE_PORT*sizeof(ULONG);

        bug("[VMWare] Found vmwareSVGA 0710 device\n");
	xsd->card = pciDevice;
    }
    else if (ProductID == DEVICE_VMWARE0405)
    {
        IPTR mmio;
        
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &mmio);
        
        xsd->data.indexReg = mmio+SVGA_INDEX_PORT;
        xsd->data.valueReg = mmio+SVGA_VALUE_PORT;

        bug("[VMWare] Found vmwareSVGA 0405 device\n");
	xsd->card = pciDevice;
    }

    AROS_USERFUNC_EXIT
}

STATIC BOOL findCard(struct VMWareGfx_staticdata *xsd) {
    struct Hook findHook = {
        h_Entry:        (IPTR (*)())VMEnumerator,
        h_Data:         xsd,
    };
    
    struct TagItem Requirements[] = {
	{tHidd_PCI_VendorID, VENDOR_VMWARE},
	{tHidd_PCI_Class,       3}, /* Display */
	{tHidd_PCI_Interface,   0},
	{TAG_DONE,            0UL}
    };
    
    HIDD_PCI_EnumDevices(xsd->pcihidd, &findHook, (struct TagItem *)&Requirements);

    if (xsd->card)
    {
	if (!initVMWareGfxHW(&xsd->data, xsd->card))
	{
		bug("[VMWare] Found unsupported vmware svga device, aborting\n");
		xsd->card = NULL;
	}
    }
    return (xsd->card) ? TRUE : FALSE;
}

static int VMWareGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    struct VMWareGfx_staticdata *xsd = &LIBBASE->vsd;

    if (!OOP_ObtainAttrBases(abd))
	goto failure;

    xsd->pcihidd = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (xsd->pcihidd == NULL)
	goto failure;

    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (HiddPCIDeviceAttrBase == 0)
	goto failure;
    
    if (!findCard(xsd))
	goto failure;
    
    D(bug("Found VMWareSVGA\n"));
    return TRUE;

failure:
    D(bug("VMWareSVGA Init failesd\n"));
    if (HiddPCIDeviceAttrBase != 0)
    {
	OOP_ReleaseAttrBase(HiddPCIDeviceAttrBase);
	HiddPCIDeviceAttrBase = 0;
    }
    
    if (xsd->pcihidd != NULL)
    {
	OOP_DisposeObject(xsd->pcihidd);
	xsd->pcihidd;
    }
    
    OOP_ReleaseAttrBases(abd);
    
    return FALSE;
}

ADD2INITLIB(VMWareGfx_Init, 0)

