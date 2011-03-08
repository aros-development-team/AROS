/*
 * intelG45_init.c
 *
 *  Created on: Apr 14, 2010
 *      $Id$
 */

#define DEBUG 0
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/bootloader.h>

#include <aros/bootloader.h>

#include <hidd/graphics.h>
#include <hidd/pci.h>
#include <hidd/i2c.h>

#include <strings.h>
#include <stdlib.h>

#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"
#include "intelG45_regs.h"

#include "intelG45_logo.h"

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

#undef HiddPCIDeviceAttrBase
#undef HiddGMABitMapAttrBase
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddGfxAttrBase
#undef HiddSyncAttrBase
#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase
#undef __IHidd_PlanarBM
#undef HiddGCAttrBase
#undef HiddCompositingAttrBase

#define HiddPCIDeviceAttrBase   (intelg45base->g45_sd.pciAttrBase)
#define HiddGMABitMapAttrBase   (intelg45base->g45_sd.gmaBitMapAttrBase)
#define HiddBitMapAttrBase      (intelg45base->g45_sd.bitMapAttrBase)
#define HiddPixFmtAttrBase      (intelg45base->g45_sd.pixFmtAttrBase)
#define HiddGfxAttrBase         (intelg45base->g45_sd.gfxAttrBase)
#define HiddSyncAttrBase        (intelg45base->g45_sd.syncAttrBase)
#define HiddI2CAttrBase         (intelg45base->g45_sd.i2cAttrBase)
#define HiddI2CDeviceAttrBase   (intelg45base->g45_sd.i2cDeviceAttrBase)
#define __IHidd_PlanarBM        (intelg45base->g45_sd.planarAttrBase)
#define HiddGCAttrBase          (intelg45base->g45_sd.gcAttrBase)
#define HiddCompositingAttrBase (intelg45base->g45_sd.compositingAttrBase)


static const char __attribute__((used)) __greet[] = "!!! This driver is sponsored by iMica !!!\n";

static BOOL IsCompatible(UWORD product_id)
{
    return product_id == 0x2582
        || product_id == 0x2772
        || product_id == 0x27a2
        || product_id == 0x27a6
        || product_id == 0x27ae
        || product_id == 0x2a42;
}

static ULONG GetGATTSize(struct intelg45base *intelg45base)
{
    struct g45staticdata *sd = &intelg45base->g45_sd;
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

#if 0
static const probe_monitor(struct intelg45base *intelg45base)
{
	struct g45staticdata *sd = &intelg45base->g45_sd;
	uint32_t port;

	D(bug("[GMA] Attempting to detect connected monitor\n"));

	for (port = G45_GPIOA; port <= G45_GPIOF; port += (G45_GPIOC - G45_GPIOA))
	{
		sd->DDCPort = port;

		D(bug("[GMA] Probing GPIO%c\n", 'A' + (port - G45_GPIOA)/(G45_GPIOB - G45_GPIOA)));

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

				D(bug("[GMA]   I2C device found\n"));

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
						if (edid[0x14] & 0x80)
						{
							D(bug("[GMA]   Digital device\n"));
						}
						else
							D(bug("[GMA]   Analog device\n"));
					}

					OOP_DisposeObject(obj);
				}
			}
			else
				D(bug("[GMA]   No I2C device found\n"));


			OOP_DisposeObject(i2c);
		}
	}
}
#endif

