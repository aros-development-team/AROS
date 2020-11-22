/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/nvme.h>
#include <oop/oop.h>

#include "nvme_intern.h"

static const char *str_devicename = "nvme.device";

static void nvme_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    register int i = size;

    while (i--)
    {
        if (str1[i] < ' ')
            str2[i] = '\0';
        else
            str2[i] = str1[i];
    }
}

OOP_Object *NVMEUnit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
#if (0)
    struct nvme_Bus *uBus = (struct nvme_Bus *)GetTagData(aHidd_DriverData, 0, msg->attrList);
#endif
    D(bug ("[NVME:Unit] Root__New()\n");)

#if (0)
    if (!uBus)
        return NULL;
#endif

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct nvme_Unit *unit = OOP_INST_DATA(cl, o);
#if (0)
        struct ata_port  *at = uBus->ab_Port->ap_ata[0];
#endif
        D(bug ("[NVME:Unit] Root__New: Unit Obj @ %p\n", o);)

        InitSemaphore(&unit->au_Lock);
        NEWLIST(&unit->au_IOs);
        
        char *DevName = (char *)GetTagData(aHidd_StorageUnit_Model, 0, msg->attrList);
        char *DevSer = (char *)GetTagData(aHidd_StorageUnit_Serial, 0, msg->attrList);
        char *DevFW = (char *)GetTagData(aHidd_StorageUnit_Revision, 0, msg->attrList);
#if (0)
        unit->au_Bus = uBus;
        uBus->ab_Unit = o;
#endif
        if (DevName)
        {
            nvme_strcpy(DevName, unit->au_Model, 40);
            D(bug ("[NVME:Unit] Root__New: Model    %s\n", unit->au_Model);)
        }
        if (DevSer)
        {
            nvme_strcpy(DevSer, unit->au_SerialNumber, 20);
            D(bug ("[NVME:Unit] Root__New: Serial   %s\n", unit->au_SerialNumber);)
        }
        if (DevFW)
        {
            nvme_strcpy(DevFW, unit->au_FirmwareRev, 8);
            D(bug ("[NVME:Unit] Root__New: FW Revis %s\n", unit->au_FirmwareRev);)
        }
    }
    return o;
}

void NVMEUnit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
#if (0)
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Unit *unit = OOP_INST_DATA(cl, o);
#endif
    D(bug ("[NVME:Unit] Root__Dispose(%p)\n", o);)

    OOP_DoSuperMethod(cl, o, msg);
}

void NVMEUnit__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct NVMEBase *NVMEBase = cl->UserData;
    struct nvme_Unit *unit = OOP_INST_DATA(cl, o);
#if (0)
    struct ata_port  *at = unit->au_Bus->ab_Port->ap_ata[0];
#endif
    ULONG idx;

    Hidd_StorageUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_StorageUnit_Device:
        *msg->storage = (IPTR)str_devicename;
        return;

    case aoHidd_StorageUnit_Number:
        *msg->storage = unit->au_UnitNum;
        return;

    case aoHidd_StorageUnit_Type:
        *msg->storage = vHidd_StorageUnit_Type_SolidStateDisk;
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

#if (0)
    Hidd_NVMEUnit_Switch (msg->attrID, idx)
    {
    case aoHidd_NVMEUnit_Features:
        *msg->storage = (IPTR)at->at_features;
        return;
    }
#endif

    OOP_DoSuperMethod(cl, o, &msg->mID);
}
