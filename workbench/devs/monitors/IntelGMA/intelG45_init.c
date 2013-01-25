/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <oop/oop.h>
#include <graphics/driver.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <hidd/i2c.h>

#include <strings.h>
#include <stdlib.h>

#include "intelG45_intern.h"
#include "intelG45_regs.h"
#include "compositing.h"

#define KBYTES 1024
#define MBYTES (1024 * 1024)

static uint32_t min(uint32_t a, uint32_t b)
{
        if (a < b)
                return a;
        else
                return b;
}

static uint32_t max(uint32_t a, uint32_t b)
{
        if (a > b)
                return a;
        else
                return b;
}

static BOOL IsCompatible(UWORD product_id)
{
    return
//        || product_id == 0x0042
//        || product_id == 0x0046
//        || product_id == 0x0102
//        || product_id == 0x0106
//        || product_id == 0x010a
//        || product_id == 0x0112
//        || product_id == 0x0116
//        || product_id == 0x0122
//        || product_id == 0x0126
//        || product_id == 0x2562
//        || product_id == 0x2572
        product_id == 0x2582
        || product_id == 0x258a
        || product_id == 0x2592
        || product_id == 0x2772
        || product_id == 0x27a2
        || product_id == 0x27ae
//        || product_id == 0x2972
//        || product_id == 0x2982
//        || product_id == 0x2992
//        || product_id == 0x29a2
//        || product_id == 0x29b2
//        || product_id == 0x29c2
//        || product_id == 0x29d2
        || product_id == 0x2a02
        || product_id == 0x2a12
        || product_id == 0x2a42
        || product_id == 0x2e02
        || product_id == 0x2e12
        || product_id == 0x2e22
        || product_id == 0x2e32
        || product_id == 0x2e42
        || product_id == 0x2e92
//        || product_id == 0x3577
//        || product_id == 0x3582
//        || product_id == 0x358e
//        || product_id == 0xa001
        || product_id == 0xa011
        ;
}

static BOOL NeedsPhysicalCursor(UWORD product_id)
{
    /* TRUE if one of i830, i85x, i915G(M), i945G(M) */
    return product_id == 0x2582
        || product_id == 0x258a
        || product_id == 0x2592
        || product_id == 0x2772
        || product_id == 0x27a2
        || product_id == 0x27ae
        || product_id == 0x3577
        || product_id == 0x3582
        || product_id == 0x358e;
}

static BOOL HasQuadBARs(UWORD product_id)
{
    return product_id > 0x2800
        && product_id < 0xa000;
}

static ULONG GetGATTSize(struct g45staticdata *sd)
{
    ULONG size = 0;

    switch (readl(sd->Card.MMIO + G45_GATT_CONTROL) >> 1 & 0x7)
    {
        case 0:
            size = 512 * KBYTES;
            break;
        case 1:
            size = 256 * KBYTES;
            break;
        case 2:
            size = 128 * KBYTES;
            break;
        case 3:
            size = 1 * MBYTES;
            break;
        case 4:
            size = 2 * MBYTES;
            break;
        case 5:
            size = 1 * MBYTES + 512 * KBYTES;
            break;
    }

    return size;
}

