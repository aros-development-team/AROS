/*
 * tap - TUN/TAP network driver for AROS
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2010-2019 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <hidd/unixio.h>
#include <proto/alib.h>
#include <proto/oop.h>

#include "tap.h"

static int GM_UNIQUENAME(init)(LIBBASETYPEPTR LIBBASE)
{
    int i;

    LIBBASE->UnixIOAttrBase = OOP_ObtainAttrBase(IID_Hidd_UnixIO);
    if (!LIBBASE->UnixIOAttrBase)
	return FALSE;

    D(bug("[tap] in init\n"));

    LIBBASE->unixio = OOP_NewObjectTags(NULL, CLID_Hidd_UnixIO,
					aHidd_UnixIO_Opener, MOD_NAME_STRING,
					aHidd_UnixIO_Architecture, AROS_ARCHITECTURE,
					TAG_DONE);
    if (LIBBASE->unixio == NULL)
    {
        kprintf("[tap] couldn't create unixio object\n");
        return FALSE;
    }

    for (i = 0; i < MAX_TAP_UNITS; i ++)
        LIBBASE->unit[i].num = i;

    return TRUE;
}

static int GM_UNIQUENAME(expunge)(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[tap] in expunge\n"));

    /* XXX kill the tasks and free memory, just in case */

    /* We don't need to dispose a unixio object, it's a singletone. */

    if (LIBBASE->UnixIOAttrBase)
	OOP_ReleaseAttrBase(IID_Hidd_UnixIO);

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

extern void tap_iotask(struct tap_base *TAPBase, struct tap_unit *unit);

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
        kprintf("[tap] request for unit %d, which is out of range (0..%d)\n", unitnum, MAX_TAP_UNITS-1);
        error = IOERR_OPENFAIL;
    }
    unit = &(LIBBASE->unit[unitnum]);

    /* allocate storage for opener state */
    if (error == 0) {
        opener = AllocVec(sizeof(struct tap_opener), MEMF_PUBLIC | MEMF_CLEAR);
        req->ios2_BufferManagement = (APTR) opener;

        if (opener == NULL) {
            kprintf("[tap] [%d] couldn't allocate opener struct\n", unit->num);
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
        fd = Hidd_UnixIO_OpenFile(LIBBASE->unixio, TAP_DEV_NODE, O_RDWR, 0, &ioerr);
        if (fd < 0) {
            kprintf("[tap] couldn't open '" TAP_DEV_NODE "' (%d)\n", ioerr);
            error = IOERR_OPENFAIL;
        }

        /* and create the virtual network */
        if (error == 0) {
            __sprintf(unit->name, TAP_IFACE_FORMAT, unit->num);

            memset(&ifr, 0, sizeof(struct ifreq));
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
            strncpy(ifr.ifr_name, unit->name, IFNAMSIZ - 1);

            if ((Hidd_UnixIO_IOControlFile(LIBBASE->unixio, fd, TUNSETIFF, &ifr, &ioerr)) < 0)
	    {
                kprintf("[tap] couldn't perform TUNSETIFF on TAP device (%d)\n", ioerr);
                error = IOERR_OPENFAIL;
            }

            else {
                unit->fd = fd;

                kprintf("[tap] unit %d attached to %s\n", unit->num, unit->name);
            }
        }

        /* its good, time to create our unit */
        if (error == 0)
	{
	    char iotask_name[32];

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

            kprintf("[tap] [%d] hardware address: %02x:%02x:%02x:%02x:%02x:%02x\n", unit->num,
                unit->hwaddr[0], unit->hwaddr[1], unit->hwaddr[2],
                unit->hwaddr[3], unit->hwaddr[4], unit->hwaddr[5]);

            /* container for the openers */
            NEWLIST(&(unit->openers));

            /* and for the trackers */
            NEWLIST(&(unit->trackers));

            /* a port to sync actions with the iotask */
            unit->iosyncport = CreateMsgPort();

            /* make a unique name for this unit */
            __sprintf(iotask_name, TAP_TASK_FORMAT, unit->num);

            /* make it fly */
            unit->iotask = NewCreateTask(TASKTAG_PC  , tap_iotask,
                                         TASKTAG_NAME, iotask_name,
                                         TASKTAG_PRI , 50,
                                         TASKTAG_ARG1, LIBBASE,
					 TASKTAG_ARG2, unit,
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

            /* close the nic */
            if (unit->fd > 0)
                Hidd_UnixIO_CloseFile(LIBBASE->unixio, unit->fd, NULL);

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

        /* close the nic */
        if (unit->fd > 0)
            Hidd_UnixIO_CloseFile(LIBBASE->unixio, unit->fd, NULL);

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
ADD2LIBS("unixio.hidd", 0, static struct Library *, unixioBase);

AROS_LH1(void, begin_io, AROS_LHA(struct IOSana2Req *, req, A1), LIBBASETYPEPTR, LIBBASE, 5, tap_device) {
    AROS_LIBFUNC_INIT

    req->ios2_Req.io_Error = 0;

    tap_handle_request(req);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(long, abort_io, AROS_LHA(struct IOSana2Req *, req, A1), LIBBASETYPEPTR, LIBBASE, 6, tap_device) {
    AROS_LIBFUNC_INIT

    /* XXX anything to do here? */

    return 1;

    AROS_LIBFUNC_EXIT
}




/* hexdumpery stoled from Dan Gudmundsson, stoled from Gordon Beaton, via Google Code Search */

#ifdef DEBUG

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

#endif
