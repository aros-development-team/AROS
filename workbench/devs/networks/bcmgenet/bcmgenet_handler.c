/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom GENETv5 SANA-II driver, the command set. Generic
          SANA-II ceremony, not GENET-specific: CmdWrite/CmdRead queue the
          request for the unit task to drain into BCMGENET_SendPacket() or
          to satisfy from a received frame.
*/
#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/errors.h>
#include <devices/newstyle.h>

#include <proto/exec.h>

#include "bcmgenet.h"

static const UWORD bcmgenet_supported_commands[] =
{
    CMD_READ,
    CMD_WRITE,
    CMD_FLUSH,
    S2_DEVICEQUERY,
    S2_GETSTATIONADDRESS,
    S2_CONFIGINTERFACE,
    S2_ADDMULTICASTADDRESS,
    S2_DELMULTICASTADDRESS,
    S2_MULTICAST,
    S2_BROADCAST,
    S2_TRACKTYPE,
    S2_UNTRACKTYPE,
    S2_GETTYPESTATS,
    S2_GETSPECIALSTATS,
    S2_GETGLOBALSTATS,
    S2_ONEVENT,
    S2_READORPHAN,
    S2_ONLINE,
    S2_OFFLINE,
    S2_ADDMULTICASTADDRESSES,
    S2_DELMULTICASTADDRESSES,
    NSCMD_DEVICEQUERY,
    0
};

static const char * const bcmgenet_special_stat_names[STAT_COUNT] =
{
    "Bad multicasts",
    "Retries",
    "Underruns"
};

/*
 * CONNECT and DISCONNECT are in here even though a wired unit never raises
 * them: sana_run() asks for OFFLINE|CONNECT|DISCONNECT in one S2_ONEVENT for
 * every interface, and rejecting the whole request as S2ERR_NOT_SUPPORTED
 * makes sana_connect() drop it without reissuing, which loses the reconnect
 * notification for good.
 */
#define KNOWN_EVENTS \
    (S2EVENT_ERROR | S2EVENT_TX | S2EVENT_RX | S2EVENT_ONLINE | \
     S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE | \
     S2EVENT_CONNECT | S2EVENT_DISCONNECT)

static BOOL CmdInvalid(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    request->ios2_Req.io_Error = IOERR_NOCMD;
    request->ios2_WireError = S2WERR_GENERIC_ERROR;

    return TRUE;
}

static BOOL CmdRead(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener;

    if ((unit->bgu_Flags & IFF_UP) == 0)
    {
        request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        request->ios2_WireError = S2WERR_UNIT_OFFLINE;
        return TRUE;
    }

    opener = request->ios2_BufferManagement;
    request->ios2_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(&opener->read_port, (struct Message *)request);

    return FALSE;
}

static BOOL CmdWrite(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    BYTE error = 0;
    ULONG wire_error = 0;

    if ((unit->bgu_Flags & IFF_UP) == 0)
    {
        error = S2ERR_OUTOFSERVICE;
        wire_error = S2WERR_UNIT_OFFLINE;
    }
    else if (request->ios2_Req.io_Command == S2_MULTICAST &&
             (request->ios2_DstAddr[0] & 0x1) == 0)
    {
        error = S2ERR_BAD_ADDRESS;
        wire_error = S2WERR_BAD_MULTICAST;
    }
    else if (request->ios2_DataLength >
             ((request->ios2_Req.io_Flags & SANA2IOF_RAW) != 0 ?
                  (ULONG)ETH_MAXPACKETSIZE : (ULONG)ETH_MTU))
    {
        error = S2ERR_MTU_EXCEEDED;
    }

    if (error != 0)
    {
        request->ios2_Req.io_Error = error;
        request->ios2_WireError = wire_error;
        return TRUE;
    }

    request->ios2_Req.io_Flags &= ~IOF_QUICK;


    D(bug("[bcmgenet] CmdWrite: len %lu, flags %08lx, unit flags %08lx\n",
          request->ios2_DataLength,
          request->ios2_Req.io_Flags,
          unit->bgu_Flags);)

    PutMsg(unit->bgu_RequestPorts[WRITE_QUEUE], (struct Message *)request);

    D(bug("[bcmgenet] CmdWrite: queued TX request\n");)
    if (unit->bgu_Task && unit->bgu_IRQSignal)
        Signal(unit->bgu_Task, unit->bgu_IRQSignal);

    return FALSE;
}

static BOOL CmdBroadcast(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    ULONG i;

    for (i = 0; i < ETH_ADDRESSSIZE; i++)
        request->ios2_DstAddr[i] = 0xff;

    return CmdWrite(base, request);
}

