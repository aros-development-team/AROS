/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI device class
    Lang: English
*/

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "pci.h"
#include "pciutil.h"

#define DEBUG 0
#include <aros/debug.h>

#ifdef HiddPCIDeviceAttrBase
#undef HiddPCIDeviceAttrBase
#endif // HiddPCIDeviceAttrBase

#define	HiddPCIDeviceAttrBase	(PSD(cl)->hiddPCIDeviceAB)

#define SysBase		(PSD(cl)->sysbase)
#define OOPBase 	(PSD(cl)->oopbase)
#define UtilityBase	(PSD(cl)->utilitybase)

static void setLong(OOP_Class *cl, OOP_Object *o, ULONG reg, ULONG value)
{
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    OOP_Object *driver = dev->driver;

    struct pHidd_PCIDriver_WriteConfigLong msg;
    
    msg.mID = PSD(cl)->mid_WL;
    msg.bus = dev->bus;
    msg.dev = dev->dev;
    msg.sub = dev->sub;
    msg.reg = reg;
    msg.val = value;

    OOP_DoMethod(driver, (OOP_Msg)&msg);
}

static void setWord(OOP_Class *cl, OOP_Object *o, ULONG reg, UWORD value)
{
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    OOP_Object *driver = dev->driver;

    struct pHidd_PCIDriver_WriteConfigWord msg;
    
    msg.mID = PSD(cl)->mid_WW;
    msg.bus = dev->bus;
    msg.dev = dev->dev;
    msg.sub = dev->sub;
    msg.reg = reg;
    msg.val = value;

    OOP_DoMethod(driver, (OOP_Msg)&msg);
}

static void setByte(OOP_Class *cl, OOP_Object *o, ULONG reg, UBYTE value)
{
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    OOP_Object *driver = dev->driver;

    struct pHidd_PCIDriver_WriteConfigByte msg;
    
    msg.mID = PSD(cl)->mid_WB;
    msg.bus = dev->bus;
    msg.dev = dev->dev;
    msg.sub = dev->sub;
    msg.reg = reg;
    msg.val = value;

    OOP_DoMethod(driver, (OOP_Msg)&msg);
}

static ULONG getLong(OOP_Class *cl, OOP_Object *o, ULONG reg)
{
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    OOP_Object *driver = dev->driver;

    struct pHidd_PCIDriver_ReadConfigLong msg;
    
    msg.mID = PSD(cl)->mid_RL;
    msg.bus = dev->bus;
    msg.dev = dev->dev;
    msg.sub = dev->sub;
    msg.reg = reg;

    return OOP_DoMethod(driver, (OOP_Msg)&msg);  
}

static UWORD getWord(OOP_Class *cl, OOP_Object *o, ULONG reg)
{
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    OOP_Object *driver = dev->driver;

    struct pHidd_PCIDriver_ReadConfigWord msg;
    
    msg.mID = PSD(cl)->mid_RW;
    msg.bus = dev->bus;
    msg.dev = dev->dev;
    msg.sub = dev->sub;
    msg.reg = reg;

    return OOP_DoMethod(driver, (OOP_Msg)&msg);  
}

static UBYTE getByte(OOP_Class *cl, OOP_Object *o, ULONG reg)
{
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    OOP_Object *driver = dev->driver;

    struct pHidd_PCIDriver_ReadConfigByte msg;
    
    msg.mID = PSD(cl)->mid_RB;
    msg.bus = dev->bus;
    msg.dev = dev->dev;
    msg.sub = dev->sub;
    msg.reg = reg;

    return OOP_DoMethod(driver, (OOP_Msg)&msg);  
}

