#ifndef _PCI_H
#define _PCI_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <oop/oop.h>

#include <aros/arossupportbase.h>
#include <exec/execbase.h>

#include LC_LIBDEFS_FILE

/* Private data and structures unavailable outside the pci base classes */

struct DriverNode {
    struct Node		node;
    OOP_Class		*driverClass;	/* Driver class */
    OOP_Object		*driverObject;	/* Driver object */
    ULONG		highBus;
    struct List		devices;	/* List of defices behind this node */
};

struct DrvInstData {
    BOOL DirectBus;
};

struct PciDevice {
    struct MinNode	node;
    OOP_Object		*device;
};

typedef struct DeviceData {
    OOP_Object		*driver;
    UBYTE		bus,dev,sub;
    UBYTE		isBridge;
    UBYTE		subbus;
    UWORD		VendorID;
    UWORD		ProductID;
    UBYTE		RevisionID;
    UBYTE		Interface;
    UBYTE		SubClass;
    UBYTE		Class;
    UWORD		SubsysVID;
    UWORD		SubsystemID;
    UBYTE		INTLine;
    UBYTE		IRQLine;
    UBYTE		HeaderType;
    struct {
	IPTR		addr;
	IPTR		size;
    } BaseReg[6];
    ULONG		RomBase;
    ULONG		RomSize;

    STRPTR		strClass;
    STRPTR		strSubClass;
    STRPTR		strInterface;
} tDeviceData;

struct pci_staticdata {
    struct ExecBase	*sysbase;
    struct Library	*utilitybase;
    
    struct SignalSemaphore driver_lock;
    struct List		drivers;

    APTR		MemPool;
    
    OOP_AttrBase	hiddAB;
    OOP_AttrBase	hiddPCIAB;
    OOP_AttrBase	hiddPCIDriverAB;
    OOP_AttrBase	hiddPCIBusAB;
    OOP_AttrBase	hiddPCIDeviceAB;

    OOP_Class		*pciClass;
    OOP_Class		*pciDeviceClass;
    OOP_Class		*pciDriverClass;

    ULONG		users;

    /* Most commonly used methods have already the mID's stored here */
    OOP_MethodID	mid_RB;
    OOP_MethodID	mid_RW;
    OOP_MethodID	mid_RL;
    OOP_MethodID	mid_WB;
    OOP_MethodID	mid_WW;
    OOP_MethodID	mid_WL;
};

struct pcibase {
    struct Library 		LibNode;
    struct ExecBase		*sysBase;
    BPTR			segList;
    APTR			MemPool;
    struct pci_staticdata	psd;
};

OOP_Class *init_pcideviceclass(struct pci_staticdata *);
void free_pcideviceclass(struct pci_staticdata *, OOP_Class *cl);

#define BASE(lib) ((struct pcibase*)(lib))

#define PSD(cl) (&BASE(cl->UserData)->psd)

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
#define PCICS_BAR0		0x10
#define PCICS_BAR1		0x14
#define PCICS_BAR2		0x18
#define PCICS_BAR3		0x1c
#define PCICS_BAR4		0x20
#define PCICS_BAR5		0x24
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
#define PCIBAR_MASK_IO		0xfffffffc

#define PCIBAR_MEMTYPE_MASK	0x06
#define PCIBAR_MEMTYPE_32BIT	0x00
#define PCIBAR_MEMTYPE_64BIT	0x04

#define PCIBARB_PREFETCHABLE	3
#define PCIBARF_PREFETCHABLE	(1 << PCIBARB_PREFETCHABLE)

/*
 * PCI-to-PCI bridge header defines
 * First 16 bytes are the same as normal PCI dev
 */
#define PCIBR_BAR0		0x10
#define PCIBR_BAR1		0x14
#define PCIBR_PRIBUS		0x18
#define PCIBR_SECBUS		0x19
#define PCIBR_SUBBUS		0x1a
#define PCIBR_SECLATENCY	0x1b
#define PCIBR_IOBASE		0x1c
#define PCIBR_IOLIMIT		0x1d
#define PCIBR_SECSTATUS		0x1e
#define PCIBR_MEMBASE		0x20
#define PCIBR_MEMLIMIT		0x22
#define PCIBR_PREFETCHBASE	0x24
#define PCIBR_PREFETCHLIMIT	0x26
#define PCIBR_PREBASEUPPER	0x28
#define PCIBR_PRELIMITUPPER	0x2c
#define PCIBR_IOBASEUPPER	0x30
#define PCIBR_IOLIMITUPPER	0x32
#define PCIBR_CAPPTR		0x34
#define PCIBR_EXPROMBASE	0x38
#define PCIBR_INT_LINE		0x3c
#define PCIBR_INT_PIN		0x3d
#define PCIBR_CONTROL		0x3e

#define PCICTRLB_ISAENABLE	2
#define PCICTRLB_VGAENABLE	3

#define PCICTRLF_ISAENABLE	(1 << PCICTRLB_ISAENABLE)
#define PCICTRLF_VGAENABLE	(1 << PCICTRLF_ISAENABLE)

#endif /* _PCI_H */

