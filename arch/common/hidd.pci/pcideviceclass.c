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

#define	HiddPCIDeviceAttrBase	(PSD(cl)->hiddPCIDeviceAB)

#define SysBase		(PSD(cl)->sysbase)
#define OOPBase 	(PSD(cl)->oopbase)
#define UtilityBase	(PSD(cl)->utilitybase)

static void pcidevice_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    int i;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
	struct TagItem *tags, *tag;
	tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl, o);
	OOP_Object *driver;
	struct DriverNode *dn;
	
	tags=msg->attrList;

	while((tag = NextTagItem((const struct TagItem **)&tags)))
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
			dev->bus = (UBYTE)tag->ti_Data;
			break;

		    case aoHidd_PCIDevice_Dev:
			dev->dev = (UBYTE)tag->ti_Data;
			break;

		    case aoHidd_PCIDevice_Sub:
			dev->sub = (UBYTE)tag->ti_Data;
			break;
		}
	    }
	}

	if (driver)
	{
	    STRPTR cDesc,sDesc,pDesc;
	    UBYTE ht;
	    struct pHidd_PCIDriver_ReadConfigByte readByte;
	    struct pHidd_PCIDriver_ReadConfigWord readWord;
	    struct pHidd_PCIDriver_ReadConfigLong readLong;
	    
	    ForeachNode(&(PSD(cl)->drivers), (struct Node *)dn)
	    {
		if (driver == dn->driverObject)
		    break;
	    }
	    if (!PSD(cl)->mid_RB)
		PSD(cl)->mid_RB = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigByte);
	    if (!PSD(cl)->mid_RW)
		PSD(cl)->mid_RW = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigWord);
	    if (!PSD(cl)->mid_RL)
		PSD(cl)->mid_RL = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);

	    readByte.mID = PSD(cl)->mid_RB;
	    readByte.bus = dev->bus;
	    readByte.dev = dev->dev;
	    readByte.sub = dev->sub;
	    readWord.mID = PSD(cl)->mid_RW;
	    readWord.bus = dev->bus;
	    readWord.dev = dev->dev;
	    readWord.sub = dev->sub;
	    readLong.mID = PSD(cl)->mid_RL;
	    readLong.bus = dev->bus;
	    readLong.dev = dev->dev;
	    readLong.sub = dev->sub;
	    
	    readByte.reg = PCICS_HEADERTYPE;
	    ht = OOP_DoMethod(driver, (OOP_Msg)&readByte) & PCIHT_MASK;
	    dev->isBridge = 0;
	    if (ht == PCIHT_BRIDGE)
	    {
		dev->isBridge = 1;
		readByte.reg = PCIBR_SUBBUS;
		dev->subbus = OOP_DoMethod(driver, (OOP_Msg)&readByte);
		
		if (dev->subbus > dn->highBus)
		    dn->highBus = dev->subbus;
	    }
	    
	    readWord.reg = PCICS_VENDOR;
	    dev->VendorID = OOP_DoMethod(driver, (OOP_Msg)&readWord);
	    readWord.reg = PCICS_PRODUCT;
	    dev->ProductID = OOP_DoMethod(driver, (OOP_Msg)&readWord);

	    readByte.reg = PCICS_REVISION;
	    dev->RevisionID = OOP_DoMethod(driver, (OOP_Msg)&readByte);
	    readByte.reg = PCICS_PROGIF;
	    dev->Interface = OOP_DoMethod(driver, (OOP_Msg)&readByte);
	    readByte.reg = PCICS_SUBCLASS;
	    dev->SubClass = OOP_DoMethod(driver, (OOP_Msg)&readByte);
	    readByte.reg = PCICS_CLASS;
	    dev->Class = OOP_DoMethod(driver, (OOP_Msg)&readByte);

	    readWord.reg = PCICS_SUBVENDOR;
	    dev->SubsysVID = OOP_DoMethod(driver, (OOP_Msg)&readWord);
	    readWord.reg = PCICS_SUBSYSTEM;
	    dev->SubsystemID = OOP_DoMethod(driver, (OOP_Msg)&readWord);

	    readByte.reg = PCICS_INT_LINE;
	    dev->INTLine = OOP_DoMethod(driver, (OOP_Msg)&readByte);
	    readByte.reg = PCICS_INT_PIN;
	    dev->IRQLine = OOP_DoMethod(driver, (OOP_Msg)&readByte);
	    
	    dev->HeaderType = ht;
	    
	    getPCIClassDesc(dev->Class, dev->SubClass, dev->Interface,
		&cDesc, &sDesc, &pDesc);

	    D(bug("[PCIDevice] %02x.%02x.%x = %04.4lx:%04.4lx (%s %s %s)\n", 
		dev->bus, dev->dev, dev->sub,
		dev->VendorID, dev->ProductID,
		cDesc, sDesc, pDesc));

	    for (i = 0; i < 6; i++)
	    {
		readLong.reg = PCICS_BAR0 + i;
		dev->BaseReg[i].addr = OOP_DoMethod(driver, (OOP_Msg)&readLong);
		dev->BaseReg[i].size = sizePCIBaseReg(driver, PSD(cl), dev->bus,
		    dev->dev, dev->sub, i);	    
	    }
	}
    }
}

static void pcidevice_get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    tDeviceData *dev = (tDeviceData *)OOP_INST_DATA(cl,o);

    if (IS_PCIDEV_ATTR(msg->attrID, idx))
    {
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
		break;

	    case aoHidd_PCIDevice_ProductID:
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
	
#define _NUM_ROOT_METHODS	2
#define _NUM_PCIDEVICE_METHODS	0   /*NUM_PCIDRIVER_METHODS*/

OOP_Class *init_pcideviceclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[_NUM_ROOT_METHODS + 1] = 
    {
	{ OOP_METHODDEF(pcidevice_new), moRoot_New },
	{ OOP_METHODDEF(pcidevice_get),	moRoot_Get },
	{ NULL, 0UL }
    };

    struct OOP_MethodDescr pcidriver_descr[_NUM_PCIDEVICE_METHODS + 1] =
    {
	{ NULL, 0UL }
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
	{ root_descr,	    IID_Root,		_NUM_ROOT_METHODS },
	{ pcidriver_descr,  IID_Hidd_PCIDevice,	_NUM_PCIDEVICE_METHODS },
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

