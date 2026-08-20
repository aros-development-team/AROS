#ifndef _RTL816X_H_
#define _RTL816X_H_

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
//#define RTL_DEBUG_PACKET
#endif

#define RTLD(d) \
        if (unit->rtl816xu_flags & IFF_DEBUG) \
        { \
                d; \
        }

#if defined(RTL_DEBUG_PACKET)
#define RTLDP(d) \
        if (unit->rtl816xu_flags & IFF_DEBUG) \
        { \
                d; \
        }
#else
#define RTLDP(d)
#endif

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

#define net_device RTL816XUnit

#define RTL816X_TASK_NAME       "%s.task"
#define RTL816X_PORT_NAME       "%s.port"

#define PCI_VENDOR_ID_REALTEK    0x10ec
#define PCI_VENDOR_ID_DLINK      0x1186
#define PCI_VENDOR_ID_AT         0x1259
#define PCI_VENDOR_ID_USROBOTICS 0x16ec
#define PCI_VENDOR_ID_LINKSYS    0x1737

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

// Rx buffer level before first PCI xfer
#define RX_FIFO_THRESH  7

// Size of the Tx bounce buffers -- must be at least (mtu+14+4)
#define TX_BUF_SIZE     1536

/** Device Driver Structures **/

extern struct Library *OOPBase;

struct RTL816XBase {
    struct Device       rtl816xb_Device;

    OOP_Object          *rtl816xb_PCI;
    OOP_AttrBase        rtl816xb_PCIDeviceAttrBase;

    ULONG               rtl816xb_UnitCount;
    struct List         rtl816xb_Units;

    /* Maximum events (Rx packets, etc.) to handle at each interrupt. */
    int                 rtl816xb_MaxIntWork;

    /* Maximum number of multicast addresses to filter (vs. Rx-all-multicast). */
    int                 rtl816xb_MulticastFilterLimit;

    /* media options */
    #define MAX_UNITS 8
    int                 speed[MAX_UNITS];
    int                 duplex[MAX_UNITS];
    int                 autoneg[MAX_UNITS];
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->rtl816xb_PCIDeviceAttrBase)

/* Handshake between CreateUnit() and the unit process. The parent owns this
   structure and frees it as soon as the child signals SIGF_SINGLE, so the
   child must not touch it after signalling. Setup failure is reported by
   the child clearing rtl816xsm_Unit before the signal. */
struct RTL816XStartup
{
    struct Task              *rtl816xsm_Parent;
    struct RTL816XUnit       *rtl816xsm_Unit;
};

enum cfg_version
{
        RTL_CFG_0 = 0x00,       /* 8169/8110 */
        RTL_CFG_1,              /* 8168/8111 */
        RTL_CFG_2,              /* 8101/8102/8136 */
        UNKNOWN_CFG
};

/* MAC versions, numbered to match the Linux r8169 scheme where an
   equivalent exists so that references translate directly. */
enum mac_version
{
        RTL_GIGA_MAC_NONE   = 0x00,
        RTL_GIGA_MAC_VER_01 = 0x01, // 8169
        RTL_GIGA_MAC_VER_02 = 0x02, // 8169S
        RTL_GIGA_MAC_VER_03 = 0x03, // 8110S
        RTL_GIGA_MAC_VER_04 = 0x04, // 8169SB
        RTL_GIGA_MAC_VER_05 = 0x05, // 8110SCd
        RTL_GIGA_MAC_VER_06 = 0x06, // 8110SCe
        RTL_GIGA_MAC_VER_11 = 0x0b, // 8168Bb
        RTL_GIGA_MAC_VER_12 = 0x0c, // 8168Be
        RTL_GIGA_MAC_VER_13 = 0x0d, // 8101Eb
        RTL_GIGA_MAC_VER_14 = 0x0e, // 8100E
        RTL_GIGA_MAC_VER_15 = 0x0f, // 8100E
        RTL_GIGA_MAC_VER_16 = 0x10, // 8101Ec
        RTL_GIGA_MAC_VER_17 = 0x11, // 8168Bf
        RTL_GIGA_MAC_VER_18 = 0x12, // 8168CP-1
        RTL_GIGA_MAC_VER_19 = 0x13, // 8168C-1
        RTL_GIGA_MAC_VER_20 = 0x14, // 8168C-2
        RTL_GIGA_MAC_VER_21 = 0x15, // 8168C-3
        RTL_GIGA_MAC_VER_24 = 0x18, // 8168CP-2
        RTL_GIGA_MAC_VER_25 = 0x19, // 8168D-1
        RTL_GIGA_MAC_VER_26 = 0x1a, // 8168D-2
        RTL_GIGA_MAC_VER_27 = 0x1b, // 8168D-3
        RTL_GIGA_MAC_VER_28 = 0x1c, // 8168DP
        RTL_GIGA_MAC_VER_32 = 0x20, // 8168E-1
        RTL_GIGA_MAC_VER_33 = 0x21, // 8168E-2
        RTL_GIGA_MAC_VER_34 = 0x22, // 8168E-VL
        RTL_GIGA_MAC_VER_35 = 0x23, // 8168F-1
        RTL_GIGA_MAC_VER_36 = 0x24, // 8168F-2
        RTL_GIGA_MAC_VER_38 = 0x26, // 8411
        RTL_GIGA_MAC_VER_40 = 0x28, // 8168G-1
        RTL_GIGA_MAC_VER_41 = 0x29, // 8168G-2
        RTL_GIGA_MAC_VER_42 = 0x2a, // 8168GU
        RTL_GIGA_MAC_VER_44 = 0x2c, // 8411B
        RTL_GIGA_MAC_VER_45 = 0x2d, // 8168H-1
        RTL_GIGA_MAC_VER_46 = 0x2e, // 8168H-2
        RTL_GIGA_MAC_VER_49 = 0x31, // 8168EP
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

struct RTL816XUnit {
    struct MinNode          rtl816xu_Node;

