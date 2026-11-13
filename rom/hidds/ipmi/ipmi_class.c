/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Base class for IPMI system interfaces. Holds the interface
          description and implements the transports.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <utility/tagitem.h>

#include "ipmi_transport.h"

#define base CSD(cl)

/*** IPMI::New() **************************************************************/

OOP_Object *IPMI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct HIDDIPMIData *data;
    struct TagItem *tag;
    ULONG idx;

    D(bug("[IPMI] Root__New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (!o)
        return NULL;

    data = OOP_INST_DATA(cl, o);

    InitSemaphore(&data->ipmi_Lock);
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
    data->ipmi_BTSeq = 0;

    for (tag = msg->attrList; tag && tag->ti_Tag != TAG_DONE; tag++)
    {
        Hidd_IPMI_Switch(tag->ti_Tag, idx)
        {
        case aoHidd_IPMI_InterfaceType:
            data->ipmi_InterfaceType = tag->ti_Data;
            break;
        case aoHidd_IPMI_SpecVersionMajor:
            data->ipmi_SpecVersionMajor = tag->ti_Data;
            break;
        case aoHidd_IPMI_SpecVersionMinor:
            data->ipmi_SpecVersionMinor = tag->ti_Data;
            break;
        case aoHidd_IPMI_I2CSlaveAddress:
            data->ipmi_I2CSlaveAddress = tag->ti_Data;
            break;
        case aoHidd_IPMI_NVStorageAddress:
            data->ipmi_NVStorageAddress = tag->ti_Data;
            break;
        case aoHidd_IPMI_BaseAddress:
            data->ipmi_BaseAddress = (IPTR)tag->ti_Data;
            break;
        case aoHidd_IPMI_BaseAddressModifier:
            data->ipmi_BaseAddressModifier = tag->ti_Data;
            break;
        case aoHidd_IPMI_AddressSpace:
            data->ipmi_AddressSpace = tag->ti_Data;
            break;
        case aoHidd_IPMI_RegisterSpacing:
            data->ipmi_RegisterSpacing = tag->ti_Data;
            break;
        case aoHidd_IPMI_InterruptNumber:
            data->ipmi_InterruptNumber = tag->ti_Data;
            break;
        }
    }

    switch (data->ipmi_RegisterSpacing)
    {
    case vHidd_IPMI_RegSpacing_4:
        data->ipmi_RegStride = 4;
        break;
    case vHidd_IPMI_RegSpacing_16:
        data->ipmi_RegStride = 16;
        break;
    default:
        data->ipmi_RegStride = 1;
        break;
    }

    /* Only register based interfaces with a known address can be driven */
    data->ipmi_Usable = (data->ipmi_InterfaceType == vHidd_IPMI_Interface_KCS
                         || data->ipmi_InterfaceType == vHidd_IPMI_Interface_BT)
                        && data->ipmi_BaseAddress != 0;
#if !defined(__i386__) && !defined(__x86_64__)
    if (data->ipmi_AddressSpace == vHidd_IPMI_AddressSpace_IO)
        data->ipmi_Usable = FALSE;
#endif

    D(bug("[IPMI] Root__New: Instance @ 0x%p, type %u, base 0x%p (%s, stride %u), %s\n", o,
        data->ipmi_InterfaceType, (APTR)data->ipmi_BaseAddress,
        data->ipmi_AddressSpace == vHidd_IPMI_AddressSpace_IO ? "I/O" : "memory",
        data->ipmi_RegStride, data->ipmi_Usable ? "usable" : "not driven"));
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

/*** IPMI::SendCommand() ******************************************************/

BOOL IPMI__Hidd_IPMI__SendCommand(OOP_Class *cl, OOP_Object *o,
    struct pHidd_IPMI_SendCommand *msg)
{
    struct HIDDIPMIData *data = OOP_INST_DATA(cl, o);
    UBYTE resp[IPMI_MAX_MSG];
    struct ipmi_msg m;
    struct ipmi_io io;
    ULONG bufSize = 0;
    BOOL ok = FALSE;

    if (msg->responseLen)
    {
        bufSize = *msg->responseLen;
        *msg->responseLen = 0;
    }
    if (!msg->response)
        bufSize = 0;

    if (!data->ipmi_Usable || msg->netfn > 0x3F
        || msg->requestLen > IPMI_MAX_REQUEST_DATA
        || (msg->requestLen && !msg->request))
        return FALSE;

    m.msg_NetFn = msg->netfn << 2;
    m.msg_Cmd = msg->command;
    m.msg_Data = msg->request;
    m.msg_DataLen = msg->requestLen;
    m.msg_Resp = resp;
    m.msg_RespLen = 0;

    ObtainSemaphore(&data->ipmi_Lock);

    if (ipmi_io_open(&io, data))
    {
        switch (data->ipmi_InterfaceType)
        {
        case vHidd_IPMI_Interface_KCS:
            ok = ipmi_kcs_transact(&io, &m);
            break;
        case vHidd_IPMI_Interface_BT:
            ok = ipmi_bt_transact(&io, &m);
            break;
        }
        ipmi_io_close(&io);
    }

    ReleaseSemaphore(&data->ipmi_Lock);

    if (ok)
    {
        if (msg->responseLen)
            *msg->responseLen = m.msg_RespLen;
        if (bufSize)
            CopyMem(resp, msg->response, m.msg_RespLen < bufSize ? m.msg_RespLen : bufSize);
    }

    D(bug("[IPMI] SendCommand(netfn 0x%02x cmd 0x%02x, %u bytes) -> %s, %u bytes, cc 0x%02x\n",
        msg->netfn, msg->command, msg->requestLen, ok ? "ok" : "failed",
        ok ? m.msg_RespLen : 0, ok ? resp[0] : 0xFF));

    return ok;
}
