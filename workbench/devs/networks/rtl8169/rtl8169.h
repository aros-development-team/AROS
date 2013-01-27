#ifndef _RTL8169_H_
#define _RTL8169_H_

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
#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>

#if DEBUG > 0
#define RTL_DEBUG
#define RTL_DEBUG_PACKET
#endif

#define RTLD(d) \
        if (unit->rtl8169u_flags & IFF_DEBUG) \
        { \
                d; \
        }

#if defined(RTL_DEBUG_PACKET)
#define RTLDP(d) \
        if (unit->rtl8169u_flags & IFF_DEBUG) \
        { \
                d; \
        }
#else
#define RTLDP(d)
#endif

//#define HAVE__PCI_MWI

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

#include LC_LIBDEFS_FILE

#define net_device RTL8169Unit

#define RTL8169_TASK_NAME       "%s.task"
#define RTL8169_PORT_NAME       "%s.port"

#define PCI_VENDOR_ID_REALTEK 0x10ec
#define PCI_VENDOR_ID_DLINK 0x1186
#define PCI_VENDOR_ID_AT 0x1259
#define PCI_VENDOR_ID_USROBOTICS 0x16ec
#define PCI_VENDOR_ID_LINKSYS 0x11ad

/** Operational parameters that are set at compile time **/
#define ETH_ZLEN  60 // Min. octets in frame sans FCS

#define PCI_ANY_ID (~0)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define swab32(x) \
 ((ULONG)( \
 (((ULONG)(x) & (ULONG) 0x000000ffUL) << 24) | \
 (((ULONG)(x) & (ULONG) 0x0000ff00UL) << 8) | \
 (((ULONG)(x) & (ULONG) 0x00ff0000UL) >> 8) | \
 (((ULONG)(x) & (ULONG) 0xff000000UL) >> 24) ))

// Maximum size of the in-memory receive ring (smaller if no memory)
#define RX_FIFO_THRESH  7         // Rx buffer level before first PCI xfer

// Size of the Tx bounce buffers -- must be at least (mtu+14+4)
#define TX_BUF_SIZE     1536
#define TX_FIFO_THRESH  256       // In bytes, rounded down to 32 byte units
#define TX_DMA_BURST    6         // Calculate as 16 << val

/** Device Driver Structures **/

extern struct Library *OOPBase;

struct RTL8169Base {
    struct Device       rtl8169b_Device;

    OOP_Object          *rtl8169b_PCI;
    OOP_AttrBase        rtl8169b_PCIDeviceAttrBase;

    ULONG               rtl8169b_UnitCount;
    struct List         rtl8169b_Units;
    
    /* TODO: move into a config block */
    /* Maximum events (Rx packets, etc.) to handle at each interrupt. */
    int                 rtl8169b_MaxIntWork;
    
    /* Maximum number of multicast addresses to filter (vs. Rx-all-multicast).
    The RTL chips use a 64 element hash table based on the Ethernet CRC. */
    int                 rtl8169b_MulticastFilterLimit;

    /* media options */
    #define MAX_UNITS 8
    int                 speed[MAX_UNITS];
    int                 duplex[MAX_UNITS];
    int                 autoneg[MAX_UNITS];
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->rtl8169b_PCIDeviceAttrBase)

struct RTL8169Startup
{
    struct MsgPort           *rtl8169sm_SyncPort;
    struct RTL8169Unit       *rtl8169sm_Unit;
};

enum cfg_version
{
        RTL_CFG_0 = 0x00,
        RTL_CFG_1,
        RTL_CFG_2,
        UNKNOWN_CFG
};

enum mac_version
{
        RTL_GIGA_MAC_VER_01 = 0x01, // 8169
        RTL_GIGA_MAC_VER_02 = 0x02, // 8169S
        RTL_GIGA_MAC_VER_03 = 0x03, // 8110S
        RTL_GIGA_MAC_VER_04 = 0x04, // 8169SB
        RTL_GIGA_MAC_VER_05 = 0x05, // 8110SCd
        RTL_GIGA_MAC_VER_06 = 0x06, // 8110SCe
        RTL_GIGA_MAC_VER_11 = 0x0b, // 8168Bb
        RTL_GIGA_MAC_VER_12 = 0x0c, // 8168Be
        RTL_GIGA_MAC_VER_13 = 0x0d, // 8101Eb
        RTL_GIGA_MAC_VER_14 = 0x0e, // 8101 ?
        RTL_GIGA_MAC_VER_15 = 0x0f, // 8101 ?
        RTL_GIGA_MAC_VER_16 = 0x11, // 8101Ec
        RTL_GIGA_MAC_VER_17 = 0x10, // 8168Bf
        RTL_GIGA_MAC_VER_18 = 0x12, // 8168CP
        RTL_GIGA_MAC_VER_19 = 0x13, // 8168C
        RTL_GIGA_MAC_VER_20 = 0x14  // 8168C
};

