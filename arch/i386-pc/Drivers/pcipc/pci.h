#ifndef _PCI_H
#define _PCI_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>

#include <dos/bptr.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

extern UBYTE LIBEND;

AROS_UFP3(struct pcibase *, Pci_init,
    AROS_UFHA(struct pcibase *, pcibase, D0),
    AROS_UFHA(BPTR, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

struct pci_staticdata {
    struct ExecBase	*sysbase;
    struct Library	*oopbase;
    struct Library	*utilitybase;
    
    OOP_AttrBase	hiddPCIDriverAB;
    OOP_AttrBase	hiddAB;

    OOP_Class		*driverClass;
};

struct pcibase {
    struct Library	    LibNode;
    struct pci_staticdata   *psd;
    struct ExecBase	    *sysBase;
    BPTR		    slist;
};

OOP_Class *init_pcidriverclass(struct pci_staticdata *);
VOID free_pcidriverclass(struct pci_staticdata *, OOP_Class *);


#define PCI_AddressPort	0x0cf8
#define PCI_DataPort	0x0cfc

#define BASE(lib) ((struct pcibase*)(lib))

#define PSD(cl) ((struct pci_staticdata*)cl->UserData)

#endif /* _PCI_H */