/*
    PCIDevice::New method is invoked by base pci class. It passes to the device
    class information about the driver this class should use and location of 
    created device on the PCI bus handled by given driver.
*/
static OOP_Object *pcidevice_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    int i;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct TagItem *tags, *tag;
	tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl, o);
	OOP_Object *driver = NULL;
	struct DriverNode *dn;
	
	tags=msg->attrList;

	/*
	    Get all information passed by pci class calling OOP_NewObject()
	*/
	while((tag = NextTagItem((struct TagItem **)&tags)))
	{
	    ULONG idx;

	    if (IS_PCIDEV_ATTR(tag->ti_Tag, idx))
	    {
		switch(idx)
		{
		    case aoHidd_PCIDevice_Driver:
			dev->driver = (OOP_Object*)tag->ti_Data;
			driver = dev->driver;
			break;
			
		    case aoHidd_PCIDevice_Bus:
			dev->bus = tag->ti_Data;
			break;

		    case aoHidd_PCIDevice_Dev:
			dev->dev = tag->ti_Data;
			break;

		    case aoHidd_PCIDevice_Sub:
			dev->sub = tag->ti_Data;
			break;
		}
	    }
	}

	/*
	    If driver is passed (*HAS TO* be passed) acquire some unchangeable
	    information regarding given device
	*/
	if (driver)
	{
	    UBYTE ht;
	    
	    /* HACK: look for the driver within private data of all pci classes */
	    ForeachNode(&(PSD(cl)->drivers), (struct Node *)dn)
	    {
		if (driver == dn->driverObject)
		    break;
	    }

	    /*
		Get the header type in order to determine whether it is a 
		device or bridge
	    */
	    ht = getByte(cl, o, PCICS_HEADERTYPE) & PCIHT_MASK;
	    dev->isBridge = 0;
	    if (ht == PCIHT_BRIDGE)
	    {
		/*  
		    It it a bridge. Get the subbus information and fix the
		    driver data in static PCI data. Sigh! should be replaced
		    with following code in base pci class:

		    pcidev->device = OOP_NewObject(NULL, CLID_Hidd_PCIDevice, (struct TagItem *)&devtags);
		    OOP_GetAttr(pcidev->device, aHidd_PCIDevice_isBridge, &bridge);
		    if (bridge)
		    {
			OOP_GetAttr(pcidev->device, aHidd_PCIDevice_SubBus, &subbus);
			if (subbus > dev->highBus)
			    dev->highBus = subbus;
		    }

		    BUT! The above code called for each device created would be
		    slightly slower (however more elegant). The pcidevice class
		    *Should* be completely independant on the pci class 
		    implementation.
		*/  
		
		dev->isBridge = 1;
		dev->subbus = getByte(cl, o, PCIBR_SUBBUS);
		
		/* Here comes ugly hack */
		if (dev->subbus > dn->highBus)
		    dn->highBus = dev->subbus;
	    }
	    
	    /* Get all constant ID's */
	    dev->VendorID = getWord(cl, o, PCICS_VENDOR);
	    dev->ProductID = getWord(cl, o, PCICS_PRODUCT);

	    dev->RevisionID = getByte(cl, o, PCICS_REVISION);
	    dev->Interface = getByte(cl, o, PCICS_PROGIF);
	    dev->SubClass = getByte(cl, o, PCICS_SUBCLASS);
	    dev->Class = getByte(cl, o, PCICS_CLASS);

	    dev->SubsysVID = getWord(cl, o, PCICS_SUBVENDOR);
	    dev->SubsystemID = getWord(cl, o, PCICS_SUBSYSTEM);
	    
	    dev->IRQLine = getByte(cl, o, PCICS_INT_PIN);

	    if (dev->IRQLine)
	    {
		dev->INTLine = getByte(cl, o, PCICS_INT_LINE);
	    }
	    else dev->INTLine = 0;
	    
	    dev->HeaderType = ht;

	    getPCIClassDesc(dev->Class, dev->SubClass, dev->Interface,
		&dev->strClass, &dev->strSubClass, &dev->strInterface);

	    /* Satisfy BUG watchers ;) */
	    D(bug("[PCIDevice] %02x.%02x.%x = %04.4lx:%04.4lx (%s %s %s)\n", 
		dev->bus, dev->dev, dev->sub,
		dev->VendorID, dev->ProductID,
		dev->strClass, dev->strSubClass, dev->strInterface));
    
	    /* Read two first base addresses */
	    for (i = 0; i < 2; i++)
	    {
		dev->BaseReg[i].addr = getLong(cl, o, PCICS_BAR0 + (i << 2));
	        dev->BaseReg[i].size = sizePCIBaseReg(driver, PSD(cl), dev->bus,
			dev->dev, dev->sub, i);
	    }

	    /* Address and size of ROM */
	    dev->RomBase = getLong(cl, o, PCICS_EXPROM_BASE);
	    dev->RomSize = sizePCIBaseReg(driver, PSD(cl), dev->bus,
			dev->dev, dev->sub, (PCICS_EXPROM_BASE - PCICS_BAR0) >> 2);
	    
	    /*
		Bridges specify only first two base addresses. If not bridge, 
		check the rest now
	    */
	    if (! dev->isBridge)
	    {
		for (i = 2; i < 6; i++)
		{
		    dev->BaseReg[i].addr = getLong(cl, o, PCICS_BAR0 + (i << 2));
		    dev->BaseReg[i].size = sizePCIBaseReg(driver, PSD(cl), dev->bus,
			dev->dev, dev->sub, i);
		}
	    }
	}
    }

    return o;
}

