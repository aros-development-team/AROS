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

/* Base PCI class */

#define CLID_Hidd_PCI	"hidd.pci"
#define IID_Hidd_PCI	"hidd.pci"

enum
{
    moHidd_PCI_AddHardwareDriver = 0,
    moHidd_PCI_FindDevice,
    moHidd_PCI_FindClass,

    NUM_PCI_METHODS
};

struct pHidd_PCI_AddHardwareDriver
{
    OOP_MethodID    mID;
    OOP_Class	    *driverClass;
    UBYTE	    managed;
};

struct pHidd_PCI_FindDevice
{
    OOP_MethodID    mID;
    UWORD	    vendorID;
    UWORD	    deviceID;
    OOP_Object	    *prevDev;
};

struct pHidd_PCI_FindClass
{
    OOP_MethodID    mID;
    ULONG	    classID;
    OOP_Object	    *prevDev;
};

/* PCI device class */

#define CLID_Hidd_PCIDevice	"hidd.pci.device"
#define IID_Hidd_PCIDevice	"hidd.pci.device"

#define HiddPCIDeviceAttrBase	__IHidd_PCIDev

extern OOP_AttrBase HiddPCIDeviceAttrBase;

enum
{
    aoHidd_PCIDevice_Driver,
    aoHidd_PCIDevice_Bus,
    aoHidd_PCIDevice_Dev,
    aoHidd_PCIDevice_Sub,
    aoHidd_PCIDevice_VendorID,
    aoHidd_PCIDevice_ProductID,

    num_Hidd_PCIDevice_Attrs
};

#define aHidd_PCIDevice_Driver	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Driver)
#define aHidd_PCIDevice_Bus	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Bus)
#define aHidd_PCIDevice_Dev	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Dev)
#define aHidd_PCIDevice_Sub	    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_Sub)
#define aHidd_PCIDevice_VendorID    (HiddPCIDeviceAttrBase + aoHidd_PCIDevice_VendorID)
#define aHidd_PCIDevice_ProductID   (HiddPCIDeviceAtrrBase + aoHidd_PCIDevice_ProductID)

#define IS_PCIDEV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddPCIDeviceAttrBase) < num_Hidd_PCIDevice_Attrs)

/* PCI driver class */

#define CLID_Hidd_PCIDriver	"hidd.pci.driver"
#define IID_Hidd_PCIDriver	"hidd.pci.driver"

enum
{
    moHidd_PCIDriver_ReadConfigByte,
    moHidd_PCIDriver_ReadConfigWord,
    moHidd_PCIDriver_ReadConfigLong,
    moHidd_PCIDriver_WriteConfigByte,
    moHidd_PCIDriver_WriteConfigWord,
    moHidd_PCIDriver_WriteConfigLong,
    
    NUM_PCIDRIVER_METHODS
};

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

#endif

