/*
    Copyright © 2003-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <utility/utility.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include <hidd/i2c.h>
#include <hidd/pci.h>
#include <hidd/graphics.h>

#include "ati.h"
#include "ids.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_macros.h"
#include LC_LIBDEFS_FILE

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->sd.pciAttrBase)
#define HiddATIBitMapAttrBase   (LIBBASE->sd.atiBitMapAttrBase)
#define HiddBitMapAttrBase      (LIBBASE->sd.bitMapAttrBase)
#define HiddPixFmtAttrBase      (LIBBASE->sd.pixFmtAttrBase)
#define HiddGfxAttrBase         (LIBBASE->sd.gfxAttrBase)
#define HiddSyncAttrBase        (LIBBASE->sd.syncAttrBase)
#define HiddI2CAttrBase         (LIBBASE->sd.i2cAttrBase)
#define HiddI2CDeviceAttrBase   (LIBBASE->sd.i2cDeviceAttrBase)
#define __IHidd_PlanarBM        (LIBBASE->sd.planarAttrBase)

static
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,hook,       A0),
    AROS_UFHA(OOP_Object *, pciDevice,  A2),
    AROS_UFHA(APTR,         message,    A1))
{
    AROS_USERFUNC_INIT  
    LIBBASETYPEPTR LIBBASE = (LIBBASETYPEPTR)hook->h_Data;
    struct ati_staticdata *sd = &LIBBASE->sd;
    
    struct ATIDevice *sup = (struct ATIDevice *)support;
    IPTR ProductID;
    IPTR VendorID;

    if (sd->PCIDevice != NULL)
        return;

    /* Get the Device's ProductID */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    
    D(bug("[ATI] Enumerator: checking productid %04x vendorid %04x %08x\n",
          ProductID, VendorID, pciDevice));

    /* And try to match it with supported cards */
    while (sup->VendorID)
    {
        BOOL found = FALSE;
    
        if (sup->VendorID == VendorID)
        {
            if (!sup->masked_check && (sup->ProductID == ProductID))
            {
                found = TRUE;
            }
            else if (sup->masked_check && (sup->ProductID == (ProductID & 0xFFF0)))
            {
                found = TRUE;
            }
        }

        if (found)
        {
            /* Matching card found */
            APTR buf;
            ULONG size;
            OOP_Object *driver;
            struct MemChunk *mc;

            struct TagItem attrs[] = {
                { aHidd_PCIDevice_isIO,     TRUE }, /* Don't listen IO transactions */
                { aHidd_PCIDevice_isMEM,    TRUE }, /* Listen to MEM transactions */
                { aHidd_PCIDevice_isMaster, TRUE }, /* Can work in BusMaster */
                { TAG_DONE, 0UL },
            };
    
            D(bug("[ATI] Enumerator: found productid %04x vendorid %04x masked_check %d\n",
                  sup->ProductID, sup->VendorID, sup->masked_check));
        
            sd->Card.ProductID = ProductID;
            sd->Card.VendorID = VendorID;
            sd->Card.Type = sup->Type;

            /*
                Fix PCI device attributes (perhaps already set, but if the 
                ATI would be the second card in the system, it may stay
                uninitialized.
            */
            OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);
            
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);
            sd->PCIDriver = driver;
    
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (APTR)&buf);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, (APTR)&size);

            sd->Card.FbAddress = (IPTR)buf;
            sd->Card.FrameBuffer = (IPTR)HIDD_PCIDriver_MapPCI(driver, buf, size);
            mc = (struct MemChunk *)sd->Card.FrameBuffer;

            sd->CardMem.mh_Node.ln_Type = NT_MEMORY;
            sd->CardMem.mh_Node.ln_Name = "ATI Framebuffer";
            sd->CardMem.mh_First = mc;
            sd->CardMem.mh_Lower = (APTR)mc;
    
            D(bug("[ATI] Got framebuffer @ %x (size=%dMiB)\n", sd->Card.FrameBuffer, size>>20));
    
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, (APTR)&buf);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, (APTR)&size);

            sd->Card.MMIO = (APTR)HIDD_PCIDriver_MapPCI(driver, buf, size);
            D(bug("[ATI] Got registers @ %x (size=%dKiB)\n", sd->Card.MMIO, size>>10));

            OOP_GetAttr(pciDevice, aHidd_PCIDevice_RomBase, (APTR)&buf);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_RomSize, (APTR)&size);

