#ifndef _NFORCE_H
#define _NFORCE_H

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

#define NFORCE_TASK_NAME	"NForce task"
#define NFORCE_PORT_NAME	"NForce port"

struct NFBase {
    struct Device       nf_Device;
    struct MsgPort      *nf_syncport;

    OOP_Object          *nf_pci;
    OOP_Object          *nf_irq;
    OOP_AttrBase        nf_pciDeviceAttrBase;

    struct Sana2DeviceQuery nf_Sana2Info;
    struct NFUnit       *nf_unit;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->nf_pciDeviceAttrBase)

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
struct ring_desc {
    IPTR PacketBuffer;
    IPTR FlagLen;
};


#define STAT_COUNT 3

struct NFUnit {
    struct MinNode          *nu_Node;
    struct MinList          nu_Openers;
    struct MinList          multicast_ranges;
    struct MinList          type_trackers;
    ULONG                   nu_UnitNum;
    LONG                    range_count;

    OOP_Object              *nu_PCIDevice;
    OOP_Object              *nu_PCIDriver;

    struct timeval          nu_toutPOLL;
    BOOL                    nu_toutNEED;
    BOOL                    nu_IntsAdded;

    struct MsgPort          *nu_TimerSlowPort;
    struct timerequest      *nu_TimerSlowReq;

    struct MsgPort          *nu_TimerFastPort;
    struct timerequest      *nu_TimerFastReq;

    struct Sana2DeviceStats stats;
    ULONG                   special_stats[STAT_COUNT];

    void                    (*initialize)(struct NFUnit *);
    void                    (*deinitialize)(struct NFUnit *);
    int                     (*start)(struct NFUnit *);
    int                     (*stop)(struct NFUnit *);
    int                     (*alloc_rx)(struct NFUnit *);
    void                    (*set_mac_address)(struct NFUnit *);
    void                    (*linkchange)(struct NFUnit *);
    void                    (*linkirq)(struct NFUnit *);
    ULONG                   (*descr_getlength)(struct ring_desc *prd, ULONG v);
    void                    (*set_multicast)(struct NFUnit *);

    int                     open_count;
    struct SignalSemaphore  unit_lock;

    struct Process          *nu_Process;

    struct NFBase           *nu_device;
    struct Interrupt        nu_irqhandler;
    struct Interrupt        nu_touthandler;
    IPTR	                nu_DeviceID;
    IPTR                    nu_DriverFlags;
    IPTR                    nu_IRQ;
    IPTR                    nu_BaseMem;
    IPTR                    nu_SizeMem;
    IPTR	                nu_BaseIO;

    BYTE                    nu_signal_0;
    BYTE                    nu_signal_1;
    BYTE                    nu_signal_2;
    BYTE                    nu_signal_3;

    struct MsgPort          *nu_input_port;

    struct MsgPort          *request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt        rx_int;
    struct Interrupt        tx_int;
    struct Interrupt        tx_end_int;

    STRPTR                  name;
    ULONG                   mtu;
    ULONG                   flags;
    ULONG                   state;
    APTR                    mc_list;
    UBYTE                   dev_addr[6];
    UBYTE                   org_addr[6];
    struct fe_priv          *nu_fe_priv;
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

static inline void netif_schedule(struct NFUnit *dev)
{
    if (!test_bit(__LINK_STATE_XOFF, &dev->state)) {
        Cause(&dev->tx_int);
    }
}


static inline void netif_start_queue(struct NFUnit *dev)
{
    clear_bit(__LINK_STATE_XOFF, &dev->state);
}

static inline void netif_wake_queue(struct NFUnit *dev)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &dev->state)) {
        Cause(&dev->tx_int);
    }
}

static inline void netif_stop_queue(struct NFUnit *dev)
{
    set_bit(__LINK_STATE_XOFF, &dev->state);
}

static inline int netif_queue_stopped(const struct NFUnit *dev)
{
    return test_bit(__LINK_STATE_XOFF, &dev->state);
}

static inline int netif_running(const struct NFUnit *dev)
{
    return test_bit(__LINK_STATE_START, &dev->state);
}

static inline int netif_carrier_ok(const struct NFUnit *dev)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &dev->state);
}

extern void __netdev_watchdog_up(struct NFUnit *dev);

static inline void netif_carrier_on(struct NFUnit *dev)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
//                linkwatch_fire_event(dev);
    }
    if (netif_running(dev)) {
//                __netdev_watchdog_up(dev);
    }
}