    struct RTL816XBase      *rtl816xu_device;

    STRPTR                  rtl816xu_name;

    ULONG                   rtl816xu_UnitNum;

    OOP_Object              *rtl816xu_PCIDevice;
    OOP_Object              *rtl816xu_PCIDriver;
    IPTR                    rtl816xu_IRQ;
    BOOL                    rtl816xu_Owned;

    int                     rtl816xu_open_count;
    struct SignalSemaphore  rtl816xu_unit_lock;

    LONG                    rtl816xu_range_count;
    struct MinList          rtl816xu_Openers;
    struct MinList          rtl816xu_multicast_ranges;
    struct MinList          rtl816xu_type_trackers;

    BOOL                    rtl816xu_IntsAdded;

    struct MsgPort          *rtl816xu_TimerSlowPort;
    struct timerequest      *rtl816xu_TimerSlowReq;

    struct MsgPort          rtl816xu_DelayPort;
    struct timerequest      rtl816xu_DelayReq;

    int                     rtl816xu_config;
    ULONG                   rtl816xu_mtu;
    ULONG                   rtl816xu_flags;
    struct Sana2DeviceQuery rtl816xu_Sana2Info;
    struct Sana2DeviceStats rtl816xu_stats;
    ULONG                   rtl816xu_special_stats[STAT_COUNT];

    const char              *rtl816xu_rtl_chipname;

    ULONG                   rtl816xu_rtl_LinkSpeed;

/* Card Funcs */
    void                    (*initialize)(struct RTL816XUnit *);
    void                    (*deinitialize)(struct RTL816XUnit *);
    int                     (*start)(struct RTL816XUnit *);
    int                     (*stop)(struct RTL816XUnit *);
    void                    (*set_mac_address)(struct RTL816XUnit *);
    void                    (*set_multicast)(struct RTL816XUnit *);

    struct Process          *rtl816xu_Process;

    struct Interrupt        rtl816xu_irqhandler;
    APTR                    rtl816xu_BaseMem;
    IPTR                    rtl816xu_SizeMem;
    APTR                    rtl816xu_BaseIO;

    BYTE                    rtl816xu_signal_0;
    BYTE                    rtl816xu_signal_1;
    BYTE                    rtl816xu_signal_2;
    BYTE                    rtl816xu_signal_3;

    struct MsgPort          *rtl816xu_input_port;

    struct MsgPort          *rtl816xu_request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt        rtl816xu_tx_int;

    ULONG                   rtl816xu_state;

    UBYTE                   rtl816xu_dev_addr[6];
    UBYTE                   rtl816xu_org_addr[6];
    struct rtl816x_priv     *rtl816xu_priv;

    UWORD                   rtl816xu_intr_event;
};

void handle_request(LIBBASETYPEPTR, struct IOSana2Req *);

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

#define IFF_MULTICAST   0x1000          /* Supports multicast           */

#define IFF_SHARED      0x10000         /* interface may be shared */
#define IFF_CONFIGURED  0x20000         /* interface already configured */

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

static inline void netif_start_queue(struct RTL816XUnit *unit)
{
    clear_bit(__LINK_STATE_XOFF, &unit->rtl816xu_state);
}

static inline void netif_wake_queue(struct RTL816XUnit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &unit->rtl816xu_state)) {
        Cause(&unit->rtl816xu_tx_int);
    }
}

