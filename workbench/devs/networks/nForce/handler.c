/*
 * $Id$
 */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA.
*/

#define DEBUG 0

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>

#include <aros/debug.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

#include <stdlib.h>

#include "nforce.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

#define KNOWN_EVENTS \
    (S2EVENT_ERROR | S2EVENT_TX | S2EVENT_RX | S2EVENT_ONLINE \
    | S2EVENT_OFFLINE | S2EVENT_BUFF | S2EVENT_HARDWARE | S2EVENT_SOFTWARE)

static BOOL CmdInvalid(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdRead(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdWrite(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdFlush(LIBBASETYPEPTR LIBBASE, struct IORequest *request);
static BOOL CmdS2DeviceQuery(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdGetStationAddress(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdConfigInterface(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdBroadcast(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdTrackType(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdUntrackType(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdGetTypeStats(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdGetGlobalStats(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdDeviceQuery(LIBBASETYPEPTR LIBBASE, struct IOStdReq *request);
static BOOL CmdOnEvent(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdReadOrphan(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdOnline(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdOffline(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdAddMulticastAddresses(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);
static BOOL CmdDelMulticastAddresses(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request);

static const UWORD supported_commands[] =
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

void handle_request(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    BOOL complete;

    switch(request->ios2_Req.io_Command)
    {
        case CMD_READ:
            complete = CmdRead(LIBBASE, request);
            break;

        case CMD_WRITE:
        case S2_MULTICAST:
            complete = CmdWrite(LIBBASE, request);
            break;

        case CMD_FLUSH:
            complete = CmdFlush(LIBBASE, (struct IORequest *)request);
            break;

        case S2_DEVICEQUERY:
            complete = CmdS2DeviceQuery(LIBBASE, request);
            break;

        case S2_GETSTATIONADDRESS:
            complete = CmdGetStationAddress(LIBBASE, request);
            break;

        case S2_CONFIGINTERFACE:
            complete = CmdConfigInterface(LIBBASE, request);
            break;

        case S2_BROADCAST:
            complete = CmdBroadcast(LIBBASE, request);
            break;

        case S2_TRACKTYPE:
            complete = CmdTrackType(LIBBASE, request);
            break;

        case S2_UNTRACKTYPE:
            complete = CmdUntrackType(LIBBASE, request);
            break;

        case S2_GETTYPESTATS:
            complete = CmdGetTypeStats(LIBBASE, request);
            break;

        case S2_GETGLOBALSTATS:
            complete = CmdGetGlobalStats(LIBBASE, request);
            break;

        case S2_ONEVENT:
            complete = CmdOnEvent(LIBBASE, request);
            break;

        case S2_READORPHAN:
            complete = CmdReadOrphan(LIBBASE, request);
            break;

        case S2_ONLINE:
            complete = CmdOnline(LIBBASE, request);
            break;

        case S2_OFFLINE:
            complete = CmdOffline(LIBBASE, request);
            break;

        case S2_ADDMULTICASTADDRESS:
        case S2_ADDMULTICASTADDRESSES:
            complete = CmdAddMulticastAddresses(LIBBASE, request);
            break;

        case S2_DELMULTICASTADDRESS:
        case S2_DELMULTICASTADDRESSES:
            complete = CmdDelMulticastAddresses(LIBBASE, request);
            break;

        case NSCMD_DEVICEQUERY:
            complete = CmdDeviceQuery(LIBBASE, (struct IOStdReq *)request);
            break;

        default:
            complete = CmdInvalid(LIBBASE, request);
    }

    if(complete && (request->ios2_Req.io_Flags & IOF_QUICK) == 0)
        ReplyMsg((APTR)request);

    ReleaseSemaphore(&((struct NFUnit *)request->ios2_Req.io_Unit)->unit_lock);
}

static BOOL CmdInvalid(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    request->ios2_Req.io_Error = IOERR_NOCMD;
    request->ios2_WireError = S2WERR_GENERIC_ERROR;

    return TRUE;
}

static BOOL CmdRead(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    struct Opener *opener;
    BOOL complete = FALSE;

    unit = (APTR)request->ios2_Req.io_Unit;

    if((unit->flags & IFF_UP) != 0)
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

static BOOL CmdWrite(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    BYTE error = 0;
    ULONG wire_error;
    BOOL complete = FALSE;

    /* Check request is valid */

    unit = (APTR)request->ios2_Req.io_Unit;
    if((unit->flags & IFF_UP) == 0)
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
        PutMsg(unit->request_ports[WRITE_QUEUE], (APTR)request);
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

static BOOL CmdFlush(LIBBASETYPEPTR LIBBASE, struct IORequest *request)
{
    FlushUnit(LIBBASE, (APTR)request->io_Unit, EVENT_QUEUE, IOERR_ABORTED);
    return TRUE;
}

static BOOL CmdS2DeviceQuery(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit = (APTR)request->ios2_Req.io_Unit;
    struct fe_priv *np = unit->nu_fe_priv;
    struct Sana2DeviceQuery *info;
    ULONG size_available, size;

    /* Copy device info */

    info = request->ios2_StatData;
    size = size_available = info->SizeAvailable;
    if(size > sizeof(struct Sana2DeviceQuery))
        size = sizeof(struct Sana2DeviceQuery);

    CopyMem(&LIBBASE->nf_Sana2Info, info, size);

    if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_100)
        info->BPS = 100000000;
    else if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000)
        info->BPS = 1000000000;
    else
        info->BPS = 10000000;

    info->SizeAvailable = size_available;
    info->SizeSupplied = size;

    /* Return */

    return TRUE;
}

static BOOL CmdGetStationAddress(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;

    /* Copy addresses */

    unit = (APTR)request->ios2_Req.io_Unit;
    CopyMem(unit->dev_addr, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(unit->org_addr, request->ios2_DstAddr, ETH_ADDRESSSIZE);

    /* Return */

    return TRUE;
}

static BOOL CmdConfigInterface(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;

    /* Configure adapter */

    unit = (APTR)request->ios2_Req.io_Unit;
    if((unit->flags & IFF_CONFIGURED) == 0)
    {
        CopyMem(request->ios2_SrcAddr, unit->dev_addr, ETH_ADDRESSSIZE);
        unit->set_mac_address(unit);
        unit->flags |= IFF_CONFIGURED;
    }
    else
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_IS_CONFIGURED;
    }

    /* Return */

    return TRUE;
}

static BOOL CmdBroadcast(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    /* Fill in the broadcast address as destination */

    *((ULONG *)request->ios2_DstAddr) = 0xffffffff;
    *((UWORD *)(request->ios2_DstAddr + 4)) = 0xffff;

    /* Queue the write as normal */

    return CmdWrite(LIBBASE, request);
}

static BOOL CmdTrackType(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    struct Opener *opener;
    ULONG packet_type, wire_error=0;
    struct TypeTracker *tracker;
    struct TypeStats *initial_stats;
    BYTE error = 0;

    unit = (APTR)request->ios2_Req.io_Unit;
    packet_type = request->ios2_PacketType;

    /* Get global tracker */

    tracker = (struct TypeTracker *)
        FindTypeStats(LIBBASE, unit, &unit->type_trackers, packet_type);

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
            AddTail((APTR)&unit->type_trackers, (APTR)tracker);
            Enable();
        }
   }

    /* Store initial figures for this opener */

    opener = request->ios2_BufferManagement;
    initial_stats = FindTypeStats(LIBBASE, unit, &opener->initial_stats, packet_type);

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

static BOOL CmdUntrackType(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    struct Opener *opener;
    ULONG packet_type;
    struct TypeTracker *tracker;
    struct TypeStats *initial_stats;

    unit = (APTR)request->ios2_Req.io_Unit;
    packet_type = request->ios2_PacketType;

    /* Get global tracker and initial figures */

    tracker = (struct TypeTracker *)
        FindTypeStats(LIBBASE, unit, &unit->type_trackers, packet_type);
    opener = request->ios2_BufferManagement;
    initial_stats = FindTypeStats(LIBBASE, unit, &opener->initial_stats, packet_type);

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

static BOOL CmdGetTypeStats(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    struct Opener *opener;
    ULONG packet_type;
    struct TypeStats *initial_stats, *tracker;
    struct Sana2PacketTypeStats *stats;

    unit = (APTR)request->ios2_Req.io_Unit;
    packet_type = request->ios2_PacketType;

    /* Get global tracker and initial figures */

    tracker = FindTypeStats(LIBBASE, unit, &unit->type_trackers, packet_type);
    opener = request->ios2_BufferManagement;
    initial_stats = FindTypeStats(LIBBASE, unit, &opener->initial_stats, packet_type);

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

static BOOL CmdGetGlobalStats(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;

    /* Update and copy stats */

    unit = (APTR)request->ios2_Req.io_Unit;
    CopyMem(&unit->stats, request->ios2_StatData,
        sizeof(struct Sana2DeviceStats));

    /* Return */

    return TRUE;
}

static BOOL CmdDeviceQuery(LIBBASETYPEPTR LIBBASE, struct IOStdReq *request)
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

static BOOL CmdOnEvent(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    ULONG events, wanted_events;
    BOOL complete = FALSE;

    /* Check if we understand the event types */

    unit = (APTR)request->ios2_Req.io_Unit;
    wanted_events = request->ios2_WireError;
    if((wanted_events & ~KNOWN_EVENTS) != 0)
    {
        request->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
        events = S2WERR_BAD_EVENT;
    }
    else
    {
        if((unit->flags & IFF_UP) != 0)
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
        PutMsg(unit->request_ports[EVENT_QUEUE], (APTR)request);
    }

    /* Return */

    return complete;
}

static BOOL CmdReadOrphan(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    BYTE error = 0;
    ULONG wire_error;
    BOOL complete = FALSE;

    /* Check request is valid */

    unit = (APTR)request->ios2_Req.io_Unit;
    if((unit->flags & IFF_UP) == 0)
    {
        error = S2ERR_OUTOFSERVICE;
        wire_error = S2WERR_UNIT_OFFLINE;
    }

    /* Queue request */

    if(error == 0)
    {
        request->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->request_ports[ADOPT_QUEUE], (struct Message *)request);
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

static BOOL CmdOnline(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit = (struct NFUnit *)request->ios2_Req.io_Unit;
    BYTE error = 0;
    ULONG wire_error = 0;
    UWORD i;

    /* Check request is valid */
    if((unit->flags & IFF_CONFIGURED) == 0)
    {
        error = S2ERR_BAD_STATE;
        wire_error = S2WERR_NOT_CONFIGURED;
    }

    /* Clear global and special stats and put adapter back online */

    if((error == 0) && ((unit->flags & IFF_UP) == 0))
    {
        unit->stats.PacketsReceived = 0;
        unit->stats.PacketsSent = 0;
        unit->stats.BadData = 0;
        unit->stats.Overruns = 0;
        unit->stats.UnknownTypesReceived = 0;
        unit->stats.Reconfigurations = 0;

        for(i = 0; i < STAT_COUNT; i++)
            unit->special_stats[i] = 0;

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

static BOOL CmdOffline(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;

    /* Put adapter offline */

    unit = (APTR)request->ios2_Req.io_Unit;
    if((unit->flags & IFF_UP) != 0)
        unit->stop(unit);

    /* Return */
    return TRUE;
}

static BOOL CmdAddMulticastAddresses(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    UBYTE *lower_bound, *upper_bound;

    unit = (APTR)request->ios2_Req.io_Unit;

    lower_bound = request->ios2_SrcAddr;
    if(request->ios2_Req.io_Command == S2_ADDMULTICASTADDRESS)
        upper_bound = lower_bound;
    else
        upper_bound = request->ios2_DstAddr;

    if(!AddMulticastRange(LIBBASE, unit, lower_bound, upper_bound))
    {
        request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        request->ios2_WireError = S2WERR_GENERIC_ERROR;
    }

    /* Return */

    return TRUE;
}

static BOOL CmdDelMulticastAddresses(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *request)
{
    struct NFUnit *unit;
    UBYTE *lower_bound, *upper_bound;

    unit = (APTR)request->ios2_Req.io_Unit;

    lower_bound = request->ios2_SrcAddr;
    if(request->ios2_Req.io_Command == S2_DELMULTICASTADDRESS)
        upper_bound = lower_bound;
    else
        upper_bound = request->ios2_DstAddr;

    if(!RemMulticastRange(LIBBASE, unit, lower_bound, upper_bound))
    {
        request->ios2_Req.io_Error = S2ERR_BAD_STATE;
        request->ios2_WireError = S2WERR_BAD_MULTICAST;
    }

    /* Return */

    return TRUE;
}
