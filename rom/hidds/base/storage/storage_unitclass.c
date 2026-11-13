/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
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

/*****************************************************************************************

    NAME
        moHidd_StorageUnit_GetCapacity

    SYNOPSIS
        BOOL HIDD_StorageUnit_GetCapacity(OOP_Object *obj,
            UQUAD *blockCount, ULONG *blockSize);

    LOCATION
        IID_Hidd_StorageUnit

    FUNCTION
        Returns the current capacity already known for a storage unit.

    INPUTS
        blockCount - Storage for the number of addressable logical blocks.
        blockSize - Storage for the size of one logical block in bytes.

    RESULT
        TRUE if both values are available and non-zero, FALSE otherwise.

    NOTES
        Both output pointers are required. On failure each non-NULL output is
        set to zero. Implementations report cached or otherwise established
        unit state; this query does not issue media I/O.

*****************************************************************************************/

BOOL StorageUnit__Hidd_StorageUnit__GetCapacity(OOP_Class *cl, OOP_Object *o,
    struct pHidd_StorageUnit_GetCapacity *msg)
{
    (void)cl;
    (void)o;

    if (msg->blockCount)
        *msg->blockCount = 0;
    if (msg->blockSize)
        *msg->blockSize = 0;

    return FALSE;
}

VOID StorageUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[Storage:Unit] Root__Dispose()\n");)
    OOP_DoSuperMethod(cl, o, msg);
}
