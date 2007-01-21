/*
 * tap - TUN/TAP network driver for AROS
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "tap.h"

/* this fires whenever data is waiting to be read on the tap descriptor */
static void tap_receive(struct tap_unit *unit) {
    unsigned char buf[ETH_FRAME_LEN], *packet;
    int nread, ioerr;
    struct ethhdr *eth;
    WORD packet_type;
    struct tap_opener *opener, *opener_next;
    struct IOSana2Req *req, *req_next;
    BOOL bcast = FALSE, mcast = FALSE;
    BOOL accepted = 0;
    struct tap_tracker *tracker, *tracker_next;

    D(bug("[tap] [io:%d] got a packet\n", unit->num));

    /* read the packet */
    nread = Hidd_UnixIO_ReadFile(unixio, unit->fd, buf, ETH_FRAME_LEN, &ioerr);
    if (ioerr != 0) {
        D(bug("[tap] [io:%d] read failed (%d)\n", unit->num, ioerr));
        return;
    }

    if (!(unit->flags & tu_ONLINE)) {
        D(bug("[tap] [io:%d] we're offline, dropping it\n", unit->num));
        return;
    }

    D(bug("[tap] [io:%d] packet dump (%d bytes):\n", unit->num, nread));
    D(tap_hexdump(buf, nread));

    eth = (struct ethhdr *) buf;
    packet_type = AROS_BE2WORD(eth->h_proto);   /* AROS_BE2WORD === ntohs */

    D(bug("[tap] [io:%d] source address: %02x:%02x:%02x:%02x:%02x:%02x\n", unit->num,
          eth->h_source[0], eth->h_source[1], eth->h_source[2],
          eth->h_source[3], eth->h_source[4], eth->h_source[5]));
    D(bug("[tap] [io:%d] dest address: %02x:%02x:%02x:%02x:%02x:%02x\n", unit->num,
          eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
          eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]));
    D(bug("[tap] [io:%d] packet type: 0x%04x\n", unit->num, packet_type));

    /* broadcast packets have the top 40 bits (5 bytes) set */
    if ((*((ULONG *) (eth->h_dest)) == 0xffffffff) &&
        (*((UWORD *) (eth->h_dest + 4)) == 0xffff)) {
        D(bug("[tap] [io:%d] broadcast packet\n"));
        bcast = TRUE;
    }

    /* multicasts have the top bit set */
    else if ((eth->h_dest[0] & 0x1) != 0) {
        D(bug("[tap] [io:%d] multicast packet\n"));
        mcast = TRUE;
    }

    /* drop multicast packets (until we have support for them) */
    if (mcast) {
        D(bug("[tap] [io:%d] no support for multicast packets, dropping it\n", unit->num));
        return;
    }

    /* now we loop through our openers, seeing if anyone wants the packet */
    ForeachNodeSafe(&(unit->openers), opener, opener_next) {

        /* loop pending read requests */
        ForeachNodeSafe(&(opener->read_pending.mp_MsgList), req, req_next) {

            if (req->ios2_PacketType == packet_type) {
                D(bug("[tap] [io:%d] found a request that wants this packet, sending it\n", unit->num));

                /* record broadcast/multicast status */
                req->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
                if (bcast)
                    req->ios2_Req.io_Flags |= SANA2IOF_BCAST;
                if (mcast)
                    req->ios2_Req.io_Flags |= SANA2IOF_MCAST;

                /* copy source, dest, type */
                CopyMem(eth->h_source, req->ios2_SrcAddr, ETH_ALEN);
                CopyMem(eth->h_dest, req->ios2_DstAddr, ETH_ALEN);
                req->ios2_PacketType = packet_type;

                /* if they want the raw header, then use as-is */
                if (req->ios2_Req.io_Flags & SANA2IOF_RAW) {
                    req->ios2_DataLength = nread;
                    packet = buf;
                }

                /* otherwise rip the header off */
                else {
                    req->ios2_DataLength = nread - ETH_HLEN;
                    packet = &(buf[ETH_HLEN]);
                }

                /* user filter. the packet gets blocked if
                 *  - the request is CMD_READ, and
                 *  - they provided a filter, and
                 *  - the filter returns false
                 */
                if (req->ios2_Req.io_Command == CMD_READ &&
                    opener->filter != NULL &&
                    ! CallHookPkt(opener->filter, req, packet)) {
                    D(bug("[tap] [io:%d] packet blocked by opener filter\n", unit->num));

                    continue;
                }

                /* hand it to the opener via the provided rx function */
                if (! opener->rx(req->ios2_Data, packet, req->ios2_DataLength)) {
                    D(bug("[tap] [io:%d] opener signaled error during rx copy\n", unit->num));

                    req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
                    req->ios2_WireError = S2WERR_BUFF_ERROR;
                }

                else {
                    D(bug("[tap] [io:%d] packet copied successfully\n", unit->num));
                }

                Disable();
                Remove((APTR) req);
                Enable();
                ReplyMsg((APTR) req);

                accepted = TRUE;
            }

            if (accepted)
                break;
        }

        if (accepted)
            break;
    }

    if (! accepted) {
        D(bug("[tap] [io:%d] no one claimed the packet, orphaning\n", unit->num));

        /* XXX handle orphans */
    }

    /* update tracked stats */
    ForeachNodeSafe(&(unit->trackers), tracker, tracker_next) {
        if (tracker->packet_type == packet_type) {
            tracker->stats.PacketsReceived ++;
            tracker->stats.BytesReceived += nread;
            if (! accepted)
                tracker->stats.PacketsDropped ++;
        }
    }

    /* update global stats too */
    unit->stats.PacketsReceived ++;
    if (! accepted)
        unit->stats.UnknownTypesReceived ++;
}

