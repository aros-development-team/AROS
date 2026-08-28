/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DesignWare MAC SANA-II driver, the command set.
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/errors.h>
#include <devices/newstyle.h>

#include <proto/exec.h>

#include "dwmac.h"

static const UWORD dwmac_supported_commands[] =
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

static const char * const dwmac_special_stat_names[STAT_COUNT] =
{
    "Bad multicasts",
    "Retries",
    "Underruns"
};

#define KNOWN_EVENTS \
    (S2EVENT_ERROR | S2EVENT_TX | S2EVENT_RX | S2EVENT_ONLINE | \
     S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE)

static BOOL CmdInvalid(struct DWMACBase *base, struct IOSana2Req *request)
{
    request->ios2_Req.io_Error = IOERR_NOCMD;
    request->ios2_WireError = S2WERR_GENERIC_ERROR;

    return TRUE;
}

static BOOL CmdRead(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener;

    if ((unit->dwu_Flags & IFF_UP) == 0)
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

static BOOL CmdWrite(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    BYTE error = 0;
    ULONG wire_error = 0;

    if ((unit->dwu_Flags & IFF_UP) == 0)
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
                  unit->dwu_MTU + ETH_HEADERSIZE : unit->dwu_MTU))
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
    PutMsg(unit->dwu_RequestPorts[WRITE_QUEUE], (struct Message *)request);

    return FALSE;
}

static BOOL CmdBroadcast(struct DWMACBase *base, struct IOSana2Req *request)
{
    ULONG i;

    for (i = 0; i < ETH_ADDRESSSIZE; i++)
        request->ios2_DstAddr[i] = 0xff;

    return CmdWrite(base, request);
}

static BOOL CmdFlush(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    DWMAC_FlushUnit(base, unit, EVENT_QUEUE, IOERR_ABORTED);

    return TRUE;
}

static BOOL CmdDeviceQuery(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Sana2DeviceQuery *info = request->ios2_StatData;
    ULONG size_available = info->SizeAvailable;

    if (size_available < sizeof(struct Sana2DeviceQuery))
    {
        request->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
        request->ios2_WireError = S2WERR_BAD_STATDATA;
        return TRUE;
    }

    CopyMem(&unit->dwu_Sana2Info, info, sizeof(struct Sana2DeviceQuery));

    info->BPS = unit->dwu_SpeedMbps * 1000000UL;
    info->SizeAvailable = size_available;
    info->SizeSupplied = sizeof(struct Sana2DeviceQuery);

    return TRUE;
}

static BOOL CmdGetStationAddress(struct DWMACBase *base,
                                 struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    CopyMem(unit->dwu_DevAddr, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(unit->dwu_OrgAddr, request->ios2_DstAddr, ETH_ADDRESSSIZE);

    return TRUE;
}

static BOOL CmdConfigInterface(struct DWMACBase *base,
                               struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->dwu_Flags & IFF_CONFIGURED) != 0)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_IS_CONFIGURED;
        return TRUE;
    }

    CopyMem(request->ios2_SrcAddr, unit->dwu_DevAddr, ETH_ADDRESSSIZE);
    DWMAC_SetMACAddress(unit->dwu_HW, unit->dwu_DevAddr);
    unit->dwu_Flags |= IFF_CONFIGURED;

    /* Going online is S2_ONLINE's job; the stack sends it right after */

    return TRUE;
}

static BOOL CmdAddMulticastAddresses(struct DWMACBase *base,
                                     struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    UBYTE *lower, *upper;

    lower = request->ios2_SrcAddr;
    upper = (request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESSES) ?
        request->ios2_DstAddr : lower;

    if (!DWMAC_AddMulticastRange(base, unit, lower, upper))
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_GENERIC_ERROR;
    }

    return TRUE;
}

static BOOL CmdDelMulticastAddresses(struct DWMACBase *base,
                                     struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    UBYTE *lower, *upper;

    lower = request->ios2_SrcAddr;
    upper = (request->ios2_Req.io_Command == S2_DELMULTICASTADDRESSES) ?
        request->ios2_DstAddr : lower;

    if (!DWMAC_RemMulticastRange(base, unit, lower, upper))
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_BAD_MULTICAST;
    }

    return TRUE;
}