enum
{
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

#define STAT_COUNT 3

struct RTL8169Unit {
    struct MinNode          rtl8169u_Node;

    struct RTL8169Base    *rtl8169u_device;

    STRPTR                  rtl8169u_name;
    
    ULONG                   rtl8169u_UnitNum;
    IPTR                    rtl8169u_DriverFlags;

    OOP_Object              *rtl8169u_PCIDevice;
    OOP_Object              *rtl8169u_PCIDriver;
    IPTR                    rtl8169u_IRQ;

    int                     rtl8169u_open_count;
    struct SignalSemaphore  rtl8169u_unit_lock;

    LONG                    rtl8169u_range_count;
    struct MinList          rtl8169u_Openers;
    struct MinList          rtl8169u_multicast_ranges;
    struct MinList          rtl8169u_type_trackers;

    struct timeval          rtl8169u_toutPOLL;
    BOOL                    rtl8169u_toutNEED;
    BOOL                    rtl8169u_IntsAdded;

    struct MsgPort          *rtl8169u_TimerSlowPort;
    struct timerequest      *rtl8169u_TimerSlowReq;

    struct MsgPort          *rtl8169u_TimerFastPort;
    struct timerequest      *rtl8169u_TimerFastReq;

    struct MsgPort          rtl8169u_DelayPort;
    struct timerequest      rtl8169u_DelayReq;

    int                     rtl8169u_config;
    ULONG                   rtl8169u_mtu;
    ULONG                   rtl8169u_flags;
    struct Sana2DeviceQuery  rtl8169u_Sana2Info;
    struct Sana2DeviceStats rtl8169u_stats;
    ULONG                   rtl8169u_special_stats[STAT_COUNT];

    char                    *rtl8169u_rtl_cardname;
    const char              *rtl8169u_rtl_chipname;
    ULONG                   rtl8169u_rtl_chipcapabilities;
    
    ULONG                   rtl8169u_rtl_LinkSpeed;
#define support_fdx     (1 << 0) // Supports Full Duplex
#define support_mii     (1 << 1)
#define support_fset    (1 << 2)
#define support_ltint   (1 << 3)
#define support_dxsuflo (1 << 4)
/* Card Funcs */
    void                    (*initialize)(struct RTL8169Unit *);
    void                    (*deinitialize)(struct RTL8169Unit *);
    int                     (*start)(struct RTL8169Unit *);
    int                     (*stop)(struct RTL8169Unit *);
    int                     (*alloc_rx)(struct RTL8169Unit *);
    void                    (*set_mac_address)(struct RTL8169Unit *);
    void                    (*linkchange)(struct RTL8169Unit *);
    void                    (*linkirq)(struct RTL8169Unit *);
//    ULONG                   (*descr_getlength)(struct ring_desc *prd, ULONG v);
    void                    (*set_multicast)(struct RTL8169Unit *);

    struct Process              *rtl8169u_Process;

    struct Interrupt            rtl8169u_irqhandler;
    struct Interrupt            rtl8169u_touthandler;
    IPTR                        rtl8169u_DeviceID;
    APTR                        rtl8169u_BaseMem;
    IPTR                        rtl8169u_SizeMem;
    APTR                        rtl8169u_BaseIO;

    BYTE                    rtl8169u_signal_0;
    BYTE                    rtl8169u_signal_1;
    BYTE                    rtl8169u_signal_2;
    BYTE                    rtl8169u_signal_3;

    struct MsgPort              *rtl8169u_input_port;

    struct MsgPort              *rtl8169u_request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt    rtl8169u_rx_int;
    struct Interrupt    rtl8169u_tx_int;

    ULONG                       rtl8169u_state;
    APTR                        rtl8169u_mc_list;
    int                 rtl8169u_mc_count;

