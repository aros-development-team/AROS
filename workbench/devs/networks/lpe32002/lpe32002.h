#ifndef _LPE32002_H_
#define _LPE32002_H_
/*
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

#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#include <aros/io.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>

#include <devices/timer.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <oop/oop.h>

#include <hidd/pci.h>

#include LC_LIBDEFS_FILE

#include "lpe32002_hw.h"

typedef BOOL bool;
typedef UBYTE u8;
typedef UWORD u16;
typedef ULONG u32;
typedef LONG s32;
typedef UQUAD u64;

#define false FALSE
#define true TRUE

#define __iomem volatile

#define LPE_TASK_NAME     "%s.task"
#define LPE_PORT_NAME     "%s.port"

/* ===== Device Base ===== */

struct LPe32002Base
{
    struct Device           lpeb_Device;

#if defined(__OOP_NOLIBBASE__)
    struct Library          *lpeb_OOPBase;
#endif
    OOP_Object              *lpeb_PCI;
#if defined(__OOP_NOATTRBASES__)
    OOP_AttrBase            lpeb_PCIDeviceAttrBase;
#endif
#if defined(__OOP_NOMETHODBASES__)
    OOP_MethodID            lpeb_HiddPCIBase;
    OOP_MethodID            lpeb_HiddPCIDeviceBase;
    OOP_MethodID            lpeb_HiddPCIDriverBase;
#endif
    ULONG                   lpeb_UnitCount;
    struct List             lpeb_Units;
};

/* ===== Startup message ===== */

struct LPeStartup
{
    struct MsgPort          *lpesm_SyncPort;
    struct LPe32002Unit     *lpesm_Unit;
};

/* ===== OOP base helpers ===== */

#if defined(__OOP_NOLIBBASE__)
#undef OOPBase
#define OOPBase   (LIBBASE->lpeb_OOPBase)
#endif

#if defined(__OOP_NOMETHODBASES__)
#undef HiddPCIBase
#define HiddPCIBase   (LIBBASE->lpeb_HiddPCIBase)
#undef HiddPCIDeviceBase
#define HiddPCIDeviceBase       (LIBBASE->lpeb_HiddPCIDeviceBase)
#undef HiddPCIDriverBase
#define HiddPCIDriverBase       (LIBBASE->lpeb_HiddPCIDriverBase)
#endif

#if defined(__OOP_NOATTRBASES__)
#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->lpeb_PCIDeviceAttrBase)
#endif

/* ===== Request Queue IDs ===== */

enum {
    WRITE_QUEUE,
    ADOPT_QUEUE,
    EVENT_QUEUE,
    GENERAL_QUEUE,
    REQUEST_QUEUE_COUNT
};

/* ===== SANA2 Opener Structure ===== */

struct Opener
{
    struct MinNode  node;
    struct MsgPort  read_port;
    BOOL            (*rx_function)(APTR, APTR, ULONG);
    BOOL            (*tx_function)(APTR, APTR, ULONG);
    struct Hook     *filter_hook;
    struct MinList  initial_stats;
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
    UBYTE lower_bound[FC_ADDR_SIZE];
    UBYTE upper_bound[FC_ADDR_SIZE];
};

/* ===== Standard interface flags ===== */

#define IFF_UP          0x1
#define IFF_BROADCAST   0x2
#define IFF_DEBUG       0x4
#define IFF_LOOPBACK    0x8
#define IFF_POINTOPOINT 0x10
#define IFF_NOTRAILERS  0x20
#define IFF_RUNNING     0x40
#define IFF_NOARP       0x80
#define IFF_PROMISC     0x100
#define IFF_ALLMULTI     0x200
#define IFF_MASTER      0x400
#define IFF_SLAVE       0x800
#define IFF_MULTICAST   0x1000
#define IFF_VOLATILE    (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST|IFF_MASTER|IFF_SLAVE|IFF_RUNNING)
#define IFF_PORTSEL     0x2000
#define IFF_AUTOMEDIA   0x4000
#define IFF_DYNAMIC     0x8000
#define IFF_SHARED      0x10000
#define IFF_CONFIGURED  0x20000

/* ===== Multicast list ===== */

#define MAX_ADDR_LEN    32

