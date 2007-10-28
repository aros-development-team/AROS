#ifndef _PCI_H
#define _PCI_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <dos/bptr.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

struct pci_staticdata {
    OOP_AttrBase	hiddPCIDriverAB;
    OOP_AttrBase	hiddAB;

    OOP_Class		*driverClass;

    int			fd;
};

struct pcibase {
    struct Library	    LibNode;
    struct pci_staticdata   psd;
};

OOP_Class *init_pcidriverclass(struct pci_staticdata *);
void free_pcidriverclass(struct pci_staticdata *, OOP_Class *cl);

#define PCI_AddressPort	0x0cf8
#define PCI_DataPort	0x0cfc

#define BASE(lib) ((struct pcibase*)(lib))

#define PSD(cl) (&((struct pcibase *)cl->UserData)->psd)

#endif /* _PCI_H */

