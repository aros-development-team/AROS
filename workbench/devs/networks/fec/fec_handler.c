/*
 * fec_handler.c
 *
 *  Created on: May 18, 2009
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/memory.h>
#include <exec/errors.h>

#include <devices/newstyle.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/openfirmware.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <inttypes.h>

#include "fec.h"

#define KNOWN_EVENTS \
    (S2EVENT_ERROR | S2EVENT_TX | S2EVENT_RX | S2EVENT_ONLINE \
    | S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE)

static BOOL CmdInvalid(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdRead(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdWrite(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdFlush(struct FECBase *FECBase, struct IORequest *request);
static BOOL CmdS2DeviceQuery(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdGetStationAddress(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdConfigInterface(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdBroadcast(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdTrackType(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdUntrackType(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdGetTypeStats(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdGetGlobalStats(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdDeviceQuery(struct FECBase *FECBase, struct IOStdReq *request);
static BOOL CmdOnEvent(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdReadOrphan(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdOnline(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdOffline(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdAddMulticastAddresses(struct FECBase *FECBase, struct IOSana2Req *request);
static BOOL CmdDelMulticastAddresses(struct FECBase *FECBase, struct IOSana2Req *request);

static const uint16_t supported_commands[] =
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
//    S2_GETSPECIALSTATS,
    S2_GETGLOBALSTATS,
    S2_ONEVENT,
    S2_READORPHAN,
    S2_ONLINE,
    S2_OFFLINE,
    NSCMD_DEVICEQUERY,
    S2_ADDMULTICASTADDRESSES,
    S2_DELMULTICASTADDRESSES,
    0
};

void handle_request(struct FECBase *FECBase, struct IOSana2Req *request)
{
	D(bug("[FEC] handle_request\n"));

	BOOL complete;

	switch(request->ios2_Req.io_Command)
	{
		case CMD_READ:
			complete = CmdRead(FECBase, request);
			break;

		case CMD_WRITE:
		case S2_MULTICAST:
			complete = CmdWrite(FECBase, request);
			break;

        case CMD_FLUSH:
            complete = CmdFlush(FECBase, (struct IORequest *)request);
            break;

        case S2_DEVICEQUERY:
            complete = CmdS2DeviceQuery(FECBase, request);
            break;

        case S2_GETSTATIONADDRESS:
            complete = CmdGetStationAddress(FECBase, request);
            break;

        case S2_CONFIGINTERFACE:
            complete = CmdConfigInterface(FECBase, request);
            break;

        case S2_BROADCAST:
            complete = CmdBroadcast(FECBase, request);
            break;

        case S2_TRACKTYPE:
            complete = CmdTrackType(FECBase, request);
            break;

        case S2_UNTRACKTYPE:
            complete = CmdUntrackType(FECBase, request);
            break;

        case S2_GETTYPESTATS:
            complete = CmdGetTypeStats(FECBase, request);
            break;

        case S2_GETGLOBALSTATS:
        	complete = CmdGetGlobalStats(FECBase, request);
        	break;

        case S2_ONEVENT:
        	complete = CmdOnEvent(FECBase, request);
        	break;

        case S2_READORPHAN:
        	complete = CmdReadOrphan(FECBase, request);
        	break;

        case S2_ONLINE:
        	complete = CmdOnline(FECBase, request);
        	break;

        case S2_OFFLINE:
        	complete = CmdOffline(FECBase, request);
        	break;

        case S2_ADDMULTICASTADDRESS:
        case S2_ADDMULTICASTADDRESSES:
        	complete = CmdAddMulticastAddresses(FECBase, request);
        	break;

        case S2_DELMULTICASTADDRESS:
        case S2_DELMULTICASTADDRESSES:
        	complete = CmdDelMulticastAddresses(FECBase, request);
        	break;

        case NSCMD_DEVICEQUERY:
        	complete = CmdDeviceQuery(FECBase, (struct IOStdReq *)request);
        	break;

		default:
			complete = CmdInvalid(FECBase, request);
	}

	if(complete && (request->ios2_Req.io_Flags & IOF_QUICK) == 0)
		ReplyMsg((APTR)request);

	ReleaseSemaphore(&((struct FECUnit *)request->ios2_Req.io_Unit)->feu_Lock);
}


static BOOL CmdInvalid(struct FECBase *FECBase, struct IOSana2Req *request)
{
    request->ios2_Req.io_Error = IOERR_NOCMD;
    request->ios2_WireError = S2WERR_GENERIC_ERROR;

    return TRUE;
}

static BOOL CmdRead(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    struct Opener *opener;
    BOOL complete = FALSE;

    unit = (APTR)request->ios2_Req.io_Unit;

D(bug("[FEC] S2CmdRead()\n"));

    if((unit->feu_Flags & IFF_UP) != 0)
    {
        opener = request->ios2_BufferManagement;
        request->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(&opener->read_port, (struct Message *)request);
    }
    else
    {
        request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        request->ios2_WireError = S2WERR_UNIT_OFFLINE;
        complete = TRUE;
    }

    /* Return */

    return complete;
}

