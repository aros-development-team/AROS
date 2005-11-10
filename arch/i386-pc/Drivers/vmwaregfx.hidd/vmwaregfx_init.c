/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: vmware gfx Hidd for standalone i386 AROS
    Lang: english
*/

#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/utility.h>

#include "onbitmap.h"
#include "offbitmap.h"
#include "hardware.h"
#include "vmwaregfxclass.h"
#include "svga_reg.h"
#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysBase)
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->SegList)
#define LC_RESIDENTNAME		vmwareGfxHidd_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		9
#define LC_LIBBASESIZE          sizeof(LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->library)

#define LC_NO_INITLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

struct VMWareGfxBase
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
OOP_AttrBase HiddPCIDeviceAttrBase;

static struct OOP_ABDescr abd[] = {
	{ IID_Hidd_PixFmt,	&HiddPixFmtAttrBase	},
	{ NULL, NULL }
};

static VOID freeclasses(struct VMWareGfx_staticdata *xsd);

static BOOL initclasses(struct VMWareGfx_staticdata *xsd) {

    /* Get some attrbases */
    
	if (!OOP_ObtainAttrBases(abd))
    	goto failure;

    xsd->vmwaregfxclass = init_vmwaregfxclass(xsd);
    if (NULL == xsd->vmwaregfxclass)
    	goto failure;

    xsd->onbmclass = init_vmwaregfxonbmclass(xsd);
    if (NULL == xsd->onbmclass)
    	goto failure;

    xsd->offbmclass = init_vmwaregfxoffbmclass(xsd);
    if (NULL == xsd->offbmclass)
    	goto failure;
#if 0
    xsd->mouseclass = init_mouseclass(xsd);
    if (NULL == xsd->mouseclass)
    	goto failure;
#endif
    return TRUE;
        
failure:
    freeclasses(xsd);

    return FALSE;
    
}

static VOID freeclasses(struct VMWareGfx_staticdata *xsd)
{
#if 0
    if (xsd->mouseclass)
    	free_mouseclass(xsd);
#endif
    if (xsd->vmwaregfxclass)
    	free_vmwaregfxclass(xsd);

    if (xsd->offbmclass)
    	free_vmwaregfxoffbmclass(xsd);

    if (xsd->onbmclass)
    	free_vmwaregfxonbmclass(xsd);

    OOP_ReleaseAttrBases(abd);
	
    return;
}


AROS_UFH3(void, Enumerator,
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
    
    if (
    	SubClass == 0x80 && /* other */
	ProductID == DEVICE_VMWARE0710
    )
    {
        xsd->data.indexReg = SVGA_LEGACY_BASE_PORT + SVGA_INDEX_PORT*sizeof(ULONG);
        xsd->data.valueReg = SVGA_LEGACY_BASE_PORT + SVGA_VALUE_PORT*sizeof(ULONG);

        bug("[VMWare] Found vmwareSVGA 0710 device\n");
	xsd->card = pciDevice;
    }
    else if (
		SubClass == 0x00 && /* VGA */
		ProductID == DEVICE_VMWARE0405
    )
    {
        IPTR mmio;
        
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &mmio);
        
        xsd->data.indexReg = mmio+SVGA_INDEX_PORT;
        xsd->data.valueReg = mmio+SVGA_VALUE_PORT;

        bug("[VMWare] Found vmwareSVGA 0405 device\n");
	xsd->card = pciDevice;
    }

    if (xsd->card)
    {
	if (!initVMWareGfxHW(&xsd->data, xsd->card))
	{
		bug("[VMWare] Found unsupported vmware svga device, aborting\n");
		xsd->card = NULL;
	}
    }
    
    AROS_USERFUNC_EXIT
}

STATIC BOOL findCard(struct VMWareGfx_staticdata *xsd) {
    struct Hook findHook = {
        h_Entry:        (IPTR (*)())Enumerator,
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

#undef SysBase
#define SysBase (LC_SYSBASE_FIELD(lh))

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR lh) {
struct VMWareGfx_staticdata *xsd;

	xsd = AllocMem(sizeof(struct VMWareGfx_staticdata), MEMF_CLEAR | MEMF_PUBLIC);
	if (xsd)
	{
		xsd->sysBase = SysBase;
		xsd->oopBase = OpenLibrary(AROSOOP_NAME, 0);
		if (xsd->oopBase)
		{
			xsd->utilityBase = OpenLibrary(UTILITYNAME, 37);
			if (xsd->utilityBase)
			{
				xsd->pcihidd = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
				if (xsd->pcihidd)
				{
                                        HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
                                        if (findCard(xsd))
					{
						D(bug("Found VMWareSVGA\n"));
						if (initclasses(xsd))
						{
							D(bug("Everything OK\n"));
							return TRUE;
						}
					}
				}
				CloseLibrary(xsd->utilityBase);
			}
			CloseLibrary(xsd->oopBase);
		}
		FreeMem(xsd, sizeof (struct VMWareGfx_staticdata));
    }
    return FALSE;
}