    UBYTE                       rtl8169u_dev_addr[6];
    UBYTE                       rtl8169u_org_addr[6];
    struct rtl8169_priv *rtl8169u_priv;
    
    UWORD                       rtl8169u_intr_event;
    UWORD                       rtl8169u_napi_event;
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

static inline void netif_schedule(struct RTL8169Unit *unit)
{
    if (!test_bit(__LINK_STATE_XOFF, &unit->rtl8169u_state)) {
            Cause(&unit->rtl8169u_tx_int);
    }
}

static inline void netif_start_queue(struct RTL8169Unit *unit)
{
    clear_bit(__LINK_STATE_XOFF, &unit->rtl8169u_state);
}

static inline void netif_wake_queue(struct RTL8169Unit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &unit->rtl8169u_state)) {
        Cause(&unit->rtl8169u_tx_int);
    }
}

static inline void netif_stop_queue(struct RTL8169Unit *unit)
{
    set_bit(__LINK_STATE_XOFF, &unit->rtl8169u_state);
}

static inline int netif_queue_stopped(const struct RTL8169Unit *unit)
{
    return test_bit(__LINK_STATE_XOFF, &unit->rtl8169u_state);
}

static inline int netif_running(const struct RTL8169Unit *unit)
{
    return test_bit(__LINK_STATE_START, &unit->rtl8169u_state);
}

static inline int netif_carrier_ok(const struct RTL8169Unit *unit)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &unit->rtl8169u_state);
}

extern VOID ReportEvents(struct RTL8169Base *, struct RTL8169Unit *, ULONG);

static inline void netif_carrier_on(struct RTL8169Unit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &unit->rtl8169u_state)) {
        unit->rtl8169u_flags |= IFF_UP;
RTLD(bug("[%s] %s: Device set as ONLINE\n",unit->rtl8169u_name, __PRETTY_FUNCTION__))
        ReportEvents(unit->rtl8169u_device, unit, S2EVENT_ONLINE);
    }
}

static inline void netif_carrier_off(struct RTL8169Unit *unit)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &unit->rtl8169u_state)) {
        unit->rtl8169u_flags &= ~IFF_UP;
RTLD(bug("[%s] %s: Device set as OFFLINE\n",unit->rtl8169u_name, __PRETTY_FUNCTION__))
        ReportEvents(unit->rtl8169u_device, unit, S2EVENT_OFFLINE);
    }
}

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

struct pci_resource {
    UBYTE                       cmd;
    UBYTE                       cls;
    UWORD                       io_base_h;
    UWORD                       io_base_l;
    UWORD                       mem_base_h;
    UWORD                       mem_base_l;
    UBYTE                       ilr;
    UWORD                       resv_0x20_h;
    UWORD                       resv_0x20_l;
    UWORD                       resv_0x24_h;
    UWORD                       resv_0x24_l;
};

struct rtl8169_priv {
    struct RTL8169Unit          *pci_dev;

    int                         chipset;
    int                         mcfg;
    UWORD                       cp_cmd;
    unsigned                    features;

    UWORD                       intr_mask;
    UWORD                       intr_event;
    UWORD                       napi_event;

    int                         phy_auto_nego_reg;
    int                         phy_1000_ctrl_reg;

    UBYTE                       autoneg;
    UWORD                       speed;
    UBYTE                       duplex;

    struct TxDesc               *TxDescArray;   /* 256-aligned Tx descriptor ring */
    struct RxDesc               *RxDescArray;   /* 256-aligned Rx descriptor ring */
    APTR                        TxPhyAddr;
    APTR                        RxPhyAddr;

    unsigned int                rtl8169_rx_config;

    unsigned                    rx_buf_sz;
        unsigned                        tx_buf_sz;

    int                         rx_fifo_overflow;

    ULONG                       tx_tcp_csum_cmd;
    ULONG                       tx_udp_csum_cmd;
    ULONG                       tx_ip_csum_cmd;

    struct pci_resource         pci_cfg_space;

    unsigned int                pci_cfg_is_read;

    ULONG                       cur_rx; /* Index into the Rx descriptor buffer of next Rx pkt. */
    ULONG                       cur_tx; /* Index into the Tx descriptor buffer of next Rx pkt. */
    ULONG                       dirty_rx;
    ULONG                       dirty_tx;

