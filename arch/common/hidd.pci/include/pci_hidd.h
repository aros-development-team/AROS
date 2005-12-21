#ifndef HIDD_PCI_H
#define HIDD_PCI_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/* Base PCI class */

#define CLID_Hidd_PCI	"hidd.pci"
#define IID_Hidd_PCI	"hidd.pci"

#define HiddPCIAttrBase	__IHidd_PCI

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddPCIAttrBase;
#endif

/* PCI Class methods */
enum
{
    moHidd_PCI_AddHardwareDriver = 0,
    moHidd_PCI_EnumDevices,
    moHidd_PCI_RemHardwareDriver,

    NUM_PCI_METHODS
};

/* Tags for EnumDevices method */
enum
{
    tHidd_PCI_VendorID		= TAG_USER,
    tHidd_PCI_ProductID,
    tHidd_PCI_RevisionID,
    tHidd_PCI_Interface,
    tHidd_PCI_Class,
    tHidd_PCI_SubClass,
    tHidd_PCI_SubsystemVendorID,
    tHidd_PCI_SubsystemID
};

struct pHidd_PCI_AddHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Class	    *driverClass;
};

struct pHidd_PCI_EnumDevices
{
    OOP_MethodID    mID;
    struct Hook	    *callback;
    struct TagItem  *requirements;
};

struct pHidd_PCI_RemHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Class	    *driverClass;
};

/* PCI device class */

#define CLID_Hidd_PCIDevice	"hidd.pci.device"
#define IID_Hidd_PCIDevice	"hidd.pci.device"

#define HiddPCIDeviceAttrBase	__IHidd_PCIDev

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddPCIDeviceAttrBase;
#endif

enum
{
    moHidd_PCIDevice_ReadConfigByte,
    moHidd_PCIDevice_ReadConfigWord,
    moHidd_PCIDevice_ReadConfigLong,
    moHidd_PCIDevice_WriteConfigByte,
    moHidd_PCIDevice_WriteConfigWord,
    moHidd_PCIDevice_WriteConfigLong,

    NUM_PCIDEVICE_METHODS
};

enum
{
    aoHidd_PCIDevice_Driver,	/* [I.G] Hardware PCI driver that handles this device */
    aoHidd_PCIDevice_Bus,	/* [I.G] Bus the device is on */
    aoHidd_PCIDevice_Dev,	/* [I.G] Device number */
    aoHidd_PCIDevice_Sub,	/* [I.G] Function number */

    aoHidd_PCIDevice_VendorID,	/* [..G] VendorID of device as defined in PCI specs */
    aoHidd_PCIDevice_ProductID,	/* [..G] ProductID */
    aoHidd_PCIDevice_RevisionID,/* [..G] RevisionID */

    aoHidd_PCIDevice_Interface,	/* [..G] */
    aoHidd_PCIDevice_Class,	/* [..G] */
    aoHidd_PCIDevice_SubClass,	/* [..G] */

    aoHidd_PCIDevice_SubsystemVendorID, /* [..G] */
    aoHidd_PCIDevice_SubsystemID,	/* [..G] */

    aoHidd_PCIDevice_INTLine,	/* [..G] */
    aoHidd_PCIDevice_IRQLine,	/* [..G] */

    aoHidd_PCIDevice_RomBase,	/* [.SG] Location of ROM on the PCI bus (if ROM exists) */
    aoHidd_PCIDevice_RomSize,	/* [..G] Size of ROM area */

    aoHidd_PCIDevice_Base0,	/* [.SG] Location of Memory Area 0 */
    aoHidd_PCIDevice_Size0,	/* [..G] Size of Memory Area 0 */
    aoHidd_PCIDevice_Type0,	/* [..G] Type of Memory Area 0 */
    aoHidd_PCIDevice_Base1,	/* [.SG] Ditto */
    aoHidd_PCIDevice_Size1,	/* [..G] */
    aoHidd_PCIDevice_Type1,	/* [..G] */
    aoHidd_PCIDevice_Base2,	/* [.SG] */
    aoHidd_PCIDevice_Size2,	/* [..G] */
    aoHidd_PCIDevice_Type2,	/* [..G] */
    aoHidd_PCIDevice_Base3,	/* [.SG] */
    aoHidd_PCIDevice_Size3,	/* [..G] */
    aoHidd_PCIDevice_Type3,	/* [..G] */
    aoHidd_PCIDevice_Base4,	/* [.SG] */
    aoHidd_PCIDevice_Size4,	/* [..G] */
    aoHidd_PCIDevice_Type4,	/* [..G] */
    aoHidd_PCIDevice_Base5,	/* [.SG] */
    aoHidd_PCIDevice_Size5,	/* [..G] */
    aoHidd_PCIDevice_Type5,	/* [..G] */

