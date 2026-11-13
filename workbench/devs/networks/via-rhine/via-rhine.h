#ifndef _VIA_RHINE_H
#define _VIA_RHINE_H

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
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#define VIARHINE_TASK_NAME	"VIA-RHINE task"
#define VIARHINE_PORT_NAME	"VIA-RHINE port"

#define TX_BUFFERS        8
#define RX_BUFFERS        16

#define BUFFER_SIZE       2048L              /* Enough to hold a 1536 Frame     */
#define MAX_FRAME_SIZE    1536               /* 1536                            */
#define MAX_MULTI         32                 /* Hardware Related, Do not change */

/** Device Driver Structures **/

struct VIARHINEBase {
    struct Device       rhineb_Device;
    struct MsgPort      *rhineb_syncport;

    OOP_Object          *rhineb_pci;
    OOP_Object          *rhineb_irq;
    OOP_AttrBase        rhineb_pciDeviceAttrBase;

    struct Sana2DeviceQuery  rhineb_Sana2Info;
    struct VIARHINEUnit            *rhineb_unit;

    /* UnitCount is used to assign unit ID's to found hardware ..*/
    unsigned int                    rhineb_UnitCount;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->rhineb_pciDeviceAttrBase)

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

#define STAT_COUNT 3

struct VIARHINEUnit {
    struct MinNode          *rhineu_Node;
    struct MinList          rhineu_Openers;
    struct MinList          rhineu_multicast_ranges;
    struct MinList          rhineu_type_trackers;
    ULONG                   rhineu_UnitNum;
    LONG                    rhineu_range_count;

    OOP_Object              *rhineu_PCIDevice;
    OOP_Object              *rhineu_PCIDriver;

    struct timeval          rhineu_toutPOLL;
    BOOL                    rhineu_toutNEED;
    BOOL                    rhineu_IntsAdded;

    struct MsgPort          *rhineu_TimerSlowPort;
    struct timerequest      *rhineu_TimerSlowReq;

    struct MsgPort          *rhineu_TimerFastPort;
    struct timerequest      *rhineu_TimerFastReq;

    struct Sana2DeviceStats rhineu_stats;
    ULONG                   rhineu_special_stats[STAT_COUNT];

	char                    *rhineu_cardname;
	int                           rhineu_chipcapabilities;
	
	ULONG                  rhineu_rtl_LinkSpeed;
#define support_fdx     (1 << 0) // Supports Full Duplex
#define support_mii     (1 << 1)
#define support_fset    (1 << 2)
#define support_ltint   (1 << 3)
#define support_dxsuflo (1 << 4)
/* Card Funcs */
    void                    (*initialize)(struct VIARHINEUnit *);
    void                    (*deinitialize)(struct VIARHINEUnit *);
    int                     (*start)(struct VIARHINEUnit *);
    int                     (*stop)(struct VIARHINEUnit *);
    int                     (*alloc_rx)(struct VIARHINEUnit *);
    void                    (*set_mac_address)(struct VIARHINEUnit *);
    void                    (*linkchange)(struct VIARHINEUnit *);
    void                    (*linkirq)(struct VIARHINEUnit *);
//    ULONG                   (*descr_getlength)(struct ring_desc *prd, ULONG v);
    void                    (*set_multicast)(struct VIARHINEUnit *);

    int                     rhineu_open_count;
    struct SignalSemaphore  rhineu_unit_lock;

    struct Process          *rhineu_Process;

    struct VIARHINEBase    *rhineu_device;
    struct Interrupt        rhineu_irqhandler;
    struct Interrupt        rhineu_touthandler;
    IPTR	                rhineu_DeviceID;
    IPTR                    rhineu_DriverFlags;
    IPTR                    rhineu_IRQ;
    IPTR                    rhineu_BaseMem;
    IPTR                    rhineu_SizeMem;
    IPTR	                  rhineu_BaseIO;

    BYTE                    rhineu_signal_0;
    BYTE                    rhineu_signal_1;
    BYTE                    rhineu_signal_2;
    BYTE                    rhineu_signal_3;

    struct MsgPort          *rhineu_input_port;

    struct MsgPort          *rhineu_request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt        rhineu_rx_int;
    struct Interrupt        rhineu_tx_int;

