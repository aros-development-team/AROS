#ifndef HIDD_PCI_H
#define HIDD_PCI_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include for the PCI HIDD.
    Lang: English.
*/


#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

/***** PCI Device data structure ******/
struct PCIDevInfo
{
    struct MinNode node;
    UWORD	Vendor;
    UWORD	Product;
    UWORD	SubVendor;
    UWORD	SubSystem;
    UBYTE	Class;
    UBYTE	SubClass;
    UBYTE	ProgIF;
    UBYTE	HeaderType;
    ULONG	Bases[6];
    ULONG	BaseSize[6];
    UBYTE	IRQPin;
    UBYTE	IRQLine;
    UBYTE	PCIBus;
    UBYTE	PCIDev;
    UBYTE	PCIFunc;
};

/* PCI Configspace offsets */
#define PCICS_VENDOR		0x00
#define PCICS_PRODUCT		0x02
#define PCICS_COMMAND		0x04
#define PCICS_STATUS		0x06
#define PCICS_REVISION		0x08
#define PCICS_PROGIF		0x09
#define PCICS_SUBCLASS		0x0a
#define PCICS_CLASS		0x0b
#define PCICS_CACHELS		0x0c
#define PCICS_LATENCY		0x0d
#define PCICS_HEADERTYPE	0x0e
#define PCICS_BIST		0x0f
#define PCICS_BAR		0x10
#define PCICS_CARDBUS_CIS	0x28
#define PCICS_SUBVENDOR		0x2c
#define PCICS_SUBSYSTEM		0x2e
#define PCICS_EXPROM_BASE	0x30
#define PCICS_CAP_PTR		0x34
#define PCICS_INT_LINE		0x3c
#define PCICS_INT_PIN		0x3d
#define PCICS_MIN_GNT		0x3e
#define PCICS_MAX_LAT		0x3f

/* PCI Headertypes */
#define PCIHT_MASK		0x7f
#define PCIHT_MULTIFUNC		0x80

#define PCIHT_NORMAL		0x00
#define PCIHT_BRIDGE		0x01
#define PCIHT_CARDBUS		0x02

/* PCI Command register bits */
#define PCICMB_IODECODE		0
#define PCICMB_MEMDECODE	1
#define PCICMB_BUSMASTER	2
#define PCICMB_SPECIAL		3
#define PCICMB_INVALIDATE	4
#define PCICMB_VGASNOOP		5
#define PCICMB_PARITY		6
#define PCICMB_STEPPING		7
#define PCICMB_SERR		8
#define PCICMB_FASTB2B		9

#define PCICMF_IODECODE		(1 << PCICMB_IODECODE)
#define PCICMF_MEMDECODE	(1 << PCICMB_MEMDECODE)
#define PCICMF_BUSMASTER	(1 << PCICMB_BUSMASTER)
#define PCICMF_SPECIAL		(1 << PCICMB_SPECIAL)
#define PCICMF_INVALIDATE	(1 << PCICMB_INVALIDATE)
#define PCICMF_VGASNOOP		(1 << PCICMB_VGASNOOP)
#define PCICMF_PARITY		(1 << PCICMB_PARITY)
#define PCICMF_STEPPING		(1 << PCICMB_STEPPING)
#define PCICMF_SERR		(1 << PCICMB_SERR)
#define PCICMF_FASTB2B		(1 << PCICMB_FASTB2B)

/* PCI Status register bits */
#define PCISTB_CAPABILITES	4
#define PCISTB_66MHZ		5
#define PCISTB_FASTB2B		7
#define PCISTB_PARITY		8
#define PCISTB_SIG_TGT_ABORT	11
#define PCISTB_REC_TGT_ABORT	12
#define PCISTB_REC_MAS_ABORT	13
#define PCISTB_SIG_SYSERR	14
#define PCISTB_PARITYERR	15

#define PCISTF_CAPABILITES	(1 << PCISTB_CAPABILITES)
#define PCISTF_66MHZ		(1 << PCISTB_66MHZ)
#define PCISTF_FASTB2B		(1 << PCISTB_FASTB2B)
#define PCISTF_PARITY		(1 << PCISTB_PARITY)
#define PCISTF_SIG_TGT_ABORT	(1 << PCISTB_SIG_TGT_ABORT)
#define PCISTF_REC_TGT_ABORT	(1 << PCISTB_REC_TGT_ABORT)
#define PCISTF_REC_MAS_ABORT	(1 << PCISTB_REC_MAS_ABORT)
#define PCISTF_SIG_SYSERR	(1 << PCISTB_SIG_SYSERR)
#define PCISTF_PARITYERR	(1 << PCISTB_PARITYERR)

#define PCIST_DEVSEL_MASK	0x600
#define PCIST_DEVSEL_FAST	0x000
#define PCIST_DEVSEL_MEDIUM	0x200
#define PCIST_DEVSEL_SLOW	0x400

/* PCI BIST register */
#define PCIBSB_START		6
#define PCIBSB_CAPABLE		7

#define PCIBSF_START		(1 << PCIBSB_START)
#define PCIBSF_CAPABLE		(1 << PCIBSB_CAPABLE)

#define PCIBS_CODEMASK		0x0f

/* PCI BaseAddressRegister defines */
#define PCIBAR_MASK_TYPE	0x01
#define PCIBAR_TYPE_MMAP	0x00
#define PCIBAR_TYPE_IO		0x01
#define PCIBAR_MASK_MEM		0xfffffff0
#define PCIBAR_MASK_IO		0xfffffffb

#define PCIBAR_MEMTYPE_MASK	0x06
#define PCIBAR_MEMTYPE_32BIT	0x00
#define PCIBAR_MEMTYPE_64BIT	0x04

#define PCIBARB_PREFETCHABLE	3
#define PCIBARF_PREFETCHABLE	(1 << PCIBARB_PREFETCHABLE)


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
    struct List		devs;
};

OOP_Class *init_pciclass  ( struct pci_staticdata * );
VOID free_pciclass  ( struct pci_staticdata * );

int PCIHWProbe(struct pci_staticdata * );
void PCIDumpDev(struct PCIDevInfo *);
void PCIGetClassDesc( UBYTE, UBYTE, UBYTE, STRPTR *, STRPTR *, STRPTR * );
ULONG PCISizeBase( UBYTE, UBYTE, UBYTE, UBYTE );

#endif /* HIDD_PCI_H */
