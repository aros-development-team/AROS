/*
 * tap - TUN/TAP network driver for AROS
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2010-2011 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#ifndef _ETH_DEVICE_H
#define _ETH_DEVICE_H 1

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/newstyle.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#define timeval sys_timeval

/* avoid conflicts between our __unused define and the ones that might come in
   via fcntl.h */
#undef __unused
#include <fcntl.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include <poll.h>
#include <stdio.h>

#undef timeval

#include LC_LIBDEFS_FILE

/* explicitly prototype rand() so we don't have pull in stdlib.h */
extern int rand(void);

/* NewStyle device support */
#define NEWSTYLE_DEVICE 1

#define MAX_ETH_UNITS (32)

#define ETH_DEV_NODE "/dev/net/eth"
#define ETH_IFACE_FORMAT "eth%ld"
#define ETH_TASK_FORMAT "eth%d"


struct eth_opener {
    struct MinNode              node;

    struct MsgPort              read_pending;

    BOOL                        (*rx)(APTR, APTR, ULONG);
    BOOL                        (*tx)(APTR, APTR, ULONG);

    struct Hook                 *filter;
};

struct eth_tracker {
    struct MinNode              node;
    ULONG                       refcount;
    ULONG                       packet_type;
    struct Sana2PacketTypeStats stats;
};

struct eth_unit
{
    ULONG                       num;
    ULONG                       refcount;

    APTR                        pd;
    int                         fd;

    char                        name[IFNAMSIZ];
    UBYTE                       hwaddr[6];

    struct Sana2DeviceQuery     info;

    ULONG                       flags;

    struct MinList              openers;

    struct MinList              trackers;
    struct Sana2DeviceStats     stats;

    struct MsgPort              *iosyncport;
    struct Task              	*iotask;

    LONG                        abort_signal;
    LONG			io_signal;

    struct MsgPort              *write_queue;

    struct uioInterrupt		irq;
};

struct eth_base
{
    struct Device           eth_device;
    struct eth_unit         unit[MAX_ETH_UNITS];
    OOP_AttrBase	    UnixIOAttrBase;
    OOP_Object		   *unixio;
};

#undef HiddUnixIOAttrBase
#define HiddUnixIOAttrBase LIBBASE->UnixIOAttrBase

/* unit flags */
#define eu_CONFIGURED   (1<<0)
#define eu_ONLINE       (1<<1)


extern void eth_handle_request(struct IOSana2Req *req);

extern void eth_hexdump(unsigned char *buf, int bufsz);

extern void eth_iotask(struct eth_base *TAPBase, struct eth_unit *unit);

#endif
