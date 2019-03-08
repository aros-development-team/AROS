/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "storage_intern.h"

OOP_Object *StorageUnit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug ("[Storage:Unit] Root__New()\n");)
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID StorageUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[Storage:Unit] Root__Dispose()\n");)
    OOP_DoSuperMethod(cl, o, msg);
}