static BOOL probe_monitor(struct intelg45base *intelg45base, uint32_t port)
{
	struct g45staticdata *sd = &intelg45base->g45_sd;
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
			
			// there is at least something?,so... 
            result = TRUE;
			
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
    struct intelg45base *intelg45base = (struct intelg45base *)hook->h_Data;
    struct g45staticdata *sd = &intelg45base->g45_sd;
    void *BootLoaderBase;
    BOOL forced = FALSE;
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

    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase != NULL)
    {
    	struct List *list;
    	struct Node *node;

    	list = (struct List *)GetBootInfo(BL_Args);
    	if (list)
    	{
    		ForeachNode(list, node)
			{
    			if (strncasecmp(node->ln_Name, "forceGMA", 8) == 0)
    			{
    				D(bug("[GMA] WARNING. ForceGMA mode activated. Driver will attempt to start regardless of known support"));
    				forced = TRUE;
    			}
			}
    	}
    }

    if (forced || IsCompatible(ProductID))
    {
    	UWORD MGCC = HIDD_PCIDevice_ReadConfigWord(pciDevice, G45_MGCC);
    	D(ULONG BSM = HIDD_PCIDevice_ReadConfigLong(pciDevice, G45_BSM));
    	D(UBYTE MSAC = HIDD_PCIDevice_ReadConfigByte(pciDevice, G45_MSAC));

    	D(bug("[GMA] MGCC=%04x, BSM=%08x, MSAC=%08x\n", MGCC, BSM, MSAC));

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

        IPTR mmio_base, window_base, gatt_base;
        IPTR mmio_size, window_size, gatt_size;

    	OOP_Object *driver;

    	struct TagItem attrs[] = {
    			{ aHidd_PCIDevice_isIO,     TRUE }, /* Don't listen IO transactions */
    			{ aHidd_PCIDevice_isMEM,    TRUE }, /* Listen to MEM transactions */
    			{ aHidd_PCIDevice_isMaster, TRUE }, /* Can work in BusMaster */
    			{ TAG_DONE, 0UL },
    	};

    	OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (IPTR *)&driver);

        if (RevisionID >= 7)
        {
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &mmio_base);
            mmio_size -= 2 * MBYTES;
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &window_base);
            gatt_base = mmio_base + 2 * MBYTES;

            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &mmio_size);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, &window_size);
            gatt_size = GetGATTSize(intelg45base);
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
    	 * obtain kernel parameters
    	 */
    	BootLoaderBase = OpenResource("bootloader.resource");
    	D(bug("[GMA] BootloaderBase = %p\n", BootLoaderBase));
    	if (BootLoaderBase != NULL)
    	{
    		struct List *list;
    		struct Node *node;

    		list = (struct List *)GetBootInfo(BL_Args);
    		if (list)
    		{
    			ForeachNode(list, node)
    	        {
    				if (strncasecmp(node->ln_Name, "GMA_mem=", 8) == 0)
    				{
						char *str = &node->ln_Name[8];
						char *endptr;

						D(bug("[GMA] Found boot parameter '%s'\n", node->ln_Name));

						uint32_t meg = strtol(str, &endptr, 0);
						if (meg < 16)
							meg = 16;

                        gfx_mem_size = max(min_gfx_mem_size, meg * MBYTES);
                        gfx_mem_size = min(gfx_mem_size, max_gfx_mem_size);

                        D(bug("[GMA] Boot parameter sets video memory to %dMB\n",
                            gfx_mem_size >> 20));
    				}
           		}
    		}
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
        if (RevisionID >= 7)
            sd->CursorBase = sd->CursorImage;
        else
            sd->CursorBase = G45_VirtualToPhysical(sd, sd->CursorImage);

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
    	writel((ULONG)sd->HardwareStatusPage, sd->Card.MMIO + 0x2080);

//    	sd->HardwareStatusPage = (APTR)readl(sd->Card.MMIO + 0x2080);
    	D(bug("[GMA] Hardware status page: %08x\n", readl(sd->Card.MMIO + 0x2080)));
    	writel(1, &sd->HardwareStatusPage[16]);

    	sd->DDCPort = G45_GPIOA;
    	//probe_monitor(intelg45base);
		
        BOOL lvds = FALSE;

        // lvds port enabled ?
        if( lvds_Enabled(sd) ) lvds = TRUE;

		// VGA port enabled by BIOS ?
		if( adpa_Enabled(sd)) lvds = FALSE;

		// Final Test: try to read EDID information from VGA port
        if( probe_monitor(intelg45base, G45_GPIOA ) ) lvds = FALSE;

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
    	 * Boot logo.
    	 *
    	 * Since development of this driver is sponsored, I kindly ask to keep the boot logo with a small "commercial" in this place!
    	 */

    	sd->initialState = AllocVecPooled(sd->MemPool, sizeof(GMAState_t));

		sd->initialBitMap = AllocBitmapArea(sd, 640, 480, 4, TRUE);
		G45_InitMode(sd, sd->initialState, 640, 480, 32, 25200, sd->initialBitMap,
								640, 480,
								656, 752, 800,
								490, 492, 525, 0);


    	uint32_t *pixel = (uint32_t *)(sd->initialBitMap + sd->Card.Framebuffer);
    	uint8_t *stream = header_data;
		int count=logo_width * logo_height;
		uint8_t split = *stream++;
		do {
				uint8_t cnt = *stream++;
				
				if (cnt >= split)
				{
					cnt -= split-1;

					while(cnt-- && count > 0)
					{
						uint8_t color = *stream++;
						count--;
						*pixel++ = 0xff000000 | (header_data_cmap[color][0] << 16) | (header_data_cmap[color][1] << 8) | (header_data_cmap[color][2]);
					}
				}
				else
				{
					uint8_t color = *stream++;

					cnt += 3;
					while(cnt-- && count > 0)
					{
						count--;
						*pixel++ = 0xff000000 | (header_data_cmap[color][0] << 16) | (header_data_cmap[color][1] << 8) | (header_data_cmap[color][2]);

					}
				}

		} while (count > 0);

		G45_LoadState(sd, sd->initialState);

		// Z   |\      _,,,---,,_
		//  z  /,`.-'`'    -.  ;-;;,_
		//    |,4-  ) )-,_..;\ (  `'-'
		//   '---''(_/--'  `-'\_)
		// In VGA ,Kitty run away too fast. 
		if(sd->pipe == PIPE_A ) delay_ms(sd,1500);

		bug("[GMA] %s", __greet);
    }

    AROS_LIBFUNC_EXIT
}