//            sd->Card.vbios_org = (APTR)HIDD_PCIDriver_MapPCI(driver, buf, size);
            sd->Card.vbios_org = (APTR)HIDD_PCIDriver_MapPCI(driver, (APTR)0x000c0000, size);
            sd->Card.VBIOS = sd->Card.vbios_org;
            D(bug("[ATI] Got BIOS @ %x (size=%dKiB)\n", sd->Card.VBIOS, size>>10));

            if (sup->Init(sd))
            {
                struct CardState *state = AllocPooled(sd->memPool, sizeof(struct CardState));

                sd->poweron_state = AllocPooled(sd->memPool, sizeof(struct CardState));
                SaveState(sd, sd->poweron_state);

                sd->CardMem.mh_Free = sd->Card.FbUsableSize;
                sd->CardMem.mh_Upper = (APTR)(sd->CardMem.mh_Free + (IPTR)mc);
        
                mc->mc_Next = NULL;
                mc->mc_Bytes = sd->CardMem.mh_Free;
        
                D(bug("[ATI] Usable size: %dKB\n", sd->CardMem.mh_Free >> 10));

                sd->scratch_buffer = AllocBitmapArea(sd, 4096, 16, 4, TRUE);
                sd->Card.CursorStart = AllocBitmapArea(sd, 64, 64, 4, TRUE);

                OUTREG(RADEON_CUR_HORZ_VERT_OFF,RADEON_CUR_LOCK |  0);
                OUTREG(RADEON_CUR_HORZ_VERT_POSN,RADEON_CUR_LOCK | 0);
                OUTREG(RADEON_CUR_OFFSET, sd->Card.CursorStart);
        
                sd->PCIDevice = pciDevice;

                /*-------- DO NOT CHANGE/REMOVE -------------*/
                bug("\003\n"); /* Tell vga text mode debug output to die */
                /*-------- DO NOT CHANGE/REMOVE -------------*/
            }
            else
            {
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, (APTR)&size);
                HIDD_PCIDriver_UnmapPCI(driver, (APTR)sd->Card.FrameBuffer, size);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, (APTR)&size);
                HIDD_PCIDriver_UnmapPCI(driver, (APTR)sd->Card.MMIO, size);
                OOP_GetAttr(pciDevice, aHidd_PCIDevice_RomSize, (APTR)&size);
                HIDD_PCIDriver_UnmapPCI(driver, (APTR)sd->Card.vbios_org, size);
                sd->PCIDevice = NULL;
            }

            break;
        }

        sup++;
    }

    D(bug("[ATI] Enumerator found a card (ProductID=%04x)\n", ProductID));
    D(bug("[ATI] The card is %ssupported\n",
            sd->PCIDevice ? "":"un"));
   
    AROS_USERFUNC_EXIT
}

