/*
    Copyright � 2004-2010, The AROS Development Team. All rights reserved.
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

#define DEBUG 1
#include <aros/debug.h>

#ifdef HiddPCIDeviceAttrBase
#undef HiddPCIDeviceAttrBase
#endif // HiddPCIDeviceAttrBase

#define	HiddPCIDeviceAttrBase	(PSD(cl)->hiddPCIDeviceAB)

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

/* Returns offset of capability area in config area or 0 of capability is not present */
static UBYTE findCapabilityOffset(OOP_Class * cl, OOP_Object *o, UBYTE capability)
{
    UWORD where = 0x34; /*  First cap list entry */
    UBYTE capid = 0;
    
    /* Check if capabilities present at all */
    if ((getWord(cl, o, PCICS_STATUS) & PCISTF_CAPABILITIES) != PCISTF_CAPABILITIES)
        return 0;
    
    /* Iterate over capabilities */
    while(where < 0xff)
    {
        where = getByte(cl, o, where);

        if (where < 0x40)
            break;

        where &= ~3;
        capid = getByte(cl, o, where);

        if (capid == 0xff)
            break;

        if (capid == capability) return (UBYTE)where;
    
        where += 1; /* next cap */
    }
    
    return 0;
}

UBYTE PCIDev__Hidd_PCIDevice__ReadConfigByte(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ReadConfigByte *msg)
{
    return getByte(cl, o, msg->reg);
}

UWORD PCIDev__Hidd_PCIDevice__ReadConfigWord(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ReadConfigWord *msg)
{
    return getWord(cl, o, msg->reg);
}

ULONG PCIDev__Hidd_PCIDevice__ReadConfigLong(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_ReadConfigLong *msg)
{
    return getLong(cl, o, msg->reg);
}

VOID PCIDev__Hidd_PCIDevice__WriteConfigByte(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_WriteConfigByte *msg)
{
    setByte(cl, o, msg->reg, msg->val);
}

VOID PCIDev__Hidd_PCIDevice__WriteConfigWord(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_WriteConfigWord *msg)
{
    setWord(cl, o, msg->reg, msg->val);
}

VOID PCIDev__Hidd_PCIDevice__WriteConfigLong(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDevice_WriteConfigLong *msg)
{
    setLong(cl, o, msg->reg, msg->val);
}


/*
    PCIDevice::New method is invoked by base pci class. It passes to the device
    class information about the driver this class should use and location of 
    created device on the PCI bus handled by given driver.
*/
OOP_Object *PCIDev__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    int i;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	const struct TagItem *tags;
	struct TagItem *tag;
	tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl, o);
	OOP_Object *driver = NULL;
	
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
	    
	    /*
		Get the header type in order to determine whether it is a 
		device or bridge
	    */
	    ht = getByte(cl, o, PCICS_HEADERTYPE) & PCIHT_MASK;
	    dev->isBridge = 0;
	    if (ht == PCIHT_BRIDGE)
	    {
		dev->isBridge = 1;
		dev->subbus = getByte(cl, o, PCIBR_SUBBUS);
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
        // print out a warning to the user in case the interrupt line is not assigned by BIOS
        if(dev->INTLine == 255)
        {
            bug("[PCIDevice] WARNING: Interrupt line is not assigned! Device may freeze or malfunction at use!\n");
        }
    
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

static void dispatch_generic(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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

static void dispatch_base(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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

static void dispatch_type(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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

static void dispatch_size(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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

static void dispatch_pci2pcibridge(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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

static void dispatch_capability(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    UBYTE capability = 0;
    //tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    idx = msg->attrID - HiddPCIDeviceAttrBase;

    switch(idx)
    {
    case aoHidd_PCIDevice_CapabilityPowerManagement:    capability = PCICAP_POWER_MANAGEMENT;break;
    case aoHidd_PCIDevice_CapabilityAGP:                capability = PCICAP_AGP;break;
    case aoHidd_PCIDevice_CapabilityVitalProductData:   capability = PCICAP_VITAL_PRODUCT_DATA;break;
    case aoHidd_PCIDevice_CapabilitySlotID:             capability = PCICAP_SLOT_ID;break;
    case aoHidd_PCIDevice_CapabilityMSI:                capability = PCICAP_MSI;break;
    case aoHidd_PCIDevice_CapabilityCPCIHotSwap:        capability = PCICAP_CPCI_HOT_SWAP;break;
    case aoHidd_PCIDevice_CapabilityPCIX:               capability = PCICAP_PCIX;break;
    case aoHidd_PCIDevice_CapabilityHyperTransport:     capability = PCICAP_HYPER_TRANSPORT;break;
    case aoHidd_PCIDevice_CapabilityVendorSpecific:     capability = PCICAP_VENDOR_SPECIFIC;break;
    case aoHidd_PCIDevice_CapabilityDebugPort:          capability = PCICAP_DEBUG_PORT;break;
    case aoHidd_PCIDevice_CapabilityCPCICRC:            capability = PCICAP_CPCI_CR;break;
    case aoHidd_PCIDevice_CapabilityHotPlugController:  capability = PCICAP_HOT_PLUG_CONTROLLER;break;
    case aoHidd_PCIDevice_CapabilitySSVPID:             capability = PCICAP_SSVPID;break;
    case aoHidd_PCIDevice_CapabilityAGP3:               capability = PCICAP_AGP3;break;
    case aoHidd_PCIDevice_CapabilityPCIE:               capability = PCICAP_PCIE;break;
    case aoHidd_PCIDevice_CapabilityMSIX:               capability = PCICAP_MSIX;break;
    case aoHidd_PCIDevice_CapabilityAdvancedFeatures:   capability = PCICAP_ADVANCED_FEATURES;break;
    }

    *msg->storage = findCapabilityOffset(cl, o, capability);
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
    
    /* Capabilities */
    [aoHidd_PCIDevice_CapabilityPowerManagement]    = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityAGP]                = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityVitalProductData]   = dispatch_capability,
    [aoHidd_PCIDevice_CapabilitySlotID]             = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityMSI]                = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityCPCIHotSwap]        = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityPCIX]               = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityHyperTransport]     = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityVendorSpecific]     = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityDebugPort]          = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityCPCICRC]            = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityHotPlugController]  = dispatch_capability,
    [aoHidd_PCIDevice_CapabilitySSVPID]             = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityAGP3]               = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityPCIE]               = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityMSIX]               = dispatch_capability,
    [aoHidd_PCIDevice_CapabilityAdvancedFeatures]   = dispatch_capability,
};

void PCIDev__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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
        case aoHidd_PCIDevice_IRQStatus:
       		*msg->storage = (
		    (getWord(cl, o, PCICS_STATUS) &
			PCISTF_INTERRUPT_STATUS)
		    == PCISTF_INTERRUPT_STATUS);
		break;
        case aoHidd_PCIDevice_CapabilitiesPresent:
       		*msg->storage = (
		    (getWord(cl, o, PCICS_STATUS) &
			PCISTF_CAPABILITIES)
		    == PCISTF_CAPABILITIES);
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

void PCIDev__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    ULONG idx;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);
    const struct TagItem *tags;
    struct TagItem *tag;

    tags = msg->attrList;

    while ((tag = NextTagItem((struct TagItem **)&tags)))
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