    struct SignalSemaphore      lock;

    UBYTE                       orig_mac[6];
};

#define pci_name(unit)  (unit->rtl8169u_name)

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

/* ***************************** */
/*       RTL8169 DEFINES         */
/* ***************************** */

#ifndef DMA_64BIT_MASK
#define DMA_64BIT_MASK          0xffffffffffffffffULL
#endif
#ifndef DMA_32BIT_MASK
#define DMA_32BIT_MASK          0x00000000ffffffffULL
#endif

#ifndef PCI_COMMAND
#define PCI_COMMAND             0x04
#endif
#ifndef PCI_CACHE_LINE_SIZE
#define PCI_CACHE_LINE_SIZE     0x0c
#endif
#ifndef PCI_LATENCY_TIMER
#define PCI_LATENCY_TIMER       0x0d
#endif
#ifndef PCI_BASE_ADDRESS_0
#define PCI_BASE_ADDRESS_0      0x10
#endif
#ifndef PCI_BASE_ADDRESS_2
#define PCI_BASE_ADDRESS_2      0x18
#endif
#ifndef PCI_BASE_ADDRESS_4
#define PCI_BASE_ADDRESS_4      0x20
#endif
#ifndef PCI_BASE_ADDRESS_5
#define PCI_BASE_ADDRESS_5      0x24
#endif
#ifndef PCI_INTERRUPT_LINE
#define PCI_INTERRUPT_LINE      0x32
#endif

#ifndef ETH_ALEN
#define ETH_ALEN                ETH_ADDRESSSIZE
#endif
#ifndef ETH_HLEN
#define ETH_HLEN                ETH_HEADERSIZE
#endif

#ifndef ADVERTISED_Pause
#define ADVERTISED_Pause        (1 << 13)
#endif
#ifndef ADVERTISED_Asym_Pause
#define ADVERTISED_Asym_Pause   (1 << 14)
#endif
#ifndef ADVERTISE_PAUSE_CAP
#define ADVERTISE_PAUSE_CAP     0x400
#endif
#ifndef ADVERTISE_PAUSE_ASYM
#define ADVERTISE_PAUSE_ASYM    0x800
#endif
#ifndef MII_CTRL1000
#define MII_CTRL1000            0x09
#endif
#ifndef ADVERTISE_1000FULL
#define ADVERTISE_1000FULL      0x200
#endif
#ifndef ADVERTISE_1000HALF
#define ADVERTISE_1000HALF      0x100
#endif
#define ADVERTISE_SLCT 0x001f  /* Selector bits               */
#define ADVERTISE_CSMA 0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF 0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_10FULL 0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_100HALF 0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_100FULL 0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_100BASE4 0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_RESV 0x1c00  /* Unused...                   */
#define ADVERTISE_RFAULT 0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK 0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE 0x8000  /* Next page bit               */

/** Generic MII Registers - TODO: should be in MII header file **/

#ifndef MII_BMCR
// Basic Mode Control Register
#define MII_BMCR                0x00
#endif
#ifndef MII_ADVERTISE
// Advertisement Control Register
#define MII_ADVERTISE           0x04
#endif

/** Basic Mode Control Register - TODO: should be in MII header file **/

#ifndef BMCR_FULLDPLX
// Full Duplex
#define BMCR_FULLDPLX           0x0100
#endif
#ifndef BMCR_ANRESTART
// AutoNeg Restart
#define BMCR_ANRESTART          0x0200
#endif
#ifndef BMCR_ANENABLE
// AutoNeg Enable
#define BMCR_ANENABLE           0x1000
#endif
#ifndef BMCR_RESET
#define BMCR_RESET              0x8000
#endif

/** Advertisement Control Register - TODO: should be in MII header file **/

#ifndef ADVERTISE_10HALF
#define ADVERTISE_10HALF        0x0020
#endif
#ifndef ADVERTISE_10FULL
#define ADVERTISE_10FULL        0x0040
#endif
#ifndef ADVERTISE_100HALF
#define ADVERTISE_100HALF       0x0080
#endif
#ifndef ADVERTISE_100FULL
#define ADVERTISE_100FULL       0x0100
#endif

/* These should also have an own header */

