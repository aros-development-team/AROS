/*
 * eth - TUN/TAP network driver for AROS
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright © 2010, The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#define DWRITE(x)

#include "eth.h"

static const UWORD eth_supported_commands[] = {
    S2_DEVICEQUERY,
    S2_GETSTATIONADDRESS,
    S2_CONFIGINTERFACE,
    S2_ONLINE,
    S2_OFFLINE,
    S2_BROADCAST,
    CMD_READ,
    CMD_WRITE,
    CMD_FLUSH,
    S2_TRACKTYPE,
    S2_UNTRACKTYPE,
    S2_GETTYPESTATS,
    S2_GETGLOBALSTATS,
#if NEWSTYLE_DEVICE
    NSCMD_DEVICEQUERY,
#endif
    0
};

static BOOL eth_device_query(struct IOSana2Req *req, struct eth_unit *unit) {
    CopyMem(&(unit->info), req->ios2_StatData, sizeof(struct Sana2DeviceQuery));

    return TRUE;
}

static BOOL eth_get_station_address(struct IOSana2Req *req, struct eth_unit *unit) {
    CopyMem(unit->hwaddr, &req->ios2_SrcAddr, ETH_ALEN);
    CopyMem(unit->hwaddr, &req->ios2_DstAddr, ETH_ALEN);

    return TRUE;
}

static BOOL eth_config_interface(struct IOSana2Req *req, struct eth_unit *unit) {
    if (unit->flags & eu_CONFIGURED) {
        D(bug("[eth] [%d] already configured\n", unit->num));
        
        req->ios2_Req.io_Error = S2ERR_BAD_STATE;
        req->ios2_WireError = S2WERR_IS_CONFIGURED;

        return TRUE;
    }

    CopyMem(unit->hwaddr, &req->ios2_SrcAddr, ETH_ALEN);
    CopyMem(unit->hwaddr, &req->ios2_DstAddr, ETH_ALEN);

    unit->flags |= eu_CONFIGURED;

    return TRUE;
}

static BOOL eth_online(struct IOSana2Req *req, struct eth_unit *unit) {
    if (unit->flags & eu_ONLINE) {
        D(bug("[eth] [%d] already online\n", unit->num));

        req->ios2_Req.io_Error = S2ERR_BAD_STATE;
        req->ios2_WireError = S2WERR_UNIT_ONLINE;

        return TRUE;
    }

    if (!(unit->flags & eu_CONFIGURED)) {
        D(bug("[eth] [%d] not configured\n", unit->num));

        req->ios2_Req.io_Error = S2ERR_BAD_STATE;
        req->ios2_WireError = S2WERR_UNIT_ONLINE;

        return TRUE;
    }

    unit->flags |= eu_ONLINE;

    D(bug("[eth] [%d] online\n", unit->num));

    return TRUE;
}

static BOOL eth_offline(struct IOSana2Req *req, struct eth_unit *unit) {
    if (!(unit->flags & eu_ONLINE)) {
        D(bug("[eth] [%d] already offline\n", unit->num));

        req->ios2_Req.io_Error = S2ERR_BAD_STATE;
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;

        return TRUE;
    }

    unit->flags &= ~eu_ONLINE;

    D(bug("[eth] [%d] offline\n", unit->num));

    return TRUE;
}

static BOOL eth_read(struct IOSana2Req *req, struct eth_unit *unit) {
    struct eth_opener *opener;

    if (!(unit->flags & eu_ONLINE)) {
        D(bug("[eth] [%d] not online\n", unit->num));

        req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;

        return TRUE;
    }

    opener = req->ios2_BufferManagement;

    req->ios2_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(&(opener->read_pending), (struct Message *) req);

    D(bug("[eth] [%d] queued read request\n", unit->num));

    return FALSE;
}

static BOOL eth_write(struct IOSana2Req *req, struct eth_unit *unit) {
    if (!(unit->flags & eu_ONLINE)) {
        DWRITE(bug("[eth] [%d] not online\n", unit->num));

        req->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
        req->ios2_WireError = S2WERR_UNIT_OFFLINE;

        return TRUE;
    }

    req->ios2_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(unit->write_queue, (struct Message *) req);

    DWRITE(bug("[eth] [%d] queued write request\n", unit->num));

    return FALSE;
}

static BOOL eth_track_type(struct IOSana2Req *req, struct eth_unit *unit) {
    struct eth_tracker *tracker, *tracker_next;
    WORD packet_type = req->ios2_PacketType;

    D(bug("[eth] [%d] track of packet type 0x%04x requested\n", unit->num, packet_type));

    ForeachNodeSafe(&(unit->trackers), tracker, tracker_next) {
        if (tracker->packet_type == packet_type) {
            D(bug("[eth] [%d] already tracking, incrementing the refcount\n", unit->num));
            tracker->refcount ++;
            return TRUE;
        }
    }

    tracker = AllocVec(sizeof(struct eth_tracker), MEMF_PUBLIC | MEMF_CLEAR);
    tracker->refcount = 1;
    tracker->packet_type = packet_type;

    Disable();
    AddTail((APTR) &(unit->trackers), (APTR) tracker);
    Enable();

    D(bug("[eth] [%d] now tracking\n", unit->num));

    return TRUE;
}

static BOOL eth_untrack_type(struct IOSana2Req *req, struct eth_unit *unit) {
    struct eth_tracker *tracker, *tracker_next;
    WORD packet_type = req->ios2_PacketType;

    D(bug("[eth] [%d] untrack of packet type 0x%04x requested\n", unit->num, packet_type));

    ForeachNodeSafe(&(unit->trackers), tracker, tracker_next) {
        if (tracker->packet_type == packet_type) {
            D(bug("[eth] [%d] found it, decrementing the refcount\n", unit->num));
            tracker->refcount --;

            if (tracker->refcount == 0) {
                D(bug("[eth] [%d] refcount is 0, not tracking any longer\n", unit->num));

                Disable();
                Remove((APTR) tracker);
                Enable();

                FreeVec(tracker);
            }

            return TRUE;
        }
    }

    D(bug("[eth] [%d] no tracking, nothing to do\n", unit->num));

    return TRUE;
}

static BOOL eth_get_type_stats(struct IOSana2Req *req, struct eth_unit *unit) {
    struct eth_tracker *tracker, *tracker_next;
    WORD packet_type = req->ios2_PacketType;

    D(bug("[eth] [%d] get stats for packet type 0x%04x requested\n", unit->num, packet_type));

    ForeachNodeSafe(&(unit->trackers), tracker, tracker_next) {
        if (tracker->packet_type == packet_type) {
            D(bug("[eth] [%d] found it, copying to caller\n", unit->num));

            CopyMem(&(tracker->stats), req->ios2_StatData, sizeof(struct Sana2PacketTypeStats));

            return TRUE;
        }
    }

    D(bug("[eth] [%d] not tracking this type\n", unit->num));

    req->ios2_Req.io_Error = S2ERR_BAD_STATE;
    req->ios2_WireError = S2WERR_NOT_TRACKED;

    return TRUE;
}

static BOOL eth_get_global_stats(struct IOSana2Req *req, struct eth_unit *unit) {
    D(bug("[eth] [%d] returning global stats\n", unit->num));

    CopyMem(&(unit->stats), req->ios2_StatData, sizeof(struct Sana2DeviceStats));

    return TRUE;
}

static BOOL eth_broadcast(struct IOSana2Req *req, struct eth_unit *unit) {
    /* just fill in the broadcast address as the dest, and write as normal */
    char *addr = req->ios2_DstAddr;
    *((ULONG *) addr) = 0xffffffff;
    *((UWORD *) (addr + 4)) = 0xffff;

    return eth_write(req, unit);
}

