/*

Copyright (C) 2001-2012 Neil Cafferkey

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

#ifndef DEVICE_H
#define DEVICE_H


#include <exec/types.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/timer.h>

#include "endian.h"
#include "io.h"
#include "ethernet.h"

#define DEVICE_NAME "rhine.device"
#define VERSION 0
#define REVISION 3
#define DATE "12.6.2012"

#define UTILITY_VERSION 39
#define PROMETHEUS_VERSION 2

#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

#ifndef REG
#if defined(__mc68000) && !defined(__AROS__)
#define _REG(A, B) B __asm(#A)
#define REG(A, B) _REG(A, B)
#else
#define REG(A, B) B
#endif
#endif

#define _STR(A) #A
#define STR(A) _STR(A)

#if defined(__mc68000) || !defined(__AROS__)
#define USE_HACKS
#endif


struct DevBase
{
   struct Device device;
   APTR seg_list;
   struct ExecBase *sys_base;
   struct UtilityBase *utility_base;
   struct Library *prometheus_base;
   struct MinList pci_units;
   struct timerequest timer_request;
   VOID (*wrapper_int_code)();
};


enum
{
   WRITE_QUEUE,
   MGMT_QUEUE,
   ADOPT_QUEUE,
   EVENT_QUEUE,
   GENERAL_QUEUE,
   REQUEST_QUEUE_COUNT
};

enum
{
   PCI_BUS
};

#define STAT_COUNT 3

#define FRAME_BUFFER_SIZE (ETH_HEADERSIZE + ETH_MTU + ETH_CRCSIZE)
#define TX_SLOT_COUNT 10
#define RX_SLOT_COUNT 10


struct DevUnit
{
   struct MinNode node;
   ULONG index;
   ULONG open_count;
   ULONG flags;
   UWORD bus;
   struct Task *task;
   struct MsgPort *request_ports[REQUEST_QUEUE_COUNT];
   struct DevBase *device;
   APTR card;
   BOOL (*insertion_function)(APTR, struct DevBase *);
   VOID (*removal_function)(APTR, struct DevBase *);
   ULONG (*ByteIn)(APTR, ULONG);
   VOID (*ByteOut)(APTR, ULONG, UBYTE);
   UWORD (*LEWordIn)(APTR, ULONG);
   ULONG (*LELongIn)(APTR, ULONG);
   VOID (*LEWordOut)(APTR, ULONG, UWORD);
   VOID (*LELongOut)(APTR, ULONG, ULONG);
   APTR (*AllocDMAMem)(APTR, UPINT, UWORD);
   VOID (*FreeDMAMem)(APTR, APTR);
   UBYTE *tx_buffer;
   UBYTE *tx_buffers[TX_SLOT_COUNT];
   UBYTE *rx_buffers[RX_SLOT_COUNT];
   ULONG card_removed_signal;
   ULONG card_inserted_signal;
   ULONG range_count;
   UBYTE address[ETH_ADDRESSSIZE];
   UBYTE default_address[ETH_ADDRESSSIZE];
   struct MinList openers;
   struct MinList type_trackers;
   struct MinList multicast_ranges;
   struct Interrupt status_int;
   struct Interrupt rx_int;
   struct Interrupt tx_int;
   struct Interrupt tx_end_int;
   struct Interrupt reset_handler;
   struct Sana2DeviceStats stats;
   ULONG special_stats[STAT_COUNT];
   ULONG *tx_descs[TX_SLOT_COUNT];
   ULONG tx_descs_p[TX_SLOT_COUNT];
   ULONG *rx_descs[TX_SLOT_COUNT];
   ULONG rx_descs_p[TX_SLOT_COUNT];
   struct IOSana2Req **tx_requests;
   struct SignalSemaphore access_lock;
   UWORD tx_in_slot;
   UWORD tx_out_slot;
   UWORD rx_slot;
   UBYTE mii_phy_no;
   ULONG speed;
};


struct Opener
{
   struct MinNode node;
   struct MsgPort read_port;
   struct MsgPort mgmt_port;
   BOOL (*rx_function)(REG(a0, APTR), REG(a1, APTR), REG(d0, ULONG));
   BOOL (*tx_function)(REG(a0, APTR), REG(a1, APTR), REG(d0, ULONG));
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   struct Hook *filter_hook;
   struct MinList initial_stats;
   const VOID *real_rx_function;
   const VOID *real_tx_function;
   const VOID *real_dma_tx_function;
};


/* Unit flags */

#define UNITF_SHARED (1 << 0)
#define UNITF_ONLINE (1 << 1)
#define UNITF_ALLMCAST (1 << 2)
#define UNITF_HAVEADAPTER (1 << 3)
#define UNITF_CONFIGURED (1 << 4)
#define UNITF_PROM (1 << 5)
#define UNITF_WASONLINE (1 << 6)   /* card was online at time of removal */
#define UNITF_TXBUFFERINUSE (1 << 7)
#define UNITF_INTADDED (1 << 8)
#define UNITF_RESETADDED (1 << 9)


/* Library and device bases */

#define SysBase (base->sys_base)
#define UtilityBase (base->utility_base)
#define PrometheusBase (base->prometheus_base)
#define TimerBase (base->timer_request.tr_node.io_Device)

#ifndef BASE_REG
#define BASE_REG a6
#endif


#endif