static inline void netif_stop_queue(struct RTL816XUnit *unit)
{
    set_bit(__LINK_STATE_XOFF, &unit->rtl816xu_state);
}

static inline int netif_queue_stopped(const struct RTL816XUnit *unit)
{
    return test_bit(__LINK_STATE_XOFF, &unit->rtl816xu_state);
}

static inline int netif_carrier_ok(const struct RTL816XUnit *unit)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &unit->rtl816xu_state);
}

extern VOID ReportEvents(struct RTL816XBase *, struct RTL816XUnit *, ULONG);

static inline void netif_carrier_on(struct RTL816XUnit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &unit->rtl816xu_state)) {
RTLD(bug("[%s] %s: link up\n",unit->rtl816xu_name, __func__))
        ReportEvents(unit->rtl816xu_device, unit, S2EVENT_ONLINE);
    }
}

static inline void netif_carrier_off(struct RTL816XUnit *unit)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &unit->rtl816xu_state)) {
RTLD(bug("[%s] %s: link down\n",unit->rtl816xu_name, __func__))
        ReportEvents(unit->rtl816xu_device, unit, S2EVENT_OFFLINE);
    }
}

struct rtl816x_priv {
    struct RTL816XUnit          *pci_dev;

    int                         mcfg;
    UWORD                       cp_cmd;

    UWORD                       intr_event;

    UBYTE                       autoneg;
    UWORD                       speed;
    UBYTE                       duplex;

    struct TxDesc               *TxDescArray;   /* 256-aligned Tx descriptor ring */
    struct RxDesc               *RxDescArray;   /* 256-aligned Rx descriptor ring */
    APTR                        TxPhyAddr;
    APTR                        RxPhyAddr;

    unsigned                    rx_buf_sz;

    ULONG                       cur_rx; /* Next Rx descriptor to look at */
    ULONG                       cur_tx; /* Next free Tx descriptor */
    ULONG                       dirty_tx;

    struct SignalSemaphore      lock;

    UBYTE                       orig_mac[6];
};

#define pci_name(unit)  (unit->rtl816xu_name)

/* ENET defines */

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
/*       RTL816X DEFINES         */
/* ***************************** */

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

#ifndef ETH_ALEN
#define ETH_ALEN                ETH_ADDRESSSIZE
#endif
#ifndef ETH_HLEN
#define ETH_HLEN                ETH_HEADERSIZE
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
#define ADVERTISE_CSMA          0x0001
#define ADVERTISE_10HALF        0x0020
#define ADVERTISE_10FULL        0x0040
#define ADVERTISE_100HALF       0x0080
#define ADVERTISE_100FULL       0x0100

/** Generic MII Registers **/

#ifndef MII_BMCR
#define MII_BMCR                0x00
#endif
#ifndef MII_ADVERTISE
#define MII_ADVERTISE           0x04
#endif

#ifndef BMCR_FULLDPLX
#define BMCR_FULLDPLX           0x0100
#endif
#ifndef BMCR_ANRESTART
#define BMCR_ANRESTART          0x0200
#endif
#ifndef BMCR_ANENABLE
#define BMCR_ANENABLE           0x1000
#endif
#ifndef BMCR_RESET
#define BMCR_RESET              0x8000
#endif

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

#define R816X_REGS_SIZE         256

#define MAC_ADDR_LEN            6

#define Reserved2_data  7
#define RX_DMA_BURST_1024       6
#define RX_DMA_BURST_unlimited  7
#define TX_DMA_BURST_unlimited  7
#define TX_DMA_BURST_1024       6
#define TX_DMA_BURST_512        5
#define EarlyTxThld     0x3F    /* 0x3F means NO early transmit */
#define InterFrameGap   0x03    /* 3 means InterFrameGap = the shortest one */

#define NUM_TX_DESC     64      /* Number of Tx descriptors */
#define NUM_RX_DESC     256     /* Number of Rx descriptors */

#define RX_BUF_SIZE     1536
#define R816X_TX_RING_BYTES     (NUM_TX_DESC * sizeof(struct TxDesc))
#define R816X_RX_RING_BYTES     (NUM_RX_DESC * sizeof(struct RxDesc))

/* All chips handled here share this RxConfig reserved-bit mask */
#define RTL_RX_CONFIG_MASK      0xff7e1880

enum RTL816X_registers
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
    EPHYAR = 0x80,
    CSIDR = 0x64,       /* 8168 family only, overlays TBICSR */
    CSIAR = 0x68,       /* 8168 family only, overlays TBI_ANAR */
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