/* this fires whenever we can write to the tap descriptor */
static void tap_send(struct tap_unit *unit) {
    struct IOSana2Req *req;
    int packet_length;
    struct tap_opener *opener;
    unsigned char buf[ETH_FRAME_LEN], *packet;
    struct ethhdr *eth;
    int ioerr, nwritten = 0;
    struct tap_tracker *tracker, *tracker_next;

    /* grab the first pending request */
    req = (struct IOSana2Req *) unit->write_queue.mp_MsgList.lh_Head;

    eth = (struct ethhdr *) buf;

    D(bug("[tap] [io:%d] buffer has %d bytes%s\n", unit->num, req->ios2_DataLength, req->ios2_Req.io_Flags & SANA2IOF_RAW ? " (RAW is set)" : ""));

    /* build a header if they didn't send us one */
    if (! (req->ios2_Req.io_Flags & SANA2IOF_RAW)) {
        D(bug("[tap] [io:%d] adding ethernet header\n", unit->num));

        CopyMem(unit->hwaddr, eth->h_source, ETH_ALEN);
        CopyMem(req->ios2_DstAddr, eth->h_dest, ETH_ALEN);
        eth->h_proto = AROS_WORD2BE(req->ios2_PacketType);  /* AROS_WORD2BE === htons */

        packet = &(buf[ETH_HLEN]);
        packet_length = ETH_HLEN + req->ios2_DataLength;
    }

    /* otherwise just use what they give us */
    else {
        packet = buf;
        packet_length = req->ios2_DataLength;
    }

    /* user magic functions */
    opener = (struct tap_opener *) req->ios2_BufferManagement;

    /* get the opener to magick into a buffer we can use */
    if (! opener->tx(packet, req->ios2_Data, req->ios2_DataLength)) {
        D(bug("[tap] [io:%d] opener signaled error during tx copy\n", unit->num));

        req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        req->ios2_WireError = S2WERR_BUFF_ERROR;
    }

    else {
        D(bug("[tap] [io:%d] packet dump (%d bytes):\n", unit->num, packet_length));
        D(tap_hexdump(buf, packet_length));

        D(bug("[tap] [io:%d] source address: %02x:%02x:%02x:%02x:%02x:%02x\n", unit->num,
              eth->h_source[0], eth->h_source[1], eth->h_source[2],
              eth->h_source[3], eth->h_source[4], eth->h_source[5]));
        D(bug("[tap] [io:%d] dest address: %02x:%02x:%02x:%02x:%02x:%02x\n", unit->num,
              eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
              eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]));
        D(bug("[tap] [io:%d] packet type: 0x%04x\n", unit->num, AROS_BE2WORD(eth->h_proto)));

        /* got a viable buffer, send it out */
        nwritten = Hidd_UnixIO_WriteFile(unixio, unit->fd, buf, packet_length, &ioerr);
        if (nwritten < 0) {
            D(bug("[tap] [io:%d] write failed (%d)\n", unit->num, ioerr));
            req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        }
        else if (nwritten < packet_length) {
            D(bug("[tap] [io:%d] short write, only %d bytes written, reporting error\n", unit->num, nwritten));
            req->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
        }
        else
            D(bug("[tap] [io:%d] wrote %d bytes\n", unit->num, nwritten));
    }

    /* update stats */
    if (req->ios2_Req.io_Error == 0) {

        /* tracked */
        ForeachNodeSafe(&(unit->trackers), tracker, tracker_next) {
            if (tracker->packet_type == req->ios2_PacketType) {
                tracker->stats.PacketsSent ++;
                tracker->stats.BytesSent += nwritten;
            }
        }

        /* global */
        unit->stats.PacketsSent ++;
    }

    /* remove and reply to the request */
    Disable();
    Remove((APTR) req);
    Enable();
    ReplyMsg((APTR) req);
};

