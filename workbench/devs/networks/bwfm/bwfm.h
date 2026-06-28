/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    bwfm.device - SANA-II network device for the Broadcom FullMAC WiFi chip
    on Raspberry Pi 3/3B+/4. The chip bring-up and SDIO access live in
    bwfm.resource; this device adds the DOS-side firmware loading and the
    SANA-II datapath / 802.11 management on top.

    Structure modelled on the AROS tap.device (APL).
*/

#ifndef BWFM_DEV_H
#define BWFM_DEV_H

#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <devices/sana2.h>
#include <utility/hooks.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#define BWFM_MAX_UNITS          1
#define ETHER_ADDR_LEN          6

/* BWFMRxFrame() *info flag (low byte = SDPCM channel); mirrors bwfm_sdio.h. */
#define BWFM_RX_EVENT           0x100

/* Per-opener state (one per OpenDevice) */
struct bwfm_opener
{
    struct MinNode      node;
    struct MsgPort      read_pending;       /* queued CMD_READ requests */
    BOOL                (*rx)(APTR, APTR, ULONG);   /* S2_CopyToBuff[16] */
    BOOL                (*tx)(APTR, APTR, ULONG);   /* S2_CopyFromBuff[16/32] */
    struct Hook        *filter;             /* S2_PacketFilter hook */
};

/* Per packet-type statistics tracker (SANA-II S2_TRACKTYPE) */
struct bwfm_tracker
{
    struct MinNode      node;
    ULONG               refcount;
    ULONG               packet_type;
    struct Sana2PacketTypeStats stats;
};

/* Per-unit state */
struct bwfm_unit
{
    LONG                num;
    ULONG               refcount;
    ULONG               state;              /* SANA2 unit flags (online etc.) */
    int                 online;             /* S2_ONLINE: deliver received frames */
    int                 joined;             /* associated to the configured AP */
    UBYTE               hwaddr[ETHER_ADDR_LEN];
    struct Sana2DeviceQuery info;
    struct Sana2DeviceStats stats;
    struct SignalSemaphore lock;        /* guards openers/read_pending/
                                         * event_pending/trackers/assoc_* and
                                         * pending_events across tasks (SMP) */
    struct MinList      openers;
    struct MinList      trackers;
    struct MinList      event_pending;      /* queued S2_ONEVENT requests */
    ULONG               pending_events;     /* edge events (CONNECT/DISCONNECT)
                                             * latched until a listener arms */
    /* Async associate job: S2_SETOPTIONS copies the params here and signals the
     * bwfm.ctrl worker, then replies at once, so BWFMJoin runs off the caller's
     * (wpa_supplicant's) event loop. */
    int                 assoc_pending;
    ULONG               assoc_ssidlen;
    ULONG               assoc_passlen;
    UBYTE               assoc_ssid[33];
    UBYTE               assoc_pass[64];
};

struct bwfm_base
{
    struct Device       device;
    struct bwfm_unit    unit[BWFM_MAX_UNITS];
    int                 fw_loaded;          /* chip firmware brought up once */
    struct Process     *pump;               /* async SDPCM RX pump process */
    struct Process     *ctrl;               /* async control worker (associate) */
    struct Task        *ctrl_task;          /* ctrl worker task, for Signal() */
    ULONG               ctrl_sig;           /* ctrl worker wake signal mask */
};

/* Pulls in genmodule's GM_UNIQUENAME / LIBBASE / LIBBASETYPEPTR macros.
 * Must follow the struct bwfm_base definition (libbasetype). */
#include LC_LIBDEFS_FILE

#endif /* BWFM_DEV_H */
