/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2711 GENET Ethernet — SANA-II Device handler (BeginIO/AbortIO)
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <devices/sana2.h>
#include <devices/newstyle.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include "genet.h"
#include LC_LIBDEFS_FILE

static const UWORD supported_commands[] = {
    CMD_READ, CMD_WRITE, CMD_FLUSH,
    S2_DEVICEQUERY, S2_GETSTATIONADDRESS, S2_CONFIGINTERFACE,
    S2_ADDMULTICASTADDRESS, S2_DELMULTICASTADDRESS,
    S2_MULTICAST, S2_BROADCAST, S2_TRACKTYPE, S2_UNTRACKTYPE,
    S2_GETTYPESTATS, S2_GETGLOBALSTATS, S2_ONEVENT,
    S2_READORPHAN, S2_ONLINE, S2_OFFLINE,
    NSCMD_DEVICEQUERY,
    0
};

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, GENETDev)
{
    AROS_LIBFUNC_INIT

    struct GENETUnit *unit;

    req->ios2_Req.io_Error = 0;
    unit = (struct GENETUnit *)req->ios2_Req.io_Unit;

    D(bug("[genet] BeginIO cmd=%ld unit=%ld\n", req->ios2_Req.io_Command, unit->gn_UnitNum));

    switch (req->ios2_Req.io_Command) {
    case S2_DEVICEQUERY:
    {
        struct Sana2DeviceQuery *query = req->ios2_StatData;
        if (query) {
            query->DevQueryFormat = 0;
            query->DeviceLevel = 0;
            query->AddrFieldSize = 48;  /* Ethernet = 48 bits */
            query->MTU = ETH_MTU;
            query->BPS = 1000000000;    /* 1 Gbps */
            query->HardwareType = S2WireType_Ethernet;
        }
        break;
    }

    case S2_GETSTATIONADDRESS:
        CopyMem(unit->gn_DevAddr, req->ios2_SrcAddr, ETH_ADDRSIZE);
        CopyMem(unit->gn_OrgAddr, req->ios2_DstAddr, ETH_ADDRSIZE);
        break;

    case S2_CONFIGINTERFACE:
        if (!(unit->gn_Flags & GNF_CONFIGURED)) {
            CopyMem(req->ios2_SrcAddr, unit->gn_DevAddr, ETH_ADDRSIZE);
            unit->gn_Flags |= GNF_CONFIGURED;
            genet_HW_SetMAC(unit, unit->gn_DevAddr);
        } else {
            req->ios2_Req.io_Error = S2ERR_BAD_STATE;
            req->ios2_WireError = S2WERR_IS_CONFIGURED;
        }
        break;

    case S2_ONLINE:
        if (!(unit->gn_Flags & GNF_ONLINE)) {
            genet_HW_Init(unit);
            genet_PHY_Init(unit);
            unit->gn_Flags |= GNF_ONLINE;
        }
        break;

    case S2_OFFLINE:
        if (unit->gn_Flags & GNF_ONLINE) {
            genet_HW_Stop(unit);
            unit->gn_Flags &= ~GNF_ONLINE;
        }
        break;

    case CMD_READ:
        if (unit->gn_Flags & GNF_ONLINE) {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            ObtainSemaphore(&unit->gn_Lock);
            AddTail((struct List *)&unit->gn_ReadList, (struct Node *)req);
            ReleaseSemaphore(&unit->gn_Lock);
            /* Signal unit task to check for pending RX */
            if (unit->gn_Task)
                Signal(unit->gn_Task, 1 << unit->gn_IntSig);
            return;
        } else {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        }
        break;

    case CMD_WRITE:
    case S2_BROADCAST:
    case S2_MULTICAST:
        if (unit->gn_Flags & GNF_ONLINE) {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            ObtainSemaphore(&unit->gn_Lock);
            AddTail((struct List *)&unit->gn_WriteList, (struct Node *)req);
            ReleaseSemaphore(&unit->gn_Lock);
            if (unit->gn_Task)
                Signal(unit->gn_Task, 1 << unit->gn_IntSig);
            return;
        } else {
            req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        }
        break;

    case CMD_FLUSH:
        /* Abort all pending requests */
        ObtainSemaphore(&unit->gn_Lock);
        {
            struct IOSana2Req *r;
            while ((r = (struct IOSana2Req *)RemHead((struct List *)&unit->gn_ReadList))) {
                r->ios2_Req.io_Error = IOERR_ABORTED;
                ReplyMsg((struct Message *)r);
            }
            while ((r = (struct IOSana2Req *)RemHead((struct List *)&unit->gn_WriteList))) {
                r->ios2_Req.io_Error = IOERR_ABORTED;
                ReplyMsg((struct Message *)r);
            }
        }
        ReleaseSemaphore(&unit->gn_Lock);
        break;

    case S2_GETGLOBALSTATS:
        CopyMem(&unit->gn_Stats, req->ios2_StatData, sizeof(struct Sana2DeviceStats));
        break;

    case NSCMD_DEVICEQUERY:
    {
        struct NSDeviceQueryResult *d = (struct NSDeviceQueryResult *)req->ios2_StatData;
        if (d) {
            d->DeviceType = NSDEVTYPE_SANA2;
            d->DeviceSubType = 0;
            d->SupportedCommands = (UWORD *)supported_commands;
        }
        break;
    }

    default:
        req->ios2_Req.io_Error = IOERR_NOCMD;
        break;
    }

    if (!(req->ios2_Req.io_Flags & IOF_QUICK))
        ReplyMsg((struct Message *)req);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, GENETDev)
{
    AROS_LIBFUNC_INIT

    struct GENETUnit *unit = (struct GENETUnit *)req->ios2_Req.io_Unit;

    ObtainSemaphore(&unit->gn_Lock);
    if (req->ios2_Req.io_Message.mn_Node.ln_Type != NT_REPLYMSG) {
        Remove((struct Node *)req);
        req->ios2_Req.io_Error = IOERR_ABORTED;
        ReplyMsg((struct Message *)req);
    }
    ReleaseSemaphore(&unit->gn_Lock);

    return 0;

    AROS_LIBFUNC_EXIT
}
