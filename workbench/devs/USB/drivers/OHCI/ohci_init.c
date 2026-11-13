/*
    Copyright (C) 2006 by Michal Schulz
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
#include <aros/symbolsets.h>

#include <asm/io.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <usb/usb.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include "ohci.h"

#undef SD
#define SD(x) (&LIBBASE->sd)

static
AROS_UFH3(void, Enumerator,
        AROS_UFHA(struct Hook *,        hook,           A0),
        AROS_UFHA(OOP_Object *,         pciDevice,      A2),
        AROS_UFHA(APTR,                 message,        A1))
{
    AROS_USERFUNC_INIT

    ohci_registers_t *regs;
    static int counter;
    struct timerequest *tr = ohci_CreateTimer();
    intptr_t tmp;

    if (counter == MAX_OHCI_DEVICES)
        return;

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;

    struct TagItem attrs[] = {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    OOP_SetAttrs(pciDevice, (struct TagItem *)attrs);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, &LIBBASE->sd.ramBase[counter]);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (void *)&LIBBASE->sd.pciDriver[counter]);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &tmp);

    LIBBASE->sd.irqNum[counter] = tmp;
    LIBBASE->sd.pciDevice[counter] = pciDevice;

    regs = (ohci_registers_t *)LIBBASE->sd.ramBase[counter];

    LIBBASE->sd.numPorts[counter] = HC_RHA_GET_NDP(AROS_OHCI2LONG(mmio(regs->HcRhDescriptorA)));

    D(bug("[OHCI]   %d-port Device %d @ %08x with MMIO @ %08x\n", LIBBASE->sd.numPorts[counter], counter + 1, pciDevice, LIBBASE->sd.ramBase[counter]));

    uint32_t ctrl = AROS_OHCI2LONG(mmio(regs->HcControl));
    if (ctrl & HC_CTRL_IR)
    {
        D(bug("[OHCI]   Performing BIOS handoff\n"));
        int delay = 500; /* 0.5 second */
        mmio(regs->HcInterruptEnable) = AROS_LONG2OHCI(HC_INTR_OC);
        mmio(regs->HcCommandStatus) = AROS_LONG2OHCI(HC_CS_OCR);

        /* Loop */
        while ((delay > 0) && AROS_OHCI2LONG(mmio(regs->HcControl) & HC_CTRL_IR))
        {
            delay -= 2;
            ohci_Delay(tr, 2);
        }
        if (delay < 0)
            D(bug("[OHCI]   BIOS handoff failed!\n"));

        mmio(regs->HcControl) = AROS_LONG2OHCI(ctrl & HC_CTRL_RWC);
    }

    /* Disable all interrupts */
    mmio(regs->HcInterruptDisable) = AROS_LONG2OHCI(0xffffffff);
    mmio(regs->HcInterruptStatus)  = AROS_LONG2OHCI(0xffffffff);

    LIBBASE->sd.numDevices = ++counter;

    ohci_DeleteTimer(tr);

    AROS_USERFUNC_EXIT
}


static int OHCI_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[OHCI] OHCI_Init()\n"));

    LIBBASE->sd.usb = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_USB, NULL);

    NEWLIST(&LIBBASE->sd.tdList);
    InitSemaphore(&LIBBASE->sd.tdLock);

    if (!LIBBASE->sd.usb)
    {
        bug("[OHCI] Cannot create the instance of base USB class\n");
        return FALSE;
    }

    if ((LIBBASE->sd.pci=OOP_NewObject(NULL, (STRPTR)CLID_Hidd_PCI, NULL)))
    {
        struct TagItem tags[] = {
                { tHidd_PCI_Class,      PCI_BASE_CLASS_SERIAL },
                { tHidd_PCI_SubClass,   PCI_SUB_CLASS_USB },
                { tHidd_PCI_Interface,  PCI_INTERFACE_OHCI },
                { TAG_DONE, 0UL }
        };

        struct OOP_ABDescr attrbases[] = {
                { (STRPTR)IID_Hidd,             &HiddAttrBase },
                { (STRPTR)IID_Hidd_PCIDevice,   &HiddPCIDeviceAttrBase },
                { (STRPTR)IID_Hidd_USBDevice,   &HiddUSBDeviceAttrBase },
                { (STRPTR)IID_Hidd_USBHub,      &HiddUSBHubAttrBase },
                { (STRPTR)IID_Hidd_USBDrv,      &HiddUSBDrvAttrBase },
                { NULL, NULL }
        };

        struct Hook FindHook = {
                h_Entry:        (IPTR (*)())Enumerator,
                h_Data:         LIBBASE,
        };

        OOP_ObtainAttrBases(attrbases);

        D(bug("[OHCI] Searching for OHCI devices...\n"));

        HIDD_PCI_EnumDevices(LIBBASE->sd.pci, &FindHook, (struct TagItem *)&tags);

        D(bug("[OHCI] Done. OHCI devices found: %d\n", LIBBASE->sd.numDevices));

        if (LIBBASE->sd.numDevices > 0)
        {
            LIBBASE->sd.memPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);

            if (LIBBASE->sd.memPool)
                return TRUE;
        }

    }

    return FALSE;
}

static int OHCI_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] = {
            { (STRPTR)IID_Hidd,                 &HiddAttrBase },
            { (STRPTR)IID_Hidd_PCIDevice,       &HiddPCIDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBDevice,       &HiddUSBDeviceAttrBase },
            { (STRPTR)IID_Hidd_USBHub,          &HiddUSBHubAttrBase },
            { (STRPTR)IID_Hidd_USBDrv,          &HiddUSBDrvAttrBase },
            { NULL, NULL }
    };

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
}

ADD2INITLIB(OHCI_Init, 0)
ADD2EXPUNGELIB(OHCI_Expunge, 0)
ADD2LIBS((STRPTR)"usb.hidd", 0, static struct Library *, __usbbase)
