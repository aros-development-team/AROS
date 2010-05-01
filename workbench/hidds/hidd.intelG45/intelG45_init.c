/*
 * intelG45_init.c
 *
 *  Created on: Apr 14, 2010
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/graphics.h>
#include <hidd/pci.h>

#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"
#include "intelG45_regs.h"

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase
#define HiddPCIDeviceAttrBase   (intelg45base->g45_sd.pciAttrBase)
#define HiddIntelG45BitMapAttrBase   (intelg45base->g45_sd.atiBitMapAttrBase)
#define HiddBitMapAttrBase      (intelg45base->g45_sd.bitMapAttrBase)
#define HiddPixFmtAttrBase      (intelg45base->g45_sd.pixFmtAttrBase)
#define HiddGfxAttrBase         (intelg45base->g45_sd.gfxAttrBase)
#define HiddSyncAttrBase        (intelg45base->g45_sd.syncAttrBase)
#define HiddI2CAttrBase         (intelg45base->g45_sd.i2cAttrBase)
#define HiddI2CDeviceAttrBase   (intelg45base->g45_sd.i2cDeviceAttrBase)
#define __IHidd_PlanarBM        (intelg45base->g45_sd.planarAttrBase)


static
AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *,hook,       A0),
    AROS_UFHA(OOP_Object *, pciDevice,  A2),
    AROS_UFHA(APTR,         message,    A1))
{
    AROS_USERFUNC_INIT
    struct intelg45base *intelg45base = (struct intelg45base *)hook->h_Data;
    struct g45staticdata *sd = &intelg45base->g45_sd;

    IPTR ProductID;
    IPTR VendorID;

    /* Get the Device's ProductID */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);

    D(bug("[GMA] Checking device %04x:%04x\n", VendorID, ProductID));

    if (ProductID == 0x2772 || ProductID == 0x27a6)
    {
    	UWORD MGCC = HIDD_PCIDevice_ReadConfigWord(pciDevice, G45_MGCC);
    	ULONG BSM = HIDD_PCIDevice_ReadConfigLong(pciDevice, G45_BSM);
    	UBYTE MSAC = HIDD_PCIDevice_ReadConfigByte(pciDevice, G45_MSAC);

    	IPTR Bar0, Bar2, Bar3;
    	IPTR Size0, Size2, Size3;

    	OOP_Object *driver;

    	struct TagItem attrs[] = {
    			{ aHidd_PCIDevice_isIO,     TRUE }, /* Don't listen IO transactions */
    			{ aHidd_PCIDevice_isMEM,    TRUE }, /* Listen to MEM transactions */
    			{ aHidd_PCIDevice_isMaster, TRUE }, /* Can work in BusMaster */
    			{ TAG_DONE, 0UL },
    	};

    	OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);

    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, &driver);

    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &Bar0);
    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base2, &Bar2);
    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base3, &Bar3);

    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size0, &Size0);
    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size2, &Size2);
    	OOP_GetAttr(pciDevice, aHidd_PCIDevice_Size3, &Size3);


    	D(bug("[GMA] MGCC=%04x, BSM=%08x, MSAC=%08x\n", MGCC, BSM, MSAC));
    	D(bug("[GMA] Bar0=%08x-%08x, Bar2=%08x-%08x, Bar3=%08x-%08x\n", Bar0, Bar0 + Size0 - 1,
    			Bar2, Bar2 + Size2 - 1, Bar3, Bar3 + Size3 - 1));

    	sd->Card.Framebuffer = HIDD_PCIDriver_MapPCI(driver, Bar2, Size2);
    	sd->Card.fb_size = Size2;
    	sd->Card.MMIO = HIDD_PCIDriver_MapPCI(driver, Bar0, Size0);
    	sd->Card.GATT = HIDD_PCIDriver_MapPCI(driver, Bar3, Size3);

    	sd->PCIDevice = pciDevice;

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
    	writel(0, sd->Card.MMIO + G45_GMBUS);

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
	        { (STRPTR)IID_Hidd_IntelG45BitMap, &HiddIntelG45BitMapAttrBase },
	        { (STRPTR)IID_Hidd_I2C,         &HiddI2CAttrBase },
	        { (STRPTR)IID_Hidd_I2CDevice,   &HiddI2CDeviceAttrBase },
	        { (STRPTR)IID_Hidd_PlanarBM,    &__IHidd_PlanarBM },
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
						D(bug("[GMA] Found supported gfx card\n"));
					}

					return TRUE;
				}
			}
		}
	}
}

ADD2INITLIB(G45_Init, 0)
ADD2LIBS((STRPTR)"graphics.hidd", 0, static struct Library *, __gfxbase);