    aoHidd_PCIDevice_isIO,	/* [.SG] Can device access IO space? */
    aoHidd_PCIDevice_isMEM,	/* [.SG] Can device access Mem space? */
    aoHidd_PCIDevice_isMaster,	/* [.SG] Can device work in BusMaster mode? */
    aoHidd_PCIDevice_paletteSnoop,  /* [.SG] Should VGA compatible card snoop the palette? */

    aoHidd_PCIDevice_is66MHz,	/* [..G] Is device 66MHz capable? */

    aoHidd_PCIDevice_ClassDesc,	    /* [..G] String description of device Class */
    aoHidd_PCIDevice_SubClassDesc,  /* [..G] String description of device SubClass */
    aoHidd_PCIDevice_InterfaceDesc, /* [..G] String description of defice Interface */

    aoHidd_PCIDevice_isBridge,	/* [..G] Is the device a PCI-PCI bridge? */
    aoHidd_PCIDevice_SubBus,	/* [..G] Bus number managed by bridge */
    aoHidd_PCIDevice_MemoryBase,/* [.SG] PCI bridge will forwart addresses from MemoryBase to */
    aoHidd_PCIDevice_MemoryLimit,/*[.SG] MemoryLimit through */
    aoHidd_PCIDevice_PrefetchableBase, /* [.SG] like above, regarding the prefetchable memory */
    aoHidd_PCIDevice_PrefetchableLimit,/* [.SG] */
    aoHidd_PCIDevice_IOBase,	/* [.SG] PCI bridge will forward IO accesses from IOBase to IOLimit */
    aoHidd_PCIDevice_IOLimit,	/* [.SG] */
    aoHidd_PCIDevice_ISAEnable,	/* [.SG] Enable ISA-specific IO forwarding */
    aoHidd_PCIDevice_VGAEnable, /* [.SG] Enable VGA-specific IO/MEM forwarding regardless of limits */
    
    num_Hidd_PCIDevice_Attrs
};

#define aHidd_PCIDevice_Driver	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Driver)
#define aHidd_PCIDevice_Bus	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Bus)
#define aHidd_PCIDevice_Dev	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Dev)
#define aHidd_PCIDevice_Sub	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Sub)
#define aHidd_PCIDevice_VendorID    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_VendorID)
#define aHidd_PCIDevice_ProductID   (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_ProductID)
#define aHidd_PCIDevice_RevisionID  (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_RevisionID)
#define aHidd_PCIDevice_Interface   (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Interface)
#define aHidd_PCIDevice_Class	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Class)
#define aHidd_PCIDevice_SubClass    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_SubClass)
#define aHidd_PCIDevice_SubsystemVendorID   (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_SubsystemVendorID)
#define aHidd_PCIDevice_SubsystemID (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_SubsystemID)
#define aHidd_PCIDevice_INTLine	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_INTLine)
#define aHidd_PCIDevice_IRQLine	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_IRQLine)
#define aHidd_PCIDevice_RomBase	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_RomBase)
#define aHidd_PCIDevice_RomSize	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_RomSize)

#define aHidd_PCIDevice_Base0	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Base0)
#define aHidd_PCIDevice_Base1	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Base1)
#define aHidd_PCIDevice_Base2	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Base2)
#define aHidd_PCIDevice_Base3	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Base3)
#define aHidd_PCIDevice_Base4	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Base4)
#define aHidd_PCIDevice_Base5	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Base5)

#define aHidd_PCIDevice_Size0	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Size0)
#define aHidd_PCIDevice_Size1	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Size1)
#define aHidd_PCIDevice_Size2	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Size2)
#define aHidd_PCIDevice_Size3	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Size3)
#define aHidd_PCIDevice_Size4	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Size4)
#define aHidd_PCIDevice_Size5	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Size5)

