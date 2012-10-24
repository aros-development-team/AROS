#ifndef _PCNET32_H
#define _PCNET32_H

/*
 * $Id$
 */

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

#define DEBUG 0

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <dos/bptr.h>

#include <oop/oop.h>

#include <hidd/pci.h>

#include <devices/timer.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/exec.h>
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#define PCNET32_TASK_NAME	"PCNet32 task"
#define PCNET32_PORT_NAME	"PCNet32 port"

struct PCN32Base {
    struct Device       pcnb_Device;
    struct MsgPort      *pcnb_syncport;

    OOP_Object          *pcnb_pci;
    OOP_Object          *pcnb_irq;
    OOP_AttrBase        pcnb_pciDeviceAttrBase;

    struct Sana2DeviceQuery pcnb_Sana2Info;
    struct PCN32Unit       *pcnb_unit;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->pcnb_pciDeviceAttrBase)

enum {
    WRITE_QUEUE,
    ADOPT_QUEUE,
    EVENT_QUEUE,
    GENERAL_QUEUE,
    REQUEST_QUEUE_COUNT
};

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
    ULONG lower_bound_left;
    ULONG upper_bound_left;
    UWORD lower_bound_right;
    UWORD upper_bound_right;
};

/* Big endian: should work, but is untested */

struct rx_ring_desc
{
    IPTR    PacketBuffer;
    UWORD   BufferLength;
    UWORD   BufferStatus;
    ULONG   BufferMsgLength;
    ULONG   Reserved;
};

struct tx_ring_desc
{
    IPTR    PacketBuffer;
    UWORD   BufferLength;
    UWORD   BufferStatus;
    ULONG   Misc;
    ULONG   Reserved;
};

#define STAT_COUNT 3

struct PCN32Unit {
    struct MinNode          *pcnu_Node;
    struct MinList          pcnu_Openers;
    struct MinList          pcnu_multicast_ranges;
    struct MinList          pcnu_type_trackers;
    ULONG                   pcnu_UnitNum;
    LONG                    pcnu_range_count;

    OOP_Object              *pcnu_PCIDevice;
    OOP_Object              *pcnu_PCIDriver;

    struct timeval          pcnu_toutPOLL;
    BOOL                    pcnu_toutNEED;
    BOOL                    pcnu_IntsAdded;

    struct MsgPort          *pcnu_TimerSlowPort;
    struct timerequest      *pcnu_TimerSlowReq;

    struct MsgPort          *pcnu_TimerFastPort;
    struct timerequest      *pcnu_TimerFastReq;

    struct Sana2DeviceStats pcnu_stats;
    ULONG                   pcnu_special_stats[STAT_COUNT];

   int                     pcnu_pcnet_chiprevision;         /* pcn32: pcnet chipset revision id          */
   char                    *pcnu_pcnet_chipname;            /* pcn32: textual name for detected chipset  */
   ULONG                   pcnu_pcnet_supported;            /* pcn32: Supported features of pcnet family */
#define support_fdx     (1 << 0) // Supports Full Duplex
#define support_mii     (1 << 1)
#define support_fset    (1 << 2)
#define support_ltint   (1 << 3)
#define support_dxsuflo (1 << 4)
/* Card Control Funcs */
    UWORD                   (*read_csr)(APTR, int);         /* pcn32: */
    void                    (*write_csr)(APTR, int, UWORD); /* pcn32: */
    UWORD                   (*read_bcr)(APTR, int);         /* pcn32: */
    void                    (*write_bcr)(APTR, int, UWORD); /* pcn32: */
    UWORD                   (*read_rap)(APTR);              /* pcn32: */
    void                    (*write_rap)(APTR, UWORD);      /* pcn32: */
    void                    (*reset)(APTR);                 /* pcn32: */
/* Card Funcs */
    void                    (*initialize)(struct PCN32Unit *);
    void                    (*deinitialize)(struct PCN32Unit *);
    int                     (*start)(struct PCN32Unit *);
    int                     (*stop)(struct PCN32Unit *);
    int                     (*alloc_rx)(struct PCN32Unit *);
    void                    (*set_mac_address)(struct PCN32Unit *);
    void                    (*linkchange)(struct PCN32Unit *);
    void                    (*linkirq)(struct PCN32Unit *);
//    ULONG                   (*descr_getlength)(struct ring_desc *prd, ULONG v);
    void                    (*set_multicast)(struct PCN32Unit *);

    int                     pcnu_open_count;
    struct SignalSemaphore  pcnu_unit_lock;