void eth_handle_request(struct IOSana2Req *req) {
    struct eth_unit *unit = (struct eth_unit *) req->ios2_Req.io_Unit;
    BOOL completed = FALSE;

    switch (req->ios2_Req.io_Command) {
#if NEWSTYLE_DEVICE
        case NSCMD_DEVICEQUERY:
        {
            struct NSDeviceQueryResult *d;
            LONG error = 0;
            struct IOStdReq *iotd;
            iotd = (struct IOStdReq *)req;
            D(bug("[eth] [%d] NSCMD_DEVICEQUERY\n", unit->num));
            if(iotd->io_Length >= sizeof(struct NSDeviceQueryResult))
            {
                if((d = (struct NSDeviceQueryResult *)iotd->io_Data))
                {
                    if ((d->DevQueryFormat == 0) && (d->SizeAvailable == 0))
                    {
                        d->SizeAvailable        = sizeof(struct NSDeviceQueryResult);
                        d->DeviceType           = NSDEVTYPE_SANA2;
                        d->DeviceSubType        = 0;
                        d->SupportedCommands    = (UWORD *)eth_supported_commands;
                        iotd->io_Actual = sizeof(struct NSDeviceQueryResult);
                    } else error = IOERR_BADLENGTH;
                } else error = IOERR_BADADDRESS;
            } else error = IOERR_BADLENGTH;
            iotd->io_Error = error;
            completed = error ? FALSE : TRUE;
            break;
        }
#endif
        case S2_DEVICEQUERY:
            D(bug("[eth] [%d] S2_DEVICEQUERY\n", unit->num));
            completed = eth_device_query(req, unit);
            break;

        case S2_GETSTATIONADDRESS:
            D(bug("[eth] [%d] S2_GETSTATIONADDRESS\n", unit->num));
            completed = eth_get_station_address(req, unit);
            break;

        case S2_CONFIGINTERFACE:
            D(bug("[eth] [%d] S2_CONFIGINTERFACE\n", unit->num));
            completed = eth_config_interface(req, unit);
            break;

        case S2_ONLINE:
            D(bug("[eth] [%d] S2_ONLINE\n", unit->num));
            completed = eth_online(req, unit);
            break;

        case S2_OFFLINE:
            D(bug("[eth] [%d] S2_OFFLINE\n", unit->num));
            completed = eth_offline(req, unit);
            break;

        case CMD_READ:
            D(bug("[eth] [%d] CMD_READ\n", unit->num));
            completed = eth_read(req, unit);
            break;

        case CMD_WRITE:
            D(bug("[eth] [%d] CMD_WRITE\n", unit->num));
            completed = eth_write(req, unit);
            break;

        case CMD_FLUSH:
            D(bug("[eth] [%d] CMD_FLUSH\n", unit->num));
            break;

        case S2_BROADCAST:
            D(bug("[eth] [%d] S2_BROADCAST\n", unit->num));
            completed = eth_broadcast(req, unit);
            break;

        case S2_TRACKTYPE:
            D(bug("[eth] [%d] S2_TRACKTYPE\n", unit->num));
            completed = eth_track_type(req, unit);
            break;

        case S2_UNTRACKTYPE:
            D(bug("[eth] [%d] S2_UNTRACKTYPE\n", unit->num));
            completed = eth_untrack_type(req, unit);
            break;

        case S2_GETTYPESTATS:
            D(bug("[eth] [%d] S2_GETTYPESTATS\n", unit->num));
            completed = eth_get_type_stats(req, unit);
            break;

        case S2_GETGLOBALSTATS:
            D(bug("[eth] [%d] S2_GETGLOBALSTATS\n", unit->num));
            completed = eth_get_global_stats(req, unit);
            break;

        default:
            D(bug("[eth] [%d] unknown command (%d)\n", unit->num, req->ios2_Req.io_Command));
            req->ios2_Req.io_Error = S2ERR_NOT_SUPPORTED;
            completed = TRUE;
            break;
    }

    if (completed && ! (req->ios2_Req.io_Flags & IOF_QUICK))
        ReplyMsg((APTR) req);
}
