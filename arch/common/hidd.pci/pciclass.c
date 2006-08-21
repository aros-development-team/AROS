/*
    Copyright (C) 2004-2006, The AROS Development Team. All rights reserved.
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

#include <aros/symbolsets.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>
#include <aros/atomic.h>

/*
    There are no static AttrBases in this class. Therefore it might be placed
    directly in ROM without any harm
*/
#undef HiddPCIAttrBase
#undef HiddPCIDeviceAttrBase

#define	HiddPCIAttrBase		(PSD(cl)->hiddPCIAB)
#define HiddPCIDeviceAttrBase	(PSD(cl)->hiddPCIDeviceAB)
#define HiddAttrBase		(PSD(cl)->hiddAB)

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
void PCI__Hidd_PCI__AddHardwareDriver(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCI_AddHardwareDriver *msg)
{
    struct DriverNode *dn = NULL;

    D(bug("[PCI] Adding Driver class 0x%08x\n", msg->driverClass));
    
    if (msg->driverClass != NULL)
    {
        // Get some extra memory for driver node
        dn = AllocPooled(PSD(cl)->MemPool, sizeof(struct DriverNode));
        if (dn)
        {
	    int bus;
	    int dev;
	    int sub;
	    int type;
	    IPTR subbus, bridge;

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
	
	    // Scan whole PCI bus looking for devices available
	    // There is no need for semaphore protected list operations at this
	    // point, because driver is still not public.
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
			    pcidev = (struct PciDevice *)AllocPooled(PSD(cl)->MemPool,
				sizeof(struct Device));
			    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, 
						(struct TagItem *)&devtags);

			    OOP_GetAttr(pcidev->device, aHidd_PCIDevice_isBridge, &bridge);
			    if (bridge)
			    {
				OOP_GetAttr(pcidev->device, aHidd_PCIDevice_SubBus, &subbus);
				if (subbus > dn->highBus)
				    dn->highBus = subbus;
			    }
			    AddTail(&dn->devices, (struct Node *)pcidev);
			    break;
			/* Cool! Multifunction device, search subfunctions then */
			case 2:
			    pcidev = (struct PciDevice *)AllocPooled(PSD(cl)->MemPool,
				sizeof(struct Device));
			    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, 
						(struct TagItem *)&devtags);
	
			    OOP_GetAttr(pcidev->device, aHidd_PCIDevice_isBridge, &bridge);
			    if (bridge)
			    {
				OOP_GetAttr(pcidev->device, aHidd_PCIDevice_SubBus, &subbus);
				if (subbus > dn->highBus)
				    dn->highBus = subbus;
			    }
			    AddTail(&dn->devices, (struct Node *)pcidev);
			    
			    for (sub=1; sub < 8; sub++)
			    {
				devtags[2].ti_Data = sub;
				if (isPCIDeviceAvailable(cl, drv, bus, dev, sub))
				{
				    pcidev = (struct PciDevice *)AllocPooled(PSD(cl)->MemPool,
					sizeof(struct Device));
				    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, 
							(struct TagItem *)&devtags);
				    OOP_GetAttr(pcidev->device, aHidd_PCIDevice_isBridge, &bridge);
				    if (bridge)
				    {
					OOP_GetAttr(pcidev->device, aHidd_PCIDevice_SubBus, &subbus);
					if (subbus > dn->highBus)
					    dn->highBus = subbus;
				    }
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

	    // Add the driver to the end of drivers list

	    ObtainSemaphore(&PSD(cl)->driver_lock);
	    AddTail(&PSD(cl)->drivers, (struct Node*)dn);
	    ReleaseSemaphore(&PSD(cl)->driver_lock);
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
void PCI__Hidd_PCI__EnumDevices(OOP_Class *cl, OOP_Object *o, struct pHidd_PCI_EnumDevices *msg)
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

    /* Lock driver list for exclusive use */
    ObtainSemaphore(&PSD(cl)->driver_lock);

    /* For every driver in the system... */
    ForeachNode(&PSD(cl)->drivers, dn)
    {
	/* ...and for every device handled by this driver */
	ForeachNode(&dn->devices, dev)
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

    ReleaseSemaphore(&PSD(cl)->driver_lock);
}

BOOL PCI__Hidd_PCI__RemHardwareDriver(OOP_Class *cl, OOP_Object *o, struct pHidd_PCI_RemHardwareDriver *msg)
{
    struct DriverNode *dn = NULL, *next = NULL, *rem = NULL;
    BOOL freed = FALSE;

    D(bug("[PCI] Removing hardware driver %x\n",msg->driverClass));
    /*
	Removing HW driver allowed only if classes unused. That means the users
	count should be == 1 (only driver itself uses pci to remove its class)
    */
    Forbid();
    if (PSD(cl)->users == 1)
    {
	/* Get exclusive lock on driver list */
	ObtainSemaphore(&PSD(cl)->driver_lock);
	ForeachNodeSafe(&PSD(cl)->drivers, dn, next)
	{
	    if (dn->driverClass == msg->driverClass)
	    {
		Remove((struct Node *)dn);
		rem = dn;
	    }
	}
        ReleaseSemaphore(&PSD(cl)->driver_lock);

	/* If driver removed, rem contains pointer to removed DriverNode */
	if (rem)
	{
	    struct PciDevice *dev, *next;

	    /* For every device */
	    ForeachNodeSafe(&rem->devices, dev, next)
	    {
		/* Dispose PCIDevice object instance */
		OOP_DisposeObject(dev->device);

		/* Remove device from device list */
		Remove((struct Node *)dev);

		/* Free memory used for device struct */
		FreePooled(PSD(cl)->MemPool, dev, sizeof(struct PciDevice));
	    }
	    
	    /* Dispose driver */
	    OOP_DisposeObject(rem->driverObject);

	    /* And free memory for DriverNode */
	    FreePooled(PSD(cl)->MemPool, rem, sizeof(struct DriverNode));

	    /* Driver removed and everything freed */
	    freed = TRUE;
	}
    }
    Permit();
    
    D(bug("[PCI] PCI::RemHardwareDriver() %s\n", freed?"succeeded":"failed"));
 
    return freed;
}

OOP_Object *PCI__Root__New(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    AROS_ATOMIC_INC(PSD(cl)->users);
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, msg);
}

VOID PCI__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    AROS_ATOMIC_DEC(PSD(cl)->users);
    OOP_DoSuperMethod(cl, o, msg);
}

/* Class initialization and destruction */

static int PCI_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI] Base Class destruction\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCI);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
}
	
static int PCI_InitClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCI] base class initialization\n"));

    LIBBASE->psd.hiddPCIAB = OOP_ObtainAttrBase(IID_Hidd_PCI);
    LIBBASE->psd.hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);

    if (LIBBASE->psd.hiddPCIAB && LIBBASE->psd.hiddPCIDeviceAB && LIBBASE->psd.hiddPCIDriverAB && LIBBASE->psd.hiddAB)
    {
	D(bug("[PCI] Everything OK\n"));
	return TRUE;
    }

    return FALSE;
}

ADD2INITLIB(PCI_InitClass, 0)
ADD2EXPUNGELIB(PCI_ExpungeClass, 0)
