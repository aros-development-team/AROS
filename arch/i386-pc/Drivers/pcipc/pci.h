#ifndef _PCI_H
#define _PCI_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <dos/bptr.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

struct pci_staticdata {
    OOP_AttrBase	hiddPCIDriverAB;
    OOP_AttrBase	hiddAB;

    OOP_Class		*driverClass;
    UBYTE		ConfType;
};

struct pcibase {
    struct Library	    LibNode;
    struct pci_staticdata   psd;
    struct ExecBase	    *sysBase;
    BPTR		    slist;
};

#define PCI_AddressPort	0x0cf8
#define PCI_ForwardPort 0x0cfa
#define PCI_TestPort	0x0cfb
#define PCI_DataPort	0x0cfc

#define PCICS_VENDOR	0x00
#define PCICS_SUBCLASS	0x0a

#define PCI_CLASS_BRIDGE_HOST	0x0600
#define PCI_CLASS_DISPLAY_VGA	0x0300

#define PCI_VENDOR_INTEL	0x8086
#define PCI_VENDOR_COMPAQ	0x0e11

#define BASE(lib) ((struct pcibase*)(lib))

#define PSD(cl) (&((struct pcibase*)cl->UserData)->psd)

#endif /* _PCI_H */