#define aHidd_PCIDevice_Type0	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Type0)
#define aHidd_PCIDevice_Type1	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Type1)
#define aHidd_PCIDevice_Type2	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Type2)
#define aHidd_PCIDevice_Type3	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Type3)
#define aHidd_PCIDevice_Type4	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Type4)
#define aHidd_PCIDevice_Type5	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Type5)

#define aHidd_PCIDevice_isIO	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_isIO)
#define aHidd_PCIDevice_isMEM	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_isMEM)
#define aHidd_PCIDevice_isMaster    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_isMaster)
#define aHidd_PCIDevice_paletteSnoop (HiddPCIDeviceAttrBase +aoHidd_PCIDevice_paletteSnoop)
#define aHidd_PCIDevice_is66MHz	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_is66MHz)

#define aHidd_PCIDevice_ClassDesc   (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_ClassDesc)
#define aHidd_PCIDevice_SubClassDesc (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_SubClassDesc)
#define aHidd_PCIDevice_InterfaceDesc (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_InterfaceDesc)

#define aHidd_PCIDevice_isBridge    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_isBridge)
#define aHidd_PCIDevice_SubBus	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_SubBus)
#define aHidd_PCIDevice_MemoryBase  (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_MemoryBase)
#define aHidd_PCIDevice_MemoryLimit (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_MemoryLimit)
#define aHidd_PCIDevice_PrefetchableBase (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_PrefetchableBase)
#define aHidd_PCIDevice_PrefetchableLimit (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_PrefetchableLimit)
#define aHidd_PCIDevice_IOBase	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_IOBase)
#define aHidd_PCIDevice_IOLimit	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_IOLimit)
#define aHidd_PCIDevice_ISAEnable   (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_ISAEnable)
#define aHidd_PCIDevice_VGAEnable   (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_VGAEnable)

#define IS_PCIDEV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddPCIDeviceAttrBase) < num_Hidd_PCIDevice_Attrs)

/* Types of BaseAddresses */
#define ADDRB_IO	0
#define ADDRB_PREFETCH	3

#define ADDRF_IO	(1 << ADDRB_IO)
#define ADDRF_PREFETCH	(1 << ADDRB_PREFETCH)

struct pHidd_PCIDevice_ReadConfigByte
{
    OOP_MethodID    mID;
    UBYTE	    reg;    /* Register number */
};

struct pHidd_PCIDevice_ReadConfigWord
{
    OOP_MethodID    mID;
    UBYTE	    reg;    /* Register number */
};

struct pHidd_PCIDevice_ReadConfigLong
{
    OOP_MethodID    mID;
    UBYTE	    reg;    /* Register number */
};

struct pHidd_PCIDevice_WriteConfigByte
{
    OOP_MethodID    mID;
    UBYTE	    reg;    /* Register number */
    UBYTE	    val;    /* Value to be written */
};

struct pHidd_PCIDevice_WriteConfigWord
{
    OOP_MethodID    mID;
    UBYTE	    reg;    /* Register number */
    UWORD	    val;    /* Value to be written */
};


struct pHidd_PCIDevice_WriteConfigLong
{
    OOP_MethodID    mID;
    UBYTE	    reg;    /* Register number */
    ULONG	    val;    /* Value to be written */
};


/* PCI driver class */

#define CLID_Hidd_PCIDriver	"hidd.pci.driver"
#define IID_Hidd_PCIDriver	"hidd.pci.driver"

#define HiddPCIDriverAttrBase	__IHidd_PCIDrv

enum
{
    moHidd_PCIDriver_ReadConfigByte,
    moHidd_PCIDriver_ReadConfigWord,
    moHidd_PCIDriver_ReadConfigLong,
    moHidd_PCIDriver_WriteConfigByte,
    moHidd_PCIDriver_WriteConfigWord,
    moHidd_PCIDriver_WriteConfigLong,
    moHidd_PCIDriver_CPUtoPCI,
    moHidd_PCIDriver_PCItoCPU,
    moHidd_PCIDriver_MapPCI,
    moHidd_PCIDriver_UnmapPCI,
    moHidd_PCIDriver_AllocPCIMem,
    moHidd_PCIDriver_FreePCIMem,

    NUM_PCIDRIVER_METHODS
};

