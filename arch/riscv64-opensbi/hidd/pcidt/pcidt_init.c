/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Library housekeeping for the device-tree PCI host bridge driver.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <hidd/pci.h>
#include <utility/utility.h>

#include LC_LIBDEFS_FILE
#include "pcidt.h"

#undef OOPBase

static int PCIDT_Init(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->psd.OOPBase;

    D(bug("[PCIDT] %s()\n", __func__);)

    LIBBASE->psd.kernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->psd.kernelBase)
        return FALSE;

    LIBBASE->psd.utilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->psd.utilityBase)
        return FALSE;

    LIBBASE->psd.pciBase = OpenLibrary("pci.hidd", 0);
    if (!LIBBASE->psd.pciBase)
    {
        bug("[PCIDT] %s: pci.hidd is not available\n", __func__);
        return FALSE;
    }

    LIBBASE->psd.hidd_PCIDeviceMB = OOP_GetMethodID(IID_Hidd_PCIDevice, 0);

    D(bug("[PCIDT] %s: PCIDevice method base = %08x\n", __func__,
          LIBBASE->psd.hidd_PCIDeviceMB);)

    return TRUE;
}

static int PCIDT_Expunge(LIBBASETYPEPTR LIBBASE)
{
    struct Library *OOPBase = LIBBASE->psd.OOPBase;
    OOP_Object *pci;
    int ok = FALSE;

    D(bug("[PCIDT] %s()\n", __func__);)

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
        struct pHidd_PCI_RemHardwareDriver msg, *pmsg = &msg;

        msg.mID = OOP_GetMethodID(IID_Hidd_PCI,
                                  moHidd_PCI_RemHardwareDriver);
        msg.driverClass = LIBBASE->psd.pcidtDriverClass;

        ok = OOP_DoMethod(pci, (OOP_Msg)pmsg);

        OOP_DisposeObject(pci);
    }

    return ok;
}

ADD2INITLIB(PCIDT_Init, 0)
ADD2EXPUNGELIB(PCIDT_Expunge, 0)

/*
 * pci.hidd is opened by hand in PCIDT_Init() rather than with
 * ADD2LIBS(). The generated open runs before any of our init functions
 * and, when it fails, aborts the whole module without a word - which
 * is exactly what happened here, leaving a driver that loaded but
 * never ran. Opening it ourselves makes a failure visible and keeps
 * the same guarantee that the base class exists before we register.
 */