/*
    PCIDevice::Get method splitted into few parts in order to make switch'es shorter.
*/

static const void dispatch_generic(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    idx = msg->attrID - HiddPCIDeviceAttrBase;

	switch(idx)
	{
	    case aoHidd_PCIDevice_Driver:
		*msg->storage = (IPTR)dev->driver;
		break;

	    case aoHidd_PCIDevice_Bus:
		*msg->storage = (IPTR)dev->bus;
		break;

	    case aoHidd_PCIDevice_Dev:
		*msg->storage = (IPTR)dev->dev;
		break;

	    case aoHidd_PCIDevice_Sub:
		*msg->storage = (IPTR)dev->sub;
		break;
		
	    case aoHidd_PCIDevice_VendorID:
		*msg->storage = (IPTR)dev->VendorID;
		break;

	    case aoHidd_PCIDevice_ProductID:
		*msg->storage = (IPTR)dev->ProductID;
		break;

	    case aoHidd_PCIDevice_RevisionID:
		*msg->storage = (IPTR)dev->RevisionID;
		break;

	    case aoHidd_PCIDevice_Interface:
		*msg->storage = (IPTR)dev->Interface;
		break;

	    case aoHidd_PCIDevice_Class:
		*msg->storage = (IPTR)dev->Class;
		break;

	    case aoHidd_PCIDevice_SubClass:
		*msg->storage = (IPTR)dev->SubClass;
		break;

	    case aoHidd_PCIDevice_SubsystemVendorID:
		*msg->storage = (IPTR)dev->SubsysVID;
		break;

	    case aoHidd_PCIDevice_SubsystemID:
		*msg->storage = (IPTR)dev->SubsystemID;
		break;

	    case aoHidd_PCIDevice_INTLine:
		*msg->storage = (IPTR)dev->INTLine;
		break;

	    case aoHidd_PCIDevice_IRQLine:
		*msg->storage = (IPTR)dev->IRQLine;
		break;

	    case aoHidd_PCIDevice_RomBase:
		*msg->storage = (IPTR)dev->RomBase;
		break;

	    case aoHidd_PCIDevice_RomSize:
		*msg->storage = (IPTR)dev->RomSize;
		break;
	}
}

static const void dispatch_base(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx,id;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    idx = msg->attrID - HiddPCIDeviceAttrBase;
    id = 0;

    switch(idx)
    {
	case aoHidd_PCIDevice_Base0:	id = 0;	    break;
	case aoHidd_PCIDevice_Base1:    id = 1;	    break;
	case aoHidd_PCIDevice_Base2:    id = 2;	    break;
	case aoHidd_PCIDevice_Base3:    id = 3;	    break;
	case aoHidd_PCIDevice_Base4:    id = 4;	    break;
	case aoHidd_PCIDevice_Base5:    id = 5;	    break;
    }

    /* Do not allow reading of more than two base addresses in case of PCI-PCI bridges */
    if (dev->isBridge && id >= 2) { *msg->storage = 0; return; }

    *msg->storage = (IPTR)dev->BaseReg[id].addr;
    if ((dev->BaseReg[id].addr & PCIBAR_MASK_TYPE)==PCIBAR_TYPE_IO)
    {
	*msg->storage &= PCIBAR_MASK_IO;
    }
    else
    {
        *msg->storage &= PCIBAR_MASK_MEM;
    }
}

