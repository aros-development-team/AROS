/*
 * tap - TUN/TAP network driver for AROS
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright © 2010, The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <exec/memory.h>

#include "tap.h"

#include <proto/alib.h>
#include <proto/hostlib.h>

#include <fcntl.h>

static const char *libc_symbols[] = {
    "open",
    "close",
    "ioctl",
    "fcntl",
    "poll",
    "read",
    "write",
    "getpid",
#ifdef HOST_OS_linux
    "__errno_location",
#else
    "__error",
#endif
    NULL
};

static int GM_UNIQUENAME(init)(LIBBASETYPEPTR TAPBase)
{
    ULONG i;

    D(bug("[tap] in init\n"));

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
	return FALSE;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
	return FALSE;

    TAPBase->LibCHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!TAPBase->LibCHandle)
	return FALSE;

    TAPBase->TAPIFace = (struct LibCInterface *)HostLib_GetInterface(TAPBase->LibCHandle, libc_symbols, &i);
    if ((!TAPBase->TAPIFace) || i)
	return FALSE;

    InitSemaphore(&TAPBase->sem);
    TAPBase->aros_pid = TAPBase->TAPIFace->getpid();
    TAPBase->errnoPtr = TAPBase->TAPIFace->__error();

    for (i = 0; i < MAX_TAP_UNITS; i ++)
    {
        TAPBase->unit[i].num = i;
    }

    return TRUE;
}

static int GM_UNIQUENAME(expunge)(LIBBASETYPEPTR TAPBase) {
    D(bug("[tap] in expunge\n"));

    if (!HostLibBase)
	return TRUE;

    /* XXX kill the tasks and free memory, just in case */

    if (TAPBase->TAPIFace)
	HostLib_DropInterface ((APTR *)TAPBase->TAPIFace);

    if (TAPBase->LibCHandle)
	HostLib_Close(TAPBase->LibCHandle, NULL);

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

struct newMemList
{
  struct Node	  nml_Node;
  UWORD 	  nml_NumEntries;
  struct MemEntry nml_ME[2];
};

#define IOTASK_STACKSIZE 16384

static struct Task *CreateIOTask(struct tap_base *TAPBase, struct tap_unit *unit)
{
    struct Task     * newtask;
    struct newMemList nml =
    {
	{ NULL, NULL},
	2,
	{
	    { { MEMF_CLEAR|MEMF_PUBLIC }, sizeof(struct Task) },
	    { { MEMF_CLEAR	       }, IOTASK_STACKSIZE    }
        }
    };
    struct MemList  *ml;

    if (NewAllocEntry((struct MemList *)&nml, &ml, NULL))
    {
	struct Task *t;

	newtask = ml->ml_ME[0].me_Addr;

	newtask->tc_Node.ln_Type = NT_TASK;
	newtask->tc_Node.ln_Pri  = 0; /* Priority */
	newtask->tc_Node.ln_Name = unit->iotask_name;

	newtask->tc_SPReg   = (APTR)((IPTR)ml->ml_ME[1].me_Addr + IOTASK_STACKSIZE);
	newtask->tc_SPLower = ml->ml_ME[1].me_Addr;
	newtask->tc_SPUpper = newtask->tc_SPReg;

	D(bug("[tap] IOTask stack %p - %p, SP %p\n", newtask->tc_SPLower, newtask->tc_SPUpper, newtask->tc_SPReg));

	NewList (&newtask->tc_MemEntry);
	AddHead (&newtask->tc_MemEntry, (struct Node *)ml);

	t = NewAddTaskTags(newtask, tap_iotask, NULL, TASKTAG_ARG1, TAPBase, TASKTAG_ARG2, unit, TAG_DONE);
	if (!t)
	{
	    FreeEntry (ml);
	    newtask = NULL;
	}
    }
    else
	newtask=NULL;

    return newtask;
} /* CreateTask */

static int GM_UNIQUENAME(open)(LIBBASETYPEPTR TAPBase, struct IOSana2Req *req, ULONG unitnum, ULONG flags)
{
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
    unit = &(TAPBase->unit[unitnum]);

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
	ObtainSemaphore(&TAPBase->sem);
	fd = TAPBase->TAPIFace->open(TAP_DEV_NODE, O_RDWR, 0);
	ioerr = *TAPBase->errnoPtr;
	ReleaseSemaphore(&TAPBase->sem);

        if (fd < 0) {
            kprintf("[tap] couldn't open '" TAP_DEV_NODE "' (%d)\n", ioerr);
            error = IOERR_OPENFAIL;
        }

        /* and create the virtual network */
        if (error == 0) {
            __sprintf(unit->name, TAP_IFACE_FORMAT, unit->num);

            memset(&ifr, 0, sizeof(struct ifreq));
            ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
            strncpy(ifr.ifr_name, unit->name, IFNAMSIZ);

	    ObtainSemaphore(&TAPBase->sem);
	    error = TAPBase->TAPIFace->ioctl(fd, TUNSETIFF, &ifr);
	    ioerr = *TAPBase->errnoPtr;
	    ReleaseSemaphore(&TAPBase->sem);

	    if (error == -1) {
                kprintf("[tap] couldn't perform TUNSETIFF on TAP device (%d)\n", ioerr);
                error = IOERR_OPENFAIL;
            }

            else
	    {
		/*
		 * Own the file descriptor and enable SIGIO.
		 * tap device has a quirk: this works only when it
		 * was permormed AFTER TUNSETIFF ioctl.
		 */
		ObtainSemaphore(&TAPBase->sem);
		TAPBase->TAPIFace->fcntl(fd, F_SETOWN, TAPBase->aros_pid);
		i = TAPBase->TAPIFace->fcntl(fd, F_GETFL);
		i |= O_ASYNC;
		TAPBase->TAPIFace->fcntl(fd, F_SETFL, i);
		ReleaseSemaphore(&TAPBase->sem);

                unit->fd = fd;
                kprintf("[tap] unit %d attached to %s\n", unit->num, unit->name);
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
            __sprintf(unit->iotask_name, TAP_TASK_FORMAT, (int) unit->num);

            /* make it fly */
	    unit->iotask = CreateIOTask(TAPBase, unit);
            
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
	    {
		ObtainSemaphore(&TAPBase->sem);
		TAPBase->TAPIFace->close(unit->fd);
		ReleaseSemaphore(&TAPBase->sem);
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

static int GM_UNIQUENAME(close)(LIBBASETYPEPTR TAPBase, struct IOSana2Req *req)
{
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
	{
	    ObtainSemaphore(&TAPBase->sem);
	    TAPBase->TAPIFace->close(unit->fd);
	    ReleaseSemaphore(&TAPBase->sem);
	}

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