#ifndef AUTONEG_DISABLE
#define AUTONEG_DISABLE         0x00
#endif
#ifndef AUTONEG_ENABLE
#define AUTONEG_ENABLE          0x01
#endif
#ifndef SPEED_10
#define SPEED_10                10
#endif
#ifndef SPEED_100
#define SPEED_100               100
#endif
#ifndef SPEED_1000
#define SPEED_1000              1000
#endif
#ifndef DUPLEX_HALF
#define DUPLEX_HALF             0x00
#endif
#ifndef DUPLEX_FULL
#define DUPLEX_FULL             0x01
#endif

/* write/read MMIO register */
#define RTL_R8(addr)            (*((volatile UBYTE *)(addr)))
#define RTL_R16(addr)           (*((volatile UWORD *)(addr)))
#define RTL_R32(addr)           (*((volatile ULONG *)(addr)))
#define RTL_W8(addr, val8)      MMIO_W8(addr, val8)
#define RTL_W16(addr, val16)    MMIO_W16(addr, val16)
#define RTL_W32(addr, val32)    MMIO_W32(addr, val32)

#define R8169_REGS_SIZE         256

#define MAC_ADDR_LEN            6

#define Reserved2_data  7
#define RX_DMA_BURST    6       /* Maximum PCI burst, '6' is 1024 */
#define TX_DMA_BURST_unlimited  7
#define TX_DMA_BURST_1024       6
#define TX_DMA_BURST_512        5
#define TX_DMA_BURST_256        4
#define TX_DMA_BURST_128        3
#define TX_DMA_BURST_64         2
#define TX_DMA_BURST_32         1
#define TX_DMA_BURST_16         0
#define EarlyTxThld     0x3F    /* 0x3F means NO early transmit */
#define RxPacketMaxSize 0x3FE8  /* 16K - 1 - ETH_HLEN - VLAN - CRC... */
#define Jumbo_Frame_2k  (2 * 1024)
#define Jumbo_Frame_3k  (3 * 1024)
#define Jumbo_Frame_4k  (4 * 1024)
#define Jumbo_Frame_5k  (5 * 1024)
#define Jumbo_Frame_6k  (6 * 1024)
#define Jumbo_Frame_7k  (7 * 1024)
#define Jumbo_Frame_8k  (8 * 1024)
#define Jumbo_Frame_9k  (9 * 1024)
#define InterFrameGap   0x03    /* 3 means InterFrameGap = the shortest one */

#define NUM_TX_DESC 64          /* Number of Tx descriptor registers */
#define NUM_RX_DESC     256             /* Number of Rx descriptor registers */

#define R8169_NAPI_WEIGHT 64
#define RX_BUF_SIZE     1536    /* 0x05F3 = 1523 Rx Buffer size */
#define R8169_TX_RING_BYTES     (NUM_TX_DESC * sizeof(struct TxDesc))
#define R8169_RX_RING_BYTES     (NUM_RX_DESC * sizeof(struct RxDesc))

enum RTL8169_DSM_STATE
{
    DSM_MAC_INIT = 1,
    DSM_NIC_GOTO_D3 = 2,
    DSM_IF_DOWN = 3,
    DSM_NIC_RESUME_D3 = 4,
    DSM_IF_UP = 5,
};

enum RTL8169_registers
{
    MAC0 = 0,           /* Ethernet hardware address. */
    MAC4 = 0x04,
    MAR0 = 8,           /* Multicast filter. */
    CounterAddrLow = 0x10,
    CounterAddrHigh = 0x14,
    TxDescStartAddrLow = 0x20,
    TxDescStartAddrHigh = 0x24,
    TxHDescStartAddrLow = 0x28,
    TxHDescStartAddrHigh = 0x2c,
    FLASH = 0x30,
    ERSR = 0x36,
    ChipCmd = 0x37,
    TxPoll = 0x38,
    RxBufPtr = 0x3b,
    IntrMask = 0x3C,
    IntrStatus = 0x3E,
    TxConfig = 0x40,
    RxConfig = 0x44,
    TCTR = 0x48,
        RxMissed = 0x4C,
    Cfg9346 = 0x50,
    Config0 = 0x51,
    Config1 = 0x52,
    Config2 = 0x53,
    Config3 = 0x54,
    Config4 = 0x55,
    Config5 = 0x56,
    TimeIntr = 0x58,
        MultiIntr = 0x5c,
    PHYAR = 0x60,
    TBICSR = 0x64,
    TBI_ANAR = 0x68,
        TBI_LPAR = 0x6a,
    PHYstatus = 0x6C,
    MACDBG = 0x6D,
    GPIO = 0x6E,
    FIFOTMS = 0x70,
    CSCR = 0x74,
    PARA78 = 0x78,
    PARA7c = 0x7c,
    EPHYAR = 0x80,
    DBG_reg = 0xD1,
    RxMaxSize = 0xDA,
    CPlusCmd = 0xE0,
    IntrMitigate = 0xE2,
    RxDescAddrLow = 0xE4,
    RxDescAddrHigh = 0xE8,
        EarlyTxThres = 0xEC,
    FuncEvent = 0xF0,
    FuncEventMask = 0xF4,
    FuncPresetState = 0xF8,
    FuncForceEvent = 0xFC,
};

