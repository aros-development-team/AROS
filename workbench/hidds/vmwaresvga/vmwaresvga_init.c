/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VMWare SVGA Hidd initialisation code
    Lang: english
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <hidd/gfx.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "vmwaresvga_intern.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase HiddPixFmtAttrBase;	// = 0;
static OOP_AttrBase HiddPCIDeviceAttrBase;

static struct OOP_ABDescr abd[] =
{
    { IID_Hidd_PixFmt,  &HiddPixFmtAttrBase },
    { NULL,             NULL                }
};

AROS_UFH3(void, VMWSVGAEnumerator,
    AROS_UFHA(struct Hook *,    hook,           A0),
    AROS_UFHA(OOP_Object *,     pciDevice,      A2),
    AROS_UFHA(APTR,             message,        A1))
{
    AROS_USERFUNC_INIT

    struct VMWareSVGA_staticdata *xsd = (struct VMWareSVGA_staticdata *)hook->h_Data;
    IPTR io_base, fb_base, mmio_base, INTLine;
    IPTR ProductID, VendorID, SubClass;

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_SubClass, &SubClass);

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &io_base);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1, &fb_base);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &mmio_base);
    xsd->data.iobase = (APTR)io_base;
    xsd->data.vrambase = (APTR)fb_base;
    xsd->data.mmiobase = (APTR)mmio_base;
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &INTLine);
    xsd->data.hwint = (ULONG)INTLine;

    D(bug("[vmwaresvga.hidd] %s: VMWare SVGA device %04x\n", __func__, ProductID);)

    if (ProductID == DEVICE_VMWARE0710)
    {
        xsd->data.indexReg = SVGA_LEGACY_BASE_PORT + SVGA_INDEX_PORT * sizeof(ULONG);
        xsd->data.valueReg = SVGA_LEGACY_BASE_PORT + SVGA_VALUE_PORT * sizeof(ULONG);

        D(bug("[vmwaresvga.hidd] %s: Found VMWare SVGA 0710 device\n", __func__);)
        xsd->card = pciDevice;
    }
    else if (ProductID == DEVICE_VMWARE0405)
    {
        xsd->data.indexReg = io_base + SVGA_INDEX_PORT;
        xsd->data.valueReg = io_base + SVGA_VALUE_PORT;

        D(bug("[vmwaresvga.hidd] %s: Found VMWare SVGA 0405 device\n", __func__);)
        xsd->card = pciDevice;
    }

    AROS_USERFUNC_EXIT
}

STATIC BOOL findCard(struct VMWareSVGA_staticdata *xsd)
{
    struct Hook findHook = {
        h_Entry:        (IPTR (*)())VMWSVGAEnumerator,
        h_Data:         xsd,
    };

    struct TagItem Requirements[] = 
        {
            {tHidd_PCI_VendorID,    VENDOR_VMWARE   },
            {tHidd_PCI_Class,       3               }, /* Display */
            {tHidd_PCI_Interface,   0               },
            {TAG_DONE,              0UL             }
        };

    HIDD_PCI_EnumDevices(xsd->pcihidd, &findHook, (struct TagItem *)&Requirements);

    if (xsd->card)
    {
        if (!initVMWareSVGAHW(&xsd->data, xsd->card))
        {
            D(bug("[vmwaresvga.hidd] %s: Unsupported VMWare SVGA device found - skipping\n", __func__);)
            xsd->card = NULL;
        }
    }
    return (xsd->card) ? TRUE : FALSE;
}

static int VMWareSVGA_Init(LIBBASETYPEPTR LIBBASE)
{
    struct VMWareSVGA_staticdata *xsd = &LIBBASE->vsd;

    xsd->VMWareSVGACyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0);
    if (xsd->VMWareSVGACyberGfxBase == NULL)
        goto failure;

    xsd->VMWareSVGAKernelBase = OpenResource("kernel.resource");
    if (xsd->VMWareSVGAKernelBase == NULL)
        goto failure;

    if (!OOP_ObtainAttrBases(abd))
        goto failure;

    xsd->basebm = OOP_FindClass(CLID_Hidd_BitMap);
    xsd->basegallium = OOP_FindClass(CLID_Hidd_Gallium);

    xsd->pcihidd = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (xsd->pcihidd == NULL)
        goto failure;

    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (HiddPCIDeviceAttrBase == 0)
        goto failure;

    xsd->hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);
    if (xsd->hiddGalliumAB == 0)
        goto failure;

    if (!findCard(xsd))
        goto failure;

    D(bug("[vmwaresvga.hidd] %s: Suitable adaptor found\n", __func__);)
    return TRUE;

failure:
    D(bug("[vmwaresvga.hidd] %s: No suitable adaptors found\n", __func__);)

    if (xsd->VMWareSVGACyberGfxBase)
        CloseLibrary(xsd->VMWareSVGACyberGfxBase);

    if (xsd->hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    if (HiddPCIDeviceAttrBase != 0)
    {
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
        HiddPCIDeviceAttrBase = 0;
    }

    if (xsd->pcihidd != NULL)
    {
        OOP_DisposeObject(xsd->pcihidd);
        xsd->pcihidd = NULL;
    }

    OOP_ReleaseAttrBases(abd);

    return FALSE;
}

ADD2INITLIB(VMWareSVGA_Init, 0)

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
