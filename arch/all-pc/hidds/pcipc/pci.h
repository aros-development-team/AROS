/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

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


struct pcipc_staticdata
{
    OOP_AttrBase        hiddPCIDriverAB;
    OOP_AttrBase        hiddAB;

    OOP_AttrBase        hidd_PCIDeviceAB;

    OOP_Class	        *driverClass;

    /* Low-level sub-methods */
    ULONG	        (*ReadConfigLong)(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
    void	        (*WriteConfigLong)(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);

    /* ACPI related */
    ACPI_TABLE_MCFG     *pcipc_acpiMcfgTbl;
    struct MinList      pcipc_irqRoutingTable;
};

struct pcibase
{
    struct Library	    LibNode;
    struct pcipc_staticdata   psd;
};

#define BASE(lib) ((struct pcibase*)(lib))
#define PSD(cl) (&((struct pcibase*)cl->UserData)->psd)
#define _psd PSD(cl)

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

#define PCICS_PRODUCT   0x02

#define PCIBR_SUBCLASS  0x0a
#define PCIBR_SECBUS    0x19


typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

static inline UWORD ReadConfigWord(struct pcipc_staticdata *psd, UBYTE bus,
    UBYTE dev, UBYTE sub, UWORD reg)
{
    pcicfg temp;

    temp.ul = psd->ReadConfigLong(bus, dev, sub, reg);
    return temp.uw[(reg&2)>>1];
}

static inline UWORD ReadConfigByte(struct pcipc_staticdata *psd, UBYTE bus,
    UBYTE dev, UBYTE sub, UWORD reg)
{
    pcicfg temp;

    temp.ul = psd->ReadConfigLong(bus, dev, sub, reg);
    return temp.ub[reg & 3];
}

ULONG ReadConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
void WriteConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);

#ifdef LEGACY_SUPPORT

void PCIPC_ProbeConfMech(struct pcipc_staticdata *psd);

#else

#define PCIPC_ProbeConfMech(x)

#endif

#endif /* _PCI_H */