static const void dispatch_type(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx,id;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    idx = msg->attrID - HiddPCIDeviceAttrBase;
    id = 0;

    switch(idx)
    {
	case aoHidd_PCIDevice_Type0:	id = 0;	    break;
	case aoHidd_PCIDevice_Type1:    id = 1;	    break;
	case aoHidd_PCIDevice_Type2:    id = 2;	    break;
	case aoHidd_PCIDevice_Type3:    id = 3;	    break;
	case aoHidd_PCIDevice_Type4:    id = 4;	    break;
	case aoHidd_PCIDevice_Type5:    id = 5;	    break;
    }

    /* Do not allow reading of more than two base addresses in case of PCI-PCI bridges */
    if (dev->isBridge && id >= 2) { *msg->storage = 0; return; }

    *msg->storage = (IPTR)dev->BaseReg[id].addr;
    if ((dev->BaseReg[id].addr & PCIBAR_MASK_TYPE)==PCIBAR_TYPE_IO)
    {
	*msg->storage &= ~PCIBAR_MASK_IO;
    }
    else
    {
        *msg->storage &= ~PCIBAR_MASK_MEM;
    }
}

static const void dispatch_size(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx,id;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    idx = msg->attrID - HiddPCIDeviceAttrBase;
    id = 0;

    switch(idx)
    {
	case aoHidd_PCIDevice_Size0:	id = 0;	    break;
	case aoHidd_PCIDevice_Size1:    id = 1;	    break;
	case aoHidd_PCIDevice_Size2:    id = 2;	    break;
	case aoHidd_PCIDevice_Size3:    id = 3;	    break;
	case aoHidd_PCIDevice_Size4:    id = 4;	    break;
	case aoHidd_PCIDevice_Size5:    id = 5;	    break;
    }
    
    /* Do not allow reading of more than two base addresses in case of PCI-PCI bridges */
    if (dev->isBridge && id >= 2) { *msg->storage = 0; return;}

    *msg->storage = (IPTR)dev->BaseReg[id].size;
}

static const void dispatch_pci2pcibridge(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    UWORD control;

    /* If device is not a PCI-PCI bridge, do nothing */
    if (!dev->isBridge) { *msg->storage = 0; return; }

    control = getWord(cl, o, PCIBR_CONTROL);

    idx = msg->attrID - HiddPCIDeviceAttrBase;

    switch(idx)
    {
	case aoHidd_PCIDevice_isBridge:
	    *msg->storage = dev->isBridge;
	    break;
	
	case aoHidd_PCIDevice_SubBus:
	    *msg->storage = dev->subbus;
	    break;
	
	case aoHidd_PCIDevice_MemoryBase:
	    *msg->storage = getWord(cl, o, PCIBR_MEMBASE) << 16;
	    break;
	
	case aoHidd_PCIDevice_MemoryLimit:
	    *msg->storage = getWord(cl, o, PCIBR_MEMLIMIT) << 16;
	    break;
	
    	case aoHidd_PCIDevice_PrefetchableBase:
	    *msg->storage = getWord(cl, o, PCIBR_PREFETCHBASE) << 16;
	    break;
	
	case aoHidd_PCIDevice_PrefetchableLimit:
	    *msg->storage = getWord(cl, o, PCIBR_PREFETCHLIMIT) << 16;
	    break;
	
	case aoHidd_PCIDevice_IOBase:
	    *msg->storage = getByte(cl, o, PCIBR_IOBASE) << 8;
	    break;
	
	case aoHidd_PCIDevice_IOLimit:
	    *msg->storage = getByte(cl, o, PCIBR_IOLIMIT) << 8;
	    break;
	
	case aoHidd_PCIDevice_ISAEnable:
	    *msg->storage = (control & PCICTRLF_ISAENABLE) == PCICTRLF_ISAENABLE;
	    break;

	case aoHidd_PCIDevice_VGAEnable:
	    *msg->storage = (control & PCICTRLF_VGAENABLE) == PCICTRLF_VGAENABLE;
	    break;
    }
}

