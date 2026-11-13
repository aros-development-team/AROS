/*

Copyright (C) 2001-2026 Neil Cafferkey

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
#include <devices/sana2wireless.h>
#include <devices/timer.h>

#include "compatibility.h"
#include "endian.h"
#include "io.h"
#include "ethernet.h"
#include "wireless.h"
#include "encryption.h"

#define DEVICE_NAME "atheros5000.device"
#define VERSION 1
#define REVISION 7
#define DATE "24.1.2026"

#define UTILITY_VERSION 39
#define PROMETHEUS_VERSION 2
#define EXPANSION_VERSION 50
#define OPENPCI_VERSION 1
#define DOS_VERSION 36


struct DevBase
{
   struct Device device;
   APTR seg_list;
   struct ExecBase *sys_base;
   struct UtilityBase *utility_base;
   struct Library *prometheus_base;
   struct Library *openpci_base;
   struct DosLibrary *dos_base;
   struct MinList pci_units;
   struct timerequest timer_request;
#ifdef __amigaos4__
   struct Library *expansion_base;
   struct ExecIFace *i_exec;
   struct UtilityIFace *i_utility;
   struct PCIIFace *i_pci;
   struct DOSIFace *i_dos;
   struct TimerIFace *i_timer;
#endif
   VOID (*wrapper_int_code)();
};


enum
{
   WRITE_QUEUE,
   MGMT_QUEUE,
   ADOPT_QUEUE,
   EVENT_QUEUE,
   SCAN_QUEUE,
   GENERAL_QUEUE,
   REQUEST_QUEUE_COUNT
};

enum
{
   PCI_BUS
};

#define STAT_COUNT 3
#define ENC_COUNT 4

#define FRAME_BUFFER_COUNT 10
#define TX_SLOT_COUNT 2
#define MGMT_SLOT_COUNT 2
#define RX_SLOT_COUNT 40


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
   struct ath_hal *hal;
   BOOL (*insertion_function)(APTR, struct DevBase *);
   VOID (*removal_function)(APTR, struct DevBase *);
   APTR (*AllocDMAMem)(APTR, UPINT, UWORD);
   VOID (*FreeDMAMem)(APTR, APTR);
   UBYTE *tx_buffer;
   UBYTE *tx_buffers[TX_SLOT_COUNT];
   ULONG tx_buffers_p[TX_SLOT_COUNT];
   UBYTE *mgmt_buffers[MGMT_SLOT_COUNT];
   ULONG mgmt_buffers_p[MGMT_SLOT_COUNT];
   UBYTE *rx_buffer;
   UBYTE *rx_buffers[RX_SLOT_COUNT];
   ULONG rx_buffers_p[RX_SLOT_COUNT];
   UBYTE *rx_frames;
   LONG rx_fragment_nos[FRAME_BUFFER_COUNT];
   ULONG card_removed_signal;
   ULONG card_inserted_signal;
   ULONG range_count;
   ULONG filter_mask;
   ULONG antenna;
   UBYTE address[ETH_ADDRESSSIZE];
   UBYTE default_address[ETH_ADDRESSSIZE];
   UBYTE bssid[ETH_ADDRESSSIZE + 2];
   struct MinList openers;
   struct MinList type_trackers;
   struct MinList multicast_ranges;
   struct Interrupt status_int;
   struct Interrupt rx_int;
   struct Interrupt tx_int;
   struct Interrupt tx_end_int;
   struct Interrupt mgmt_int;
   struct Interrupt mgmt_end_int;
   struct Interrupt reset_handler;
   struct Sana2DeviceStats stats;
   ULONG special_stats[STAT_COUNT];
   struct ath_desc *tx_descs;
   ULONG tx_descs_p;
   struct ath_desc *mgmt_descs;
   ULONG mgmt_descs_p;
   struct ath_desc *rx_descs;
   ULONG rx_descs_p;
   struct IOSana2Req **tx_requests;
   struct IOSana2Req **mgmt_requests;
   ULONG speed;
   struct Sana2SignalQuality signal_quality;
   struct SignalSemaphore access_lock;
   UWORD tx_queue_no;
   UWORD mgmt_queue_no;
   UWORD tx_in_slot;
   UWORD tx_out_slot;
   UWORD mgmt_in_slot;
   UWORD mgmt_out_slot;
   UWORD rx_slot;
   UWORD mode;
   UWORD band;
   APTR channels;
   UWORD channel_count;
   UWORD channel;
   UWORD assoc_id;
   UWORD capabilities;
   ULONG band_mask;
   const VOID *rate_table;
   const ULONG *tx_rates;
   ULONG mgmt_rate;
   UWORD tx_rate_codes[4];
   UWORD mgmt_rate_code;
   UWORD tx_key_no;
   struct KeyUnion keys[WIFI_KEYCOUNT];
   UWORD iv_sizes[ENC_COUNT];
   VOID (*fragment_encrypt_functions[ENC_COUNT])(struct DevUnit *, UBYTE *,
      UBYTE *, UWORD *, UBYTE *, struct DevBase *);
   BOOL (*fragment_decrypt_functions[ENC_COUNT])(struct DevUnit *, UBYTE *,
      UBYTE *, UWORD *, UBYTE *, struct DevBase *);
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
#define UNITF_HAVEADAPTER (1 << 2)
#define UNITF_CONFIGURED (1 << 3)
#define UNITF_PROM (1 << 4)
#define UNITF_WASONLINE (1 << 5)   /* card was online at time of removal */
#define UNITF_HARDWEP (1 << 6)
#define UNITF_HARDTKIP (1 << 7)
#define UNITF_HARDMIC (1 << 8)
#define UNITF_HARDCCMP (1 << 9)
#define UNITF_SHORTPREAMBLE (1 << 10)
#define UNITF_SLOWRETRIES (1 << 11)
#define UNITF_ALLMCAST (1 << 12)
#define UNITF_HASADHOC (1 << 13)
#define UNITF_INTADDED (1 << 14)
#define UNITF_TASKADDED (1 << 15)
#define UNITF_RESETADDED (1 << 16)


/* Library and device bases */

#define SysBase (base->sys_base)
#define UtilityBase (base->utility_base)
#define ExpansionBase (base->expansion_base)
#define OpenPciBase (base->openpci_base)
#define PrometheusBase (base->prometheus_base)
#define DOSBase (base->dos_base)
#define TimerBase (base->timer_request.tr_node.io_Device)

#ifdef __amigaos4__
#define IExec (base->i_exec)
#define IUtility (base->i_utility)
#define IDOS (base->i_dos)
#define ITimer (base->i_timer)
#endif

extern struct DevBase *hal_dev_base;


#endif
