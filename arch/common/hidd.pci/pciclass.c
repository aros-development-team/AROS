#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#define	HiddPCIAB		(PSD(cl)->hiddPCIAB)
#define HiddPCIDeviceAttrBase	(PSD(cl)->hiddPCIDeviceAB)

#define SysBase		(PSD(cl)->sysbase)
#define OOPBase 	(PSD(cl)->oopbase)
#define UtilityBase	(PSD(cl)->utilitybase)

/* 
    Returns 0 for no device, 1 for non-multi device and 2 for
    a multifunction device

    cl points to the base pci class which is used to extract static data
    o  points to the driver class which is used to read from config space
*/
static int isPCIDeviceAvailable(OOP_Class *cl, OOP_Object *o, UBYTE bus, UBYTE dev, UBYTE sub)
{
    UWORD Vend;
    UBYTE Type;

    struct pHidd_PCIDriver_ReadConfigWord rw;
    struct pHidd_PCIDriver_ReadConfigByte rb;

    if (!PSD(cl)->mid_RW) PSD(cl)->mid_RW =
	    OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigWord);
    if (!PSD(cl)->mid_RB) PSD(cl)->mid_RB =
	    OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigByte);

    rw.mID = PSD(cl)->mid_RW;
    rb.mID = PSD(cl)->mid_RB;
    
    rw.bus = bus;
    rw.dev = dev;
    rw.sub = sub;
    rw.reg = PCICS_VENDOR;

    Vend = OOP_DoMethod(o, (OOP_Msg)&rw);

    if ((Vend == 0xffff) || (Vend == 0x0000))
    {
	/* 0xffff is an invalid vendor ID, and so is 0x0000
	 * (Well, actually 0x0000 belongs to Gammagraphx, but this really
	 * clashes with multifunc device scanning, so lets just hope nobody
	 * has a card from them :) )
	 */

	return 0;
    }

    rb.bus = bus;
    rb.dev = dev;
    rb.sub = sub;
    rb.reg = PCICS_HEADERTYPE;

    Type = OOP_DoMethod(o, (OOP_Msg)&rb);

    if ((Type & PCIHT_MULTIFUNC) == PCIHT_MULTIFUNC)
	return 2;

    return 1;
}

/*
    PCI::AddHardwareDriver(OOP_Class *driverClass, UBYTE managed)

    Adds new PCI hardware driver to the PCI subsystem. A PCI hardware driver
    is a class which delivers Read/Write to the PCI config space.

    The PCI bus handled through driver added is scanned, and all available
    PCI devices are added to the device chain.

    If the class should be removed, when base PCI class is detached, the 
    managed parameter should be a non-zero value
*/
static void _PCI_AddHwDrv(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCI_AddHardwareDriver *msg)
{
    struct DriverNode *dn = NULL;

    D(bug("[PCI] Adding Driver class 0x%08x\n", msg->driverClass));
        
    // Get some extra memory for driver node
    dn = AllocMem(sizeof(struct DriverNode), MEMF_CLEAR);
    if (dn)
    {
	int bus;
	int dev;
	int sub;
	int type;
	OOP_Object drv;
	struct TagItem devtags[] = {
	    { aHidd_PCIDevice_Bus, 0 },
	    { aHidd_PCIDevice_Dev, 0 },
	    { aHidd_PCIDevice_Sub, 0 },
	    { aHidd_PCIDevice_Driver, 0 },
	    { TAG_DONE, 0UL }
	};
	struct PciDevice *pcidev;
	
	dn->managed = msg->managed;
	dn->driverClass = msg->driverClass;
	dn->driverObject = OOP_NewObject(dn->driverClass, NULL, NULL);
	dn->highBus = 0;

	NEWLIST(&dn->devices);
    
	drv = dn->driverObject;

	devtags[3].ti_Data = drv;
	
	// Add the driver to the end of drivers list
	AddTail(&PSD(cl)->drivers, (struct Node*)dn);

	// and scan whole PCI bus looking for devices available
	bus = 0;
	do
	{
	    D(bug("[PCI] Scanning bus %d\n",bus));

	    devtags[0].ti_Data = bus;
	    
	    for (dev=0; dev < 32; dev++)
	    {
		devtags[1].ti_Data = dev;
		devtags[2].ti_Data = 0;

		type = isPCIDeviceAvailable(cl, drv, bus,dev,0);
		switch(type)
		{
		    case 1:
			pcidev = (struct PciDevice *)AllocMem(sizeof(struct Device), MEMF_CLEAR);
			pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, &devtags);
			AddTail(&dn->devices, (struct Node *)pcidev);
			break;
		    case 2:
			pcidev = (struct PciDevice *)AllocMem(sizeof(struct Device), MEMF_CLEAR);
			pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, &devtags);
			AddTail(&dn->devices, (struct Node *)pcidev);
			for (sub=1; sub < 8; sub++)
			{
			    devtags[2].ti_Data = sub;
			    if (isPCIDeviceAvailable(cl, drv, bus, dev, sub))
			    {
			    	pcidev = (struct PciDevice *)AllocMem(sizeof(struct Device), MEMF_CLEAR);
				pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, &devtags);
				AddTail(&dn->devices, (struct Node *)pcidev);
			    }
			}
			break;
		    default:
			break;
		}
	    }
	    bus++;
	} while (bus <= dn->highBus);
    }
}

/* Class initialization and destruction */

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase	    (psd->sysbase)
#define OOPBase	    (psd->oopbase)
#define UtilityBase (psd->utilitybase)

void free_pciclass(struct pci_staticdata *psd, OOP_Class *cl)
{
    D(bug("[PCI] Subsystem destruction\n"));
    
    if (psd)
    {
	OOP_RemoveClass(cl);

	if (cl)
	    OOP_DisposeObject((OOP_Object *)cl);
	
	OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    }
}
	
#define _NUM_PCI_METHODS	1 //NUM_PCIDRIVER_METHODS

OOP_Class *init_pciclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr pci_descr[_NUM_PCI_METHODS + 1] =
    {
	{ OOP_METHODDEF(_PCI_AddHwDrv), moHidd_PCI_AddHardwareDriver },
	{ NULL, 0UL }
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
	{ pci_descr,  	    IID_Hidd_PCI,	_NUM_PCI_METHODS },
	{ NULL, NULL, 0UL }
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
	{ aMeta_SuperID,	(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	(IPTR)0 },
	{ aMeta_ID,		(IPTR)CLID_Hidd_PCI },
	{ TAG_DONE, 0UL }
    };

    D(bug("[PCI] base class initialization\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if (cl)
	{
	    cl->UserData = (APTR)psd;
	    psd->hiddPCIAB = OOP_ObtainAttrBase(IID_Hidd_PCI);
	    psd->hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

	    if (psd->hiddPCIAB && psd->hiddPCIDeviceAB)
	    {
		D(bug("[PCI] Everything OK\n"));
		OOP_AddClass(cl);
		psd->pciClass = cl;
		psd->pciObject = OOP_NewObject(cl, NULL, NULL);
	    }
	    else
	    {
		free_pciclass(psd, cl);
		cl = NULL;
	    }
	}
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("[PCI] ClassPtr = 0x%08x\n", cl));

    return cl;
}