static BOOL probe_monitor(struct g45staticdata *sd, uint32_t port)
{
        BOOL result = FALSE;
        
        bug("[GMA] Attempting to detect connected monitor\n");
        sd->DDCPort = port;
        bug("[GMA] Probing GPIO%c\n", 'A' + (port - G45_GPIOA)/4);
        OOP_Object *i2c = OOP_NewObject(sd->IntelI2C, NULL, NULL);

        if (i2c)
        {
                if (HIDD_I2C_ProbeAddress(i2c, 0xa1))
                {
                        struct TagItem attrs[] = {
                                        { aHidd_I2CDevice_Driver,   (IPTR)i2c       },
                                        { aHidd_I2CDevice_Address,  0xa0            },
                                        { aHidd_I2CDevice_Name,     (IPTR)"Display" },
                                        { TAG_DONE, 0UL }
                        };

                        bug("[GMA]   I2C device found\n");
                        
                        OOP_Object *obj = OOP_NewObject(NULL, CLID_Hidd_I2CDevice, attrs);

                        if (obj)
                        {
                                uint8_t edid[128];
                                char wb[2] = {0, 0};
                                struct pHidd_I2CDevice_WriteRead msg;
                                uint8_t chksum = 0;
                                int i;

                                msg.mID = OOP_GetMethodID((STRPTR)IID_Hidd_I2CDevice, moHidd_I2CDevice_WriteRead);
                                msg.readBuffer = &edid[0];
                                msg.readLength = 128;
                                msg.writeBuffer = &wb[0];
                                msg.writeLength = 1;

                                OOP_DoMethod(obj, &msg.mID);

                                for (i=0; i < 128; i++)
                                        chksum += edid[i];

                                if (chksum == 0 &&
                                                edid[0] == 0 && edid[1] == 0xff && edid[2] == 0xff && edid[3] == 0xff &&
                                                edid[4] == 0xff && edid[5] == 0xff && edid[6] == 0xff && edid[7] == 0)
                                {
                                        result = TRUE;
                                        if (edid[0x14] & 0x80)
                                        {
                                                bug("[GMA]       Digital device\n");
                                        }
                                        else
                                        {
                                                bug("[GMA]       Analog device\n");
                                        }
                                }

                                OOP_DisposeObject(obj);
                        }
                }
                else bug("[GMA]   No I2C device found\n");

                OOP_DisposeObject(i2c);
        }
        
        return result;
}