enum RTL8169_CSCRbits
{
        CSCR_LinkOKBit = 0x400,
        CSCR_LinkDownOffCmd = 0x3c0,
        CSCR_LinkChangeBit = 0x800,
        CSCR_LinkStatusBits = 0xf000,
        CSCR_LinkDownCmd = 0xf3c0
};

enum RTL8169_register_content
{
    /* InterruptStatusBits */
    TxOutOfWindow = 0x20000000,
    TxAborted     = 0x40000000,
    TxCarrierLost = 0x80000000,
    SYSErr              = 0x8000,
    PCIErr              = 0x8000,
    TxStatOK    = 0x8000,
    PCSTimeout  = 0x4000,
    TxUnderrun  = 0x4000,
    TxHostOwns  = 0x2000,
    SWInt               = 0x0100,
    TxDescUnavail = 0x0080,
    RxFIFOOver  = 0x0040,
    LinkChg             = 0x0020,
    RxUnderrun  = 0x0020,
    RxOverflow  = 0x0010,
    TxErr               = 0x0008,
    RxTooLong   = 0x0008,
    TxOK                = 0x0004,
    RxCRCErr    = 0x0004,
    RxErr               = 0x0002,
    RxBadAlign  = 0x0002,
    RxOK                = 0x0001,
    RxStatusOK  = 0x0001,

    /* RxStatusDesc */
        RxFOVF = (1 << 23),
    RxRWT = (1 << 22),
    RxRES = (1 << 21),
    RxRUNT = (1 << 20),
    RxCRC = (1 << 19),

    /* ChipCmdBits */
    StopReq  = 0x80,
    CmdReset = 0x10,
    CmdRxEnb = 0x08,
    CmdTxEnb = 0x04,
    RxBufEmpty = 0x01,

    /* Cfg9346Bits */
    Cfg9346_Lock = 0x00,
    Cfg9346_Unlock = 0xC0,

    /* rx_mode_bits */
    AcceptErr = 0x20,
    AcceptRunt = 0x10,
    AcceptBroadcast = 0x08,
    AcceptMulticast = 0x04,
    AcceptMyPhys = 0x02,
    AcceptAllPhys = 0x01,

        /* Config2 register p. 25 */
        PCI_Clock_66MHz = 0x01,
        PCI_Clock_33MHz = 0x00,

    /* Transmit Priority Polling*/
    HPQ = 0x80,
    NPQ = 0x40,
    FSWInt = 0x01,

    /* RxConfigBits */
        RxCfgFIFOShift  = 13,
    RxCfgDMAShift = 8,
    RxCfg_128_int_en = (1 << 15),
    RxCfg_fet_multi_en = (1 << 14),
    RxCfg_half_refetch = (1 << 13),

    /* TxConfigBits */
    TxInterFrameGapShift = 24,
    TxDMAShift = 8,     /* DMA burst value (0-7) is shift this many bits */
    TxMACLoopBack = (1 << 17),  /* MAC loopback */

    /* Config1 register p.24 */
    LEDS1               = (1 << 7),
    LEDS0               = (1 << 6),
    Speed_down  = (1 << 4),
    MEMMAP              = (1 << 3),
    IOMAP               = (1 << 2),
    VPD         = (1 << 1),
        MSIEnable       = (1 << 5),     /* Enable Message Signaled Interrupt */
    PMEnable    = (1 << 0),     /* Power Management Enable */

