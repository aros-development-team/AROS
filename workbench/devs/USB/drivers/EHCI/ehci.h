#ifndef EHCI_H_
#define EHCI_H_

/*
    Copyright (C) 2006-2007 by Michal Schulz
    $Id$

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as 
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <inttypes.h>
#include <aros/asmcall.h>
#include LC_LIBDEFS_FILE

#define PCI_BASE_CLASS_SERIAL   0x0c
#define PCI_SUB_CLASS_USB       0x03
#define PCI_INTERFACE_EHCI      0x20

#define CLID_Drv_USB_EHCI "Bus::Drv::EHCI"
#define IID_Drv_USB_EHCI  "Bus::Drv::EHCI"

#define mmio_l(var) (*(volatile uint32_t *)(var))
#define mmio_w(var) (*(volatile uint16_t *)(var))
#define mmio_b(var) (*(volatile uint8_t *)(var))

#undef HiddPCIDeviceAttrBase
#undef HiddUSBDeviceAttrBase
#undef HiddUSBHubAttrBase
#undef HiddUSBDrvAttrBase
#undef HiddOHCIAttrBase
#undef HiddAttrBase

#define HiddPCIDeviceAttrBase (SD(cl)->HiddPCIDeviceAB)
#define HiddUSBDeviceAttrBase (SD(cl)->HiddUSBDeviceAB)
#define HiddUSBHubAttrBase (SD(cl)->HiddUSBHubAB)
#define HiddUSBDrvAttrBase (SD(cl)->HiddUSBDrvAB)
#define HiddEHCIAttrBase (SD(cl)->HiddEHCIAB)
#define HiddAttrBase (SD(cl)->HiddAB)

#define BASE(lib)((struct ohcibase*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)

#define METHOD(base, id, name) \
    base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

struct ehci_staticdata
{
    OOP_Class           *ehciClass;

    OOP_Object          *irq;
    OOP_Object          *usb;
    OOP_Object          *pci;
    
    OOP_AttrBase        HiddPCIDeviceAB;
    OOP_AttrBase        HiddUSBDeviceAB;
    OOP_AttrBase        HiddUSBHubAB;
    OOP_AttrBase        HiddUSBDrvAB;
    OOP_AttrBase        HiddEHCIAB;
    OOP_AttrBase        HiddAB;
};

struct ehcibase
{
    struct Library          LibNode;
    struct ehci_staticdata  sd;
};

typedef struct EHCIData {
    struct ehci_staticdata      *sd;
} EHCIData;

#endif /*EHCI_H_*/