static BOOL CmdWrite(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    BYTE error = 0;
    ULONG wire_error = S2WERR_GENERIC_ERROR;
    BOOL complete = FALSE;

    /* Check request is valid */

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdWrite()\n"));

    if((unit->feu_Flags & IFF_UP) == 0)
    {
        error = S2ERR_OUTOFSERVICE;
        wire_error = S2WERR_UNIT_OFFLINE;
    }
    else if((request->ios2_Req.io_Command == S2_MULTICAST) &&
            ((request->ios2_DstAddr[0] & 0x1) == 0))
    {
        error = S2ERR_BAD_ADDRESS;
        wire_error = S2WERR_BAD_MULTICAST;
    }

    /* Queue request for sending */

    if(error == 0) {
        request->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->feu_RequestPorts[WRITE_QUEUE], (APTR)request);
    }
    else
    {
        request->ios2_Req.io_Error = error;
        request->ios2_WireError = wire_error;
        complete = TRUE;
    }

    /* Return */
    return complete;
}

static BOOL CmdFlush(struct FECBase *FECBase, struct IORequest *request)
{
#warning TODO: Implement CmdFlush!!!!!!!!!
    //    FlushUnit(LIBBASE, (APTR)request->io_Unit, EVENT_QUEUE, IOERR_ABORTED);
    return TRUE;
}

static BOOL CmdS2DeviceQuery(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit = (APTR)request->ios2_Req.io_Unit;
    //struct fe_priv *np = unit->pcnu_fe_priv;
    struct Sana2DeviceQuery *info;
    ULONG size_available, size;

D(bug("[FEC] S2CmdDeviceQuery()\n"));

    /* Copy device info */

    info = request->ios2_StatData;
    size = size_available = info->SizeAvailable;
    if(size > sizeof(struct Sana2DeviceQuery))
        size = sizeof(struct Sana2DeviceQuery);

    CopyMem(&FECBase->feb_Sana2Info, info, size);

    info->BPS = unit->feu_speed * 1000000;
    info->MTU = ETH_MTU;
    info->HardwareType = S2WireType_Ethernet;
    info->SizeAvailable = size_available;
    info->SizeSupplied = size;

    /* Return */

    return TRUE;
}

static BOOL CmdGetStationAddress(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;

    /* Copy addresses */

    unit = (APTR)request->ios2_Req.io_Unit;

D(bug("[FEC] S2CmdGetStationAddress()\n"));

    CopyMem(unit->feu_DevAddr, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(unit->feu_OrgAddr, request->ios2_DstAddr, ETH_ADDRESSSIZE);

    /* Return */

    return TRUE;
}

static BOOL CmdConfigInterface(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;

    /* Configure adapter */

    unit = (APTR)request->ios2_Req.io_Unit;

D(bug("[FEC] S2CmdConfigInterface()\n"));

    if((unit->feu_Flags & IFF_CONFIGURED) == 0)
    {
        CopyMem(request->ios2_SrcAddr, unit->feu_DevAddr, ETH_ADDRESSSIZE);
        unit->set_mac_address(unit);
        unit->feu_Flags |= IFF_CONFIGURED;
    }
    else
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_IS_CONFIGURED;
    }

    /* Return */

    return TRUE;
}

static BOOL CmdBroadcast(struct FECBase *FECBase, struct IOSana2Req *request)
{
    UWORD i;

    /* Fill in the broadcast address as destination */

    for(i = 0; i < ETH_ADDRESSSIZE; i++)
        request->ios2_DstAddr[i] = 0xff;

    /* Queue the write as normal */

    return CmdWrite(FECBase, request);
}

