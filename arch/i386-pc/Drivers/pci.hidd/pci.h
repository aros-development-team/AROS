#ifndef HIDD_PCI_H
#define HIDD_PCI_H

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id: 

    Desc: Include for the vga gfx HIDD.
    Lang: English.
*/


#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include "pcibios.h"

/***** PCI bus HIDD *******************/

/* IDs */
#define IID_Hidd_PCIbus		"hidd.bus.pci"
#define CLID_Hidd_PCIbus	"hidd.bus.pci"

/* misc */

struct pci_staticdata
{
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;
};

Class *init_pciclass  ( struct pci_staticdata * );
VOID free_pciclass  ( struct pci_staticdata * );

#endif /* HIDD_VGA_H */
