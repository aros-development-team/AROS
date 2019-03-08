/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "storage_intern.h"

OOP_Object *StorageBus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug ("[Storage:Bus] Root__New()\n");)
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
    }
    D(bug ("[Storage:Bus] Root__New: Instance @ 0x%p\n", o);)
    return o;
}

VOID StorageBus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[Storage:Bus] Root__Dispose(0x%p)\n", o);)
    OOP_DoSuperMethod(cl, o, msg);
}

void StorageBus__Hidd_StorageBus__EnumUnits(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageBus_EnumUnits *msg)
{
    D(bug ("[Storage:Bus] Hidd_StorageBus__EnumUnits(0x%p)\n", o);)
}