static
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,hook,       A0),
    AROS_UFHA(OOP_Object *, pciDevice,  A2),
    AROS_UFHA(APTR,         message,    A1))
{
    AROS_USERFUNC_INIT

    struct g45staticdata *sd = hook->h_Data;
    ULONG gfx_mem_size = 64 * MBYTES, min_gfx_mem_size = 8 * MBYTES,
        max_gfx_mem_size, extra_mem_size;
    IPTR ProductID;
    IPTR VendorID;
    IPTR RevisionID;

    /* Get the Device's ProductID */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_RevisionID, &RevisionID);

    D(bug("[GMA] Checking device %04x:%04x\n", VendorID, ProductID));

    if (sd->forced || IsCompatible(ProductID))
    {
        UWORD MGCC;

        if (HIDD_PCIDevice_Obtain(pciDevice, "IntelGMA"))
        {
            D(bug("[GMA] Failed to obtain device, already owned\n"));
            return;
        }

        MGCC = HIDD_PCIDevice_ReadConfigWord(pciDevice, G45_MGCC);
        D(bug("[GMA] MGCC=%04x, BSM=%08x, MSAC=%08x\n", MGCC, 
              HIDD_PCIDevice_ReadConfigLong(pciDevice, G45_BSM),
              HIDD_PCIDevice_ReadConfigByte(pciDevice, G45_MSAC)));

        /*-------- DO NOT CHANGE/REMOVE -------------*/
        bug("\003\n"); /* Tell vga text mode debug output to die */
        /*-------- DO NOT CHANGE/REMOVE -------------*/

        switch (MGCC & G45_MGCC_GMS_MASK)
        {
        case G45_MGCC_GMS_1M:
                sd->Card.Stolen_size = 0x00100000;
                break;
        case G45_MGCC_GMS_4M:
                sd->Card.Stolen_size = 0x00400000;
                break;
        case G45_MGCC_GMS_8M:
                sd->Card.Stolen_size = 0x00800000;
                break;
        case G45_MGCC_GMS_16M:
                sd->Card.Stolen_size = 0x01000000;
                break;
        case G45_MGCC_GMS_32M:
                sd->Card.Stolen_size = 0x02000000;
                break;
        case G45_MGCC_GMS_48M:
                sd->Card.Stolen_size = 0x03000000;
                break;
        case G45_MGCC_GMS_64M:
                sd->Card.Stolen_size = 0x04000000;
                break;
        }

        sd->ProductID = ProductID;

        IPTR mmio_base, window_base, gatt_base;
        IPTR mmio_size, window_size, gatt_size;
        OOP_Object *driver;
        struct TagItem attrs[] =
        {
            { aHidd_PCIDevice_isIO,     TRUE }, /* Don't listen IO transactions */
            { aHidd_PCIDevice_isMEM,    TRUE }, /* Listen to MEM transactions */
            { aHidd_PCIDevice_isMaster, TRUE }, /* Can work in BusMaster */
            { TAG_DONE, 0UL },
        };

        OOP_SetAttrs(pciDevice, attrs);
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (IPTR *)&driver);

        if (HasQuadBARs(ProductID))
        {
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &mmio_base);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &window_base);

            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &mmio_size);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, &window_size);

            mmio_size /= 2;
            gatt_base = mmio_base + mmio_size;
            gatt_size = GetGATTSize(sd);
        }
        else
        {
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &mmio_base);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &window_base);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base3, &gatt_base);

            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &mmio_size);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, &window_size);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size3, &gatt_size);
        }

        /* Gain access to PCI resource regions used */
        sd->Card.Framebuffer = HIDD_PCIDriver_MapPCI(driver,
            (APTR)window_base, window_size);
        sd->Card.Framebuffer_size = window_size;
        sd->Card.MMIO = HIDD_PCIDriver_MapPCI(driver,
            (APTR)mmio_base, mmio_size);
        sd->Card.GATT = HIDD_PCIDriver_MapPCI(driver,
            (APTR)gatt_base, gatt_size);
        sd->Card.GATT_size = gatt_size;

        /* Estimate the common size of the GATT and the graphics aperature.
         * Substract 16MB for scratch area */
        max_gfx_mem_size = min(window_size, gatt_size * 1024) - 16 * MBYTES;

        /* Ensure graphics buffer isn't smaller than stolen memory */
        gfx_mem_size = max(sd->Card.Stolen_size, gfx_mem_size);

        /*
         * start initialization:
         * obtain parameters
         */
        if (sd->memsize)
        {
            uint32_t meg = sd->memsize;

            if (meg < 16)
                meg = 16;

            gfx_mem_size = max(min_gfx_mem_size, meg * MBYTES);
            gfx_mem_size = min(gfx_mem_size, max_gfx_mem_size);

            D(bug("[GMA] Driver parameter sets video memory to %dMB\n", gfx_mem_size >> 20));
        }

        /* Calculate amount of extra memory to request */
        if (gfx_mem_size > sd->Card.Stolen_size)
            extra_mem_size = gfx_mem_size - sd->Card.Stolen_size;
        else
            extra_mem_size = 0;

        /* GATT table and a 4K popup are in stolen memory, so we don't get
         * quite the full amount */
        gfx_mem_size -= sd->Card.GATT_size + 4096;

        D(bug("[GMA] GATT space for %d entries\n", gatt_size / 4));

        D(bug("[GMA] Stolen memory size: %dMB\n", sd->Card.Stolen_size >> 20));
        D(bug("[GMA] Framebuffer window size: %d MB\n", sd->Card.Framebuffer_size >> 20));

        struct MemChunk *mc = (struct MemChunk *)sd->Card.Framebuffer;

        sd->CardMem.mh_Node.ln_Type = NT_MEMORY;
        sd->CardMem.mh_Node.ln_Name = "Intel GMA Framebuffer";
        sd->CardMem.mh_First = mc;
        sd->CardMem.mh_Lower = (APTR)mc;

        sd->CardMem.mh_Free = gfx_mem_size;
        sd->CardMem.mh_Upper = (APTR)(sd->CardMem.mh_Free + (IPTR)mc);

        mc->mc_Next = NULL;
        mc->mc_Bytes = sd->CardMem.mh_Free;

        sd->PCIDevice = pciDevice;

        D(bug("[GMA] Usable memory: %d MB\n", gfx_mem_size >> 20));
        D(bug("[GMA] Requesting %d KB memory\n", extra_mem_size >> 10));

        /* Virtual address of end of stolen graphics memory */
        uintptr_t virtual = sd->Card.Stolen_size - sd->Card.GATT_size - 4096;

        if (extra_mem_size != 0)
        {
            /* Get memory */
            uintptr_t phys_memory =
                (uintptr_t)AllocMem(extra_mem_size + 4095, MEMF_REVERSE);
            D(bug("[GMA] Got %08x\n", phys_memory));

            /* Align it to the page size boundary (we allocated one page
             * more already) */
            phys_memory = (phys_memory + 4095) &~4095;

            D(bug("[GMA] Mapping physical %08x to virtual %08x with size %08x\n",
                phys_memory, virtual, extra_mem_size));
            G45_AttachMemory(sd, phys_memory, virtual, extra_mem_size);
        }
        sd->ScratchArea = virtual + extra_mem_size;
        D(bug("[GMA] Scratch area at %08x\n", sd->ScratchArea));

        uint32_t val;

        /* SCL as output */
        val = G45_GPIO_CLOCK_DIR_MASK | G45_GPIO_CLOCK_DIR_VAL;
        /* update SCL value */
        val |= G45_GPIO_CLOCK_DATA_MASK;
        /* SDA as output */
        val = G45_GPIO_DATA_DIR_MASK | G45_GPIO_DATA_DIR_VAL;
        /* update SDA value */
        val |= G45_GPIO_DATA_MASK;

        /* set SCL */

        val |= G45_GPIO_CLOCK_DATA_VAL;
        val |= G45_GPIO_DATA_VAL;

        writel(val, sd->Card.MMIO + G45_GPIOA);
        writel(val, sd->Card.MMIO + G45_GPIOC);
        writel(val, sd->Card.MMIO + G45_GPIOE);

        writel(0, sd->Card.MMIO + G45_GMBUS);

        /* Ring buffer. The very first allocation, therefore I take for granted it's aligned on 4K page boundary.
         * Get 8KB of it. */
        sd->RingBufferPhys = Allocate(&sd->CardMem, 64 * 4096);
        sd->RingBuffer = (intptr_t)sd->RingBufferPhys - (intptr_t)sd->Card.Framebuffer;
        sd->RingBufferSize = 64*4096;
        sd->RingBufferTail = 0;

        /* Reserve some memory for HW cursor */
        sd->CursorImage =  ((intptr_t)Allocate(&sd->CardMem, 64*64*4)) - (intptr_t)sd->Card.Framebuffer;
        if (NeedsPhysicalCursor(ProductID))
            sd->CursorBase = G45_VirtualToPhysical(sd, sd->CursorImage);
        else
            sd->CursorBase = sd->CursorImage;

        D(bug("[GMA] Using ARGB cursor at graphics address %08x (physical address %08x)\n",
                        sd->CursorImage, sd->CursorBase));
        sd->CursorVisible = FALSE;

        D(bug("[GMA] Initializing CMD ring buffer\n"));

        writel(sd->RingBuffer, sd->Card.MMIO + G45_RING_BASE);
        writel(0, sd->Card.MMIO + G45_RING_TAIL);
        writel(0, sd->Card.MMIO + G45_RING_HEAD);
        writel((63 << G45_RING_CONTROL_LENGTH_SHIFT) | G45_RING_CONTROL_ENABLE, sd->Card.MMIO + G45_RING_CONTROL);

        sd->Engine2DOwner = NULL;

        sd->HardwareStatusPage = (void*)(((intptr_t)AllocPooled(sd->MemPool, 4096 + 4095) + 4095) & ~4095);
        writel((ULONG)(IPTR)sd->HardwareStatusPage, sd->Card.MMIO + 0x2080);

