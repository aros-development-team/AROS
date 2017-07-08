/*

Copyright (C) 2001-2017 Neil Cafferkey

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

#include "wireless.h"
#include "io.h"

#define DEVICE_NAME "realtek8180.device"
#define VERSION 0
#define REVISION 5
#define DATE "7.7.2017"

#define UTILITY_VERSION 39
#define POSEIDON_VERSION 0

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

#ifdef __AROS__
#include <aros/debug.h>
#define DebugPrintF kprintf
#endif

#ifndef __AROS__
#define USE_HACKS
#endif


struct DevBase
{
   struct Device device;
   APTR seg_list;
   struct ExecBase *sys_base;
   struct UtilityBase *utility_base;
   struct Library *poseidon_base;
   struct MinList usb_units;
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
   MGMT_QUEUE,
   ADOPT_QUEUE,
   EVENT_QUEUE,
   SCAN_QUEUE,
   GENERAL_QUEUE,
   REQUEST_QUEUE_COUNT
};

enum
{
   PCI_BUS,
   USB_BUS,
};

enum
{
   RTL8187L_GEN,
   RTL8187B0_GEN,
   RTL8187B1_GEN,
   RTL8187B2_GEN,
};

enum
{
   RTL8225_RADIO,
   RTL8225V2_RADIO,
   RTL8225V2B_RADIO,
};

#define ETH_ADDRESSSIZE 6
#define ETH_HEADERSIZE 14
#define ETH_MTU 1500
#define ETH_MAXPACKETSIZE (ETH_HEADERSIZE + ETH_MTU)

#define ETH_PACKET_DEST 0
#define ETH_PACKET_SOURCE 6
#define ETH_PACKET_TYPE 12
#define ETH_PACKET_IEEELEN 12
#define ETH_PACKET_DATA 14

#define SNAP_HEADERSIZE 8

#define SNAP_FRM_TYPE 6

#define CHANNEL_COUNT 14

#define IV_SIZE 4
#define EIV_SIZE 8
#define ICV_SIZE 4
#define MIC_SIZE 8

#define STAT_COUNT 3
#define ENC_COUNT 4

#define FRAME_BUFFER_SIZE (WIFI_FRM_DATA + SNAP_HEADERSIZE \
   + ETH_MTU + EIV_SIZE + ICV_SIZE + MIC_SIZE)
#define FRAME_BUFFER_COUNT 10
#define TX_SLOT_COUNT 30
#define RX_SLOT_COUNT 30


struct KeyUnion
{
   UWORD type;
   union
   {
      struct WEPKey
      {
         UWORD length;
         UBYTE key[13];
         ULONG tx_iv;
      }
      wep;
      struct TKIPKey
      {
         UWORD key[8];
         ULONG tx_mic_key[2];
         ULONG rx_mic_key[2];
         UWORD tx_iv_low;
         ULONG tx_iv_high;
         UWORD rx_iv_low;
         ULONG rx_iv_high;
         UWORD tx_ttak[5];
         BOOL tx_ttak_set;
         UWORD rx_ttak[5];
         BOOL rx_ttak_set;
      }
      tkip;
      struct CCMPKey
      {
         UBYTE key[16];
         BOOL stream_set;
         ULONG stream[44];
         UWORD tx_iv_low;
         ULONG tx_iv_high;
         UWORD rx_iv_low;
         ULONG rx_iv_high;
      }
      ccmp;
   }
   u;
};


struct DevUnit
{
   struct MinNode node;
   ULONG index;
   ULONG open_count;
   ULONG flags;
   UWORD bus;
   UWORD generation;
   UWORD eeprom_addr_size;
   UWORD radio_type;
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
   VOID (*SendFrame)(APTR, APTR, ULONG);
   VOID (*ReceiveFrame)(APTR, APTR, ULONG);
   APTR (*AllocDMAMem)(APTR, UPINT, UWORD);
   VOID (*FreeDMAMem)(APTR, APTR);
   UBYTE *tx_buffer;
   UBYTE *tx_buffers[TX_SLOT_COUNT];
   UBYTE *rx_buffer;
   UBYTE *rx_buffers[RX_SLOT_COUNT];
   UBYTE *rx_frames;
   LONG rx_fragment_nos[FRAME_BUFFER_COUNT];
   ULONG card_removed_signal;
   ULONG card_inserted_signal;
   ULONG range_count;
   UBYTE address[ETH_ADDRESSSIZE];
   UBYTE default_address[ETH_ADDRESSSIZE];
   UBYTE bssid[ETH_ADDRESSSIZE + 2];
   struct MinList openers;
   struct MinList type_trackers;
   struct MinList multicast_ranges;
   struct Interrupt rx_int;
   struct Interrupt tx_int;
   struct Interrupt mgmt_int;
   struct Sana2DeviceStats stats;
   ULONG special_stats[STAT_COUNT];
   UBYTE *tx_descs[TX_SLOT_COUNT];
   UBYTE *rx_descs[RX_SLOT_COUNT];
   struct IOSana2Req **tx_requests;
   ULONG speed;
   struct Sana2SignalQuality signal_quality;
   struct SignalSemaphore access_lock;
   UWORD tx_in_slot;
   UWORD tx_out_slot;
   UWORD rx_slot;
   UWORD mode;
   UWORD band;
   UWORD channel;
   UBYTE base_cck_power;
   UBYTE base_ofdm_power;
   UBYTE cck_power[CHANNEL_COUNT + 1];
   UBYTE ofdm_power[CHANNEL_COUNT + 1];
   UWORD assoc_id;
   UWORD capabilities;
   UWORD tx_rate;
   UWORD mgmt_rate;
   UWORD tx_rate_code;
   UWORD mgmt_rate_code;
   UWORD tx_sequence;
   UWORD tx_key_no;
   struct KeyUnion keys[WIFI_KEYCOUNT];
   UWORD iv_sizes[ENC_COUNT];
   VOID (*fragment_encrypt_functions[ENC_COUNT])(struct DevUnit *, UBYTE *,
      UBYTE *, UWORD *, UBYTE *, struct DevBase *);
   BOOL (*fragment_decrypt_functions[ENC_COUNT])(struct DevUnit *, UBYTE *,
      UBYTE *, UWORD *, UBYTE *, struct DevBase *);
   UWORD retries_offset;
   UWORD tx_desc_size;
   UWORD rx_desc_size;
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
#define UNITF_HARDCCMP (1 << 8)
#define UNITF_ALLMCAST (1 << 9)
#define UNITF_HASADHOC (1 << 10)
#define UNITF_SHORTPREAMBLE (1 << 11)
#define UNITF_INTADDED (1 << 12)
#define UNITF_RESETADDED (1 << 13)
#define UNITF_TASKADDED (1 << 14)


/* Endianness macros */

