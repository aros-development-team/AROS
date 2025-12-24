/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "telemetry_intern.h"

# define base CSD(cl)

OOP_Object *Telemetry__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[Telemetry] Root__New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    D(bug ("[Telemetry] Root__New: Instance @ 0x%p\n", o);)
    return o;
}

/*** Telemetry::Dispose() **********************************************************/
VOID Telemetry__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[Telemetry] Root__Dispose(0x%p)\n", o));

    OOP_DoSuperMethod(cl, o, msg);
}

/*** Telemetry::Get() **************************************************************/

VOID Telemetry__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    Hidd_Telemetry_Switch(msg->attrID, idx)
    {
    case aoHidd_Telemetry_EntryCount:
        *msg->storage = (IPTR)0;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*** Telemetry::Set() **************************************************************/

VOID Telemetry__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*** Telemetry::Hidd_Telemetry() *************************************************/

BOOL Telemetry__Hidd_Telemetry__GetEntryAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_GetEntryAttribs *msg)
{
    (void)cl;
    (void)o;
    (void)msg;
    return FALSE;
}

BOOL Telemetry__Hidd_Telemetry__SetEntryValue(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Telemetry_SetEntryValue *msg)
{
    (void)cl;
    (void)o;
    (void)msg;
    return FALSE;
}
