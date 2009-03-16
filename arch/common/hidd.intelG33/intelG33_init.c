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
#include <aros/bootloader.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/bootloader.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>

#include <hidd/graphics.h>
#include <hidd/irq.h>
#include <hidd/pci.h>
#include <hidd/graphics.h>

#include <oop/oop.h>

#include <utility/utility.h>

#include <inttypes.h>

#include LC_LIBDEFS_FILE

#include "intelG33_intern.h"
#include "intelG33_regs.h"

static void IntelG33_int(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw) {
    D(bug("[G33IRQ] IntelG33 INTERRUPT\n"));
}

static inline __attribute__((always_inline)) ULONG pciReadLong(struct staticdata *sd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg) {
    struct pHidd_PCIDriver_ReadConfigLong __msg = {
	    sd->mid_ReadLong,
	    bus, dev, sub, reg
    }, *msg = &__msg;
    
    return (ULONG)OOP_DoMethod(sd->pciDriver, (OOP_Msg)msg);
}

static inline __attribute__((always_inline)) UWORD pciReadWord(struct staticdata *sd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg) {
    struct pHidd_PCIDriver_ReadConfigWord __msg = {
	    sd->mid_ReadWord,
	    bus, dev, sub, reg
    }, *msg = &__msg;
    
    return (UWORD)OOP_DoMethod(sd->pciDriver, (OOP_Msg)msg);
}

static inline __attribute__((always_inline)) UWORD pciReadByte(struct staticdata *sd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg) {
    struct pHidd_PCIDriver_ReadConfigByte __msg = {
	    sd->mid_ReadByte,
	    bus, dev, sub, reg
    }, *msg = &__msg;
    
    return (UBYTE)OOP_DoMethod(sd->pciDriver, (OOP_Msg)msg);
}