static BOOL CmdTrackType(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener = request->ios2_BufferManagement;
    ULONG packet_type = request->ios2_PacketType;
    struct TypeStats *initial;
    struct TypeTracker *tracker;

    if (DWMAC_FindTypeStats(&opener->initial_stats, packet_type) != NULL)
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
        DWMAC_FindTypeStats(&unit->dwu_TypeTrackers, packet_type);
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
        AddTail((struct List *)&unit->dwu_TypeTrackers,
                (struct Node *)tracker);
        Enable();
    }

    initial->packet_type = packet_type;
    initial->stats = tracker->stats;
    AddTail((struct List *)&opener->initial_stats, (struct Node *)initial);

    return TRUE;
}

static BOOL CmdUntrackType(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener = request->ios2_BufferManagement;
    ULONG packet_type = request->ios2_PacketType;
    struct TypeStats *initial;
    struct TypeTracker *tracker;

    initial = DWMAC_FindTypeStats(&opener->initial_stats, packet_type);
    if (!initial)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_TRACKED;
        return TRUE;
    }

    Remove((struct Node *)initial);
    FreeMem(initial, sizeof(struct TypeStats));

    tracker = (struct TypeTracker *)
        DWMAC_FindTypeStats(&unit->dwu_TypeTrackers, packet_type);
    if (tracker && --tracker->user_count == 0)
    {
        Disable();
        Remove((struct Node *)tracker);
        Enable();
        FreeMem(tracker, sizeof(struct TypeTracker));
    }

    return TRUE;
}

static BOOL CmdGetTypeStats(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct Opener *opener = request->ios2_BufferManagement;
    ULONG packet_type = request->ios2_PacketType;
    struct TypeStats *initial, *tracker;
    struct Sana2PacketTypeStats *stats;

    initial = DWMAC_FindTypeStats(&opener->initial_stats, packet_type);
    tracker = DWMAC_FindTypeStats(&unit->dwu_TypeTrackers, packet_type);

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

static BOOL CmdGetSpecialStats(struct DWMACBase *base,
                               struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
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
        record->Count = unit->dwu_SpecialStats[i];
        record->String = (STRPTR)dwmac_special_stat_names[i];
    }
    header->RecordCountSupplied = stat_count;

    return TRUE;
}

static BOOL CmdGetGlobalStats(struct DWMACBase *base,
                              struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    CopyMem(&unit->dwu_Stats, request->ios2_StatData,
            sizeof(struct Sana2DeviceStats));

    return TRUE;
}

static BOOL CmdOnEvent(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
    ULONG events, wanted_events;

    wanted_events = request->ios2_WireError;
    if ((wanted_events & ~KNOWN_EVENTS) != 0)
    {
        request->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
        request->ios2_WireError = S2WERR_BAD_EVENT;
        return TRUE;
    }

    if ((unit->dwu_Flags & IFF_UP) != 0)
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
    PutMsg(unit->dwu_RequestPorts[EVENT_QUEUE], (struct Message *)request);

    return FALSE;
}

static BOOL CmdReadOrphan(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->dwu_Flags & IFF_UP) == 0)
    {
        request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        request->ios2_WireError = S2WERR_UNIT_OFFLINE;
        return TRUE;
    }

    request->ios2_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(unit->dwu_RequestPorts[ADOPT_QUEUE], (struct Message *)request);

    return FALSE;
}

static BOOL CmdOnline(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->dwu_Flags & IFF_CONFIGURED) == 0)
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_CONFIGURED;
        return TRUE;
    }

    if ((unit->dwu_Flags & IFF_UP) == 0)
        DWMAC_GoOnline(base, unit);

    return TRUE;
}

static BOOL CmdOffline(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;

    if ((unit->dwu_Flags & IFF_UP) != 0)
        DWMAC_GoOffline(base, unit);

    return TRUE;
}

static BOOL CmdNSDeviceQuery(struct DWMACBase *base,
                             struct IOSana2Req *request)
{
    struct IOStdReq *io = (struct IOStdReq *)request;
    struct NSDeviceQueryResult *info = io->io_Data;

    io->io_Actual = info->SizeAvailable =
        offsetof(struct NSDeviceQueryResult, SupportedCommands) +
        sizeof(APTR);
    info->DeviceType = NSDEVTYPE_SANA2;
    info->DeviceSubType = 0;
    info->SupportedCommands = (UWORD *)dwmac_supported_commands;

    return TRUE;
}

void DWMAC_HandleRequest(struct DWMACBase *base, struct IOSana2Req *request)
{
    struct DWMACUnit *unit = (APTR)request->ios2_Req.io_Unit;
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

    if (complete && (request->ios2_Req.io_Flags & IOF_QUICK) == 0)
        ReplyMsg((struct Message *)request);

    ReleaseSemaphore(&unit->dwu_Lock);
}
