/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <utility/tagitem.h>

#include "ipmi_intern.h"

#define base CSD(cl)

/*** IPMI::New() **************************************************************/

OOP_Object *IPMI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct HIDDIPMIData *data;

    D(bug("[IPMI] Root__New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o)
        return NULL;

    data = OOP_INST_DATA(cl, o);
    data->ipmi_InterfaceType = vHidd_IPMI_Interface_Unknown;
    data->ipmi_SpecVersionMajor = 0;
    data->ipmi_SpecVersionMinor = 0;
    data->ipmi_I2CSlaveAddress = 0;
    data->ipmi_NVStorageAddress = 0;
    data->ipmi_BaseAddress = 0;
    data->ipmi_BaseAddressModifier = 0;
    data->ipmi_AddressSpace = vHidd_IPMI_AddressSpace_Memory;
    data->ipmi_RegisterSpacing = vHidd_IPMI_RegSpacing_1;
    data->ipmi_InterruptNumber = 0;

    D(bug("[IPMI] Root__New: Instance @ 0x%p\n", o));
    return o;
}

/*** IPMI::Dispose() **********************************************************/
VOID IPMI__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[IPMI] Root__Dispose(0x%p)\n", o));
    OOP_DoSuperMethod(cl, o, msg);
}

/*** IPMI::Get() **************************************************************/

VOID IPMI__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct HIDDIPMIData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_IPMI_Switch(msg->attrID, idx)
    {
    case aoHidd_IPMI_InterfaceType:
        *msg->storage = (IPTR)data->ipmi_InterfaceType;
        return;
    case aoHidd_IPMI_SpecVersionMajor:
        *msg->storage = (IPTR)data->ipmi_SpecVersionMajor;
        return;
    case aoHidd_IPMI_SpecVersionMinor:
        *msg->storage = (IPTR)data->ipmi_SpecVersionMinor;
        return;
    case aoHidd_IPMI_I2CSlaveAddress:
        *msg->storage = (IPTR)data->ipmi_I2CSlaveAddress;
        return;
    case aoHidd_IPMI_NVStorageAddress:
        *msg->storage = (IPTR)data->ipmi_NVStorageAddress;
        return;
    case aoHidd_IPMI_BaseAddress:
        *msg->storage = (IPTR)data->ipmi_BaseAddress;
        return;
    case aoHidd_IPMI_BaseAddressModifier:
        *msg->storage = (IPTR)data->ipmi_BaseAddressModifier;
        return;
    case aoHidd_IPMI_AddressSpace:
        *msg->storage = (IPTR)data->ipmi_AddressSpace;
        return;
    case aoHidd_IPMI_RegisterSpacing:
        *msg->storage = (IPTR)data->ipmi_RegisterSpacing;
        return;
    case aoHidd_IPMI_InterruptNumber:
        *msg->storage = (IPTR)data->ipmi_InterruptNumber;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*** IPMI::Set() **************************************************************/

VOID IPMI__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct HIDDIPMIData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tag;
    ULONG idx;

    for (tag = msg->attrList; tag && tag->ti_Tag != TAG_DONE; tag++)
    {
        Hidd_IPMI_Switch(tag->ti_Tag, idx)
        {
        case aoHidd_IPMI_InterfaceType:
            data->ipmi_InterfaceType = tag->ti_Data;
            continue;
        case aoHidd_IPMI_SpecVersionMajor:
            data->ipmi_SpecVersionMajor = tag->ti_Data;
            continue;
        case aoHidd_IPMI_SpecVersionMinor:
            data->ipmi_SpecVersionMinor = tag->ti_Data;
            continue;
        case aoHidd_IPMI_I2CSlaveAddress:
            data->ipmi_I2CSlaveAddress = tag->ti_Data;
            continue;
        case aoHidd_IPMI_NVStorageAddress:
            data->ipmi_NVStorageAddress = tag->ti_Data;
            continue;
        case aoHidd_IPMI_BaseAddress:
            data->ipmi_BaseAddress = (IPTR)tag->ti_Data;
            continue;
        case aoHidd_IPMI_BaseAddressModifier:
            data->ipmi_BaseAddressModifier = tag->ti_Data;
            continue;
        case aoHidd_IPMI_AddressSpace:
            data->ipmi_AddressSpace = tag->ti_Data;
            continue;
        case aoHidd_IPMI_RegisterSpacing:
            data->ipmi_RegisterSpacing = tag->ti_Data;
            continue;
        case aoHidd_IPMI_InterruptNumber:
            data->ipmi_InterruptNumber = tag->ti_Data;
            continue;
        }
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

/*** IPMI::Hidd_IPMI() *********************************************************/

BOOL IPMI__Hidd_IPMI__SendCommand(OOP_Class *cl, OOP_Object *o,
    struct pHidd_IPMI_SendCommand *msg)
{
    (void)cl;
    (void)o;
    if (msg->responseLen)
        *msg->responseLen = 0;
    return FALSE;
}
