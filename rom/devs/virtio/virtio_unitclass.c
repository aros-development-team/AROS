/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <hidd/storage.h>
#include <hidd/virtio.h>
#include <oop/oop.h>

#include LC_LIBDEFS_FILE

#include "virtio_debug.h"
#include "virtio_intern.h"

extern const char GM_UNIQUENAME(LibName)[];

static void virtio_strcpy(const UBYTE *src, UBYTE *dst, ULONG size)
{
    register int i = size;
    while (i--) {
        if (src[i] < ' ') dst[i] = '\0';
        else              dst[i] = src[i];
    }
}

OOP_Object *VirtIOUnit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct VirtIOBase *VirtIOBase = (struct VirtIOBase *)cl->UserData;
    int unitnsno = (int)GetTagData(aHidd_StorageUnit_Number, 0, msg->attrList);
    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);

    if (!dev) return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o) {
        struct virtio_Unit *unit = OOP_INST_DATA(cl, o);
        char *DevName, *DevSer, *DevFW;

        InitSemaphore(&unit->au_Lock);
        NEWLIST(&unit->au_IOs);
        unit->au_UnitNum = (dev->dev_HostID << 12) | unitnsno;

        DevName = (char *)GetTagData(aHidd_StorageUnit_Model, 0, msg->attrList);
        DevSer  = (char *)GetTagData(aHidd_StorageUnit_Serial, 0, msg->attrList);
        DevFW   = (char *)GetTagData(aHidd_StorageUnit_Revision, 0, msg->attrList);

        if (DevName) virtio_strcpy((const UBYTE *)DevName, unit->au_Model,        40);
        if (DevSer)  virtio_strcpy((const UBYTE *)DevSer,  unit->au_SerialNumber, 20);
        if (DevFW)   virtio_strcpy((const UBYTE *)DevFW,   unit->au_FirmwareRev,   8);
    }
    return o;
}

void VirtIOUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    OOP_DoSuperMethod(cl, o, msg);
}

void VirtIOUnit__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct VirtIOBase *VirtIOBase = (struct VirtIOBase *)cl->UserData;
    struct virtio_Unit *unit = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_StorageUnit_Switch(msg->attrID, idx) {
    case aoHidd_StorageUnit_Device:
        *msg->storage = (IPTR)GM_UNIQUENAME(LibName);
        return;
    case aoHidd_StorageUnit_Number:
        *msg->storage = unit->au_UnitNum;
        return;
    case aoHidd_StorageUnit_Type:
        *msg->storage = vHidd_StorageUnit_Type_FixedDisk;
        return;
    case aoHidd_StorageUnit_Model:
        *msg->storage = (IPTR)unit->au_Model;
        return;
    case aoHidd_StorageUnit_Revision:
        *msg->storage = (IPTR)unit->au_FirmwareRev;
        return;
    case aoHidd_StorageUnit_Serial:
        *msg->storage = (IPTR)unit->au_SerialNumber;
        return;
    case aoHidd_StorageUnit_Removable:
        *msg->storage = (IPTR)FALSE;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}