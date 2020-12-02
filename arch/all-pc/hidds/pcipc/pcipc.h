/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _PCIPC_H
#define _PCIPC_H

#include <aros/debug.h>
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
    struct Library              *OOPBase;
    struct Library              *utilityBase;
    APTR                        kernelBase;

    OOP_AttrBase                hiddPCIDriverAB;
    OOP_AttrBase                hiddAB;

    OOP_AttrBase                hidd_PCIDeviceAB;

    OOP_MethodID                hidd_PCIDeviceMB;

    OOP_Class                   *pcipcDriverClass;
    OOP_Class                   *pcipcDeviceClass;

    /* Low-level sub-methods */
    ULONG                       (*ReadConfigLong)(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
    void                        (*WriteConfigLong)(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);

    /* ACPI related */
    ACPI_TABLE_MCFG             *pcipc_acpiMcfgTbl;
};

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase
#undef HiddPCIDeviceAttrBase

#undef HiddPCIDeviceBase

#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase            (PSD(cl)->hiddAB)
#define HiddPCIDeviceAttrBase   (PSD(cl)->hidd_PCIDeviceAB)

#define HiddPCIDeviceBase       (PSD(cl)->hidd_PCIDeviceMB)

#define KernelBase              (PSD(cl)->kernelBase)
#define UtilityBase             (PSD(cl)->utilityBase)
#define OOPBase                 (PSD(cl)->OOPBase)

struct PCIPCBase
{
    struct Library              LibNode;
    struct pcipc_staticdata     psd;
};

#define BASE(lib)               ((struct PCIPCBase*)(lib))
#define PSD(cl)                 (&((struct PCIPCBase*)cl->UserData)->psd)
#define _psd                    PSD(cl)

struct PCIPCBusData
{
    APTR                        ecam;
    struct MinList              irqRoutingTable;
};

struct PCIPCDeviceData
{
    APTR                        mmconfig;
};

/* PCI configuration mechanism 1 registers */
#define PCI_AddressPort         0x0cf8
#define PCI_DataPort            0x0cfc

/*
 * PCI configuration mechanism 2 registers
 * This mechanism is obsolete long ago. But AROS runs on old hardware,
 * and we support this.
 */
#define PCI_CSEPort             0x0cf8
#define PCI_ForwardPort         0x0cfa

/*
 * PCI configuration mechanism selector register.
 * Supported by some transition-time chipsets, like Intel Neptune.
 */
#define PCI_MechSelect          0x0cfb

typedef union _pcicfg
{
    ULONG                       ul;
    UWORD                       uw[2];
    UBYTE                       ub[4];
} pcicfg;

static inline UWORD ReadConfigWord(struct pcipc_staticdata *psd, UBYTE bus,
    UBYTE dev, UBYTE sub, UWORD reg)
{
    pcicfg temp;

    if (reg & 1)
    {
        bug("[PCIPC] %s: missaligned word access! (reg = %u)\n", __func__, reg);
    }
    
    temp.ul = psd->ReadConfigLong(bus, dev, sub, (reg & ~3));
    return temp.uw[(reg&2)>>1];
}

static inline UWORD ReadConfigByte(struct pcipc_staticdata *psd, UBYTE bus,
    UBYTE dev, UBYTE sub, UWORD reg)
{
    pcicfg temp;

    temp.ul = psd->ReadConfigLong(bus, dev, sub, (reg & ~3));
    return temp.ub[reg & 3];
}

ULONG ReadConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
void WriteConfig1Long(UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);

#ifdef LEGACY_SUPPORT

void PCIPC_ProbeConfMech(struct pcipc_staticdata *psd);

#else

#define PCIPC_ProbeConfMech(x)

#endif

#endif /* _PCIPC_H */