static int ATI_Init(LIBBASETYPEPTR LIBBASE)
{
    struct ati_staticdata *sd = &LIBBASE->sd;

    struct OOP_ABDescr attrbases[] = 
    {
        { (STRPTR)IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
        { (STRPTR)IID_Hidd_BitMap,      &HiddBitMapAttrBase },
        { (STRPTR)IID_Hidd_PixFmt,      &HiddPixFmtAttrBase },
        { (STRPTR)IID_Hidd_Sync,        &HiddSyncAttrBase },
        { (STRPTR)IID_Hidd_Gfx,         &HiddGfxAttrBase },
        { (STRPTR)IID_Hidd_ATIBitMap,   &HiddATIBitMapAttrBase },
        { (STRPTR)IID_Hidd_I2C,         &HiddI2CAttrBase },
        { (STRPTR)IID_Hidd_I2CDevice,   &HiddI2CDeviceAttrBase },
        { (STRPTR)IID_Hidd_PlanarBM,    &__IHidd_PlanarBM },
        { NULL, NULL }
    };

    D(bug("[ATI] Init\n"));

    sd->memPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);

    if (sd->memPool)
    {
        if (OOP_ObtainAttrBases(attrbases))
        {
            sd->mid_CopyMemBox8     = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox8);
            sd->mid_CopyMemBox16    = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox16);
            sd->mid_CopyMemBox32    = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyMemBox32);
            sd->mid_PutMem32Image8  = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMem32Image8);
            sd->mid_PutMem32Image16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMem32Image16);
            sd->mid_GetMem32Image8  = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetMem32Image8);
            sd->mid_GetMem32Image16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetMem32Image16);
            sd->mid_Clear           = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_Clear);
            sd->mid_PutMemTemplate8 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate8);
            sd->mid_PutMemTemplate16= OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate16);
            sd->mid_PutMemTemplate32= OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemTemplate32);
            sd->mid_PutMemPattern8  = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemPattern8);
            sd->mid_PutMemPattern16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemPattern16);
            sd->mid_PutMemPattern32 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_PutMemPattern32);
            sd->mid_CopyLUTMemBox16 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox16);
            sd->mid_CopyLUTMemBox32 = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_CopyLUTMemBox32);

            InitSemaphore(&LIBBASE->sd.HWLock);
            InitSemaphore(&LIBBASE->sd.MultiBMLock);
        
            LIBBASE->sd.sysbase = LIBBASE->sysbase;
    
            if ((LIBBASE->sd.PCIObject = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_PCI, NULL)))
            {
                struct Hook FindHook = {
                    h_Entry:    (IPTR (*)())Enumerator,
                    h_Data:     LIBBASE,
                };
        
                struct TagItem Requirements[] = {
                    { tHidd_PCI_Interface,  0x00 },
                    { tHidd_PCI_Class,  0x03 },
                    { tHidd_PCI_SubClass,   0x00 },
                    { tHidd_PCI_VendorID,   0x1002 }, // ATI VendorID. May require more of them
                    { TAG_DONE, 0UL }
                };
                
                HIDD_PCI_EnumDevices(LIBBASE->sd.PCIObject, &FindHook, Requirements);
                
                return TRUE;
            }
            
            OOP_ReleaseAttrBases(attrbases);
        }

        DeletePool(LIBBASE->sd.memPool);
    }
    
    return FALSE;
}

static int ATI_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct ati_staticdata *sd = &LIBBASE->sd;
    
    struct OOP_ABDescr attrbases[] = 
    {
        { (STRPTR)IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
        { (STRPTR)IID_Hidd_BitMap,      &HiddBitMapAttrBase },
        { (STRPTR)IID_Hidd_PixFmt,      &HiddPixFmtAttrBase },
        { (STRPTR)IID_Hidd_Sync,        &HiddSyncAttrBase },
        { (STRPTR)IID_Hidd_Gfx,         &HiddGfxAttrBase },
        { (STRPTR)IID_Hidd_ATIBitMap,   &HiddATIBitMapAttrBase },
        { (STRPTR)IID_Hidd_I2C,         &HiddI2CAttrBase },
        { (STRPTR)IID_Hidd_I2CDevice,   &HiddI2CDeviceAttrBase },
        { (STRPTR)IID_Hidd_PlanarBM,    &__IHidd_PlanarBM },
        { NULL, NULL }
    };
    
    if (sd->PCIDevice)
    {
        IPTR size;
        OOP_GetAttr(sd->PCIDevice, aHidd_PCIDevice_Size0, (APTR)&size);
        HIDD_PCIDriver_UnmapPCI(sd->PCIDriver, (APTR)sd->Card.FrameBuffer, size);
        OOP_GetAttr(sd->PCIDevice, aHidd_PCIDevice_Size2, (APTR)&size);
        HIDD_PCIDriver_UnmapPCI(sd->PCIDriver, (APTR)sd->Card.MMIO, size);
        OOP_GetAttr(sd->PCIDevice, aHidd_PCIDevice_RomSize, (APTR)&size);
        HIDD_PCIDriver_UnmapPCI(sd->PCIDriver, (APTR)sd->Card.vbios_org, size);
        sd->PCIDevice = NULL;
    }

    OOP_DisposeObject(sd->PCIObject);
    OOP_ReleaseAttrBases(attrbases);
    DeletePool(sd->memPool);

    return TRUE;
}

ADD2INITLIB(ATI_Init, 0)
ADD2EXPUNGELIB(ATI_Expunge, 0)
ADD2LIBS((STRPTR)"graphics.hidd", 0, static struct Library *, __gfxbase);
