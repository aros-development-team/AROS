#ifndef HIDD_PCI_H
#define HIDD_PCI_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for the PCI HIDD system.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#	include <utility/tagitem.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>

#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <utility/utility.h>

#define CLID_Hidd_PCIBus       "hidd.bus.pci"
#define IID_Hidd_PCIBus        "hidd.bus.pci"

/**** pci definitions ****************************************************/

enum
{
	/* Methods for PCI hidd */

	moHidd_PCI_FindDevice = 0,
	moHidd_PCI_FreeQuery,
	
	moHidd_PCI_NumMethods
};

enum
{
	/* Parameters for FindDevice method */
	
	tHidd_PCI_VendorID		= TAG_USER,
	tHidd_PCI_DeviceID,
	tHidd_PCI_RevisionID,
	tHidd_PCI_Class,
	tHidd_PCI_SubClass,
	tHidd_PCI_Interface,
	tHidd_PCI_SubsystemVendorID,
	tHidd_PCI_SubsystemID
};

typedef struct
{
	UWORD	Address;
	UWORD	VendorID;
	UWORD	DeviceID;
	UBYTE	Class;
	UBYTE	SubClass;
	UBYTE	Interface;
	UBYTE	RevisionID;
	UWORD	SubsysVID;
	UWORD	SubsystemID;
	APTR	BaseAddress[6];
	ULONG	BaseSizes[6];
	UBYTE	INTLine;
	UBYTE	IRQLine;
	UBYTE	HeaderType;
} HIDDT_PCI_Device;

#define PCI_BUS(addr) ((addr) >> 8)
#define PCI_DEV(addr) ((addr) >> 3 & 0x1f)
#define PCI_SUB(addr) ((addr) & 0x7)

/* Messages for PCI hidd */

struct pHidd_PCI_FindDevice
{
	OOP_MethodID		mID;
	struct	TagItem		*deviceTags;
};

struct pHidd_PCI_FreeQuery
{
	OOP_MethodID		mID;
	HIDDT_PCI_Device	**devices;
};

#endif /* HIDD_IRQ_H */