static BOOL Chip_Init(struct staticdata *sd) {
    D(bug("[G33] IntelG33 chip init\n"));

//    D(bug("[G33]   ADPA %08x\n",G33_RD_REGL(MMADR, ADPA) ));
//    G33_SETBMASK_REGL(MMADR, ADPA, 0x0c00); // Warning! Turns monitor OFF! Just testing mmio register reads/writes...
//    D(bug("[G33]   ADPA %08x\n",G33_RD_REGL(MMADR, ADPA) ));
//    G33_CLRBMASK_REGL(MMADR, ADPA, 0x0c00); // Warning! Turns monitor ON! Just testing mmio register reads/writes...
//    D(bug("[G33]   ADPA %08x\n",G33_RD_REGL(MMADR, ADPA) ));

	if ( G33_RD_REGL(MMADR, ADPA) & (1<<31) )
        D(bug("[G33]    Analog display port A enabled\n"));
	if ( G33_RD_REGL(MMADR, DDPBC) & (1<<31) )
        D(bug("[G33]    Digital display port B enabled\n"));;
	if ( G33_RD_REGL(MMADR, DDPCC) & (1<<31) )
        D(bug("[G33]    Digital display port C enabled\n"));;

    D(bug("[G33]   VGACNTRL %08x\n",G33_RD_REGL(MMADR, VGACNTRL) ));
    G33_SETBMASK_REGL(MMADR, VGACNTRL, VGADisable); //Disable VGA
    D(bug("[G33]   VGACNTRL %08x\n",G33_RD_REGL(MMADR, VGACNTRL) ));

    init_GMBus(sd);
//    D(bug("[G33]   GMBUS status %04x\n",status_GMBus(sd))); // Testing hardware "semaphore"
//    D(bug("[G33]   GMBUS status %04x\n",status_GMBus(sd)));
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


    /* Intel doesn't produce any addon gfx cards so it might be pointless to check if allready found intel chip */
    if( (IS_G33(ProductID) & (sd->pciG33 == NULL)) ){
        D(bug("[G33]   found (%04x:%04x)",VendorID, ProductID));

/*-------- DO NOT CHANGE/REMOVE -------------*/
        bug("\003\n");
/*-------- DO NOT CHANGE/REMOVE -------------*/

        APTR Base0, Base1, Base2, Base3;
        IPTR sizeBase0, sizeBase1, sizeBase2, sizeBase3;
        IPTR bus, dev, sub;
        IPTR BSM, MGGC, MSAC, sizeGTT, sizeMemory;

        OOP_Object *pciDriver;

        sd->Chipset.ProductID = ProductID;
        sd->Chipset.VendorID = VendorID;
        sd->pciG33 = pciDevice;

        sizeGTT = 0;
        sizeMemory = 0;

        struct pHidd_PCIDriver_MapPCI mappci,*msg = &mappci;

        struct TagItem attrs[] = {
            { aHidd_PCIDevice_isIO,    FALSE },	/* Don't listen IO transactions */
            { aHidd_PCIDevice_isMEM,    TRUE },	/* Listen to MEM transactions */
            { aHidd_PCIDevice_isMaster, TRUE },	/* Can work in BusMaster */
            { TAG_DONE, 0UL },
        };
        OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&pciDriver);
        sd->pciDriver = pciDriver;

        sd->mid_ReadLong = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);
        sd->mid_ReadWord = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigWord);

        /*
          Read some PCI config registers
        */
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Bus, (APTR)&bus);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Dev, (APTR)&dev);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Sub, (APTR)&sub);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (APTR)&Base0);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, (APTR)&sizeBase0);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base1, (APTR)&Base1);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size1, (APTR)&sizeBase1);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, (APTR)&Base2);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, (APTR)&sizeBase2);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base3, (APTR)&Base3);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size3, (APTR)&sizeBase3);

        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &sd->G33IntLine);

        MGGC = pciReadWord(sd, bus, dev, sub, 0x52);
        MGGC = pciReadWord(sd, 0, 0, 0, 0x52);    //Read from bridge

        BSM = pciReadLong(sd, bus, dev, sub, 0x5c);
        sd->Chipset.StolenMemory = (APTR *)BSM;

        MSAC = pciReadByte(sd, bus, dev, sub, 0x62);
        D(bug("        MSAC =%x\n",MSAC));

        switch((MSAC & 0x6)) {
            case 0:
                D(bug("        Gfx Aperture size 128MB\n"));
                break;
            case 3:
                D(bug("        Gfx Aperture size 256MB\n"));
                break;
            case 6:
                D(bug("        Gfx Aperture size 512MB\n"));
                break;
            default:
                D(bug("        Gfx Aperture size is illegal!\n"));
                break;
        }

		switch ((MGGC & GTT_MASK)) {
			case GTT_1M:
				sizeGTT = 1*(1024*1024);
				break;
			case GTT_2M:
				sizeGTT = 2*(1024*1024);
				break;
		}
        sd->Chipset.sizeGTT = sizeGTT;

        switch ((MGGC & STOLEN_MEMORY_MASK)) {
            case STOLEN_MEMORY_1M:
                sizeMemory = 1*(1024*1024);
                break;
            case STOLEN_MEMORY_4M:
                sizeMemory = 4*(1024*1024);
                break;
            case STOLEN_MEMORY_8M:
                sizeMemory = 8*(1024*1024);
                break;
            case STOLEN_MEMORY_16M:
                sizeMemory = 16*(1024*1024);
                break;
            case STOLEN_MEMORY_32M:
                sizeMemory = 32*(1024*1024);
                break;
            case STOLEN_MEMORY_48M:
                sizeMemory = 48*(1024*1024);
                break;
            case STOLEN_MEMORY_64M:
                sizeMemory = 64*(1024*1024);
                break;
            case STOLEN_MEMORY_96M:
                sizeMemory = 96*(1024*1024);
                break;
            case STOLEN_MEMORY_128M:
                sizeMemory = 128*(1024*1024);
                break;
            case STOLEN_MEMORY_160M:
                sizeMemory = 160*(1024*1024);
                break;
            case STOLEN_MEMORY_224M:
                sizeMemory = 224*(1024*1024);
                break;
            case STOLEN_MEMORY_256M:
                sizeMemory = 256*(1024*1024);
                break;
            case STOLEN_MEMORY_352M:
                sizeMemory = 352*(1024*1024);
                break;
        }
        sd->Chipset.sizeStolenMemory = sizeMemory;

        D(bug("        Bus =%x, Dev =%x, Sub =%x\n",bus, dev, sub));
        D(bug("        MGGC      =%x\n\n",MGGC));
        D(bug("        MMADR     =%x (%x)\n",Base0, sizeBase0));   //MMADR
        D(bug("        IOBAR     =%x (%x)\n",Base1, sizeBase1));   //IOBAR
        D(bug("        GMADR     =%x (%dMB)\n",Base2, sizeBase2/(1024*1024)));   //GMADR
        D(bug("        GTTADR    =%x (%dMB)\n",Base3, sizeBase3/(1024*1024)));   //GTTADR
        D(bug("        IRQ       =%d\n",sd->G33IntLine));

        D(bug("        StolenMem =%p (%dMB)\n",sd->Chipset.StolenMemory, sd->Chipset.sizeStolenMemory/(1024*1024)));
        D(bug("        %dMB for video memory\n",sd->Chipset.sizeVMemory/(1024*1024)));
        /*
          Map the PCI address space to CPU address space
        */
        mappci.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);
        mappci.PCIAddress = Base0;
        mappci.Length = sizeBase0;
        sd->Chipset.MMADR = (APTR)OOP_DoMethod(pciDriver, (OOP_Msg)msg);

        Chip_Init(sd);

    }else{
        D(bug("[G33]   not supported (%04x:%04x)\n",VendorID, ProductID));
    }

    AROS_USERFUNC_EXIT
}

