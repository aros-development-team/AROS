#ifndef _HIDD_PCI_H
#define _HIDD_PCI_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the irq system HIDD.
    Lang: English.
*/

#include <hidd/pci.h>
#include <exec/lists.h>

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

/***** IRQ system HIDD *******************/

/* IDs */
#define IID_Hidd_PCI        "hidd.bus.pci"
#define CLID_Hidd_PCI       "hidd.bus.pci"

/* misc */

struct pci_staticdata
{
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;
	struct List		devices;
    OOP_Class		*pciclass;
};

typedef struct
{
	struct Node			node;
	HIDDT_PCI_Device	dev;
} Noded_PCI_Device;

OOP_Class *init_pciclass  ( struct pci_staticdata * );
VOID free_pciclass  ( struct pci_staticdata * );

#define PSD(cl) ((struct pci_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)PSD(cl)->oopbase)
#define UtilityBase	((struct Library *)PSD(cl)->utilitybase)
#define SysBase		(PSD(cl)->sysbase)

#endif /* _HIDD_PCI_H */