static int G45_Init(struct intelg45base *intelg45base)
{
	struct g45staticdata *sd = &intelg45base->g45_sd;

	struct OOP_ABDescr attrbases[] =
	    {
	        { (STRPTR)IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
	        { (STRPTR)IID_Hidd_BitMap,      &HiddBitMapAttrBase },
	        { (STRPTR)IID_Hidd_PixFmt,      &HiddPixFmtAttrBase },
	        { (STRPTR)IID_Hidd_Sync,        &HiddSyncAttrBase },
	        { (STRPTR)IID_Hidd_Gfx,         &HiddGfxAttrBase },
	        { (STRPTR)IID_Hidd_IntelG45BitMap, &HiddGMABitMapAttrBase },
	        { (STRPTR)IID_Hidd_I2C,         &HiddI2CAttrBase },
	        { (STRPTR)IID_Hidd_I2CDevice,   &HiddI2CDeviceAttrBase },
	        { (STRPTR)IID_Hidd_PlanarBM,    &__IHidd_PlanarBM },
	        { (STRPTR)IID_Hidd_GC,          &HiddGCAttrBase },
			{ (STRPTR)IID_Hidd_Compositing, &HiddCompositingAttrBase },
			{ NULL, NULL }
	    };

	D(bug("[GMA] Init\n"));

	sd->MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);

	if (sd->MemPool)
	{
		D(bug("[GMA] MemPool created\n"));

		if (OOP_ObtainAttrBases(attrbases))
		{
			D(bug("[GMA] AttrBases obtained\n"));

			InitSemaphore(&sd->HWLock);
			InitSemaphore(&sd->MultiBMLock);

			sd->PCIDevice = NULL;

			/* Initialize MsgPort */
			sd->MsgPort.mp_SigBit = SIGB_SINGLE;
			sd->MsgPort.mp_Flags = PA_SIGNAL;
			sd->MsgPort.mp_SigTask = FindTask(NULL);
			sd->MsgPort.mp_Node.ln_Type = NT_MSGPORT;
			NEWLIST(&sd->MsgPort.mp_MsgList);

			sd->tr.tr_node.io_Message.mn_ReplyPort = &sd->MsgPort;
			sd->tr.tr_node.io_Message.mn_Length = sizeof(sd->tr);

			if (!OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)&sd->tr, 0))
			{
				D(bug("[GMA] UNIT_MICROHZ of timer.device opened\n"));
				if ((sd->PCIObject = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_PCI, NULL)))
				{
					struct Hook FindHook = {
							h_Entry:    (IPTR (*)())Enumerator,
							h_Data:     intelg45base,
					};

					struct TagItem Requirements[] = {
							{ tHidd_PCI_Interface,  0x00 },
							{ tHidd_PCI_Class,  	0x03 },
							{ tHidd_PCI_SubClass,   0x00 },
							{ tHidd_PCI_VendorID,   0x8086 },
							{ TAG_DONE, 0UL }
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
						sd->mid_GetImage	= OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetImage);

						sd->mid_BitMapPositionChanged   =
							OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapPositionChanged);
						sd->mid_BitMapRectChanged       = 
							OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_BitMapRectChanged);
						sd->mid_ValidateBitMapPositionChange =
							OOP_GetMethodID((STRPTR)IID_Hidd_Compositing, moHidd_Compositing_ValidateBitMapPositionChange);
						return TRUE;
					}
					D(bug("[GMA] No supported cards found\n"));
					OOP_DisposeObject(sd->PCIObject);
				}
				CloseDevice((struct IORequest *)&sd->tr);
			}
			OOP_ReleaseAttrBases(attrbases);
		}
		DeletePool(sd->MemPool);
	}

	return FALSE;
}

ADD2INITLIB(G45_Init, 0)
ADD2LIBS((STRPTR)"graphics.hidd", 0, static struct Library *, __gfxbase);
