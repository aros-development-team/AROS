/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: IPMI interfaces discovered from the PC firmware tables
          (SMBIOS type 38, ACPI SPMI). The base class does the work;
          this class only marks where the description came from.
*/

#include <aros/debug.h>

#include "ipmi_intern.h"

OOP_Object *HWIPMI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[HWIPMI] Root__New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct HWIPMIData *data = OOP_INST_DATA(cl, o);

        data->hwipmi_Source = 0;
        D(bug("[HWIPMI] Root__New: Instance @ 0x%p\n", o));
    }

    return o;
}

VOID HWIPMI__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[HWIPMI] Root__Dispose(0x%p)\n", o));
    OOP_DoSuperMethod(cl, o, msg);
}