    STRPTR                  rhineu_name;
    ULONG                   rhineu_mtu;
    ULONG                   rhineu_flags;
    ULONG                   rhineu_state;
    APTR                    rhineu_mc_list;
    UBYTE                   rhineu_dev_addr[6];
    UBYTE                   rhineu_org_addr[6];
    struct fe_priv          *rhineu_fe_priv;
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

static inline void netif_schedule(struct VIARHINEUnit *dev)
{
    if (!test_bit(__LINK_STATE_XOFF, &dev->rhineu_state)) {
        Cause(&dev->rhineu_tx_int);
    }
}


static inline void netif_start_queue(struct VIARHINEUnit *dev)
{
    clear_bit(__LINK_STATE_XOFF, &dev->rhineu_state);
}

static inline void netif_wake_queue(struct VIARHINEUnit *dev)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &dev->rhineu_state)) {
        Cause(&dev->rhineu_tx_int);
    }
}

static inline void netif_stop_queue(struct VIARHINEUnit *dev)
{
    set_bit(__LINK_STATE_XOFF, &dev->rhineu_state);
}

static inline int netif_queue_stopped(const struct VIARHINEUnit *dev)
{
    return test_bit(__LINK_STATE_XOFF, &dev->rhineu_state);
}

static inline int netif_running(const struct VIARHINEUnit *dev)
{
    return test_bit(__LINK_STATE_START, &dev->rhineu_state);
}

static inline int netif_carrier_ok(const struct VIARHINEUnit *dev)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &dev->rhineu_state);
}

extern void __netdev_watchdog_up(struct VIARHINEUnit *dev);

static inline void netif_carrier_on(struct VIARHINEUnit *dev)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &dev->rhineu_state)) {
//                linkwatch_fire_event(dev);
    }
    if (netif_running(dev)) {
//                __netdev_watchdog_up(dev);
    }
}