struct dev_mc_list
{
    struct dev_mc_list      *next;
    UBYTE                   dmi_addr[MAX_ADDR_LEN];
    unsigned char           dmi_addrlen;
    int                     dmi_users;
    int                     dmi_gusers;
};

/* ===== Network state bits ===== */

enum netdev_state_t
{
    __LINK_STATE_XOFF=0,
    __LINK_STATE_START,
    __LINK_STATE_PRESENT,
    __LINK_STATE_SCHED,
    __LINK_STATE_NOCARRIER,
    __LINK_STATE_RX_SCHED,
    __LINK_STATE_LINKWATCH_PENDING
};

static inline int test_bit(int nr, const volatile ULONG *addr)
{
    return ((1UL << (nr & 31)) & (addr[nr >> 5])) != 0;
}

static inline void set_bit(int nr, volatile ULONG *addr)
{
    addr[nr >> 5] |= 1UL << (nr & 31);
}

static inline void clear_bit(int nr, volatile ULONG *addr)
{
    addr[nr >> 5] &= ~(1UL << (nr & 31));
}

static inline int test_and_set_bit(int nr, volatile ULONG *addr)
{
    int oldbit = test_bit(nr, addr);
    set_bit(nr, addr);
    return oldbit;
}

static inline int test_and_clear_bit(int nr, volatile ULONG *addr)
{
    int oldbit = test_bit(nr, addr);
    clear_bit(nr, addr);
    return oldbit;
}

/* ===== FC Frame buffer ===== */

/*
 * FC frame for send/receive through the SANA2 interface.
 * Uses WWPN-based addressing (8 bytes) and FC-sized payloads.
 */
#define FC_PACKET_DEST      0
#define FC_PACKET_SOURCE    8
#define FC_PACKET_TYPE      16
#define FC_PACKET_DATA      18

#define FC_RXTX_ALLOC_BUFSIZE   (FC_MAX_FRAME_SIZE + 32)

struct fc_eth_frame {
    UBYTE   fc_packet_dest[FC_ADDR_SIZE];
    UBYTE   fc_packet_source[FC_ADDR_SIZE];
    UWORD   fc_packet_type;
    UBYTE   fc_packet_data[FC_MTU];
    UBYTE   fc_packet_crc[FC_CRC_SIZE];
    UBYTE   fc_pad[FC_RXTX_ALLOC_BUFSIZE - (FC_ADDR_SIZE*2 + 2 + FC_MTU + FC_CRC_SIZE)];
} __attribute__((packed));

#define fc_packet_ieeelen   fc_packet_type

/* ===== Unit Structure ===== */

#define STAT_COUNT      3
#define LPE_MAX_UNITS   8

struct LPe32002Unit {
    struct Node             lpeu_Node;

    struct LPe32002Base     *lpeu_device;

    STRPTR                  lpeu_name;
    ULONG                   lpeu_UnitNum;

    OOP_Object              *lpeu_PCIDevice;
    OOP_Object              *lpeu_PCIDriver;
    IPTR                    lpeu_IRQ;

    int                     lpeu_open_count;
    struct SignalSemaphore  lpeu_unit_lock;

    LONG                    lpeu_range_count;
    struct MinList          lpeu_Openers;
    struct MinList          lpeu_multicast_ranges;
    struct MinList          lpeu_type_trackers;

    struct timeval          lpeu_toutPOLL;
    BOOL                    lpeu_toutNEED;
    BOOL                    lpeu_IntsAdded;

    struct MsgPort          *lpeu_TimerSlowPort;
    struct timerequest      *lpeu_TimerSlowReq;

    struct MsgPort          lpeu_DelayPort;
    struct timerequest      lpeu_DelayReq;

    ULONG                   lpeu_mtu;
    ULONG                   lpeu_ifflags;
    struct Sana2DeviceQuery lpeu_Sana2Info;
    struct Sana2DeviceStats lpeu_stats;
    ULONG                   lpeu_special_stats[STAT_COUNT];

    ULONG                   lpeu_state;

    /* Hardware addressing */
    UBYTE                   lpeu_dev_addr[FC_ADDR_SIZE];
    UBYTE                   lpeu_org_addr[FC_ADDR_SIZE];

    /* PCI BARs (mapped) */
    APTR                    lpeu_bar0;      /* SLI-4 config registers */
    IPTR                    lpeu_bar0_size;
    APTR                    lpeu_bar1;      /* SLI-4 control registers */
    IPTR                    lpeu_bar1_size;