#define FlipWord(A) \
   ({ \
      UWORD _FlipWord_A = (A); \
      _FlipWord_A = (_FlipWord_A << 8) | (_FlipWord_A >> 8); \
   })

#define FlipLong(A) \
   ({ \
      ULONG _FlipLong_A = (A); \
      _FlipLong_A = \
         (FlipWord(_FlipLong_A) << 16) | FlipWord(_FlipLong_A >> 16); \
   })

#define FlipQuad(A) \
   ({ \
      UQUAD _FlipQuad_A = (A); \
      _FlipQuad_A = \
         (FlipLong(_FlipQuad_A) << 32) | FlipLong(_FlipQuad_A >> 32); \
   })

#ifndef __i386__ /* Big endian */

#define BEWord(A) \
   (A)

#define BELong(A) \
   (A)

#define LEWord(A) \
   FlipWord(A)

#define LELong(A) \
   FlipLong(A)

#define LEQuad(A) \
   FlipQuad(A)

#else

#define BEWord(A) \
   FlipWord(A)

#define BELong(A) \
   FlipLong(A)

#define LEWord(A) \
   (A)

#define LELong(A) \
   (A)

#define LEQuad(A) \
   (A)

#endif

#define MakeBEWord(A) \
   BEWord(A)

#define MakeBELong(A) \
   BELong(A)

#define MakeLEWord(A) \
   LEWord(A)

#define MakeLELong(A) \
   LELong(A)


/* Library and device bases */

#define SysBase (base->sys_base)
#define UtilityBase (base->utility_base)
#define PsdBase (base->poseidon_base)
#define TimerBase (base->timer_request.tr_node.io_Device)

#ifdef __amigaos4__
#define IExec (base->i_exec)
#define IUtility (base->i_utility)
#define ITimer (base->i_timer)
#endif

#ifdef __AROS__
#define ps PsdBase
#endif

#ifndef BASE_REG
#define BASE_REG a6
#endif


#endif
