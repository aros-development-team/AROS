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

#include <libraries/acpica.h>

#include LC_LIBDEFS_FILE

#ifdef __i386__
/*
 * On i386 we can support very old Pentium-1 motherboards with PCI
 * configuration mechanism 2.
 * x86-64 is much more legacy-free...
 */
#define LEGACY_SUPPORT
#endif

struct pci_staticdata
{
    OOP_AttrBase   hiddPCIDriverAB;
    OOP_AttrBase   hiddAB;

    OOP_Class	  *driverClass;

    /* Low-level sub-methods */
    ULONG	 (*ReadConfigLong)(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
    void	 (*WriteConfigLong)(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);

    BOOL     (*isExtendedConfig)(UBYTE bus, UBYTE dev, UBYTE sub);

    struct Library *ACPICABase;

};

struct pcibase
{
    struct Library	    LibNode;
    struct pci_staticdata   psd;
};

/* PCI configuration mechanism 1 registers */
#define PCI_AddressPort	0x0cf8
#define PCI_DataPort	0x0cfc

/*
 * PCI configuration mechanism 2 registers
 * This mechanism is obsolete long ago. But AROS runs on old hardware,
 * and we support this.
 */
#define PCI_CSEPort	0x0cf8
#define PCI_ForwardPort 0x0cfa

/*
 * PCI configuration mechanism selector register.
 * Supported by some transition-time chipsets, like Intel Neptune.
 */
#define PCI_MechSelect	0x0cfb

#define PCICS_VENDOR	0x00
#define PCICS_PRODUCT   0x02
#define PCICS_SUBCLASS	0x0a

#define PCI_CLASS_BRIDGE_HOST	0x0600
#define PCI_CLASS_DISPLAY_VGA	0x0300

#define PCI_VENDOR_INTEL	0x8086
#define PCI_VENDOR_COMPAQ	0x0e11

#define BASE(lib) ((struct pcibase*)(lib))

#define PSD(cl) (&((struct pcibase*)cl->UserData)->psd)

typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

static inline UWORD ReadConfigWord(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    pcicfg temp;

    temp.ul = psd->ReadConfigLong(bus, dev, sub, reg);
    return temp.uw[(reg&2)>>1];
}

ULONG ReadConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
void WriteConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);

#ifdef LEGACY_SUPPORT

void ProbePCI(struct pci_staticdata *psd);

#else

#define ProbePCI(x)

#endif

#endif /* _PCI_H */