    struct Process          *pcnu_Process;

    struct PCN32Base     *pcnu_device;
    struct Interrupt        pcnu_irqhandler;
    struct Interrupt        pcnu_touthandler;
    IPTR	                  pcnu_DeviceID;
    IPTR                    pcnu_DriverFlags;
    IPTR                    pcnu_IRQ;
    APTR                    pcnu_BaseMem;
    IPTR                    pcnu_SizeMem;
    IPTR	                  pcnu_BaseIO;

    BYTE                    pcnu_signal_0;
    BYTE                    pcnu_signal_1;
    BYTE                    pcnu_signal_2;
    BYTE                    pcnu_signal_3;

    struct MsgPort          *pcnu_input_port;

    struct MsgPort          *pcnu_request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt        pcnu_rx_int;
    struct Interrupt        pcnu_tx_int;
    struct Interrupt        pcnu_tx_end_int;

    STRPTR                  pcnu_name;
    ULONG                   pcnu_mtu;
    ULONG                   pcnu_flags;
    ULONG                   pcnu_state;
    APTR                    pcnu_mc_list;
    UBYTE                   pcnu_dev_addr[6];
    UBYTE                   pcnu_org_addr[6];
    struct fe_priv          *pcnu_fe_priv;
};

void handle_request(LIBBASETYPEPTR, struct IOSana2Req *);

/* Media selection options. */
enum {
        IF_PORT_UNKNOWN = 0,
        IF_PORT_10BASE2,
        IF_PORT_10BASET,
        IF_PORT_AUI,
        IF_PORT_100BASET,
        IF_PORT_100BASETX,
        IF_PORT_100BASEFX
};

/* These flag bits are private to the generic network queueing
 * layer, they may not be explicitly referenced by any other
 * code.
 */

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

static inline void netif_schedule(struct PCN32Unit *dev)
{
    if (!test_bit(__LINK_STATE_XOFF, &dev->pcnu_state)) {
        Cause(&dev->pcnu_tx_int);
    }
}


static inline void netif_start_queue(struct PCN32Unit *dev)
{
    clear_bit(__LINK_STATE_XOFF, &dev->pcnu_state);
}

static inline void netif_wake_queue(struct PCN32Unit *dev)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &dev->pcnu_state)) {
        Cause(&dev->pcnu_tx_int);
    }
}

static inline void netif_stop_queue(struct PCN32Unit *dev)
{
    set_bit(__LINK_STATE_XOFF, &dev->pcnu_state);
}

static inline int netif_queue_stopped(const struct PCN32Unit *dev)
{
    return test_bit(__LINK_STATE_XOFF, &dev->pcnu_state);
}

static inline int netif_running(const struct PCN32Unit *dev)
{
    return test_bit(__LINK_STATE_START, &dev->pcnu_state);
}

static inline int netif_carrier_ok(const struct PCN32Unit *dev)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &dev->pcnu_state);
}

extern void __netdev_watchdog_up(struct PCN32Unit *dev);

static inline void netif_carrier_on(struct PCN32Unit *dev)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &dev->pcnu_state)) {
//                linkwatch_fire_event(dev);
    }
    if (netif_running(dev)) {
//                __netdev_watchdog_up(dev);
    }
}

static inline void netif_carrier_off(struct PCN32Unit *dev)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &dev->pcnu_state)) {
//                linkwatch_fire_event(dev);
    }
}

/* Standard interface flags (netdevice->flags). */
#define IFF_UP          0x1             /* interface is up              */
#define IFF_BROADCAST   0x2             /* broadcast address valid      */
#define IFF_DEBUG       0x4             /* turn on debugging            */
#define IFF_LOOPBACK    0x8             /* is a loopback net            */
#define IFF_POINTOPOINT 0x10            /* interface is has p-p link    */
#define IFF_NOTRAILERS  0x20            /* avoid use of trailers        */
#define IFF_RUNNING     0x40            /* resources allocated          */
#define IFF_NOARP       0x80            /* no ARP protocol              */
#define IFF_PROMISC     0x100           /* receive all packets          */
#define IFF_ALLMULTI    0x200           /* receive all multicast packets*/

#define IFF_MASTER      0x400           /* master of a load balancer    */
#define IFF_SLAVE       0x800           /* slave of a load balancer     */

#define IFF_MULTICAST   0x1000          /* Supports multicast           */

#define IFF_VOLATILE    (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST|IFF_MASTER|IFF_SLAVE|IFF_RUNNING)

