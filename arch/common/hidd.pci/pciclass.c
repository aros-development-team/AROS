/*
    Copyright (C) 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "pci.h"

#define DEBUG 0
#include <aros/debug.h>

/*
    There are no static AttrBases in this class. Therefore it might be placed
    directly in ROM without any harm
*/
#undef HiddPCIAttrBase
#undef HiddPCIDeviceAttrBase

#define	HiddPCIAttrBase		(PSD(cl)->hiddPCIAB)
#define HiddPCIDeviceAttrBase	(PSD(cl)->hiddPCIDeviceAB)
#define HiddAttrBase		(PSD(cl)->hiddAB)

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
    PCI::AddHardwareDriver(OOP_Class *driverClass)

    Adds new PCI hardware driver to the PCI subsystem. A PCI hardware driver
    is a class which delivers Read/Write to the PCI config space.

    The PCI bus handled through driver added is scanned, and all available
    PCI devices are added to the device chain.
*/
static void _PCI_AddHwDrv(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCI_AddHardwareDriver *msg)
{
    struct DriverNode *dn = NULL;

    D(bug("[PCI] Adding Driver class 0x%08x\n", msg->driverClass));
    
    if (msg->driverClass != NULL)
    {
        // Get some extra memory for driver node
        dn = AllocMem(sizeof(struct DriverNode), MEMF_CLEAR);
        if (dn)
        {
	    int bus;
	    int dev;
	    int sub;
	    int type;
	    OOP_Object *drv;
	    struct TagItem devtags[] = {
		{ aHidd_PCIDevice_Bus, 0 },
		{ aHidd_PCIDevice_Dev, 0 },
		{ aHidd_PCIDevice_Sub, 0 },
		{ aHidd_PCIDevice_Driver, 0 },
		{ TAG_DONE, 0UL }
	    };
	    struct PciDevice *pcidev;
	    STRPTR string, string2;
	
	    dn->driverClass = msg->driverClass;
	    drv = dn->driverObject = OOP_NewObject(dn->driverClass, NULL, NULL);
	    dn->highBus = 0;

	    OOP_GetAttr(drv, aHidd_Name, (APTR)&string);
	    OOP_GetAttr(drv, aHidd_HardwareName, (APTR)&string2);
	    D(bug("[PCI] Adding driver %s (%s) to the system\n", string, string2));

	    NEWLIST(&dn->devices);
    
	    devtags[3].ti_Data = (IPTR)drv;
	
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

		    /* Knock knock! Is any device here? */
		    type = isPCIDeviceAvailable(cl, drv, bus,dev,0);
		    switch(type)
		    {
			/* Regular device */
			case 1:
			    pcidev = (struct PciDevice *)AllocMem(sizeof(struct Device), MEMF_CLEAR);
			    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, 
						(struct TagItem *)&devtags);
			    AddTail(&dn->devices, (struct Node *)pcidev);
			    break;
			/* Cool! Multifunction device, search subfunctions then */
			case 2:
			    pcidev = (struct PciDevice *)AllocMem(sizeof(struct Device), MEMF_CLEAR);
			    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, 
						(struct TagItem *)&devtags);
			    AddTail(&dn->devices, (struct Node *)pcidev);
			    for (sub=1; sub < 8; sub++)
			    {
				devtags[2].ti_Data = sub;
				if (isPCIDeviceAvailable(cl, drv, bus, dev, sub))
				{
				    pcidev = (struct PciDevice *)AllocMem(sizeof(struct Device), MEMF_CLEAR);
				    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, 
							(struct TagItem *)&devtags);
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
}

/*
    PCI::EnumDevices(struct Hook *Callback, struct TagItem **requirements)

    This method calls the callback hook for every PCI device in the system
    that mets requirements specified (or every device if tags=NULL). It
    iterates not only through one PCI bus, but instead through all buses
    managed by all drivers present in the system.
*/
static void _PCI_EnumDevs(OOP_Class *cl, OOP_Object *o, struct pHidd_PCI_EnumDevices *msg)
{
    ULONG   VendorID, ProductID, RevisionID, Interface, _Class, SubClass, 
	    SubsystemVendorID, SubsystemID;
    
    ULONG   value;
    struct  DriverNode	*dn;
    struct  PciDevice	*dev;
    BOOL    ok;
    
    /* Get requirements */
    VendorID	= GetTagData(tHidd_PCI_VendorID,    0xffffffff, msg->requirements);
    ProductID	= GetTagData(tHidd_PCI_ProductID,   0xffffffff, msg->requirements);
    RevisionID	= GetTagData(tHidd_PCI_RevisionID,  0xffffffff, msg->requirements);
    Interface	= GetTagData(tHidd_PCI_Interface,   0xffffffff, msg->requirements);
    _Class	= GetTagData(tHidd_PCI_Class,	    0xffffffff, msg->requirements);
    SubClass	= GetTagData(tHidd_PCI_SubClass,    0xffffffff, msg->requirements);
    SubsystemID	= GetTagData(tHidd_PCI_SubsystemID, 0xffffffff, msg->requirements);
    SubsystemVendorID = GetTagData(tHidd_PCI_SubsystemVendorID, 0xffffffff, msg->requirements);

    /* For every driver in the system... */
    ForeachNode(&(PSD(cl)->drivers), (struct Node *)dn)
    {
	/* ...and for every device handled by this driver */
	ForeachNode(&(dn->devices), (struct Node *)dev)
	{
	    /* check the requirements with it's properties */
	    ok = TRUE;
	    if (VendorID != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_VendorID, &value);
		ok &= (value == VendorID);
	    }

	    if (ProductID != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_ProductID, &value);
		ok &= (value == ProductID);
	    }

	    if (RevisionID != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_RevisionID, &value);
		ok &= (value == RevisionID);
	    }
	    
	    if (Interface != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_Interface, &value);
		ok &= (value == Interface);
	    }

	    if (_Class != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_Class, &value);
		ok &= (value == _Class);
	    }

	    if (SubClass != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_SubClass, &value);
		ok &= (value == SubClass);
	    }

	    if (SubsystemVendorID != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_SubsystemVendorID, &value);
		ok &= (value == SubsystemVendorID);
	    }

	    if (SubsystemID != 0xffffffff)
	    {
		OOP_GetAttr(dev->device, aHidd_PCIDevice_SubsystemID, &value);
		ok &= (value == SubsystemID);
	    }

	    /* If requirements met, call Hook */
	    if (ok)
	    {
		CALLHOOKPKT(msg->callback, dev->device, NULL);
	    }
	}
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
	
#define _NUM_PCI_METHODS	2 //NUM_PCIDRIVER_METHODS

OOP_Class *init_pciclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr pci_descr[_NUM_PCI_METHODS + 1] =
    {
	{ OOP_METHODDEF(_PCI_AddHwDrv), moHidd_PCI_AddHardwareDriver },
	{ OOP_METHODDEF(_PCI_EnumDevs), moHidd_PCI_EnumDevices },
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
	    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);

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