static BOOL CmdTrackType(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    struct Opener *opener;
    ULONG packet_type, wire_error=0;
    struct TypeTracker *tracker;
    struct TypeStats *initial_stats;
    BYTE error = 0;

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdTrackType(%d)\n", request->ios2_PacketType));

    packet_type = request->ios2_PacketType;

    /* Get global tracker */
    tracker = (struct TypeTracker *)
        FindTypeStats(FECBase, unit, &unit->feu_TypeTrackers, packet_type);

    if(tracker != NULL)
        tracker->user_count++;
    else
    {
        tracker =
            AllocMem(sizeof(struct TypeTracker), MEMF_PUBLIC|MEMF_CLEAR);
        if(tracker != NULL)
        {
            tracker->packet_type = packet_type;
            tracker->user_count = 1;

            Disable();
            AddTail((APTR)&unit->feu_TypeTrackers, (APTR)tracker);
            Enable();
        }
   }

    /* Store initial figures for this opener */

    opener = request->ios2_BufferManagement;
    initial_stats = FindTypeStats(FECBase, unit, &opener->initial_stats, packet_type);
    if(initial_stats != NULL)
    {
        error = S2ERR_BAD_STATE;
        wire_error = S2WERR_ALREADY_TRACKED;
    }

    if(error == 0)
    {
        initial_stats = AllocMem(sizeof(struct TypeStats), MEMF_PUBLIC);
        if(initial_stats == NULL)
        {
            error = S2ERR_NO_RESOURCES;
            wire_error = S2WERR_GENERIC_ERROR;
        }
    }

    if(error == 0)
    {
        CopyMem(tracker, initial_stats, sizeof(struct TypeStats));
        AddTail((APTR)&opener->initial_stats, (APTR)initial_stats);
    }

    /* Return */

    request->ios2_Req.io_Error = error;
    request->ios2_WireError = wire_error;
    return TRUE;
}

static BOOL CmdUntrackType(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    struct Opener *opener;
    ULONG packet_type;
    struct TypeTracker *tracker;
    struct TypeStats *initial_stats;

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdUntrackType()\n"));

    packet_type = request->ios2_PacketType;

    /* Get global tracker and initial figures */

    tracker = (struct TypeTracker *)
        FindTypeStats(FECBase, unit, &unit->feu_TypeTrackers, packet_type);
    opener = request->ios2_BufferManagement;
    initial_stats = FindTypeStats(FECBase, unit, &opener->initial_stats, packet_type);

    /* Decrement tracker usage and free unused structures */

    if(initial_stats != NULL)
    {
        if((--tracker->user_count) == 0)
        {
            Disable();
            Remove((APTR)tracker);
            Enable();
            FreeMem(tracker, sizeof(struct TypeTracker));
        }

        Remove((APTR)initial_stats);
        FreeMem(initial_stats, sizeof(struct TypeStats));
    }
    else
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_TRACKED;
    }

    /* Return */

    return TRUE;
}

static BOOL CmdGetTypeStats(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    struct Opener *opener;
    ULONG packet_type;
    struct TypeStats *initial_stats, *tracker;
    struct Sana2PacketTypeStats *stats;

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdGetTypeStats()\n"));

    packet_type = request->ios2_PacketType;

    /* Get global tracker and initial figures */

    tracker = FindTypeStats(FECBase, unit, &unit->feu_TypeTrackers, packet_type);
    opener = request->ios2_BufferManagement;
    initial_stats = FindTypeStats(FECBase, unit, &opener->initial_stats, packet_type);

    /* Copy and adjust figures */
    if(initial_stats != NULL)
    {
        stats = request->ios2_StatData;
        CopyMem(&tracker->stats, stats, sizeof(struct Sana2PacketTypeStats));
        stats->PacketsSent -= initial_stats->stats.PacketsSent;
        stats->PacketsReceived -= initial_stats->stats.PacketsReceived;
        stats->BytesSent -= initial_stats->stats.BytesSent;
        stats->BytesReceived -= initial_stats->stats.BytesReceived;
        stats->PacketsDropped -= initial_stats->stats.PacketsDropped;
    }
    else
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_NOT_TRACKED;
    }

    /* Return */

    return TRUE;
}

static BOOL CmdGetGlobalStats(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;

    /* Update and copy stats */

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdGetGlobalStats()\n"));

    CopyMem(&unit->feu_Stats, request->ios2_StatData,
        sizeof(struct Sana2DeviceStats));

    /* Return */

    return TRUE;
}

static BOOL CmdDeviceQuery(struct FECBase *FECBase, struct IOStdReq *request)
{
    struct NSDeviceQueryResult *info;

    /* Set structure size twice */

    info = request->io_Data;
    request->io_Actual = info->SizeAvailable =
        offsetof(struct NSDeviceQueryResult, SupportedCommands) + sizeof(APTR);

    /* Report device details */

    info->DeviceType = NSDEVTYPE_SANA2;
    info->DeviceSubType = 0;

    info->SupportedCommands = (APTR)supported_commands;

    /* Return */

    return TRUE;
}