    /* Config3 register */
    MagicPacket = (1 << 5),     /* Wake up when receives a Magic Packet */
    LinkUp              = (1 << 4),     /* This bit is reserved in RTL8168B.*/
                                    /* Wake up when the cable connection is re-established */
    ECRCEN              = (1 << 3),     /* This bit is reserved in RTL8168B*/
    Jumbo_En0   = (1 << 2),     /* This bit is reserved in RTL8168B*/
    RDY_TO_L23  = (1 << 1),     /* This bit is reserved in RTL8168B*/
    Beacon_en   = (1 << 0),     /* This bit is reserved in RTL8168B*/

    /* Config4 register */
    Jumbo_En1   = (1 << 1),     /* This bit is reserved in RTL8168B*/

    /* Config5 register */
    BWF         = (1 << 6),     /* Accept Broadcast wakeup frame */
    MWF         = (1 << 5),     /* Accept Multicast wakeup frame */
    UWF         = (1 << 4),     /* Accept Unicast wakeup frame */
    LanWake             = (1 << 1),     /* LanWake enable/disable */
    PMEStatus   = (1 << 0),     /* PME status can be reset by PCI RST# */

        /* TBICSR p.28 */
        TBIReset = 0x80000000,
        TBILoopback     = 0x40000000,
        TBINwEnable     = 0x20000000,
        TBINwRestart = 0x10000000,
        TBILinkOk = 0x02000000,
        TBINwComplete = 0x01000000,

    /* CPlusCmd */
    EnableBist  = (1 << 15),
    Macdbgo_oe  = (1 << 14),
    Normal_mode = (1 << 13),
    Force_halfdup       = (1 << 12),
    Force_rxflow_en     = (1 << 11),
    Force_txflow_en     = (1 << 10),
    Cxpl_dbg_sel        = (1 << 9),//This bit is reserved in RTL8168B
    ASF         = (1 << 8),//This bit is reserved in RTL8168C
    PktCntrDisable      = (1 << 7),
    RxVlan              = (1 << 6),
    RxChkSum    = (1 << 5),
        PCIDAC          = (1 << 4),
        PCIMulRW        = (1 << 3),
    Macdbgo_sel = 0x001C,
    INTT_0              = 0x0000,
    INTT_1              = 0x0001,
    INTT_2              = 0x0002,
    INTT_3              = 0x0003,
    
    /* rtl8169_PHYstatus */
        TBI_Enable      = 0x80,
    TxFlowCtrl = 0x40,
    RxFlowCtrl = 0x20,
    _1000bpsF = 0x10,
    _100bps = 0x08,
    _10bps = 0x04,
    LinkStatus = 0x02,
    FullDup = 0x01,

    /* DBG_reg */
    Fix_Nak_1 = (1 << 4),
    Fix_Nak_2 = (1 << 3),
    DBGPIN_E2 = (1 << 0),

    /* DumpCounterCommand */
    CounterDump = 0x8,

    /* PHY access */
    PHYAR_Flag = 0x80000000,
    PHYAR_Write = 0x80000000,
    PHYAR_Read = 0x00000000,
    PHYAR_Reg_Mask = 0x1f,
    PHYAR_Reg_shift = 16,
    PHYAR_Data_Mask = 0xffff,

    /* EPHY access */
    EPHYAR_Flag = 0x80000000,
    EPHYAR_Write = 0x80000000,
    EPHYAR_Read = 0x00000000,
    EPHYAR_Reg_Mask = 0x1f,
    EPHYAR_Reg_shift = 16,
    EPHYAR_Data_Mask = 0xffff,

    /* CSI access */    
    CSIAR_Flag = 0x80000000,
    CSIAR_Write = 0x80000000,
    CSIAR_Read = 0x00000000,
    CSIAR_ByteEn = 0x0f,
    CSIAR_ByteEn_shift = 12,
    CSIAR_Addr_Mask = 0x0fff,

    /* GPIO */
    GPIO_en = (1 << 0), 
};

enum _DescStatusBit
{
    DescOwn             = (1 << 31), /* Descriptor is owned by NIC */
    RingEnd             = (1 << 30), /* End of descriptor ring */
    FirstFrag   = (1 << 29), /* First segment of a packet */
    LastFrag    = (1 << 28), /* Final segment of a packet */