//      sd->HardwareStatusPage = (APTR)readl(sd->Card.MMIO + 0x2080);
        D(bug("[GMA] Hardware status page: %08x\n", readl(sd->Card.MMIO + 0x2080)));
        writel(1, &sd->HardwareStatusPage[16]);

        BOOL lvds = FALSE;

        // lvds port enabled ?
        if( lvds_Enabled(sd) ) lvds = TRUE;

        // VGA port enabled by BIOS ?
        if( adpa_Enabled(sd)) lvds = FALSE;

        // Final Test: try to read EDID information from VGA port
        if (probe_monitor(sd, G45_GPIOA ))
            lvds = FALSE;

        if( lvds )
        {
            bug("[GMA] lvds Enabled\n");
            sd->pipe = PIPE_B;
            GetSync(sd,&sd->lvds_fixed,PIPE_B); 
        }
        else
        {
            bug("[GMA] analog VGA connector Enabled\n");
            sd->pipe = PIPE_A;
        }

        /*
         * Set up initial screen mode.
         */

        sd->initialState = AllocVecPooled(sd->MemPool, sizeof(GMAState_t));
        sd->initialBitMap = AllocBitmapArea(sd, 640, 480, 4);
        G45_InitMode(sd, sd->initialState, 640, 480, 32, 25200, sd->initialBitMap,
                                                                640, 480,
                                                                656, 752, 800,
                                                                490, 492, 525, 0);

        /* Clear initial buffer */
        uint32_t i,
            *pixel = (uint32_t *)(sd->initialBitMap + sd->Card.Framebuffer);
        for(i = 0; i < 640 * 480; i++)
            *pixel++ = 0;

        /*
         * URGENT FIXME!!!
         * The driver must not modify hardware state before it's instantiated.
         * AddDisplayDriverA() function intentionally takes class pointer, not an object.
         * Before instantiating the driver it performs boot mode drivers check. If at
         * least one of them is in use, it can't be shut down. This means the newly
         * installed driver can conflict with it, if they ocassionally use the same hardware.
         */
        G45_LoadState(sd, sd->initialState);

        AddDisplayDriverA(sd->IntelG45Class, NULL, NULL);
    }

    AROS_LIBFUNC_EXIT
}