static BOOL CmdOnEvent(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    ULONG events, wanted_events;
    BOOL complete = FALSE;

    /* Check if we understand the event types */

    unit = (struct FECUnit *)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdOnEvent()\n"));

    wanted_events = request->ios2_WireError;
    if((wanted_events & ~KNOWN_EVENTS) != 0)
    {
        request->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
        events = S2WERR_BAD_EVENT;
    }
    else
    {
        if((unit->feu_Flags & IFF_UP) != 0)
            events = S2EVENT_ONLINE;
        else
            events = S2EVENT_OFFLINE;

        events &= wanted_events;
    }

    /* Reply request if a wanted event has already occurred */
    if(events != 0)
    {
        request->ios2_WireError = events;
        complete = TRUE;
    }
    else
    {
        request->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->feu_RequestPorts[EVENT_QUEUE], (APTR)request);
    }

    /* Return */

    return complete;
}

static BOOL CmdReadOrphan(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    BYTE error = 0;
    ULONG wire_error;
    BOOL complete = FALSE;

    /* Check request is valid */

    unit = (struct FECUnit *)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdReadOrphan()\n"));

    if((unit->feu_Flags & IFF_UP) == 0)
    {
        error = S2ERR_OUTOFSERVICE;
        wire_error = S2WERR_UNIT_OFFLINE;
    }

    /* Queue request */

    if(error == 0)
    {
        request->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->feu_RequestPorts[ADOPT_QUEUE], (struct Message *)request);
    }
    else
    {
        request->ios2_Req.io_Error = error;
        request->ios2_WireError = wire_error;
        complete = TRUE;
    }

    /* Return */

    return complete;
}

static BOOL CmdOnline(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit = (struct FECUnit *)request->ios2_Req.io_Unit;
    BYTE error = 0;
    ULONG wire_error = 0;
    UWORD i;

    D(bug("[FEC] S2CmdOnline()\n"));

    /* Check request is valid */
    if((unit->feu_Flags & IFF_CONFIGURED) == 0)
    {
        error = S2ERR_BAD_STATE;
        wire_error = S2WERR_NOT_CONFIGURED;
    }

    /* Clear global and special stats and put adapter back online */

    if((error == 0) && ((unit->feu_Flags & IFF_UP) == 0))
    {
        unit->feu_Stats.PacketsReceived = 0;
        unit->feu_Stats.PacketsSent = 0;
        unit->feu_Stats.BadData = 0;
        unit->feu_Stats.Overruns = 0;
        unit->feu_Stats.UnknownTypesReceived = 0;
        unit->feu_Stats.Reconfigurations = 0;

        for(i = 0; i < STAT_COUNT; i++)
            unit->feu_SpecialStats[i] = 0;

        if (unit->start(unit)) {
            error = S2ERR_OUTOFSERVICE;
            wire_error = S2WERR_GENERIC_ERROR;
        }
    }

    /* Return */
    request->ios2_Req.io_Error = error;
    request->ios2_WireError = wire_error;
    return TRUE;
}

static BOOL CmdOffline(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;

    /* Put adapter offline */

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdOffline()\n"));

    if((unit->feu_Flags & IFF_UP) != 0)
        unit->stop(unit);

    /* Return */
    return TRUE;
}

static BOOL CmdAddMulticastAddresses(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    UBYTE *lower_bound, *upper_bound;

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdAddMulticastAddresses()\n"));

    lower_bound = request->ios2_SrcAddr;
    if(request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESS)
        upper_bound = lower_bound;
    else
        upper_bound = request->ios2_DstAddr;

    if(!AddMulticastRange(FECBase, unit, lower_bound, upper_bound))
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_GENERIC_ERROR;
    }

    /* Return */

    return TRUE;
}

static BOOL CmdDelMulticastAddresses(struct FECBase *FECBase, struct IOSana2Req *request)
{
    struct FECUnit *unit;
    UBYTE *lower_bound, *upper_bound;

    unit = (APTR)request->ios2_Req.io_Unit;

    D(bug("[FEC] S2CmdDelMulticastAddresses()\n"));

    lower_bound = request->ios2_SrcAddr;
    if(request->ios2_Req.io_Command == S2_DELMULTICASTADDRESS)
        upper_bound = lower_bound;
    else
        upper_bound = request->ios2_DstAddr;

    if(!RemMulticastRange(FECBase, unit, lower_bound, upper_bound))
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_BAD_MULTICAST;
    }

    /* Return */

    return TRUE;
}

