/*
    Copyright © 2009-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: intelG33_init.c
    Lang: English
*/


#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>

#include <exec/types.h>
#include <exec/lists.h>

#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <hidd/graphics.h>

#include <oop/oop.h>

#include <utility/utility.h>

#include <inttypes.h>

#include LC_LIBDEFS_FILE

#include "intelG33_intern.h"
#include "intelG33_regs.h"

static BOOL Chip_Init(struct staticdata *sd) {
    D(bug("[G33] IntelG33 chip init\n"));

    D(bug("[G33]   ADPA %08x\n",G33_RD_REGL(MMADR, ADPA) ));
    G33_RMW_REGL(MMADR, ADPA, 0x0c00); // Warning! Turns monitor OFF! Just testing mmio register reads/writes...
    D(bug("[G33]   ADPA %08x\n",G33_RD_REGL(MMADR, ADPA) ));

    GMBUS_Init(sd);
    D(bug("[G33]   GMBUS status %04x\n",GMBUS_GetStatus(sd)));
    D(bug("[G33]   GMBUS status %04x\n",GMBUS_GetStatus(sd)));
    return TRUE;
}

AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,	hook,	    A0),
    AROS_UFHA(OOP_Object *,     pciDevice,  A2),
    AROS_UFHA(APTR,             message,    A1))
{
    AROS_USERFUNC_INIT

    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;
    struct staticdata *sd = &LIBBASE->sd;

    IPTR VendorID, ProductID;

    /* Get the ID's */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);


    if(IS_G33(ProductID)){
        D(bug("[G33]   found (%04x:%04x)",VendorID, ProductID));

/*-------- DO NOT CHANGE/REMOVE -------------*/
        bug("\003\n");
/*-------- DO NOT CHANGE/REMOVE -------------*/

        APTR Base0;
        IPTR Base0size;

        OOP_Object *pciDriver;

        sd->Chipset.ProductID = ProductID;
        sd->Chipset.VendorID = VendorID;
        sd->pciG33 = pciDevice;

        struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;

        struct TagItem attrs[] = {
            { aHidd_PCIDevice_isIO,    FALSE },	/* Don't listen IO transactions */
            { aHidd_PCIDevice_isMEM,    TRUE },	/* Listen to MEM transactions */
            { aHidd_PCIDevice_isMaster, TRUE },	/* Can work in BusMaster */
            { TAG_DONE, 0UL },
        };
        OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

        /*
          Read some PCI config registers
        */
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&pciDriver);
        sd->pciDriver = pciDriver;

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0,  (APTR)&Base0);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0,  (APTR)&Base0size);

        /*
          Maps the PCI address space to CPU address space
        */
        mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
        mappci.PCIAddress = Base0;
        mappci.Length = Base0size;
        sd->Chipset.MMADR = (APTR)OOP_DoMethod(pciDriver, (OOP_Msg)msg);

        Chip_Init(sd);

    }else{
        D(bug("[G33]   not supported (%04x:%04x)",VendorID, ProductID));
    }

    AROS_USERFUNC_EXIT
}

static int IntelG33_Init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[G33] IntelG33 hidd init\n"));

    struct staticdata *sd = &LIBBASE->sd;

    sd->memPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);
    if ((sd->memPool == NULL))
        return FALSE;

    struct OOP_ABDescr attrbases[] = {
        { (STRPTR)IID_Hidd_PCIDevice, &HiddPCIDeviceAttrBase },
        { (STRPTR)IID_Hidd_BitMap,    &HiddBitMapAttrBase },
        { (STRPTR)IID_Hidd_PixFmt,    &HiddPixFmtAttrBase },
        { (STRPTR)IID_Hidd_Sync,      &HiddSyncAttrBase },
        { (STRPTR)IID_Hidd_Gfx,       &HiddGfxAttrBase },
        { (STRPTR)IID_Hidd_PlanarBM,  &__IHidd_PlanarBM },
        { (STRPTR)IID_Hidd_G33BitMap, &HiddG33BitMapAttrBase },
        { NULL, NULL }
    };

    {		
        if (OOP_ObtainAttrBases(attrbases)) {
            sd->pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
            if (sd->pci) {
                struct Hook FindHook = {
                    h_Entry:    (IPTR (*)())Enumerator,
                    h_Data:     LIBBASE,
                };
                struct TagItem Requirements[] = {
                    { tHidd_PCI_Interface, 0x00 },
                    { tHidd_PCI_Class,     0x03 },
                    { tHidd_PCI_SubClass,  0x00 },
                    { tHidd_PCI_VendorID,  INTEL_VENDOR_ID },
                    { TAG_DONE, 0UL }
                };

                HIDD_PCI_EnumDevices(LIBBASE->sd.pci, &FindHook, Requirements);

                if (sd->pciG33 != NULL) {
                    D(bug("[G33] IntelG33 hidd init (exit TRUE)\n"));
                    return TRUE;
                }
                OOP_DisposeObject(sd->pci);
            }
            OOP_ReleaseAttrBases(attrbases);
        }
    }

    DeletePool(sd->memPool);
    D(bug("[G33] IntelG33 hidd init (exit FALSE)\n"));
    return FALSE;
}

ADD2INITLIB(IntelG33_Init, 0)