const static void (*Dispatcher[num_Hidd_PCIDevice_Attrs])(OOP_Class *, OOP_Object *, struct pRoot_Get *) __attribute__((section(".rodata"))) =
{
    [aoHidd_PCIDevice_Driver]	    = dispatch_generic,
    [aoHidd_PCIDevice_Bus]	    = dispatch_generic,
    [aoHidd_PCIDevice_Dev]	    = dispatch_generic,
    [aoHidd_PCIDevice_Sub]	    = dispatch_generic,
    [aoHidd_PCIDevice_VendorID]	    = dispatch_generic,
    [aoHidd_PCIDevice_ProductID]    = dispatch_generic,
    [aoHidd_PCIDevice_RevisionID]   = dispatch_generic,
    [aoHidd_PCIDevice_Interface]    = dispatch_generic,
    [aoHidd_PCIDevice_Class]	    = dispatch_generic,
    [aoHidd_PCIDevice_SubClass]	    = dispatch_generic,
    [aoHidd_PCIDevice_SubsystemVendorID] = dispatch_generic,
    [aoHidd_PCIDevice_SubsystemID]  = dispatch_generic,
    [aoHidd_PCIDevice_INTLine]	    = dispatch_generic,
    [aoHidd_PCIDevice_IRQLine]	    = dispatch_generic,
    [aoHidd_PCIDevice_RomBase]	    = dispatch_generic,
    [aoHidd_PCIDevice_RomSize]	    = dispatch_generic,
    [aoHidd_PCIDevice_Base0]	    = dispatch_base,
    [aoHidd_PCIDevice_Base1]	    = dispatch_base,
    [aoHidd_PCIDevice_Base2]	    = dispatch_base,
    [aoHidd_PCIDevice_Base3]	    = dispatch_base,
    [aoHidd_PCIDevice_Base4]	    = dispatch_base,
    [aoHidd_PCIDevice_Base5]	    = dispatch_base,
    [aoHidd_PCIDevice_Size0]	    = dispatch_size,
    [aoHidd_PCIDevice_Size1]	    = dispatch_size,
    [aoHidd_PCIDevice_Size2]	    = dispatch_size,
    [aoHidd_PCIDevice_Size3]	    = dispatch_size,
    [aoHidd_PCIDevice_Size4]	    = dispatch_size,
    [aoHidd_PCIDevice_Size5]	    = dispatch_size,
    [aoHidd_PCIDevice_Type0]	    = dispatch_type,
    [aoHidd_PCIDevice_Type1]	    = dispatch_type,
    [aoHidd_PCIDevice_Type2]	    = dispatch_type,
    [aoHidd_PCIDevice_Type3]	    = dispatch_type,
    [aoHidd_PCIDevice_Type4]	    = dispatch_type,
    [aoHidd_PCIDevice_Type5]	    = dispatch_type,

    /* Bridge attributes */
    [aoHidd_PCIDevice_isBridge]	    = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_SubBus]	    = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_MemoryBase]   = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_MemoryLimit]  = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_PrefetchableBase] = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_PrefetchableLimit] = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_IOBase]	    = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_IOLimit]	    = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_ISAEnable]    = dispatch_pci2pcibridge,
    [aoHidd_PCIDevice_VGAEnable]    = dispatch_pci2pcibridge,

};

static void pcidevice_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    if (IS_PCIDEV_ATTR(msg->attrID, idx))
    {
	if (Dispatcher[idx] != NULL)
	    Dispatcher[idx](cl, o, msg);
	else switch(idx)
	{
	    case aoHidd_PCIDevice_isMEM:
		*msg->storage = (
		    (getWord(cl, o, PCICS_COMMAND) &
			PCICMF_MEMDECODE)
		    == PCICMF_MEMDECODE);
		break;

	    case aoHidd_PCIDevice_isIO:
		*msg->storage = (
		    (getWord(cl, o, PCICS_COMMAND) &
			PCICMF_IODECODE)
		    == PCICMF_IODECODE);
		break;

	    case aoHidd_PCIDevice_isMaster:
		*msg->storage = (
		    (getWord(cl, o, PCICS_COMMAND) &
			PCICMF_BUSMASTER)
		    == PCICMF_BUSMASTER);
		break;

    	    case aoHidd_PCIDevice_paletteSnoop:
		*msg->storage = (
		    (getWord(cl, o, PCICS_COMMAND) &
			PCICMF_VGASNOOP)
		    == PCICMF_VGASNOOP);
		break;

	    case aoHidd_PCIDevice_is66MHz:
		*msg->storage = (
		    (getWord(cl, o, PCICS_STATUS) &
			PCISTF_66MHZ)
		    == PCISTF_66MHZ);
		break;

	    case aoHidd_PCIDevice_ClassDesc:
		*msg->storage = (IPTR)dev->strClass;
		break;

	    case aoHidd_PCIDevice_SubClassDesc:
		*msg->storage = (IPTR)dev->strSubClass;
		break;

	    case aoHidd_PCIDevice_InterfaceDesc:
		*msg->storage = (IPTR)dev->strInterface;
		break;

	    default:
		OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
		break;
	}
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    }
}

