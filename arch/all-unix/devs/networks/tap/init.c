/*
 * tap - TUN/TAP network driver for AROS
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include "tap.h"

HIDD    *unixio = NULL;

static int GM_UNIQUENAME(init)(LIBBASETYPEPTR LIBBASE) {
    int i;

    D(bug("[tap] in init\n"));

    unixio = (HIDD *) OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (unixio == NULL) {
        D(bug("[tap] couldn't create read unixio object\n"));
        return FALSE;
    }

    for (i = 0; i < MAX_TAP_UNITS; i ++)
        LIBBASE->unit[i].num = i;

    return TRUE;
}

static int GM_UNIQUENAME(expunge)(LIBBASETYPEPTR LIBBASE) {
    D(bug("[tap] in expunge\n"));

    /* XXX kill the tasks and free memory, just in case */

    if (unixio != NULL) {
        OOP_DisposeObject((OOP_Object *) unixio);
        unixio = NULL;
    }

    return TRUE;
}

static const ULONG rx_tags[] = { 
    S2_CopyToBuff,
    S2_CopyToBuff16
};
    
static const ULONG tx_tags[] = {
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

extern void tap_iotask(void);

static int GM_UNIQUENAME(open)(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *req, ULONG unitnum, ULONG flags) {
    struct TagItem *tags;
    BYTE error = 0;
    struct tap_unit *unit;
    struct tap_opener *opener = NULL;
    int fd, ioerr;
    struct ifreq ifr;
    int i;

    D(bug("[tap] in open\n"));

    D(bug("[tap] unit %ld, flags [0x%08x]%s%s\n", unitnum, flags, 
            flags & SANA2OPF_PROM ? " SANA2OPF_PROM" : "",
            flags & SANA2OPF_MINE ? " SANA2OPF_MINE" : ""));
    
    req->ios2_Req.io_Unit = NULL;

    /* remember the callers buffer management functions */
    tags = req->ios2_BufferManagement;
    req->ios2_BufferManagement = NULL;

    /* make sure the requested unit number is in range */
    if (error == 0 && (unitnum < 0 || unitnum >= MAX_TAP_UNITS)) {
        D(bug("[tap] request for unit %d, which is out of range\n", unitnum));
        error = IOERR_OPENFAIL;
    }
    unit = &(LIBBASE->unit[unitnum]);

    /* allocate storage for opener state */
    if (error == 0) {
        opener = AllocVec(sizeof(struct tap_opener), MEMF_PUBLIC | MEMF_CLEAR);
        req->ios2_BufferManagement = (APTR) opener;

        if (opener == NULL) {
            D(bug("[tap] [%d] couldn't allocate opener struct\n", unit->num));
            error = IOERR_OPENFAIL;
        }
    }

    /* prepare the opener port and buffer management functions */
    if (error == 0) {
        int i;

        /* pending read requests get queued up in here */
        NEWLIST(&(opener->read_pending.mp_MsgList));
        opener->read_pending.mp_Flags = PA_IGNORE;

        /* take the best rx/tx function from the ones offered by the caller */
        for (i = 0; i < 2; i ++)
            opener->rx = (APTR) GetTagData(rx_tags[i], (IPTR) opener->rx, tags);
        for (i = 0; i < 3; i ++)
            opener->tx = (APTR) GetTagData(tx_tags[i], (IPTR) opener->tx, tags);

        /* the filter, if they have one */
        opener->filter = (APTR) GetTagData(S2_PacketFilter, 0, tags);

        D(bug("[tap] [%d] rx at 0x%08x, tx at 0x%08x, filter at 0x%08x\n", unit->num, opener->rx, opener->tx, opener->filter));
    }

    /* if the unit hasn't been setup previously, do that now */
    if (error == 0 && unit->refcount == 0) {
        D(bug("[tap] [%d] refcount is 0, opening device\n", unit->num));

        /* open the tun/tap device */
        fd = Hidd_UnixIO_OpenFile(unixio, TAP_DEV_NODE, O_RDWR, 0, &ioerr);
        if (fd < 0) {
            D(bug("[tap] couldn't open '" TAP_DEV_NODE "' (%d)\n", ioerr));
            error = IOERR_OPENFAIL;
        }

        /* and create the virtual network */
        if (error == 0) {
            snprintf(unit->name, IFNAMSIZ, TAP_IFACE_FORMAT, unit->num);

            memset(&ifr, 0, sizeof(struct ifreq));
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
            strncpy(ifr.ifr_name, unit->name, IFNAMSIZ);

            if ((Hidd_UnixIO_IOControlFile(unixio, fd, TUNSETIFF, &ifr, &ioerr)) < 0) {
                D(bug("[tap] couldn't perform TUNSETIFF on TAP device (%d)\n", ioerr));
                error = IOERR_OPENFAIL;
            }

            else {
                unit->fd = fd;

                D(bug("[tap] [%d] opened on fd %d, interface %s\n", unit->num, unit->fd, unit->name));
            }
        }

        /* its good, time to create our unit */
        if (error == 0) {

            /* we're faking a 10Mbit ethernet card here */
            unit->info.SizeAvailable = unit->info.SizeSupplied = sizeof(struct Sana2DeviceQuery);
            unit->info.DevQueryFormat = 0;
            unit->info.DeviceLevel = 0;
            unit->info.AddrFieldSize = 48;  
            unit->info.MTU = 1500;
            unit->info.BPS = 10000000;
            unit->info.HardwareType = S2WireType_Ethernet;

            /* create a random hardware address */
            for (i = 0; i < ETH_ALEN; i ++)
                unit->hwaddr[i] = (unsigned char) (rand() % 0xff);

            unit->hwaddr[0] &= 0xfe;    /* clear multicast bit */
            unit->hwaddr[0] |= 0x02;    /* set local assignment bit (IEEE802) */

            D(bug("[tap] [%d] hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n", unit->num,
                unit->hwaddr[0], unit->hwaddr[1], unit->hwaddr[2],
                unit->hwaddr[3], unit->hwaddr[4], unit->hwaddr[5]));

            /* container for the openers */
            NEWLIST(&(unit->openers));

            /* and for the trackers */
            NEWLIST(&(unit->trackers));

            /* prepare write queue */
            NEWLIST(&(unit->write_queue.mp_MsgList));
            unit->write_queue.mp_Node.ln_Type = NT_MSGPORT;
            unit->write_queue.mp_Flags = PA_IGNORE;

            /* a port to sync actions with the iotask */
            unit->iosyncport = CreateMsgPort();

            /* make a unique name for this unit */
            snprintf(unit->iotask_name, sizeof(unit->iotask_name), TAP_TASK_FORMAT, (int) unit->num);

            /* make it fly */
            unit->iotask = CreateNewProcTags(NP_Entry,      (IPTR) tap_iotask,
                                             NP_Name,       unit->iotask_name,
                                             NP_Priority,   50,
                                             NP_UserData,   (IPTR) unit,
                                             TAG_DONE);
            
            /* wait until its ready to go */
            WaitPort(unit->iosyncport);
            ReplyMsg(GetMsg(unit->iosyncport));

            D(bug("[tap] [%d] unit created and running\n", unit->num));
        }
    }

    /* at this point the unit is online, and the opener state is initialised.
     * all that remains is to hook the two together */
    if (error == 0) {
        req->ios2_Req.io_Unit = (APTR) unit;

        Disable();
        AddTail((APTR) &(unit->openers), (APTR) opener);
        Enable();

        unit->refcount ++;

        D(bug("[tap] [%d] refcount is now %d\n", unit->num, unit->refcount));
    }

    /* otoh, if it blew up, we've got some cleaning to do */
    else {

        /* shutdown the unit if there's noone using it */
        if (unit->refcount == 0) {
            D(bug("[tap] [%d] open failed, and there's no other users of this unit, killing it\n", unit->num));

            /* kill the io task */
            if (unit->iotask != NULL) {
                Signal((struct Task *) unit->iotask, 1 << unit->abort_signal);

                /* wait for it to die */
                WaitPort(unit->iosyncport);
                ReplyMsg(GetMsg(unit->iosyncport));

                /* done with this */
                DeleteMsgPort(unit->iosyncport);
            }

            /* fastest way to kill it */
            memset(unit, 0, sizeof(struct tap_unit));
            unit->num = unitnum;
        }

        /* free the opener structure too */
        FreeVec(opener);
    }

    req->ios2_Req.io_Error = error;

    D(bug("[tap] open returning %d\n", error));

    return (error == 0) ? TRUE : FALSE;
}

static int GM_UNIQUENAME(close)(LIBBASETYPEPTR LIBBASE, struct IOSana2Req *req) {
    struct tap_unit *unit = (struct tap_unit *) req->ios2_Req.io_Unit;
    struct tap_opener *opener = (struct tap_opener *) req->ios2_BufferManagement;
    struct tap_tracker *tracker, *tracker_next;
    ULONG unitnum = unit->num;

    D(bug("[tap] in close\n"));

    unit->refcount --;

    D(bug("[tap] [%d] refcount is now %d\n", unit->num, unit->refcount));

    if (unit->refcount == 0) {
        D(bug("[tap] [%d] last user closed unit, stopping iotask\n", unit->num));

        /* kill the io task */
        Signal((struct Task *) unit->iotask, 1 << unit->abort_signal);

        /* wait for it to die */
        WaitPort(unit->iosyncport);
        ReplyMsg(GetMsg(unit->iosyncport));

        /* done with this */
        DeleteMsgPort(unit->iosyncport);

        /* XXX return outstanding requests? */

        /* kill trackers */
        ForeachNodeSafe(&(unit->trackers), tracker, tracker_next)
            FreeVec(tracker);

        /* fastest way to kill it */
        memset(unit, 0, sizeof(struct tap_unit));
        unit->num = unitnum;
    }

    /* cleanup the opener structure too */
    Disable();
    Remove((APTR) opener);
    Enable();
    FreeVec(opener);

    req->ios2_Req.io_Unit = NULL;
    req->ios2_BufferManagement = NULL;

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init),0)
ADD2EXPUNGELIB(GM_UNIQUENAME(expunge),0)
ADD2OPENDEV(GM_UNIQUENAME(open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(close),0)

AROS_LH1(void, begin_io, AROS_LHA(struct IOSana2Req *, req, A1), LIBBASETYPEPTR, LIBBASE, 5, tap_device) {
    AROS_LIBFUNC_INIT

    req->ios2_Req.io_Error = 0;

    tap_handle_request(req);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, abort_io, AROS_LHA(struct IOSana2Req *, req, A1), LIBBASETYPEPTR, LIBBASE, 5, tap_device) {
    AROS_LIBFUNC_INIT

    /* XXX what is this */

    return 1;

    AROS_LIBFUNC_EXIT
}




/* hexdumpery stoled from Dan Gudmundsson, stoled from Gordon Beaton, via Google Code Search */

#define CHUNK 16

void tap_hexdump(unsigned char *buf, int bufsz)
{
  int i,j;
  int count;

  /* do this in chunks of CHUNK bytes */
  for (i=0; i<bufsz; i+=CHUNK) {
    /* show the offset */
    bug("0x%06x  ", i);

    /* max of CHUNK or remaining bytes */
    count = ((bufsz-i) > CHUNK ? CHUNK : bufsz-i);

    /* show the bytes */
    for (j=0; j<count; j++) {
      if (j==CHUNK/2) bug(" ");
      bug("%02x ",buf[i+j]);
    }

    /* pad with spaces if less than CHUNK */
    for (j=count; j<CHUNK; j++) {
      if (j==CHUNK/2) bug(" ");
      bug("   ");
    }

    /* divider between hex and ascii */
    bug(" ");

    /*
    for (j=0; j<count; j++)
      bug("%c",(isprint(buf[i+j]) ? buf[i+j] : '.'));
    */

    bug("\n\r");
  }
}