static inline void netif_carrier_off(struct NFUnit *dev)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &dev->state)) {
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

struct fe_priv {
    struct NFUnit   *pci_dev;
    int in_shutdown;
    ULONG linkspeed;
    int duplex;
    int autoneg;
    int fixed_mode;
    int phyaddr;
    int wolenabled;
    unsigned int phy_oui;
    UWORD gigabit;
    ULONG desc_ver;
    struct SignalSemaphore  lock;

    IPTR ring_addr;
    struct eth_frame *rx_buffer;
    struct eth_frame *tx_buffer;

    struct ring_desc *rx_ring;
    ULONG cur_rx, refill_rx;

    struct ring_desc *tx_ring;
    ULONG next_tx, nic_tx;
    ULONG tx_flags;

    ULONG irqmask;
    ULONG need_linktimer;
    struct timeval link_timeout;
    ULONG orig_mac[2];
};

#define pci_name(unit)  (unit->name)

/* NFORCE ENET defines */

#define HZ              1000000
#define ETH_DATA_LEN    1500

#define ETH_ADDRESSSIZE 6
#define ETH_HEADERSIZE 14
#define ETH_MTU (ETH_DATA_LEN)
#define ETH_MAXPACKETSIZE ((ETH_HEADERSIZE) + (ETH_MTU))

#define ETH_PACKET_DEST 0
#define ETH_PACKET_SOURCE 6
#define ETH_PACKET_TYPE 12
#define ETH_PACKET_IEEELEN 12
#define ETH_PACKET_SNAPTYPE 20
#define ETH_PACKET_DATA 14

#define NFORCE_MCPNET1_ID       0x01c3
#define NFORCE_MCPNET2_ID       0x0066
#define NFORCE_MCPNET3_ID       0x00d6
#define NFORCE_MCPNET4_ID       0x0086
#define NFORCE_MCPNET5_ID       0x008c
#define NFORCE_MCPNET6_ID       0x00e6
#define NFORCE_MCPNET7_ID       0x00df
#define NFORCE_MCPNET8_ID       0x0056
#define NFORCE_MCPNET9_ID       0x0057
#define NFORCE_MCPNET10_ID      0x0037
#define NFORCE_MCPNET11_ID      0x0038

/*
 * Hardware access:
 */

#define DEV_NEED_LASTPACKET1    0x0001  /* set LASTPACKET1 in tx flags */
#define DEV_IRQMASK_1           0x0002  /* use NVREG_IRQMASK_WANTED_1 for irq mask */
#define DEV_IRQMASK_2           0x0004  /* use NVREG_IRQMASK_WANTED_2 for irq mask */
#define DEV_NEED_TIMERIRQ       0x0008  /* set the timer irq flag in the irq mask */
#define DEV_NEED_LINKTIMER      0x0010  /* poll link settings. Relies on the timer irq */

enum {
        NvRegIrqStatus = 0x000,
#define NVREG_IRQSTAT_MIIEVENT  0x040
#define NVREG_IRQSTAT_MASK              0x1ff
        NvRegIrqMask = 0x004,
#define NVREG_IRQ_RX_ERROR              0x0001
#define NVREG_IRQ_RX                    0x0002
#define NVREG_IRQ_RX_NOBUF              0x0004
#define NVREG_IRQ_TX_ERR                0x0008
#define NVREG_IRQ_TX2                   0x0010
#define NVREG_IRQ_TIMER                 0x0020
#define NVREG_IRQ_LINK                  0x0040
#define NVREG_IRQ_TX1                   0x0100
#define NVREG_IRQMASK_WANTED_1          0x005f
#define NVREG_IRQMASK_WANTED_2          0x0147
#define NVREG_IRQ_UNKNOWN               (~(NVREG_IRQ_RX_ERROR|NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF|NVREG_IRQ_TX_ERR|NVREG_IRQ_TX2|NVREG_IRQ_TIMER|NVREG_IRQ_LINK|NVREG_IRQ_TX1))

        NvRegUnknownSetupReg6 = 0x008,
#define NVREG_UNKSETUP6_VAL             3

/*
 * NVREG_POLL_DEFAULT is the interval length of the timer source on the nic
 * NVREG_POLL_DEFAULT=97 would result in an interval length of 1 ms
 */
        NvRegPollingInterval = 0x00c,
#define NVREG_POLL_DEFAULT      970
        NvRegMisc1 = 0x080,
#define NVREG_MISC1_HD          0x02
#define NVREG_MISC1_FORCE       0x3b0f3c

        NvRegTransmitterControl = 0x084,
#define NVREG_XMITCTL_START     0x01
        NvRegTransmitterStatus = 0x088,
#define NVREG_XMITSTAT_BUSY     0x01

        NvRegPacketFilterFlags = 0x8c,
#define NVREG_PFF_ALWAYS        0x7F0008
#define NVREG_PFF_PROMISC       0x80
#define NVREG_PFF_MYADDR        0x20

        NvRegOffloadConfig = 0x90,
#define NVREG_OFFLOAD_HOMEPHY   0x601
#define NVREG_OFFLOAD_NORMAL    RX_NIC_BUFSIZE
        NvRegReceiverControl = 0x094,
#define NVREG_RCVCTL_START      0x01
        NvRegReceiverStatus = 0x98,
#define NVREG_RCVSTAT_BUSY      0x01

        NvRegRandomSeed = 0x9c,
#define NVREG_RNDSEED_MASK      0x00ff
#define NVREG_RNDSEED_FORCE     0x7f00
#define NVREG_RNDSEED_FORCE2    0x2d00
#define NVREG_RNDSEED_FORCE3    0x7400
        NvRegUnknownSetupReg1 = 0xA0,
#define NVREG_UNKSETUP1_VAL     0x16070f
        NvRegUnknownSetupReg2 = 0xA4,
#define NVREG_UNKSETUP2_VAL     0x16
        NvRegMacAddrA = 0xA8,
        NvRegMacAddrB = 0xAC,
        NvRegMulticastAddrA = 0xB0,
#define NVREG_MCASTADDRA_FORCE  0x01
        NvRegMulticastAddrB = 0xB4,
        NvRegMulticastMaskA = 0xB8,
        NvRegMulticastMaskB = 0xBC,

        NvRegPhyInterface = 0xC0,
#define PHY_RGMII               0x10000000

        NvRegTxRingPhysAddr = 0x100,
        NvRegRxRingPhysAddr = 0x104,
        NvRegRingSizes = 0x108,
#define NVREG_RINGSZ_TXSHIFT 0
#define NVREG_RINGSZ_RXSHIFT 16
        NvRegUnknownTransmitterReg = 0x10c,
        NvRegLinkSpeed = 0x110,
#define NVREG_LINKSPEED_FORCE 0x10000
#define NVREG_LINKSPEED_10      1000
#define NVREG_LINKSPEED_100     100
#define NVREG_LINKSPEED_1000    50
#define NVREG_LINKSPEED_MASK    (0xFFF)
        NvRegUnknownSetupReg5 = 0x130,
#define NVREG_UNKSETUP5_BIT31   (1<<31)
        NvRegUnknownSetupReg3 = 0x13c,
#define NVREG_UNKSETUP3_VAL1    0x200010
        NvRegTxRxControl = 0x144,
#define NVREG_TXRXCTL_KICK      0x0001
#define NVREG_TXRXCTL_BIT1      0x0002
#define NVREG_TXRXCTL_BIT2      0x0004
#define NVREG_TXRXCTL_IDLE      0x0008
#define NVREG_TXRXCTL_RESET     0x0010
#define NVREG_TXRXCTL_RXCHECK   0x0400
        NvRegMIIStatus = 0x180,
#define NVREG_MIISTAT_ERROR             0x0001
#define NVREG_MIISTAT_LINKCHANGE        0x0008
#define NVREG_MIISTAT_MASK              0x000f
#define NVREG_MIISTAT_MASK2             0x000f
        NvRegUnknownSetupReg4 = 0x184,
#define NVREG_UNKSETUP4_VAL     8

        NvRegAdapterControl = 0x188,
#define NVREG_ADAPTCTL_START    0x02
#define NVREG_ADAPTCTL_LINKUP   0x04
#define NVREG_ADAPTCTL_PHYVALID 0x40000
#define NVREG_ADAPTCTL_RUNNING  0x100000
#define NVREG_ADAPTCTL_PHYSHIFT 24
        NvRegMIISpeed = 0x18c,
#define NVREG_MIISPEED_BIT8     (1<<8)
#define NVREG_MIIDELAY  5
        NvRegMIIControl = 0x190,
#define NVREG_MIICTL_INUSE      0x08000
#define NVREG_MIICTL_WRITE      0x00400
#define NVREG_MIICTL_ADDRSHIFT  5
        NvRegMIIData = 0x194,
        NvRegWakeUpFlags = 0x200,
#define NVREG_WAKEUPFLAGS_VAL           0x7770
#define NVREG_WAKEUPFLAGS_BUSYSHIFT     24
#define NVREG_WAKEUPFLAGS_ENABLESHIFT   16
#define NVREG_WAKEUPFLAGS_D3SHIFT       12
#define NVREG_WAKEUPFLAGS_D2SHIFT       8
#define NVREG_WAKEUPFLAGS_D1SHIFT       4
#define NVREG_WAKEUPFLAGS_D0SHIFT       0
#define NVREG_WAKEUPFLAGS_ACCEPT_MAGPAT         0x01
#define NVREG_WAKEUPFLAGS_ACCEPT_WAKEUPPAT      0x02
#define NVREG_WAKEUPFLAGS_ACCEPT_LINKCHANGE     0x04
#define NVREG_WAKEUPFLAGS_ENABLE        0x1111

        NvRegPatternCRC = 0x204,
        NvRegPatternMask = 0x208,
        NvRegPowerCap = 0x268,
#define NVREG_POWERCAP_D3SUPP   (1<<30)
#define NVREG_POWERCAP_D2SUPP   (1<<26)
#define NVREG_POWERCAP_D1SUPP   (1<<25)
        NvRegPowerState = 0x26c,
#define NVREG_POWERSTATE_POWEREDUP      0x8000
#define NVREG_POWERSTATE_VALID          0x0100
#define NVREG_POWERSTATE_MASK           0x0003
#define NVREG_POWERSTATE_D0             0x0000
#define NVREG_POWERSTATE_D1             0x0001
#define NVREG_POWERSTATE_D2             0x0002
#define NVREG_POWERSTATE_D3             0x0003
};

#define FLAG_MASK_V1 0xffff0000
#define FLAG_MASK_V2 0xffffc000
#define LEN_MASK_V1 (0xffffffff ^ FLAG_MASK_V1)
#define LEN_MASK_V2 (0xffffffff ^ FLAG_MASK_V2)

#define NV_TX_LASTPACKET        (1<<16)
#define NV_TX_RETRYERROR        (1<<19)
#define NV_TX_LASTPACKET1       (1<<24)
#define NV_TX_DEFERRED          (1<<26)
#define NV_TX_CARRIERLOST       (1<<27)
#define NV_TX_LATECOLLISION     (1<<28)
#define NV_TX_UNDERFLOW         (1<<29)
#define NV_TX_ERROR             (1<<30)
#define NV_TX_VALID             (1<<31)

#define NV_TX2_LASTPACKET       (1<<29)
#define NV_TX2_RETRYERROR       (1<<18)
#define NV_TX2_LASTPACKET1      (1<<23)
#define NV_TX2_DEFERRED         (1<<25)
#define NV_TX2_CARRIERLOST      (1<<26)
#define NV_TX2_LATECOLLISION    (1<<27)
#define NV_TX2_UNDERFLOW        (1<<28)
/* error and valid are the same for both */
#define NV_TX2_ERROR            (1<<30)
#define NV_TX2_VALID            (1<<31)

#define NV_RX_DESCRIPTORVALID   (1<<16)
#define NV_RX_MISSEDFRAME       (1<<17)
#define NV_RX_SUBSTRACT1        (1<<18)
#define NV_RX_ERROR1            (1<<23)
#define NV_RX_ERROR2            (1<<24)
#define NV_RX_ERROR3            (1<<25)
#define NV_RX_ERROR4            (1<<26)
#define NV_RX_CRCERR            (1<<27)
#define NV_RX_OVERFLOW          (1<<28)
#define NV_RX_FRAMINGERR        (1<<29)
#define NV_RX_ERROR             (1<<30)
#define NV_RX_AVAIL             (1<<31)

#define NV_RX2_CHECKSUMMASK     (0x1C000000)
#define NV_RX2_CHECKSUMOK1      (0x10000000)
#define NV_RX2_CHECKSUMOK2      (0x14000000)
#define NV_RX2_CHECKSUMOK3      (0x18000000)
#define NV_RX2_DESCRIPTORVALID  (1<<29)
#define NV_RX2_SUBSTRACT1       (1<<25)
#define NV_RX2_ERROR1           (1<<18)
#define NV_RX2_ERROR2           (1<<19)
#define NV_RX2_ERROR3           (1<<20)
#define NV_RX2_ERROR4           (1<<21)
#define NV_RX2_CRCERR           (1<<22)
#define NV_RX2_OVERFLOW         (1<<23)
#define NV_RX2_FRAMINGERR       (1<<24)
/* error and avail are the same for both */
#define NV_RX2_ERROR            (1<<30)
#define NV_RX2_AVAIL            (1<<31)

/* Miscelaneous hardware related defines: */
#define NV_PCI_REGSZ            0x270

/* various timeout delays: all in usec */
#define NV_TXRX_RESET_DELAY     4
#define NV_TXSTOP_DELAY1        10
#define NV_TXSTOP_DELAY1MAX     500000
#define NV_TXSTOP_DELAY2        100
#define NV_RXSTOP_DELAY1        10
#define NV_RXSTOP_DELAY1MAX     500000
#define NV_RXSTOP_DELAY2        100
#define NV_SETUP5_DELAY         5
#define NV_SETUP5_DELAYMAX      50000
#define NV_POWERUP_DELAY        5
#define NV_POWERUP_DELAYMAX     5000
#define NV_MIIBUSY_DELAY        50
#define NV_MIIPHY_DELAY 10
#define NV_MIIPHY_DELAYMAX      10000

#define NV_WAKEUPPATTERNS       5
#define NV_WAKEUPMASKENTRIES    4

/* General driver defaults */
#define NV_WATCHDOG_TIMEO       (5*HZ)

#define RX_RING         128
#define TX_RING         64
/*
 * If your nic mysteriously hangs then try to reduce the limits
 * to 1/0: It might be required to set NV_TX_LASTPACKET in the
 * last valid ring entry. But this would be impossible to
 * implement - probably a disassembly error.
 */
#define TX_LIMIT_STOP   63
#define TX_LIMIT_START  62

/* rx/tx mac addr + type + vlan + align + slack*/
#define RX_NIC_BUFSIZE          (ETH_DATA_LEN + 64)
/* even more slack */
//#define RX_ALLOC_BUFSIZE        (ETH_DATA_LEN + 128)
#define RX_ALLOC_BUFSIZE        (ETH_DATA_LEN + 164)

#define OOM_REFILL      (HZ/20)
#define POLL_WAIT       (HZ/100)
#define LINK_TIMEOUT    (3*HZ)

/*
 * desc_ver values:
 * This field has two purposes:
 * - Newer nics uses a different ring layout. The layout is selected by
 *   comparing np->desc_ver with DESC_VER_xy.
 * - It contains bits that are forced on when writing to NvRegTxRxControl.
 */
#define DESC_VER_1      0x0
#define DESC_VER_2      (0x02100|NVREG_TXRXCTL_RXCHECK)

/* PHY defines */
#define PHY_OUI_MARVELL 0x5043
#define PHY_OUI_CICADA  0x03f1
#define PHYID1_OUI_MASK 0x03ff
#define PHYID1_OUI_SHFT 6
#define PHYID2_OUI_MASK 0xfc00
#define PHYID2_OUI_SHFT 10
#define PHY_INIT1       0x0f000
#define PHY_INIT2       0x0e00
#define PHY_INIT3       0x01000
#define PHY_INIT4       0x0200
#define PHY_INIT5       0x0004
#define PHY_INIT6       0x02000
#define PHY_GIGABIT     0x0100

#define PHY_TIMEOUT     0x1
#define PHY_ERROR       0x2

#define PHY_100 0x1
#define PHY_1000        0x2
#define PHY_HALF        0x100

/* FIXME: MII defines that should be added to <linux/mii.h> */
#define MII_1000BT_CR   0x09
#define MII_1000BT_SR   0x0a
#define ADVERTISE_1000FULL      0x0200
#define ADVERTISE_1000HALF      0x0100
#define LPA_1000FULL    0x0800
#define LPA_1000HALF    0x0400

/* MII defines from linux/mii.h */

/* Generic MII registers. */

#define MII_BMCR            0x00        /* Basic mode control register */
#define MII_BMSR            0x01        /* Basic mode status register  */
#define MII_PHYSID1         0x02        /* PHYS ID 1                   */
#define MII_PHYSID2         0x03        /* PHYS ID 2                   */
#define MII_ADVERTISE       0x04        /* Advertisement control reg   */
#define MII_LPA             0x05        /* Link partner ability reg    */
#define MII_EXPANSION       0x06        /* Expansion register          */
#define MII_DCOUNTER        0x12        /* Disconnect counter          */
#define MII_FCSCOUNTER      0x13        /* False carrier counter       */
#define MII_NWAYTEST        0x14        /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER     0x15        /* Receive error counter       */
#define MII_SREVISION       0x16        /* Silicon revision            */
#define MII_RESV1           0x17        /* Reserved...                 */
#define MII_LBRERROR        0x18        /* Lpback, rx, bypass error    */
#define MII_PHYADDR         0x19        /* PHY address                 */
#define MII_RESV2           0x1a        /* Reserved...                 */
#define MII_TPISTATUS       0x1b        /* TPI status for 10mbps       */
#define MII_NCONFIG         0x1c        /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV               0x003f  /* Unused...                   */
#define BMCR_SPEED1000          0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST               0x0080  /* Collision test              */
#define BMCR_FULLDPLX           0x0100  /* Full duplex                 */
#define BMCR_ANRESTART          0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE            0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN              0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE           0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100           0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK           0x4000  /* TXD loopback bits           */
#define BMCR_RESET              0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP              0x0001  /* Ext-reg capability          */
#define BMSR_JCD                0x0002  /* Jabber detected             */
#define BMSR_LSTATUS            0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE        0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT             0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE       0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV               0x07c0  /* Unused...                   */
#define BMSR_10HALF             0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL             0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF            0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL            0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4           0x8000  /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT          0x001f  /* Selector bits               */
#define ADVERTISE_CSMA          0x0001  /* Only selector supported     */
#define ADVERTISE_10HALF        0x0020  /* Try for 10mbps half-duplex  */
#define ADVERTISE_10FULL        0x0040  /* Try for 10mbps full-duplex  */
#define ADVERTISE_100HALF       0x0080  /* Try for 100mbps half-duplex */
#define ADVERTISE_100FULL       0x0100  /* Try for 100mbps full-duplex */
#define ADVERTISE_100BASE4      0x0200  /* Try for 100mbps 4k packets  */
#define ADVERTISE_RESV          0x1c00  /* Unused...                   */
#define ADVERTISE_RFAULT        0x2000  /* Say we can detect faults    */
#define ADVERTISE_LPACK         0x4000  /* Ack link partners response  */
#define ADVERTISE_NPAGE         0x8000  /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
                        ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT                0x001f  /* Same as advertise selector  */
#define LPA_10HALF              0x0020  /* Can do 10mbps half-duplex   */
#define LPA_10FULL              0x0040  /* Can do 10mbps full-duplex   */
#define LPA_100HALF             0x0080  /* Can do 100mbps half-duplex  */
#define LPA_100FULL             0x0100  /* Can do 100mbps full-duplex  */
#define LPA_100BASE4            0x0200  /* Can do 100mbps 4k packets   */
#define LPA_RESV                0x1c00  /* Unused...                   */
#define LPA_RFAULT              0x2000  /* Link partner faulted        */
#define LPA_LPACK               0x4000  /* Link partner acked us       */
#define LPA_NPAGE               0x8000  /* Next page bit               */

#define LPA_DUPLEX              (LPA_10FULL | LPA_100FULL)
#define LPA_100                 (LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY          0x0001  /* Can do N-way auto-nego      */
#define EXPANSION_LCWP          0x0002  /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE   0x0004  /* This enables npage words    */
#define EXPANSION_NPCAPABLE     0x0008  /* Link partner supports npage */
#define EXPANSION_MFAULTS       0x0010  /* Multiple faults detected    */
#define EXPANSION_RESV          0xffe0  /* Unused...                   */

/* N-way test register. */
#define NWAYTEST_RESV1          0x00ff  /* Unused...                   */
#define NWAYTEST_LOOPBACK       0x0100  /* Enable loopback for N-way   */
#define NWAYTEST_RESV2          0xfe00  /* Unused...                   */

struct eth_frame {
    UBYTE eth_packet_dest[6];
    UBYTE eth_packet_source[6];
    UWORD eth_packet_type;
    UBYTE eth_packet_data[ETH_MTU];
    UBYTE eth_pad[RX_ALLOC_BUFSIZE - ETH_MAXPACKETSIZE];
} __attribute__((packed));
#define eth_packet_ieeelen eth_packet_type

void nv_get_functions(struct NFUnit *Unit);

#endif

