/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI class
    Lang: english
*/


#include <hidd/pcibus.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <aros/system.h>

#include "pci.h"

#define DEBUG 0
#include <aros/debug.h>

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddPCIAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_PCIBus,	&HiddPCIAttrBase	},
    { NULL, NULL }
};

struct pci_data
{
    int	dummy;	//dummy!!!!!!!!!
};

/*** PCI::FindDevice() *******************************************/

static HIDDT_PCI_Device **pci_finddevice(OOP_Class *cl, OOP_Object *obj, struct pHidd_PCI_FindDevice *msg)
{
	HIDDT_PCI_Device mask,*dev;
	Noded_PCI_Device *ndev;
	APTR *ret, *walk;
	int length;
	
	EnterFunc(bug("PCI::FindDevice()\n"));

	mask.VendorID			= GetTagData(tHidd_PCI_VendorID, -1, msg->deviceTags);
	mask.DeviceID			= GetTagData(tHidd_PCI_DeviceID, -1, msg->deviceTags);
	mask.RevisionID			= GetTagData(tHidd_PCI_RevisionID, -1, msg->deviceTags);
	mask.Class				= GetTagData(tHidd_PCI_Class, -1, msg->deviceTags);
	mask.SubClass			= GetTagData(tHidd_PCI_SubClass, -1, msg->deviceTags);
	mask.Interface			= GetTagData(tHidd_PCI_Interface, -1, msg->deviceTags);
	mask.SubsysVID	= GetTagData(tHidd_PCI_SubsystemVendorID, -1, msg->deviceTags);
	mask.SubsystemID		= GetTagData(tHidd_PCI_SubsystemID, -1, msg->deviceTags);

	D(bug("mask: %04.4lx:%04.4lx %d %d/%d/%d %x %x\n", mask.VendorID, mask.DeviceID, mask.RevisionID, mask.Class,
		mask.SubClass, mask.Interface, mask.SubsysVID, mask.SubsystemID));

	ListLength(&PSD(cl)->devices, length);

	D(bug("List is %d long \n", length));
	
	ret = walk = AllocVec(sizeof(APTR)*length, MEMF_PUBLIC | MEMF_CLEAR);

	ForeachNode(&PSD(cl)->devices, ndev)
	{
		dev = &ndev->dev;
	
		D(bug("d: %04.4lx:%04.4lx %d %d/%d/%d %x %x\n", dev->VendorID, dev->DeviceID, dev->RevisionID, dev->Class,
			dev->SubClass, dev->Interface, dev->SubsysVID, dev->SubsystemID));
	
		if (mask.VendorID != 0xffff)
		{
			if (mask.VendorID != dev->VendorID)
				continue;
		}
		if (mask.DeviceID != 0xffff)
		{
			if (mask.DeviceID != dev->DeviceID)
				continue;
		}
		if (mask.RevisionID != 0xff)
		{
			if (mask.RevisionID != dev->RevisionID)
				continue;
		}
		if (mask.Class != 0xff)
		{
			if (mask.Class != dev->Class)
				continue;
		}
		if (mask.SubClass != 0xff)
		{
			if (mask.SubClass != dev->SubClass)
				continue;
		}
		if (mask.Interface != 0xff)
		{
			if (mask.Interface != dev->Interface)
				continue;
		}
		if (mask.SubsysVID != 0xffff)
		{
			if (mask.SubsysVID != dev->SubsysVID)
				continue;
		}
		if (mask.SubsystemID != 0xffff)
		{
			if (mask.SubsystemID != dev->SubsystemID)
				continue;
		}
		*walk++ = dev;
		
		D(bug("Found suitable device\n"));
	}

	ReturnPtr("PCI::FindDevice", HIDDT_PCI_Device **, ret);
}

/*** PCI::FreeQuery() ********************************************/

static void pci_freequery(OOP_Class *cl, OOP_Object *obj, struct pHidd_PCI_FreeQuery *msg)
{
	if (msg->devices)
	{
		FreeVec(msg->devices);
	}
}

/*************************** Classes *****************************/

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase     (psd->sysbase)
#define OOPBase     (psd->oopbase)
#define UtilityBase (psd->utilitybase)

#define NUM_PCI_METHODS moHidd_PCI_NumMethods

OOP_Class *init_pciclass (struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;
    BOOL  ok  = FALSE;
    
    struct OOP_MethodDescr pcihidd_descr[NUM_PCI_METHODS + 1] = 
    {
		{(IPTR (*)())pci_finddevice,	moHidd_PCI_FindDevice},
		{(IPTR (*)())pci_freequery,		moHidd_PCI_FreeQuery},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {pcihidd_descr, IID_Hidd_PCIBus, NUM_PCI_METHODS},
        {NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
        
    struct TagItem tags[] =
    {
        { aMeta_SuperID,                (IPTR)CLID_Root},

        { aMeta_InterfaceDescr,         (IPTR)ifdescr},
        { aMeta_ID,                     (IPTR)CLID_Hidd_PCIBus},
        { aMeta_InstSize,               (IPTR)sizeof (struct pci_data) },
        {TAG_DONE, 0UL}
    };
    
    EnterFunc(bug("    init_pciclass(psd=%p)\n", psd));

    cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    D(bug("Class=%p\n", cl));
    if(cl)
    {
        D(bug("PCIClass ok\n"));
        cl->UserData = (APTR)psd;
       	psd->pciclass = cl;
	psd->highBus = 0;
        
        ok = TRUE;
    }

    if(ok == FALSE)
    {
        free_pciclass(psd);
        cl = NULL;
    }
    else
    {
        OOP_AddClass(cl);
    }

    ReturnPtr("init_pciclass", OOP_Class *, cl);
}

void free_pciclass(struct pci_staticdata *psd)
{
    EnterFunc(bug("free_pciclass(psd=%p)\n", psd));

    if(psd)
    {
        OOP_RemoveClass(psd->pciclass);
	
        if(psd->pciclass) OOP_DisposeObject((OOP_Object *) psd->pciclass);
        psd->pciclass = NULL;
    }

    ReturnVoid("free_pciclass");
}

