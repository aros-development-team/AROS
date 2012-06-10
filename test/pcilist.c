/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#define __DOS_STDLIBBASE__
#define __OOP_STDLIBBASE__

#include <aros/system.h>
#include <hidd/pci.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>

struct Library *DOSBase;
struct Library *OOPBase;
OOP_AttrBase HiddPCIDeviceAttrBase;

#define PATTR(tag) ({ IPTR _val = ~0; \
    OOP_GetAttr(obj, aHidd_PCIDevice_##tag, &_val); \
    _val; })

AROS_UFH3(void, Callback,
        AROS_UFHA(struct Hook *,        hook,   A0),
        AROS_UFHA(OOP_Object *, obj,    A2),
        AROS_UFHA(APTR,         msg,    A1))
{
    AROS_USERFUNC_INIT

    Printf("%lx: %ld:%ld.%ld %04lx:%04lx, INT %ld, IRQ %ld\n",
            PATTR(Driver), PATTR(Bus), PATTR(Dev), PATTR(Sub),
            PATTR(VendorID), PATTR(ProductID),
            PATTR(INTLine), PATTR(IRQLine));

    AROS_USERFUNC_EXIT
}

struct Hook PCIHook = {
    .h_Entry = (APTR)Callback,
};

int __startup _main(void)
{
    int ret = RETURN_FAIL;

    if ((DOSBase = OpenLibrary("dos.library", 0))) {

        if ((OOPBase = OpenLibrary("oop.library", 0))) {
            OOP_Object *pci;

            if ((pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL))) {
                struct pHidd_PCI_EnumDevices msg;
                Printf("PCI Devices:\n");
                HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

                msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices);
                msg.callback = &PCIHook;
                msg.requirements = NULL;

                OOP_DoMethod(pci, (OOP_Msg)&msg);
                OOP_DisposeObject(pci);
                ret = RETURN_OK;
            } else {
                Printf("Can't open pci.hidd\n");
            }
            CloseLibrary(OOPBase);
        } else {
            Printf("Can't open oop.library\n");
        }
        CloseLibrary(DOSBase);
    }

    return ret;
}