/* Replies every request still queued for this unit with IOERR_ABORTED */
static void BCMGENET_FlushUnit(struct BCMGENETUnit *unit)
{
    struct IOSana2Req *request, *tail, *next;
    struct List *list;
    ULONG q;

    for (q = 0; q < REQUEST_QUEUE_COUNT; q++)
    {
        list = &unit->bgu_RequestPorts[q]->mp_MsgList;
        next = (APTR)list->lh_Head;
        tail = (APTR)&list->lh_Tail;

        Disable();
        while (next != tail)
        {
            request = next;
            next = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

            Remove((APTR)request);
            request->ios2_Req.io_Error = IOERR_ABORTED;
            request->ios2_WireError = S2WERR_GENERIC_ERROR;
            ReplyMsg((APTR)request);
        }
        Enable();
    }
}

static BOOL CmdFlush(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    BCMGENET_FlushUnit(unit);

    return TRUE;
}

static BOOL CmdDeviceQuery(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Sana2DeviceQuery *info = request->ios2_StatData;
    ULONG size_available = info->SizeAvailable;

    if (size_available < sizeof(struct Sana2DeviceQuery))
    {
        request->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        request->ios2_WireError = S2WERR_BAD_STATDATA;
        return TRUE;
    }

    CopyMem(&unit->bgu_Sana2Info, info, sizeof(struct Sana2DeviceQuery));

    info->BPS = unit->bgu_SpeedMbps * 1000000UL;
    info->SizeAvailable = size_available;
    info->SizeSupplied = sizeof(struct Sana2DeviceQuery);

    return TRUE;
}

static BOOL CmdGetStationAddress(struct BCMGENETBase *base,
                                 struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    CopyMem(unit->bgu_DevAddr, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(unit->bgu_OrgAddr, request->ios2_DstAddr, ETH_ADDRESSSIZE);

    return TRUE;
}

static BOOL CmdConfigInterface(struct BCMGENETBase *base,
                               struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->bgu_Flags & IFF_CONFIGURED) != 0)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_IS_CONFIGURED;
        return TRUE;
    }

    CopyMem(request->ios2_SrcAddr, unit->bgu_DevAddr, ETH_ADDRESSSIZE);
    BCMGENET_SetMACAddress(unit->bgu_HW, unit->bgu_DevAddr);
    unit->bgu_Flags |= IFF_CONFIGURED;

    /* The filter carries our own address in a slot of its own */
    if (unit->bgu_Flags & IFF_UP)
        BCMGENET_SetRXFilter(base, unit);

    D(bug("[bcmgenet] configured with %02x:%02x:%02x:%02x:%02x:%02x\n",
          unit->bgu_DevAddr[0], unit->bgu_DevAddr[1], unit->bgu_DevAddr[2],
          unit->bgu_DevAddr[3], unit->bgu_DevAddr[4], unit->bgu_DevAddr[5]);)

    /* Going online is S2_ONLINE's job; the stack sends it right after */

    return TRUE;
}

static BOOL CmdAddMulticastAddresses(struct BCMGENETBase *base,
                                     struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    UBYTE *lower, *upper;

    lower = request->ios2_SrcAddr;
    upper = (request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESSES) ?
        request->ios2_DstAddr : lower;

    if (!BCMGENET_AddMulticastRange(base, unit, lower, upper))
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_GENERIC_ERROR;
    }

    return TRUE;
}

static BOOL CmdDelMulticastAddresses(struct BCMGENETBase *base,
                                     struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    UBYTE *lower, *upper;

    lower = request->ios2_SrcAddr;
    upper = (request->ios2_Req.io_Command == S2_DELMULTICASTADDRESSES) ?
        request->ios2_DstAddr : lower;

    if (!BCMGENET_RemMulticastRange(base, unit, lower, upper))
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_BAD_MULTICAST;
    }

    return TRUE;
}

