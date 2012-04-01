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

#include <devices/timer.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include "ehci.h"

#undef SD
#define SD(x) (&LIBBASE->sd)

static
AROS_UFH3(void, Enumerator,
        AROS_UFHA(struct Hook *,        hook,           A0),
        AROS_UFHA(OOP_Object *,         pciDevice,      A2),
        AROS_UFHA(APTR,                 message,        A1))
{
    AROS_USERFUNC_INIT

    LIBBASETYPE *LIBBASE = (LIBBASETYPE *)hook->h_Data;
    char *base, *reg_base;
    OOP_Object *driver;
    uint8_t cap_length, offset;
    int count = 64;

	struct MsgPort *port;
	struct timerequest *tr;

	port = CreateMsgPort();
	tr = CreateIORequest(port, sizeof(struct timerequest));

	OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0);

    struct pHidd_PCIDevice_WriteConfigLong wl;
    struct pHidd_PCIDevice_ReadConfigLong rl;
    struct pHidd_PCIDevice_WriteConfigByte wb;

    wl.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);
    rl.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
    wb.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);

    struct TagItem attrs[] = {
            { aHidd_PCIDevice_isMEM,    TRUE },
            { aHidd_PCIDevice_isMaster, TRUE },
            { TAG_DONE, 0UL },
    };

    OOP_SetAttrs(pciDevice, (struct TagItem *)attrs);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Base0, (IPTR *)&base);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (void *)&driver);

    cap_length = mmio_b(base);
    reg_base = base + cap_length;

    D(bug("[EHCI]  EHCI device Device @ %p, cap_length: %d\n", base, cap_length));

    uint32_t hcc_params = mmio_l(base + 0x08);
    offset = (hcc_params >> 8) & 0xff;                  // Get the address of first capability

    D(bug("[EHCI]  hcc_params=%08x\n", hcc_params));

    D(bug("[EHCI]  Try to perform the BIOS handoff procedure\n"));

    while(offset && count--)
    {
        uint32_t cap;
        rl.reg = offset;
        cap = OOP_DoMethod(driver, &rl.mID);

        if ((cap & 0xff) == 1)
        {
            D(bug("[EHCI]  cap=%08x\n", cap));

            D(bug("[EHCI]  LEGSUP capability found\n"));
            uint8_t delay = 200;

            if ((cap & 0x10000))
            {
                D(bug("[EHCI]  BIOS was owning the EHCI. Changing it.\n"));
                wb.reg = offset + 3;
                wb.val = 1;
                OOP_DoMethod(driver, &wb.mID);
            }

            while((cap & 0x10000) && delay > 0)
            {
            	tr->tr_node.io_Command = TR_ADDREQUEST;
            	tr->tr_time.tv_sec = 0;
            	tr->tr_time.tv_usec = 40000;
            	DoIO((struct IORequest *)tr);
                delay--;
                rl.reg = offset;
                cap = OOP_DoMethod(driver, &rl.mID);
            }

            if (cap & 0x10000)
            {
                D(bug("[EHCI]  BIOS is not going to give up! Forcing it...\n"));
                wb.val = 0;
                wb.reg = offset + 2;

                OOP_DoMethod(driver, &wb.mID);
            }

            D(bug("[EHCI]  Disabling SMI\n"));
            wl.reg = offset + 4;
            wl.val = 0;
            OOP_DoMethod(driver, &wl.mID);
        }
        else if ((cap & 0xff) == 0)
            cap = 0;

        offset = (cap >> 8) & 0xff;
    }

    D(bug("[EHCI]  Performing full reset of the EHCI\n"));

    uint32_t value = mmio_l(reg_base + 0x04); // Read the status register
    if ((value & 0x1000) == 0)       // Halted flag not cleared? Then the EHCI is still running. Stop it.
    {
        int nloop = 10;

        value = mmio_l(reg_base + 0);     // USBCMD reg
        value &= ~1;                      // clear RUN flag
        mmio_l(reg_base + 0) = value;

        do {
            mmio_l(reg_base + 0x04) = 0x3f;
        	tr->tr_node.io_Command = TR_ADDREQUEST;
        	tr->tr_time.tv_sec = 0;
        	tr->tr_time.tv_usec = 40000;
        	DoIO((struct IORequest *)tr);
            value = mmio_l(reg_base + 0x04);
            if ((value == ~(uint32_t)0) || value & 0x1000)
                break;
        } while(--nloop > 0);
    }

    mmio_l(reg_base + 0x08) = 0;                // USBINTR = 0, no interrupts allowed
    mmio_l(reg_base + 0x04) = 0x3f;             // USBSTS flags cleared
    mmio_l(reg_base + 0x40) = 0;                // Unconfigure the chip

    mmio_l(reg_base + 0) = 2;
	tr->tr_node.io_Command = TR_ADDREQUEST;
	tr->tr_time.tv_sec = 0;
	tr->tr_time.tv_usec = 100000;
	DoIO((struct IORequest *)tr);
    mmio_l(reg_base + 0) = 0;

    hcc_params &= 0xf;

    while (hcc_params--)
    {
        mmio_l(reg_base + 0x44 + 4*hcc_params) = 1 << 13;
    }

    CloseDevice((struct IORequest *)tr);
    DeleteIORequest(tr);
    DeleteMsgPort(port);

    AROS_USERFUNC_EXIT
}

static int EHCI_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[EHCI] EHCI_Init()\n"));

    LIBBASE->sd.usb = OOP_NewObject(NULL, (STRPTR)CLID_Hidd_USB, NULL);

    if (!LIBBASE->sd.usb)
    {
        bug("[EHCI] Cannot create the instance of base USB class\n");
        return FALSE;
    }

    if ((LIBBASE->sd.pci=OOP_NewObject(NULL, (STRPTR)CLID_Hidd_PCI, NULL)))
    {
        struct TagItem tags[] = {
                { tHidd_PCI_Class,      PCI_BASE_CLASS_SERIAL },
                { tHidd_PCI_SubClass,   PCI_SUB_CLASS_USB },
                { tHidd_PCI_Interface,  PCI_INTERFACE_EHCI },
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

        D(bug("[EHCI] Searching for EHCI devices...\n"));

        HIDD_PCI_EnumDevices(LIBBASE->sd.pci, &FindHook, (struct TagItem *)&tags);

        D(bug("[EHCI] Done\n"));

        OOP_DisposeObject(LIBBASE->sd.pci);
    }

    return FALSE;
}

static int EHCI_Expunge(LIBBASETYPEPTR LIBBASE)
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

OOP_Object *METHOD(EHCI, Root, New)
{
    return NULL;
}

struct pRoot_Dispose {
    OOP_MethodID        mID;
};

void METHOD(EHCI, Root, Dispose)
{
}

ADD2INITLIB(EHCI_Init, 0)
ADD2EXPUNGELIB(EHCI_Expunge, 0)
ADD2LIBS((STRPTR)"usb.hidd", 0, static struct Library *, __usbbase)