enum RTL816X_register_content
{
    /* InterruptStatusBits */
    SYSErr        = 0x8000,
    PCSTimeout    = 0x4000,
    SWInt         = 0x0100,
    TxDescUnavail = 0x0080,
    RxFIFOOver    = 0x0040,
    LinkChg       = 0x0020,
    RxOverflow    = 0x0010,     /* aka RxDescUnavail */
    TxErr         = 0x0008,
    TxOK          = 0x0004,
    RxErr         = 0x0002,
    RxOK          = 0x0001,

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

    /* Config2 register */
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
    RxCfg_early_off = (1 << 11),

    /* TxConfigBits */
    TxInterFrameGapShift = 24,
    TxDMAShift = 8,     /* DMA burst value (0-7) is shift this many bits */
    TxMACLoopBack = (1 << 17),  /* MAC loopback */

    /* Config1 register */
    LEDS1       = (1 << 7),
    LEDS0       = (1 << 6),
    Speed_down  = (1 << 4),
    MEMMAP      = (1 << 3),
    IOMAP       = (1 << 2),
    VPD         = (1 << 1),
    PMEnable    = (1 << 0),     /* Power Management Enable */

    /* Config3 register */
    MagicPacket = (1 << 5),     /* Wake up when receives a Magic Packet */
    LinkUp      = (1 << 4),
    ECRCEN      = (1 << 3),
    Jumbo_En0   = (1 << 2),
    RDY_TO_L23  = (1 << 1),
    Beacon_en   = (1 << 0),

    /* Config4 register */
    Jumbo_En1   = (1 << 1),

    /* Config5 register */
    BWF         = (1 << 6),     /* Accept Broadcast wakeup frame */
    MWF         = (1 << 5),     /* Accept Multicast wakeup frame */
    UWF         = (1 << 4),     /* Accept Unicast wakeup frame */
    LanWake     = (1 << 1),     /* LanWake enable/disable */
    PMEStatus   = (1 << 0),     /* PME status can be reset by PCI RST# */

    /* TBICSR */
    TBIReset = 0x80000000,
    TBILoopback = 0x40000000,
    TBINwEnable = 0x20000000,
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
    Cxpl_dbg_sel        = (1 << 9),
    ASF         = (1 << 8),
    PktCntrDisable      = (1 << 7),
    RxVlan      = (1 << 6),
    RxChkSum    = (1 << 5),
    PCIDAC      = (1 << 4),
    PCIMulRW    = (1 << 3),
    Macdbgo_sel = 0x001C,
    INTT_0      = 0x0000,
    INTT_1      = 0x0001,
    INTT_2      = 0x0002,
    INTT_3      = 0x0003,

    /* PHYstatus */
    TBI_Enable = 0x80,
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
    DescOwn     = (1 << 31), /* Descriptor is owned by NIC */
    RingEnd     = (1 << 30), /* End of descriptor ring */
    FirstFrag   = (1 << 29), /* First segment of a packet */
    LastFrag    = (1 << 28), /* Final segment of a packet */
};

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
    int config;
};

extern const struct card_def cards[];

void rtl816x_get_functions(struct RTL816XUnit *Unit);
void rtl816x_USecDelay(struct net_device *, ULONG);
void rtl816x_CheckLinkStatus(struct net_device *);
void rtl816x_irq_mask_and_ack(struct net_device *);
ULONG mdio_read(struct net_device *unit, int RegAddr);
void mdio_write(struct net_device *unit, int RegAddr, UWORD value);
void rtl_phy_write(struct net_device *unit, const struct phy_reg *regs, int len);
UWORD rtl_ephy_read(struct net_device *unit, int RegAddr);
void rtl_ephy_write(struct net_device *unit, int RegAddr, UWORD value);
ULONG rtl_csi_read(struct net_device *unit, int addr);
void rtl_csi_write(struct net_device *unit, int addr, ULONG value);
void rtl_set_rx_mode(struct net_device *unit);
void rtl_set_rx_max_size(struct net_device *unit);
void rtl_set_rx_tx_desc_registers(struct net_device *unit);
void rtl_set_rx_tx_config_registers(struct net_device *unit);
void rtl_pcie_maxread_tweak(struct net_device *unit, int cfgoff, UBYTE val);
void rtl8169_write_gmii_reg_bit(struct net_device *unit, int reg, int bitnum, int bitval);
UBYTE *get_hwbase(struct net_device *unit);
struct rtl816x_priv *get_pcnpriv(struct net_device *unit);
void MMIO_W32(APTR addr, ULONG val32);
void MMIO_W16(APTR addr, UWORD val16);
void MMIO_W8(APTR addr, UBYTE val8);

#define udelay(usec) rtl816x_USecDelay(unit, usec)
#endif