static BOOL CmdTrackType(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener = request->ios2_BufferManagement;
    ULONG packet_type = request->ios2_PacketType;
    struct TypeStats *initial;
    struct TypeTracker *tracker;

    if (BCMGENET_FindTypeStats(&opener->initial_stats, packet_type) != NULL)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_ALREADY_TRACKED;
        return TRUE;
    }

    initial = AllocMem(sizeof(struct TypeStats), MEMF_PUBLIC | MEMF_CLEAR);
    if (!initial)
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        return TRUE;
    }

    tracker = (struct TypeTracker *)
        BCMGENET_FindTypeStats(&unit->bgu_TypeTrackers, packet_type);
    if (tracker)
        tracker->user_count++;
    else
    {
        tracker = AllocMem(sizeof(struct TypeTracker),
                           MEMF_PUBLIC | MEMF_CLEAR);
        if (!tracker)
        {
            FreeMem(initial, sizeof(struct TypeStats));
            request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            return TRUE;
        }
        tracker->packet_type = packet_type;
        tracker->user_count = 1;
        Disable();
        AddTail((struct List *)&unit->bgu_TypeTrackers,
                (struct Node *)tracker);
        Enable();
    }

    initial->packet_type = packet_type;
    initial->stats = tracker->stats;
    AddTail((struct List *)&opener->initial_stats, (struct Node *)initial);

    return TRUE;
}

static BOOL CmdUntrackType(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener = request->ios2_BufferManagement;
    ULONG packet_type = request->ios2_PacketType;
    struct TypeStats *initial;
    struct TypeTracker *tracker;

    initial = BCMGENET_FindTypeStats(&opener->initial_stats, packet_type);
    if (!initial)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_TRACKED;
        return TRUE;
    }

    Remove((struct Node *)initial);
    FreeMem(initial, sizeof(struct TypeStats));

    tracker = (struct TypeTracker *)
        BCMGENET_FindTypeStats(&unit->bgu_TypeTrackers, packet_type);
    if (tracker && --tracker->user_count == 0)
    {
        Disable();
        Remove((struct Node *)tracker);
        Enable();
        FreeMem(tracker, sizeof(struct TypeTracker));
    }

    return TRUE;
}

static BOOL CmdGetTypeStats(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener = request->ios2_BufferManagement;
    ULONG packet_type = request->ios2_PacketType;
    struct TypeStats *initial, *tracker;
    struct Sana2PacketTypeStats *stats;

    initial = BCMGENET_FindTypeStats(&opener->initial_stats, packet_type);
    tracker = BCMGENET_FindTypeStats(&unit->bgu_TypeTrackers, packet_type);

    if (!initial || !tracker)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_TRACKED;
        return TRUE;
    }

    stats = request->ios2_StatData;
    stats->PacketsSent = tracker->stats.PacketsSent -
                         initial->stats.PacketsSent;
    stats->PacketsReceived = tracker->stats.PacketsReceived -
                             initial->stats.PacketsReceived;
    stats->BytesSent = tracker->stats.BytesSent - initial->stats.BytesSent;
    stats->BytesReceived = tracker->stats.BytesReceived -
                           initial->stats.BytesReceived;
    stats->PacketsDropped = tracker->stats.PacketsDropped -
                            initial->stats.PacketsDropped;

    return TRUE;
}

static BOOL CmdGetSpecialStats(struct BCMGENETBase *base,
                               struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Sana2SpecialStatHeader *header = request->ios2_StatData;
    struct Sana2SpecialStatRecord *record;
    ULONG stat_count, i;

    stat_count = header->RecordCountMax;
    if (stat_count > STAT_COUNT)
        stat_count = STAT_COUNT;

    record = (struct Sana2SpecialStatRecord *)(header + 1);
    for (i = 0; i < stat_count; i++, record++)
    {
        record->Type = (S2WireType_Ethernet << 16) + i;
        record->Count = unit->bgu_SpecialStats[i];
        record->String = (STRPTR)bcmgenet_special_stat_names[i];
    }
    header->RecordCountSupplied = stat_count;

    return TRUE;
}

static BOOL CmdGetGlobalStats(struct BCMGENETBase *base,
                              struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    CopyMem(&unit->bgu_Stats, request->ios2_StatData,
            sizeof(struct Sana2DeviceStats));

    return TRUE;
}

static BOOL CmdOnEvent(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    ULONG events, wanted_events;

    wanted_events = request->ios2_WireError;
    if ((wanted_events & ~KNOWN_EVENTS) != 0)
    {
        request->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
        request->ios2_WireError = S2WERR_BAD_EVENT;
        return TRUE;
    }

    if ((unit->bgu_Flags & IFF_UP) != 0)
        events = S2EVENT_ONLINE;
    else
        events = S2EVENT_OFFLINE;

    events &= wanted_events;
    if (events != 0)
    {
        request->ios2_WireError = events;
        return TRUE;
    }

    request->ios2_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(unit->bgu_RequestPorts[EVENT_QUEUE], (struct Message *)request);

    return FALSE;
}

