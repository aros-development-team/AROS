#ifndef HIDD_PCI_H
#define HIDD_PCI_H

/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
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

#define CLID_Hidd_PCI   "hidd.pci"

#include <interface/Hidd_PCI.h>

/* Tags for EnumDevices method */
enum
{
    tHidd_PCI_VendorID          = TAG_USER,
    tHidd_PCI_ProductID,
    tHidd_PCI_RevisionID,
    tHidd_PCI_Interface,
    tHidd_PCI_Class,
    tHidd_PCI_SubClass,
    tHidd_PCI_SubsystemVendorID,
    tHidd_PCI_SubsystemID
};

/* PCI device class */

#define CLID_Hidd_PCIDevice     "hidd.pci.device"

#include <interface/Hidd_PCIDevice.h>

/* ABIv0 compatability */
#define __IHidd_PCIDev __IHidd_PCIDevice

#define IS_PCIDEV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddPCIDeviceAttrBase) < num_Hidd_PCIDevice_Attrs)

/* Types of BaseAddresses */
#define ADDRB_IO        0
#define ADDRB_PREFETCH  3

#define ADDRF_IO        (1 << ADDRB_IO)
#define ADDRF_PREFETCH  (1 << ADDRB_PREFETCH)

/* PCI driver class */

#define CLID_Hidd_PCIDriver     "hidd.pci.driver"

#include <interface/Hidd_PCIDriver.h>

/* ABIv0 compatability */
#define __IHidd_PCIDrv __IHidd_PCIDriver

#define IS_PCIDRV_ATTR(attr, idx) \
    (((idx) = (attr) - HiddPCIDriverAttrBase) < num_Hidd_PCIDriver_Attrs)

#endif