    /* SLI-4 parameters read from firmware */
    struct sli4_params      lpeu_sli_params;

    /* SLI-4 Queues */
    struct sli4_queue       lpeu_eq;        /* Event Queue */
    struct sli4_queue       lpeu_cq_wq;     /* CQ for WQ completions */
    struct sli4_queue       lpeu_cq_rq;     /* CQ for RQ completions */
    struct sli4_queue       lpeu_cq_mq;     /* CQ for MQ completions */
    struct sli4_queue       lpeu_wq;        /* Work Queue (TX) */
    struct sli4_queue       lpeu_rq_hdr;    /* Receive Queue headers */
    struct sli4_queue       lpeu_rq_data;   /* Receive Queue data */
    struct sli4_queue       lpeu_mq;        /* Mailbox Queue */

    /* TX/RX buffer arrays */
    struct lpe_tx_desc      *lpeu_tx_descs;
    ULONG                   lpeu_tx_count;
    ULONG                   lpeu_tx_prod;
    ULONG                   lpeu_tx_cons;

    struct lpe_rx_desc      *lpeu_rx_descs;
    ULONG                   lpeu_rx_count;
    ULONG                   lpeu_rx_prod;
    ULONG                   lpeu_rx_cons;

    /* Bootstrap mailbox (DMA-able) */
    struct sli4_mbox_cmd    *lpeu_bmbx;
    APTR                    lpeu_bmbx_dma;

    /* Interrupt and task */
    struct Process          *lpeu_Process;

    struct Interrupt        lpeu_irqhandler;
    struct Interrupt        lpeu_touthandler;
    struct Interrupt        lpeu_tx_int;

    BYTE                    lpeu_signal_0;
    BYTE                    lpeu_signal_1;
    BYTE                    lpeu_signal_2;
    BYTE                    lpeu_signal_3;

    struct MsgPort          *lpeu_input_port;
    struct MsgPort          *lpeu_request_ports[REQUEST_QUEUE_COUNT];
};

/* ===== Helper macros ===== */

#define pci_name(unit) (unit->lpeu_name)

static inline void netif_schedule(struct LPe32002Unit *unit)
{
    if (!test_bit(__LINK_STATE_XOFF, &unit->lpeu_state))
        Cause(&unit->lpeu_tx_int);
}

static inline void netif_start_queue(struct LPe32002Unit *unit)
{
    clear_bit(__LINK_STATE_XOFF, &unit->lpeu_state);
}

static inline void netif_wake_queue(struct LPe32002Unit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &unit->lpeu_state))
        Cause(&unit->lpeu_tx_int);
}

static inline void netif_stop_queue(struct LPe32002Unit *unit)
{
    set_bit(__LINK_STATE_XOFF, &unit->lpeu_state);
}

static inline int netif_queue_stopped(const struct LPe32002Unit *unit)
{
    return test_bit(__LINK_STATE_XOFF, &unit->lpeu_state);
}

static inline int netif_running(const struct LPe32002Unit *unit)
{
    return test_bit(__LINK_STATE_START, &unit->lpeu_state);
}

static inline int netif_carrier_ok(const struct LPe32002Unit *unit)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &unit->lpeu_state);
}

extern VOID ReportEvents(struct LPe32002Base *, struct LPe32002Unit *, ULONG);

static inline void netif_carrier_on(struct LPe32002Unit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &unit->lpeu_state)) {
        unit->lpeu_ifflags |= IFF_UP;
        D(bug("[%s] %s: Device set as ONLINE\n", unit->lpeu_name, __func__));
        ReportEvents(unit->lpeu_device, unit, S2EVENT_ONLINE);
    }
}

static inline void netif_carrier_off(struct LPe32002Unit *unit)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &unit->lpeu_state)) {
        unit->lpeu_ifflags &= ~IFF_UP;
        D(bug("[%s] %s: Device set as OFFLINE\n", unit->lpeu_name, __func__));
        ReportEvents(unit->lpeu_device, unit, S2EVENT_OFFLINE);
    }
}

/* ===== Handler entry point ===== */

void handle_request(LIBBASETYPEPTR, struct IOSana2Req *);

#endif /* _LPE32002_H_ */