enum
{
    aoHidd_PCIDriver_DirectBus,	 /* [..G] DirectBus shows whether CPUtoPCI and PCItoCPU methods are usable */

    num_Hidd_PCIDriver_Attrs
};

#define aHidd_PCIDriver_DirectBus   (aoHidd_PCIDriver_DirectBus + HiddPCIDriverAttrBase)

#define IS_PCIDRV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddPCIDriverAttrBase) < num_Hidd_PCIDriver_Attrs)


struct pHidd_PCIDriver_ReadConfigByte
{
    OOP_MethodID    mID;
    UBYTE	    bus;    /* Bus number */
    UBYTE	    dev;    /* Device number */
    UBYTE	    sub;    /* Function number */
    UBYTE	    reg;    /* Register number */
};

struct pHidd_PCIDriver_ReadConfigWord
{
    OOP_MethodID    mID;
    UBYTE	    bus;    /* Bus number */
    UBYTE	    dev;    /* Device number */
    UBYTE	    sub;    /* Function number */
    UBYTE	    reg;    /* Register number */
};

struct pHidd_PCIDriver_ReadConfigLong
{
    OOP_MethodID    mID;
    UBYTE	    bus;    /* Bus number */
    UBYTE	    dev;    /* Device number */
    UBYTE	    sub;    /* Function number */
    UBYTE	    reg;    /* Register number */
};

struct pHidd_PCIDriver_WriteConfigByte
{
    OOP_MethodID    mID;
    UBYTE	    bus;    /* Bus number */
    UBYTE	    dev;    /* Device number */
    UBYTE	    sub;    /* Function number */
    UBYTE	    reg;    /* Register number */
    UBYTE	    val;    /* Value to be written */
};

struct pHidd_PCIDriver_WriteConfigWord
{
    OOP_MethodID    mID;
    UBYTE	    bus;    /* Bus number */
    UBYTE	    dev;    /* Device number */
    UBYTE	    sub;    /* Function number */
    UBYTE	    reg;    /* Register number */
    UWORD	    val;    /* Value to be written */
};


struct pHidd_PCIDriver_WriteConfigLong
{
    OOP_MethodID    mID;
    UBYTE	    bus;    /* Bus number */
    UBYTE	    dev;    /* Device number */
    UBYTE	    sub;    /* Function number */
    UBYTE	    reg;    /* Register number */
    ULONG	    val;    /* Value to be written */
};

struct pHidd_PCIDriver_CPUtoPCI
{
    OOP_MethodID    mID;
    APTR	    address;	/* CPU address to be translated */
};

struct pHidd_PCIDriver_PCItoCPU
{
    OOP_MethodID    mID;
    APTR	    address;	/* PCI address to be translated */
};

struct pHidd_PCIDriver_MapPCI
{
    OOP_MethodID    mID;
    APTR	    PCIAddress;	/* Address on the PCIBus to be mapped to CPU address space */
    ULONG	    Length;	/* Length of mapped area */
};

struct pHidd_PCIDriver_UnmapPCI
{
    OOP_MethodID    mID;
    APTR	    CPUAddress;	/* Address as seen by the CPU of the PCI address space to unmap */
    ULONG	    Length;	/* Length of unmapped area */
};

struct pHidd_PCIDriver_AllocPCIMem
{
    OOP_MethodID    mID;
    ULONG	    Size;
};

struct pHidd_PCIDriver_FreePCIMem
{
    OOP_MethodID    mID;
    APTR	    Address;
};

/* Prototypes for stubs */
VOID HIDD_PCI_EnumDevices(OOP_Object *obj, struct Hook *hook, struct TagItem *requirements);
VOID HIDD_PCI_AddHardwareDriver(OOP_Object *obj, OOP_Class *driver);
APTR HIDD_PCIDriver_CPUtoPCI(OOP_Object *obj, APTR address);
APTR HIDD_PCIDriver_PCItoCPU(OOP_Object *obj, APTR address);
APTR HIDD_PCIDriver_MapPCI(OOP_Object *obj, APTR address, ULONG length);
VOID HIDD_PCIDriver_UnmapPCI(OOP_Object *obj, APTR address, ULONG length);
APTR HIDD_PCIDriver_AllocPCIMem(OOP_Object *obj, ULONG length);
VOID HIDD_PCIDriver_FreePCIMem(OOP_Object *obj, APTR address);

#endif