static int IntelG33_Init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[G33] IntelG33 hidd init\n"));

    struct staticdata *sd = &LIBBASE->sd;

    sd->memPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);
    if ((sd->memPool == NULL))
        return FALSE;

    struct BootLoaderBase   *BootLoaderBase;
    BootLoaderBase = OpenResource("bootloader.resource");

    /*
        Use default of 256MB for video memory, unless user want's less, then go down to 32MB
    */
    sd->Chipset.sizeVMemory = 256*(1024*1024);

    if (BootLoaderBase != NULL) {
        struct List *list;
        struct Node *node;

        list = (struct List *)GetBootInfo(BL_Args);
        if (list) {
            ForeachNode(list, node) {
                if (strncmp(node->ln_Name, "iGfxMem", 7) == 0) {
                    if (strstr(node->ln_Name, "=128MB")) {
                        sd->Chipset.sizeVMemory = 128*(1024*1024);
                    }
                    if (strstr(node->ln_Name, "=64MB")) {
                        sd->Chipset.sizeVMemory = 64*(1024*1024);
                    }
                    if (strstr(node->ln_Name, "=32MB")) {
                        sd->Chipset.sizeVMemory = 32*(1024*1024);
                    }
                }
            }
        }
    }

	InitSemaphore(&sd->Chipset.Locks.DPMS);

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

                    OOP_Object *irq = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);

                    sd->G33IRQ = AllocVec(sizeof (HIDDT_IRQ_Handler), MEMF_CLEAR | MEMF_PUBLIC);

                    if (sd->G33IRQ) {
                        struct pHidd_IRQ_AddHandler __msg__ = {
                            mID:         OOP_GetMethodID(CLID_Hidd_IRQ, moHidd_IRQ_AddHandler),
                            handlerinfo: sd->G33IRQ,
                            id:          sd->G33IntLine,
                        }, *msg = &__msg__;
      
                        sd->G33IRQ->h_Node.ln_Pri = 0;
                        sd->G33IRQ->h_Node.ln_Name = "G33 Int";
                        sd->G33IRQ->h_Code = IntelG33_int;
                        sd->G33IRQ->h_Data = sd;
	
                        OOP_DoMethod(irq, (OOP_Msg)msg);
                        OOP_DisposeObject(irq);
                        D(bug("[G33]   Created interrupt handler\n"));
                        D(bug("[G33] IntelG33 hidd init (exit TRUE)\n"));
                        return TRUE;
                    }
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