static void pcidevice_set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    ULONG idx;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    struct TagItem *tags, *tag;

    tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
	if (IS_PCIDEV_ATTR(tag->ti_Tag, idx))
	{
	    switch(idx)
	    {
		case aoHidd_PCIDevice_Base0:
		    setLong(cl, o, PCICS_BAR0, tag->ti_Data);
		    dev->BaseReg[0].addr = getLong(cl, o, PCICS_BAR0);
		    break;
		
		case aoHidd_PCIDevice_Base1:
		    setLong(cl, o, PCICS_BAR1, tag->ti_Data);
		    dev->BaseReg[1].addr = getLong(cl, o, PCICS_BAR1);
		    break;

		case aoHidd_PCIDevice_Base2:
		    if (!dev->isBridge)
		    {
		        setLong(cl, o, PCICS_BAR2, tag->ti_Data);
			dev->BaseReg[2].addr = getLong(cl, o, PCICS_BAR2);
		    }
		    break;
		
		case aoHidd_PCIDevice_Base3:
		    if (!dev->isBridge)
		    {
		        setLong(cl, o, PCICS_BAR3, tag->ti_Data);
			dev->BaseReg[3].addr = getLong(cl, o, PCICS_BAR3);
		    }
		    break;

		case aoHidd_PCIDevice_Base4:
		    if (!dev->isBridge)
		    {
		        setLong(cl, o, PCICS_BAR4, tag->ti_Data);
			dev->BaseReg[4].addr = getLong(cl, o, PCICS_BAR4);
		    }
		    break;

		case aoHidd_PCIDevice_Base5:
		    if (!dev->isBridge)
		    {
		        setLong(cl, o, PCICS_BAR5, tag->ti_Data);
			dev->BaseReg[5].addr = getLong(cl, o, PCICS_BAR5);
		    }
		    break;

		case aoHidd_PCIDevice_RomBase:
		    setLong(cl, o, PCICS_EXPROM_BASE, tag->ti_Data);
		    dev->RomBase = getLong(cl, o, PCICS_EXPROM_BASE);
		    break;

		case aoHidd_PCIDevice_MemoryBase:
		    if (dev->isBridge) setWord(cl, o, PCIBR_MEMBASE, tag->ti_Data >> 16); break;

		case aoHidd_PCIDevice_MemoryLimit:
		    if (dev->isBridge) setWord(cl, o, PCIBR_MEMLIMIT, tag->ti_Data >> 16); break;
 
		case aoHidd_PCIDevice_PrefetchableBase:
		    if (dev->isBridge) setWord(cl, o, PCIBR_PREFETCHBASE, tag->ti_Data >> 16); break;

		case aoHidd_PCIDevice_PrefetchableLimit:
		    if (dev->isBridge) setWord(cl, o, PCIBR_PREFETCHLIMIT, tag->ti_Data >> 16); break;
	
		case aoHidd_PCIDevice_IOBase:
		    if (dev->isBridge) setByte(cl, o, PCIBR_IOBASE, tag->ti_Data >> 8); break;

		case aoHidd_PCIDevice_IOLimit:
		    if (dev->isBridge) setByte(cl, o, PCIBR_IOLIMIT, tag->ti_Data >> 8); break;

		case aoHidd_PCIDevice_isIO:
		    {
			UWORD command = getWord(cl, o, PCICS_COMMAND) & ~PCICMF_IODECODE;
			if (tag->ti_Data)
			    command |= PCICMF_IODECODE;
			setWord(cl, o, PCICS_COMMAND, command);
		    }
		    break;
		
		case aoHidd_PCIDevice_isMEM:
		    {
			UWORD command = getWord(cl, o, PCICS_COMMAND) & ~PCICMF_MEMDECODE;
			if (tag->ti_Data)
			    command |= PCICMF_MEMDECODE;
			setWord(cl, o, PCICS_COMMAND, command);
		    }
		    break;
	
		case aoHidd_PCIDevice_isMaster:
		    {
			UWORD command = getWord(cl, o, PCICS_COMMAND) & ~PCICMF_BUSMASTER;
			if (tag->ti_Data)
			    command |= PCICMF_BUSMASTER;
			setWord(cl, o, PCICS_COMMAND, command);
		    }
		    break;
		
		case aoHidd_PCIDevice_paletteSnoop:
		    {
			UWORD command = getWord(cl, o, PCICS_COMMAND) & ~PCICMF_VGASNOOP;
			if (tag->ti_Data)
			    command |= PCICMF_VGASNOOP;
			setWord(cl, o, PCICS_COMMAND, command);
		    }
		    break;

		case aoHidd_PCIDevice_ISAEnable:
		    if (dev->isBridge)
		    {
			UWORD control = getWord(cl, o, PCIBR_CONTROL) & ~PCICTRLF_ISAENABLE;
			if (tag->ti_Data)
			    control |= PCICTRLF_ISAENABLE;
			setWord(cl, o, PCIBR_CONTROL, control);
		    }
		    break;

		case aoHidd_PCIDevice_VGAEnable:
		    if (dev->isBridge)
		    {
			UWORD control = getWord(cl, o, PCIBR_CONTROL) & ~PCICTRLF_VGAENABLE;
			if (tag->ti_Data)
			    control |= PCICTRLF_VGAENABLE;
			setWord(cl, o, PCIBR_CONTROL, control);
		    }
		    break;

		default:
		    bug("[PCIDevice] Trying to set nonsettable attribute %d!\n", idx);
		    break;
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

void free_pcideviceclass(struct pci_staticdata *psd, OOP_Class *cl)
{
    D(bug("[PCIDevice] Class destruction\n"));
    
    if (psd)
    {
	OOP_RemoveClass(cl);

	if (cl)
	    OOP_DisposeObject((OOP_Object *)cl);
	
	OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    }
}
	
#define _NUM_ROOT_METHODS	3
#define _NUM_PCIDEVICE_METHODS	0   /*NUM_PCIDRIVER_METHODS*/

OOP_Class *init_pcideviceclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[_NUM_ROOT_METHODS + 1] = 
    {
	{ OOP_METHODDEF(pcidevice_new), moRoot_New },
	{ OOP_METHODDEF(pcidevice_get),	moRoot_Get },
	{ OOP_METHODDEF(pcidevice_set), moRoot_Set },
	{ NULL, 0UL }
    };

    struct OOP_MethodDescr pcidevice_descr[_NUM_PCIDEVICE_METHODS + 1] =
    {
	{ NULL, 0UL }
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
	{ root_descr,	    IID_Root,		_NUM_ROOT_METHODS },
	{ pcidevice_descr,  IID_Hidd_PCIDevice,	_NUM_PCIDEVICE_METHODS },
	{ NULL, NULL, 0UL }
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
	{ aMeta_SuperID,	(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	(IPTR)sizeof(struct DeviceData) },
	{ aMeta_ID,		(IPTR)CLID_Hidd_PCIDevice },
	{ TAG_DONE, 0UL }
    };

    D(bug("[PCIDevice] initialization\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if (cl)
	{
	    cl->UserData = (APTR)psd;
	    psd->hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
	    if (psd->hiddPCIDeviceAB)
	    {
		OOP_AddClass(cl);
		D(bug("[PCIDevice] Class OK\n"));
	    }
	    else
	    {
		free_pcideviceclass(psd, cl);
		cl = NULL;
	    }
	}
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("[PCIDevice] ClassPtr = 0x08%x\n", cl));

    return cl;
}

