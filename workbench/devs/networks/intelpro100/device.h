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

#include "io.h"
#include "endian.h"
#undef Lock

#define DEVICE_NAME "intelpro100.device"
#define VERSION 1
#define REVISION 1
#define DATE "12.6.2012"

#define UTILITY_VERSION 39
#define PROMETHEUS_VERSION 2
#define POWERPCI_VERSION 2
#define EXPANSION_VERSION 50
#define OPENPCI_VERSION 1

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

#ifndef __AROS__
#define USE_HACKS
#endif


struct DevBase
{
   struct Device device;
   APTR seg_list;
   struct ExecBase *sys_base;
   struct UtilityBase *utility_base;
   struct Library *prometheus_base;
   struct Library *powerpci_base;
   struct Library *openpci_base;
   struct MinList pci_units;
   struct timerequest timer_request;
#ifdef __amigaos4__
   struct Library *expansion_base;
   struct ExecIFace *i_exec;
   struct UtilityIFace *i_utility;
   struct PCIIFace *i_pci;
   struct TimerIFace *i_timer;
#endif
   VOID (*wrapper_int_code)();
};


enum
{
   WRITE_QUEUE,
   ADOPT_QUEUE,
   EVENT_QUEUE,
   GENERAL_QUEUE,
   REQUEST_QUEUE_COUNT
};

enum
{
   PCI_BUS
};

enum
{
   I82557_GEN,
   I82558_GEN,
   I82559_GEN,
   I82550_GEN,
   I82551_GEN,
   I82562_GEN
};

#define IO_WINDOW_SIZE 0x40

#define ETH_ADDRESSSIZE 6
#define ETH_HEADERSIZE 14
#define ETH_MTU 1500
#define ETH_MAXPACKETSIZE ((ETH_HEADERSIZE) + (ETH_MTU))

#define ETH_PACKET_DEST 0
#define ETH_PACKET_SOURCE 6
#define ETH_PACKET_TYPE 12
#define ETH_PACKET_IEEELEN 12
#define ETH_PACKET_SNAPTYPE 20
#define ETH_PACKET_DATA 14

#define SPECIAL_STAT_COUNT 3

#define TX_SLOT_COUNT 10
#define RX_SLOT_COUNT 10
#define TCB_SIZE \
   (sizeof(ULONG) * (PROCB_EXTBUFFER + PRO_FRAGLEN * 2) \
   + ((ETH_HEADERSIZE + 3) & ~3))
#define RCB_SIZE \
   (sizeof(ULONG) * PROCB_BUFFER + ((ETH_MAXPACKETSIZE + 3) & ~3))


struct DevUnit
{
   struct MinNode node;
   ULONG index;
   ULONG open_count;
   UWORD flags;
   struct Task *task;
   struct MsgPort *request_ports[REQUEST_QUEUE_COUNT];
   struct DevBase *device;
   APTR card;
   ULONG (*ByteIn)(APTR, UBYTE);
   VOID (*ByteOut)(APTR, ULONG, UBYTE);
   UWORD (*LEWordIn)(APTR, ULONG);
   ULONG (*LELongIn)(APTR, ULONG);
   VOID (*LEWordOut)(APTR, ULONG, UWORD);
   VOID (*LELongOut)(APTR, ULONG, ULONG);
   APTR (*AllocDMAMem)(APTR, UPINT, UWORD);
   VOID (*FreeDMAMem)(APTR, APTR);
   UBYTE *tx_buffer;
   UBYTE address[ETH_ADDRESSSIZE];
   UBYTE default_address[ETH_ADDRESSSIZE];
   struct MinList openers;
   struct MinList type_trackers;
   struct MinList multicast_ranges;
   struct MinList tx_requests;
   struct Interrupt status_int;
   struct Interrupt rx_int;
   struct Interrupt tx_int;
   struct Interrupt tx_end_int;
   struct Sana2DeviceStats stats;
   ULONG *stats_buffer;
   ULONG special_stats[SPECIAL_STAT_COUNT];
   ULONG *tcbs;
   ULONG *rcbs;
   ULONG *last_rcb;
   ULONG *first_tcb;
   ULONG *last_tcb;
   ULONG *multicast_cb;
   ULONG *link_cb;
   ULONG speed;
   struct SignalSemaphore access_lock;
   UWORD phy_info;
   UWORD bus;
   UWORD eeprom_addr_size;
};


struct Opener
{
   struct MinNode node;
   struct MsgPort read_port;
   BOOL (*rx_function)(REG(a0, APTR), REG(a1, APTR), REG(d0, ULONG));
   BOOL (*tx_function)(REG(a0, APTR), REG(a1, APTR), REG(d0, ULONG));
   UBYTE *(*dma_tx_function)(REG(a0, APTR));
   struct Hook *filter_hook;
   struct MinList initial_stats;
#if defined(__amigaos4__) || defined(__MORPHOS__) || defined(__AROS__)
   const VOID *real_rx_function;
   const VOID *real_tx_function;
   const VOID *real_dma_tx_function;
#endif
};


struct TypeStats
{
   struct MinNode node;
   ULONG packet_type;
   struct Sana2PacketTypeStats stats;
};


struct TypeTracker
{
   struct MinNode node;
   ULONG packet_type;
   struct Sana2PacketTypeStats stats;
   ULONG user_count;
};


struct AddressRange
{
   struct MinNode node;
   ULONG add_count;
   ULONG lower_bound_left;
   ULONG upper_bound_left;
   UWORD lower_bound_right;
   UWORD upper_bound_right;
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
#define UNITF_MCASTBUFFERINUSE (1 << 8)


/* Library and device bases */

#define SysBase (base->sys_base)
#define UtilityBase (base->utility_base)
#define ExpansionBase (base->expansion_base)
#define OpenPciBase (base->openpci_base)
#define PrometheusBase (base->prometheus_base)
#define PowerPCIBase (base->powerpci_base)
#define TimerBase (base->timer_request.tr_node.io_Device)

#ifdef __amigaos4__
#define IExec (base->i_exec)
#define IUtility (base->i_utility)
#define ITimer (base->i_timer)
#endif

#ifndef BASE_REG
#define BASE_REG a6
#endif


#endif