static BOOL CmdReadOrphan(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->bgu_Flags & IFF_UP) == 0)
    {
        request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        request->ios2_WireError = S2WERR_UNIT_OFFLINE;
        return TRUE;
    }

    request->ios2_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(unit->bgu_RequestPorts[ADOPT_QUEUE], (struct Message *)request);

    return FALSE;
}

static BOOL CmdOnline(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->bgu_Flags & IFF_CONFIGURED) == 0)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_CONFIGURED;
        return TRUE;
    }

    if ((unit->bgu_Flags & IFF_UP) == 0)
        BCMGENET_GoOnline(base, unit);

    return TRUE;
}

static BOOL CmdOffline(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->bgu_Flags & IFF_UP) != 0)
        BCMGENET_GoOffline(base, unit);

    return TRUE;
}

static BOOL CmdNSDeviceQuery(struct BCMGENETBase *base,
                             struct IOSana2Req *request)
{
    struct IOStdReq *io = (struct IOStdReq *)request;
    struct NSDeviceQueryResult *info = io->io_Data;

    io->io_Actual = info->SizeAvailable =
        offsetof(struct NSDeviceQueryResult, SupportedCommands) +
        sizeof(APTR);
    info->DeviceType = NSDEVTYPE_SANA2;
    info->DeviceSubType = 0;
    info->SupportedCommands = (UWORD *)bcmgenet_supported_commands;

    return TRUE;
}

void BCMGENET_HandleRequest(struct BCMGENETBase *base, struct IOSana2Req *request)
{
    struct BCMGENETUnit *unit = (APTR)request->ios2_Req.io_Unit;
    BOOL complete;

    /*
     * Off every queue while it is being looked at, so AbortIO can tell
     * a request it may Remove() from one it may not. Queueing it again
     * or replying restores a valid node type.
     */
    request->ios2_Req.io_Message.mn_Node.ln_Type = NT_FREEMSG;
    request->ios2_Req.io_Error = 0;
    request->ios2_WireError = 0;

    switch (request->ios2_Req.io_Command)
    {
    case CMD_READ:
        complete = CmdRead(base, request);
        break;
    case CMD_WRITE:
    case S2_MULTICAST:
        complete = CmdWrite(base, request);
        break;
    case S2_BROADCAST:
        complete = CmdBroadcast(base, request);
        break;
    case CMD_FLUSH:
        complete = CmdFlush(base, request);
        break;
    case S2_DEVICEQUERY:
        complete = CmdDeviceQuery(base, request);
        break;
    case S2_GETSTATIONADDRESS:
        complete = CmdGetStationAddress(base, request);
        break;
    case S2_CONFIGINTERFACE:
        complete = CmdConfigInterface(base, request);
        break;
    case S2_ADDMULTICASTADDRESS:
    case S2_ADDMULTICASTADDRESSES:
        complete = CmdAddMulticastAddresses(base, request);
        break;
    case S2_DELMULTICASTADDRESS:
    case S2_DELMULTICASTADDRESSES:
        complete = CmdDelMulticastAddresses(base, request);
        break;
    case S2_TRACKTYPE:
        complete = CmdTrackType(base, request);
        break;
    case S2_UNTRACKTYPE:
        complete = CmdUntrackType(base, request);
        break;
    case S2_GETTYPESTATS:
        complete = CmdGetTypeStats(base, request);
        break;
    case S2_GETSPECIALSTATS:
        complete = CmdGetSpecialStats(base, request);
        break;
    case S2_GETGLOBALSTATS:
        complete = CmdGetGlobalStats(base, request);
        break;
    case S2_ONEVENT:
        complete = CmdOnEvent(base, request);
        break;
    case S2_READORPHAN:
        complete = CmdReadOrphan(base, request);
        break;
    case S2_ONLINE:
        complete = CmdOnline(base, request);
        break;
    case S2_OFFLINE:
        complete = CmdOffline(base, request);
        break;
    case NSCMD_DEVICEQUERY:
        complete = CmdNSDeviceQuery(base, request);
        break;
    default:
        complete = CmdInvalid(base, request);
        break;
    }

    D(bug("[bcmgenet] cmd %04lx: error %ld/%08lx, unit flags %04lx\n",
          (ULONG)request->ios2_Req.io_Command,
          (LONG)request->ios2_Req.io_Error,
          (ULONG)request->ios2_WireError, (ULONG)unit->bgu_Flags);)

    if (complete && (request->ios2_Req.io_Flags & IOF_QUICK) == 0)
        ReplyMsg((struct Message *)request);

    ReleaseSemaphore(&unit->bgu_Lock);
}