const struct TagItem Requirements[] =
{
    {tHidd_PCI_Interface, 0x00 },
    {tHidd_PCI_Class,     0x03 },
    {tHidd_PCI_SubClass,  0x00 },
    {tHidd_PCI_VendorID,  0x8086 },
    {TAG_DONE, 0UL }
};

int G45_Init(struct g45staticdata *sd)
{
    D(bug("[GMA] Init\n"));

#ifdef __i386__
    /* Temporary workaround for testing on old kickstart - sonic */
    sd->MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);
#else
    sd->MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED | MEMF_31BIT, 8192, 4096);
#endif
    if (!sd->MemPool)
        return FALSE;

    InitSemaphore(&sd->HWLock);
    InitSemaphore(&sd->MultiBMLock);

    sd->PCIDevice = NULL;

    /* Initialize reusable MsgPort */
    sd->MsgPort.mp_SigBit = SIGB_SINGLE;
    sd->MsgPort.mp_Flags = PA_SIGNAL;
    sd->MsgPort.mp_SigTask = FindTask(NULL);
    sd->MsgPort.mp_Node.ln_Type = NT_MSGPORT;
    NEWLIST(&sd->MsgPort.mp_MsgList);

    sd->tr = (struct timerequest *)CreateIORequest(&sd->MsgPort, sizeof(struct timerequest));
    if (sd->tr)
    {
        if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, &sd->tr->tr_node, 0))
        {
            D(bug("[GMA] UNIT_MICROHZ of timer.device opened\n"));
            if ((sd->PCIObject = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_PCI, NULL)))
            {
                struct Hook FindHook =
                {
                    h_Entry: (IPTR (*)())Enumerator,
                    h_Data:  sd,
                };

                HIDD_PCI_EnumDevices(sd->PCIObject, &FindHook, Requirements);

                if (sd->PCIDevice)
                {
                    D(bug("[GMA] Found supported gfx card\n\003"));

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
                    sd->mid_GetImage    = OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetImage);

                    sd->mid_BitMapPositionChanged        = OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapPositionChanged);
                    sd->mid_BitMapRectChanged            = OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapRectChanged);
                    sd->mid_ValidateBitMapPositionChange = OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_ValidateBitMapPositionChange);

                    return TRUE;
                }
                D(bug("[GMA] No supported cards found\n"));
                OOP_DisposeObject(sd->PCIObject);
            }
           CloseDevice(&sd->tr->tr_node);
        }
        DeleteIORequest(&sd->tr->tr_node);
    }
    DeletePool(sd->MemPool);

    return FALSE;
}

ADD2LIBS((STRPTR)"graphics.hidd", 0, static struct Library *, __gfxbase);