AROS_UFH3(void, tap_iotask,
          AROS_UFHA(STRPTR,            argPtr,  A0),
          AROS_UFHA(ULONG,             argSize, D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    struct tap_unit *unit = FindTask(NULL)->tc_UserData;
    struct MsgPort *ioport, *reply_port;
    struct Message *msg;
    ULONG write_signal_mask, abort_signal_mask, signal_mask, signaled;
    struct uioMessage *iomsg;
    ULONG mode;
    BOOL write_enabled = FALSE;

    D(bug("[tap] [io:%d] iotask starting up\n", unit->num));

    /* create the port for unixio to talk to us through */
    ioport = CreateMsgPort();

    /* make some signals, one for them to ask for writes, and one for them to
     * shut us down */
    unit->write_signal = AllocSignal(-1);
    unit->abort_signal = AllocSignal(-1);
    write_signal_mask = 1 << unit->write_signal;
    abort_signal_mask = 1 << unit->abort_signal;

    /* bundle all the signals together */
    signal_mask = write_signal_mask | abort_signal_mask | (1 << ioport->mp_SigBit);

    /* start waiting for read events */
    /* XXX check for failure */
    Hidd_UnixIO_AsyncIO(unixio, unit->fd, vHidd_UnixIO_Terminal, ioport, vHidd_UnixIO_Read, SysBase);

    reply_port = CreateMsgPort();
    msg = (struct Message *) AllocVec(sizeof(struct Message), MEMF_PUBLIC | MEMF_CLEAR);
    msg->mn_ReplyPort = reply_port;
    msg->mn_Length = sizeof(struct Message);
    PutMsg(unit->iosyncport, msg);
    WaitPort(reply_port);
    GetMsg(reply_port);
    FreeVec(msg);
    DeleteMsgPort(reply_port);

    D(bug("[tap] [io:%d] iotask entering loop\n", unit->num));

    while (1) {
        D(bug("[tap] [0] waiting for an event\n", unit->num));

        if (IsListEmpty(&(ioport->mp_MsgList))) {
            signaled = Wait(signal_mask);

            if (signaled & write_signal_mask) {
                D(bug("[tap] [io:%d] write requested, enabling\n", unit->num));
                if (! write_enabled) {
                    Hidd_UnixIO_AsyncIO(unixio, unit->fd, vHidd_UnixIO_Terminal, ioport, vHidd_UnixIO_Write, SysBase);
                    write_enabled = TRUE;
                }

                continue;
            }
                
            if (signaled & abort_signal_mask) {
                D(bug("[tap] [io:%d] iotask received abort signal\n", unit->num));
                break;
            }
        }

        if (IsListEmpty(&(ioport->mp_MsgList)))
            continue;

        iomsg = (struct uioMessage *) GetMsg(ioport);
        mode = iomsg->mode;
        FreeMem(iomsg, sizeof(struct uioMessage));

        D(bug("[tap] [io:%d] iotask signaled\n", unit->num));

        if (mode & vHidd_UnixIO_Read) {
            D(bug("[tap] [io:%d] ready for read\n", unit->num));
            tap_receive(unit);
        }

        else if (mode & vHidd_UnixIO_Write) {
            D(bug("[tap] [io:%d] ready for write\n", unit->num));
            tap_send(unit);

            if (IsMsgPortEmpty(&(unit->write_queue))) {
                D(bug("[tap] [io:%d] all packets sent, will ignore future write events\n", unit->num));
                write_enabled = FALSE;
            }
        }

        else {
            D(bug("[tap] [io:%d] unknown ioevent mode 0x%02x\n", unit->num, mode));
        }

        D(bug("[tap] [io:%d] resetting async events\n", unit->num));

        /* reset unixio to make sure we get more events. this is horribly
         * inefficient, but it'll do for now until unixio gets a little love */
        /* XXX check for failure */
        Hidd_UnixIO_AbortAsyncIO(unixio, unit->fd, SysBase);
        Hidd_UnixIO_AsyncIO(unixio, unit->fd, vHidd_UnixIO_Terminal, ioport, vHidd_UnixIO_Read, SysBase);
        if (write_enabled)
            Hidd_UnixIO_AsyncIO(unixio, unit->fd, vHidd_UnixIO_Terminal, ioport, vHidd_UnixIO_Write, SysBase);

        D(bug("[tap] [io:%d] event processed, replying and looping\n", unit->num));
    }

    D(bug("[tap] [io:%d] iotask exiting\n", unit->num));

    Hidd_UnixIO_AbortAsyncIO(unixio, unit->fd, SysBase);

    DeleteMsgPort(ioport);

    FreeSignal(unit->abort_signal);
    FreeSignal(unit->write_signal);

    msg = (struct Message *) AllocVec(sizeof(struct Message), MEMF_PUBLIC | MEMF_CLEAR);
    PutMsg(unit->iosyncport, msg);
    FreeVec(msg);

    AROS_USERFUNC_EXIT
}