#define IFF_PORTSEL     0x2000          /* can set media type           */
#define IFF_AUTOMEDIA   0x4000          /* auto media select active     */
#define IFF_DYNAMIC     0x8000          /* dialup device with changing addresses*/
#define IFF_SHARED      0x10000         /* interface may be shared */
#define IFF_CONFIGURED  0x20000         /* interface already configured */

/*
 *      We tag multicasts with these structures.
 */

#define MAX_ADDR_LEN    32

struct dev_mc_list
{
    struct dev_mc_list      *next;
    UBYTE                   dmi_addr[MAX_ADDR_LEN];
    unsigned char           dmi_addrlen;
    int                     dmi_users;
    int                     dmi_gusers;
};

struct pcnet32_init_block { //  - The PCnet32 32-bit initialisation block.
  //                              desribed in databook.
    UWORD   mode;
    UWORD   tlen_rlen;
    UBYTE   phys_addr[6];
    UWORD   reserved;
    ULONG   filter[2];
    /* Recieve and Transmit ring base */
    ULONG   rx_ring;
    ULONG   tx_ring;
}; // pcnet32_init_block

struct fe_priv {
    struct PCN32Unit   *pci_dev;
    int     in_shutdown;
    ULONG   linkspeed;
    int     duplex;
    int     autoneg;
    int     fixed_mode;
    int     phyaddr;
    int     wolenabled;
    unsigned int phy_oui;
    UWORD   gigabit;
    ULONG   desc_ver;
    struct SignalSemaphore  lock;

    APTR     ring_addr;
   
    struct   eth_frame *rx_buffer;
    struct   eth_frame *tx_buffer;

    ULONG   cur_rx, refill_rx;

    ULONG   next_tx, nic_tx;
    ULONG   tx_flags;

    ULONG   irqmask;
    ULONG   need_linktimer;
    struct  timeval link_timeout;
    ULONG   orig_mac[2];
    struct  pcnet32_init_block *fep_pcnet_init_block;
};

#define pci_name(unit)  (unit->pcnu_name)

/* ENET defines */

#define HZ                  1000000
#define ETH_DATA_LEN        1500

#define ETH_ADDRESSSIZE     6
#define ETH_HEADERSIZE      14
#define ETH_CRCSIZE         4
#define ETH_MTU             (ETH_DATA_LEN)
#define ETH_MAXPACKETSIZE   ((ETH_HEADERSIZE) + (ETH_MTU) + (ETH_CRCSIZE))

#define ETH_PACKET_DEST     0
#define ETH_PACKET_SOURCE   6
#define ETH_PACKET_TYPE     12
#define ETH_PACKET_IEEELEN  12
#define ETH_PACKET_SNAPTYPE 20
#define ETH_PACKET_DATA     14
#define ETH_PACKET_CRC      (ETH_PACKET_DATA + ETH_MTU)

#define RXTX_ALLOC_BUFSIZE  (ETH_MAXPACKETSIZE + 26)
 
#define TX_LIMIT_STOP   63
#define TX_LIMIT_START  62

struct eth_frame {
    UBYTE eth_packet_dest[6];
    UBYTE eth_packet_source[6];
    UWORD eth_packet_type;
    UBYTE eth_packet_data[ETH_MTU];
    UBYTE eth_packet_crc[4];
    UBYTE eth_pad[RXTX_ALLOC_BUFSIZE - ETH_MAXPACKETSIZE];
} __attribute__((packed));
#define eth_packet_ieeelen eth_packet_type

void pcn32_get_functions(struct PCN32Unit *Unit);

/* ************************************** */
/*     BEGIN : REAL PCNET32 DEFINES       */
/* ************************************** */

#ifndef PCNET32_LOG_TX_BUFFERS
#define PCNET32_LOG_TX_BUFFERS      3
#define PCNET32_LOG_RX_BUFFERS      4
#endif

#define TX_RING_SIZE                (1 << (PCNET32_LOG_TX_BUFFERS))
#define TX_RING_MOD_MASK            (TX_RING_SIZE - 1)
#define TX_RING_LEN_BITS            ((PCNET32_LOG_TX_BUFFERS) << 12)

#define RX_RING_SIZE                (1 << (PCNET32_LOG_RX_BUFFERS))
#define RX_RING_MOD_MASK            (RX_RING_SIZE - 1)
#define RX_RING_LEN_BITS            ((PCNET32_LOG_RX_BUFFERS) << 4)

#endif