    /* Tx private */
    /*------ offset 0 of tx descriptor ------*/
    LargeSend   = (1 << 27), /* TCP Large Send Offload (TSO) */
    MSSShift    = 16,        /* MSS value position */
    MSSMask             = 0xfff,     /* MSS value + LargeSend bit: 12 bits */
    TxIPCS              = (1 << 18), /* Calculate IP checksum */
    TxUDPCS             = (1 << 17), /* Calculate UDP/IP checksum */
    TxTCPCS             = (1 << 16), /* Calculate TCP/IP checksum */
    TxVlanTag   = (1 << 17), /* Add VLAN tag */

    /*@@@@@@ offset 4 of tx descriptor => bits for RTL8168C/CP only             begin @@@@@@*/
    TxUDPCS_C   = (1 << 31), /* Calculate UDP/IP checksum */
    TxTCPCS_C   = (1 << 30), /* Calculate TCP/IP checksum */
    TxIPCS_C    = (1 << 29), /* Calculate IP checksum */
    /*@@@@@@ offset 4 of tx descriptor => bits for RTL8168C/CP only             end @@@@@@*/


    /* Rx private */
    /*------ offset 0 of rx descriptor ------*/
    PID1                = (1 << 18), /* Protocol ID bit 1/2 */
    PID0                = (1 << 17), /* Protocol ID bit 2/2 */

#define RxProtoUDP      (PID1)
#define RxProtoTCP      (PID0)
#define RxProtoIP       (PID1 | PID0)
#define RxProtoMask     RxProtoIP

    RxIPF               = (1 << 16), /* IP checksum failed */
    RxUDPF              = (1 << 15), /* UDP/IP checksum failed */
    RxTCPF              = (1 << 14), /* TCP/IP checksum failed */
    RxVlanTag   = (1 << 16), /* VLAN tag available */

    /*@@@@@@ offset 0 of rx descriptor => bits for RTL8168C/CP only             begin @@@@@@*/
    RxUDPT              = (1 << 18),
    RxTCPT              = (1 << 17),
    /*@@@@@@ offset 0 of rx descriptor => bits for RTL8168C/CP only             end @@@@@@*/

    /*@@@@@@ offset 4 of rx descriptor => bits for RTL8168C/CP only             begin @@@@@@*/
    RxV6F               = (1 << 31),
    RxV4F               = (1 << 30),
    /*@@@@@@ offset 4 of rx descriptor => bits for RTL8168C/CP only             end @@@@@@*/
};

enum features
{
    RTL_FEATURE_WOL     = (1 << 0),
    RTL_FEATURE_MSI     = (1 << 1),
};

#define RsvdMask        0x3fffc000

struct TxDesc
{
    ULONG opts1;
    ULONG opts2;
    UQUAD addr;
};

struct RxDesc
{
    ULONG opts1;
    ULONG opts2;
    UQUAD addr;
};

struct phy_reg
{
        UWORD reg;
        UWORD val;
};

struct card_def
{
    int vendorID;
    int productID;
    int sub_vendorID;
    int sub_productID;
    int config;
};

#define NBR_CARDS 11
struct card_def cards[NBR_CARDS];

void rtl8169_get_functions(struct RTL8169Unit *Unit);
void rtl8169_USecDelay(struct net_device *, ULONG);
ULONG mdio_read(struct net_device *unit, int RegAddr);
void mdio_write(struct net_device *unit, int RegAddr, UWORD value);
void rtl_phy_write(struct net_device *unit, struct phy_reg *regs, int len);
void rtl_set_rx_mode(struct net_device *unit);
UWORD rtl_rw_cpluscmd(struct net_device *unit);
void rtl_set_rx_max_size(struct net_device *unit);
void rtl_set_rx_tx_desc_registers(struct net_device *unit);
void rtl_set_rx_tx_config_registers(struct net_device *unit);
void rtl8169_write_gmii_reg_bit(struct net_device *unit, int reg, int bitnum, int bitval);
UBYTE *get_hwbase(struct net_device *unit);
struct rtl8169_priv *get_pcnpriv(struct net_device *unit);
void MMIO_W32(APTR addr, ULONG val32);
void MMIO_W16(APTR addr, UWORD val16);
void MMIO_W8(APTR addr, UBYTE val8);

#define udelay(usec) rtl8169_USecDelay(unit, usec)
#endif