static inline void netif_carrier_off(struct VIARHINEUnit *dev)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &dev->rhineu_state)) {
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

/*
 * RX Descriptor
 */
typedef struct
{
	ULONG		rx_status;
	ULONG		desc_length;

	ULONG		addr;
	ULONG		next;
} viarhine_rx_desc;

/*
 * TX Descriptor
 */
typedef struct
{
	ULONG		tx_status;
	ULONG		desc_length;

	ULONG		addr;
	ULONG		next;
} viarhine_tx_desc;

struct fe_priv {
	/* Start - via rhine new */
	viarhine_rx_desc      rx_desc[RX_BUFFERS];  /* RX Frame Descriptors */
	viarhine_tx_desc       tx_desc[TX_BUFFERS];  /* TX Frame Descriptors */

    struct   eth_frame   *rx_buffer;
	UBYTE                      rx_thresh;
	UBYTE                      rx_current;

    struct   eth_frame    *tx_buffer;
	UBYTE					    tx_thresh;
	UBYTE                      tx_current;

    int                           full_duplex;

	char                       mii_phys[4]; //MII device address
	unsigned short    mii_advertising;  //NWay media advertising

	UWORD				 	cmd;
/* End - via rhine new */

    struct VIARHINEUnit   *pci_dev;
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

    IPTR     ring_addr;


	
    ULONG   cur_rx, refill_rx;

    ULONG   next_tx, nic_tx;
    ULONG   tx_flags;

    ULONG   irqmask;
    ULONG   need_linktimer;
    struct  timeval link_timeout;
    UBYTE   orig_mac[6];
};

#define pci_name(unit)  (unit->rhineu_name)

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

void viarhinenic_get_functions(struct VIARHINEUnit *Unit);

/* **************************** */
/*     OLD PCNET32 DEFINES       */
/* **************************** */

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

/* ***************************** */
/*     REAL VIA RHINE DEFINES       */
/* ***************************** */

/*
 * Chip Compatibility
 */
enum viarhine_chip_capability_flags
{
	RTLc_CanHaveMII = 1
};

/*
 * Registers Offsets
 */
enum viarhine_register_offsets
{
	VIAR_StationAddr      = 0x00,
	VIAR_RxConfig         = 0x06,
	VIAR_TxConfig         = 0x07,
	VIAR_ChipCmd          = 0x08,
	VIAR_IntrStatus       = 0x0C,
	VIAR_IntrEnable       = 0x0E,
	VIAR_MulticastFilter0 = 0x10,
	VIAR_MulticastFilter1 = 0x14,
	VIAR_RxRingPtr        = 0x18,
	VIAR_TxRingPtr        = 0x1C,
	VIAR_MIIPhyAddr       = 0x6C,
	VIAR_MIIStatus        = 0x6D,
	VIAR_PCIBusConfig     = 0x6E,
	VIAR_MIICmd           = 0x70,
	VIAR_MIIRegAddr       = 0x71,
	VIAR_MIIData          = 0x72,
	VIAR_Config           = 0x78,
	VIAR_RxMissed         = 0x7C,
	VIAR_RxCRCErrs        = 0x7E,
};

/*
 * Command Bits
 */
enum viarhine_chip_cmd_bits
{
	CmdInit     = 0x0001,
	CmdStart    = 0x0002,
	CmdStop     = 0x0004,
	CmdRxOn     = 0x0008,
	CmdTxOn     = 0x0010,
	CmdTxDemand = 0x0020,
	CmdRxDemand = 0x0040,
	CmdEarlyRx  = 0x0100,
	CmdEarlyTx  = 0x0200,
	CmdFDuplex  = 0x0400,
	CmdNoTxPoll = 0x0800,
	CmdReset    = 0x8000
};

/*
 * Interrupt Status Bits
 */
enum viarhine_intr_status_bits
{
	IntrRxDone          = 0x0001,
	IntrRxErr           = 0x0004,
	IntrRxEmpty         = 0x0020,
	IntrTxDone          = 0x0002,
	IntrTxAbort         = 0x0008,
	IntrTxUnderrun      = 0x0010,
	IntrPCIErr          = 0x0040,
	IntrStatsMax        = 0x0080,
	IntrRxEarly         = 0x0100,
	IntrMIIChange       = 0x0200,
	IntrRxOverflow      = 0x0400,
	IntrRxDropped       = 0x0800,
	IntrRxNoBuf         = 0x1000,
	IntrTxAborted       = 0x2000,
	IntrLinkChange      = 0x4000,
	IntrRxWakeUp        = 0x8000,
	IntrNormalSummary   = 0x0003,
	IntrAbnormalSummary = 0xC260,
};

/*
 * RX Status Bits
 */
enum viarhine_rx_status_bits
{
	RxOK       = 0x8000,
	RxWholePkt = 0x0300,
	RxErr      = 0x008F
};

/*
 * Desc Status Bits
 */
enum viarhine_desc_status_bits
{
	DescOwn       = 0x80000000,
	DescEndPacket = 0x4000,
	DescIntr      = 0x1000
};

/** Serial EEPROM section **/

//  EEPROM_Ctrl bits

#define EE_SHIFT_CLK      0x04  // EEPROM shift clock
#define EE_CS                  0x08  // EEPROM chip select
#define EE_DATA_WRITE   0x02  // EEPROM chip data in
#define EE_WRITE_0         0x00
#define EE_WRITE_1         0x02
#define EE_DATA_READ    0x01  // EEPROM chip data out
#define EE_ENB               (0x80 | EE_CS)

// Delay between EEPROM clock transitions.
// No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.

#define eeprom_delay(ee_addr)  LONGIN(ee_addr)

// The EEPROM commands include the alway-set leading bit

#define EE_WRITE_CMD  (5)
#define EE_READ_CMD   (6)
#define EE_ERASE_CMD  (7)

/** MII serial management **/

// Read and write the MII management registers using software-generated
// serial MDIO protocol.
// The maximum data clock rate is 2.5 Mhz.  The minimum timing is usually
// met by back-to-back PCI I/O cycles, but we insert a delay to avoid
// "overclocking" issues

#define MDIO_DIR              0x80
#define MDIO_DATA_OUT  0x04
#define MDIO_DATA_IN      0x02
#define MDIO_CLK             0x01
#define MDIO_WRITE0       (MDIO_DIR)
#define MDIO_WRITE1       (MDIO_DIR | MDIO_DATA_OUT)

#define mdio_delay(mdio_addr) LONGIN(mdio_addr)

#endif

