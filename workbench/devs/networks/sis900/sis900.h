#ifndef _SIS900_H
#define _SIS900_H

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

#define SiS900_TASK_NAME	"%s.task"
#define SiS900_PORT_NAME	"%s.port"

/** Operational parameters that are set at compile time **/
#define ETH_ZLEN  60 // Min. octets in frame sans FCS

#define NUM_TX_DESC     16      	/* Number of Tx descriptor registers. */
#define NUM_RX_DESC     16       	/* Number of Rx descriptor registers. */

/** Device Driver Structures **/

struct SiS900Base {
    struct Device       sis900b_Device;

    OOP_Object          *sis900b_PCI;
    OOP_AttrBase        sis900b_PCIDeviceAttrBase;

    ULONG               sis900b_UnitCount;
    struct List         sis900b_Units;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->sis900b_PCIDeviceAttrBase)

struct SiS900Startup
{
    struct MsgPort           *sis900sm_SyncPort;
    struct SiS900Unit        *sis900sm_Unit;
};

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

/* SiS900 Specific Structs */

struct mii_phy {
	struct mii_phy   *next;
	int              phy_addr;
	UWORD            phy_id0;
	UWORD            phy_id1;
	UWORD            status;
	UBYTE            phy_types;
};

typedef struct _BufferDesc {
	ULONG link;
	ULONG cmdsts;
	ULONG bufptr;
} BufferDesc;

#define SANA2_SPECIAL_STAT_COUNT 3

/* Per-Unit Device struct */
struct SiS900Unit {
    struct MinNode          sis900u_Node;

    struct SiS900Base       *sis900u_device;

    STRPTR                  sis900u_name;

    ULONG                   sis900u_UnitNum;
    IPTR                    sis900u_DriverFlags;

    OOP_Object              *sis900u_PCIDevice;
    OOP_Object              *sis900u_PCIDriver;
    IPTR                    sis900u_IRQ;
    BOOL                    sis900u_IntsAdded;

    int                     sis900u_open_count;
    struct SignalSemaphore  sis900u_unit_lock;

    LONG                    sis900u_range_count;
    struct MinList          sis900u_Openers;
    struct MinList          sis900u_multicast_ranges;
    struct MinList          sis900u_type_trackers;

    ULONG                   sis900u_mtu;
    ULONG                   sis900u_ifflags;
    struct Sana2DeviceQuery sis900u_Sana2Info;
    struct Sana2DeviceStats sis900u_stats;
    ULONG                   sis900u_special_stats[SANA2_SPECIAL_STAT_COUNT];

    struct Process          *sis900u_Process;

    struct MsgPort          *sis900u_input_port;

    struct MsgPort          *sis900u_request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt         sis900u_irqhandler;
    struct Interrupt         sis900u_touthandler;

    struct MsgPort          *sis900u_TimerSlowPort;
    struct timerequest      *sis900u_TimerSlowReq;

    struct MsgPort          *sis900u_TimerFastPort;
    struct timerequest      *sis900u_TimerFastReq;

    struct MsgPort          e1ku_DelayPort;
    struct timerequest      e1ku_DelayReq;

	char                    *sis900u_rtl_cardname;
	char                    *sis900u_rtl_chipname;

    IPTR	                sis900u_DeviceID;
    IPTR	                sis900u_RevisionID;
    IPTR                    sis900u_HostRevisionID;
    IPTR                    sis900u_BaseMem;
    IPTR                    sis900u_SizeMem;
    IPTR	                sis900u_BaseIO;

    BYTE                    sis900u_signal_0;
    BYTE                    sis900u_signal_1;
    BYTE                    sis900u_signal_2;
    BYTE                    sis900u_signal_3;

    UBYTE                   sis900u_dev_addr[6];
    UWORD                   sis900u_org_addr[3];

    struct Interrupt        sis900u_rx_int;
    struct Interrupt        sis900u_tx_int;

    ULONG                   sis900u_state;
    APTR                    sis900u_mc_list;

/* SiS900 - New!! */
	APTR                    tx_buffers[NUM_TX_DESC];
	APTR                    rx_buffers[NUM_RX_DESC];

	BufferDesc              *tx_ring;
	BufferDesc              *rx_ring;

	APTR                    tx_ring_dma;
	APTR                    rx_ring_dma;

	struct mii_phy          *mii;
	struct mii_phy          *first_mii; /* record the first mii structure */
	unsigned int            cur_phy;

	unsigned int            tx_full; /* The Tx queue is full. */

	unsigned int            cur_rx,
                            dirty_rx; /* producer/comsumer pointers for Tx/Rx ring */
	unsigned int            cur_tx,
                            dirty_tx;
    
    UBYTE                   autong_complete; /* 1: auto-negotiate complete  */
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

static inline void netif_schedule(struct SiS900Unit *unit)
{
    if (!test_bit(__LINK_STATE_XOFF, &unit->sis900u_state)) {
        Cause(&unit->sis900u_tx_int);
    }
}

static inline void netif_start_queue(struct SiS900Unit *unit)
{
    clear_bit(__LINK_STATE_XOFF, &unit->sis900u_state);
}

static inline void netif_wake_queue(struct SiS900Unit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_XOFF, &unit->sis900u_state)) {
        Cause(&unit->sis900u_tx_int);
    }
}

static inline void netif_stop_queue(struct SiS900Unit *unit)
{
    set_bit(__LINK_STATE_XOFF, &unit->sis900u_state);
}

static inline int netif_queue_stopped(const struct SiS900Unit *unit)
{
    return test_bit(__LINK_STATE_XOFF, &unit->sis900u_state);
}

static inline int netif_running(const struct SiS900Unit *unit)
{
    return test_bit(__LINK_STATE_START, &unit->sis900u_state);
}

static inline int netif_carrier_ok(const struct SiS900Unit *unit)
{
    return !test_bit(__LINK_STATE_NOCARRIER, &unit->sis900u_state);
}

extern void __netdev_watchdog_up(struct SiS900Unit *unit);

static inline void netif_carrier_on(struct SiS900Unit *unit)
{
    if (test_and_clear_bit(__LINK_STATE_NOCARRIER, &unit->sis900u_state)) {
//                linkwatch_fire_event(unit);
    }
    if (netif_running(unit)) {
//                __netdev_watchdog_up(unit);
    }
}

static inline void netif_carrier_off(struct SiS900Unit *unit)
{
    if (!test_and_set_bit(__LINK_STATE_NOCARRIER, &unit->sis900u_state)) {
//                linkwatch_fire_event(unit);
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
/*     SiS900 DEFINES       */
/* ***************************** */

/* The I/O extent, SiS 900 needs 256 bytes of io address */
#define SIS900_TOTAL_SIZE 0x100

/* Symbolic offsets to registers. */
enum sis900_registers {
	cr=0x0,                 //Command Register
	cfg=0x4,                //Configuration Register
	mear=0x8,               //EEPROM Access Register
	ptscr=0xc,              //PCI Test Control Register
	isr=0x10,               //Interrupt Status Register
	imr=0x14,               //Interrupt Mask Register
	ier=0x18,               //Interrupt Enable Register
	epar=0x18,              //Enhanced PHY Access Register
	txdp=0x20,              //Transmit Descriptor Pointer Register
        txcfg=0x24,             //Transmit Configuration Register
        rxdp=0x30,              //Receive Descriptor Pointer Register
        rxcfg=0x34,             //Receive Configuration Register
        flctrl=0x38,            //Flow Control Register
        rxlen=0x3c,             //Receive Packet Length Register
        rfcr=0x48,              //Receive Filter Control Register
        rfdr=0x4C,              //Receive Filter Data Register
        pmctrl=0xB0,            //Power Management Control Register
        pmer=0xB4               //Power Management Wake-up Event Register
};

/* Symbolic names for bits in various registers */
enum sis900_command_register_bits {
	RELOAD  = 0x00000400, ACCESSMODE = 0x00000200,/* ET */
	RESET   = 0x00000100, SWI = 0x00000080, RxRESET = 0x00000020,
	TxRESET = 0x00000010, RxDIS = 0x00000008, RxENA = 0x00000004,
	TxDIS   = 0x00000002, TxENA = 0x00000001
};

enum sis900_configuration_register_bits {
	DESCRFMT = 0x00000100 /* 7016 specific */, REQALG = 0x00000080,
	SB    = 0x00000040, POW = 0x00000020, EXD = 0x00000010, 
	PESEL = 0x00000008, LPM = 0x00000004, BEM = 0x00000001,
	/* 635 & 900B Specific */
	RND_CNT = 0x00000400, FAIR_BACKOFF = 0x00000200,
	EDB_MASTER_EN = 0x00002000
};

enum sis900_eeprom_access_reigster_bits {
	MDC  = 0x00000040, MDDIR = 0x00000020, MDIO = 0x00000010, /* 7016 specific */ 
	EECS = 0x00000008, EECLK = 0x00000004, EEDO = 0x00000002,
	EEDI = 0x00000001
};

enum sis900_interrupt_register_bits {
	WKEVT  = 0x10000000, TxPAUSEEND = 0x08000000, TxPAUSE = 0x04000000,
	TxRCMP = 0x02000000, RxRCMP = 0x01000000, DPERR = 0x00800000,
	SSERR  = 0x00400000, RMABT  = 0x00200000, RTABT = 0x00100000,
	RxSOVR = 0x00010000, HIBERR = 0x00008000, SWINT = 0x00001000,
	MIBINT = 0x00000800, TxURN  = 0x00000400, TxIDLE  = 0x00000200,
	TxERR  = 0x00000100, TxDESC = 0x00000080, TxOK  = 0x00000040,
	RxORN  = 0x00000020, RxIDLE = 0x00000010, RxEARLY = 0x00000008,
	RxERR  = 0x00000004, RxDESC = 0x00000002, RxOK  = 0x00000001
};

enum sis900_interrupt_enable_reigster_bits {
	IE = 0x00000001
};

/* maximum dma burst for transmission and receive */
#define MAX_DMA_RANGE	7	/* actually 0 means MAXIMUM !! */
#define TxMXDMA_shift   	20
#define RxMXDMA_shift    20

enum sis900_tx_rx_dma{
	DMA_BURST_512 = 0,	DMA_BURST_64 = 5
};

/* transmit FIFO thresholds */
#define TX_FILL_THRESH   16	/* 1/4 FIFO size */
#define TxFILLT_shift   	8
#define TxDRNT_shift    	0
#define TxDRNT_100      	48	/* 3/4 FIFO size */
#define TxDRNT_10		16 	/* 1/2 FIFO size */

enum sis900_transmit_config_register_bits {
	TxCSI = 0x80000000, TxHBI = 0x40000000, TxMLB = 0x20000000,
	TxATP = 0x10000000, TxIFG = 0x0C000000, TxFILLT = 0x00003F00,
	TxDRNT = 0x0000003F
};

/* recevie FIFO thresholds */
#define RxDRNT_shift     1
#define RxDRNT_100	16	/* 1/2 FIFO size */
#define RxDRNT_10		24 	/* 3/4 FIFO size */

enum sis900_reveive_config_register_bits {
	RxAEP  = 0x80000000, RxARP = 0x40000000, RxATX = 0x10000000,
	RxAJAB = 0x08000000, RxDRNT = 0x0000007F
};

#define RFAA_shift      28
#define RFADDR_shift    16

enum sis900_receive_filter_control_register_bits {
	RFEN  = 0x80000000, RFAAB = 0x40000000, RFAAM = 0x20000000,
	RFAAP = 0x10000000, RFPromiscuous = (RFAAB|RFAAM|RFAAP)
};

enum sis900_reveive_filter_data_mask {
	RFDAT =  0x0000FFFF
};

/* EEPROM Addresses */
enum sis900_eeprom_address {
	EEPROMSignature = 0x00, EEPROMVendorID = 0x02, EEPROMDeviceID = 0x03,
	EEPROMMACAddr   = 0x08, EEPROMChecksum = 0x0b
};

/* The EEPROM commands include the alway-set leading bit. Refer to NM93Cxx datasheet */
enum sis900_eeprom_command {
	EEread     = 0x0180, EEwrite    = 0x0140, EEerase = 0x01C0, 
	EEwriteEnable = 0x0130, EEwriteDisable = 0x0100,
	EEeraseAll = 0x0120, EEwriteAll = 0x0110, 
	EEaddrMask = 0x013F, EEcmdShift = 16
};

/* For SiS962 or SiS963, request the eeprom software access */
enum sis96x_eeprom_command {
	EEREQ = 0x00000400, EEDONE = 0x00000200, EEGNT = 0x00000100
};

/* Management Data I/O (mdio) frame */
#define MIIread         0x6000
#define MIIwrite        0x5002
#define MIIpmdShift     7
#define MIIregShift     2
#define MIIcmdLen       16
#define MIIcmdShift     16

/* Buffer Descriptor Status*/
enum sis900_buffer_status {
	OWN    = 0x80000000, MORE   = 0x40000000, INTR = 0x20000000,
	SUPCRC = 0x10000000, INCCRC = 0x10000000,
	OK     = 0x08000000, DSIZE  = 0x00000FFF
};
/* Status for TX Buffers */
enum sis900_tx_buffer_status {
	ABORT   = 0x04000000, UNDERRUN = 0x02000000, NOCARRIER = 0x01000000,
	DEFERD  = 0x00800000, EXCDEFER = 0x00400000, OWCOLL    = 0x00200000,
	EXCCOLL = 0x00100000, COLCNT   = 0x000F0000
};

enum sis900_rx_bufer_status {
	OVERRUN = 0x02000000, DEST = 0x00800000,     BCAST = 0x01800000,
	MCAST   = 0x01000000, UNIMATCH = 0x00800000, TOOLONG = 0x00400000,
	RUNT    = 0x00200000, RXISERR  = 0x00100000, CRCERR  = 0x00080000,
	FAERR   = 0x00040000, LOOPBK   = 0x00020000, RXCOL   = 0x00010000
};

/* MII register offsets */
enum mii_registers {
	MII_CONTROL = 0x0000, MII_STATUS = 0x0001, MII_PHY_ID0 = 0x0002,
	MII_PHY_ID1 = 0x0003, MII_ANADV  = 0x0004, MII_ANLPAR  = 0x0005,
	MII_ANEXT   = 0x0006
};

/* mii registers specific to SiS 900 */
enum sis_mii_registers {
	MII_CONFIG1 = 0x0010, MII_CONFIG2 = 0x0011, MII_STSOUT = 0x0012,
	MII_MASK    = 0x0013, MII_RESV    = 0x0014
};

/* mii registers specific to ICS 1893 */
enum ics_mii_registers {
	MII_EXTCTRL  = 0x0010, MII_QPDSTS = 0x0011, MII_10BTOP = 0x0012,
	MII_EXTCTRL2 = 0x0013
};

/* mii registers specific to AMD 79C901 */
enum amd_mii_registers {
	MII_STATUS_SUMMARY = 0x0018
};

/* MII Control register bit definitions. */
enum mii_control_register_bits {
	MII_CNTL_FDX     = 0x0100, MII_CNTL_RST_AUTO = 0x0200, 
	MII_CNTL_ISOLATE = 0x0400, MII_CNTL_PWRDWN   = 0x0800,
	MII_CNTL_AUTO    = 0x1000, MII_CNTL_SPEED    = 0x2000,
	MII_CNTL_LPBK    = 0x4000, MII_CNTL_RESET    = 0x8000
};

/* MII Status register bit  */
enum mii_status_register_bits {
	MII_STAT_EXT    = 0x0001, MII_STAT_JAB        = 0x0002, 
	MII_STAT_LINK   = 0x0004, MII_STAT_CAN_AUTO   = 0x0008, 
	MII_STAT_FAULT  = 0x0010, MII_STAT_AUTO_DONE  = 0x0020,
	MII_STAT_CAN_T  = 0x0800, MII_STAT_CAN_T_FDX  = 0x1000,
	MII_STAT_CAN_TX = 0x2000, MII_STAT_CAN_TX_FDX = 0x4000,
	MII_STAT_CAN_T4 = 0x8000
};

#define		MII_ID1_OUI_LO		0xFC00	/* low bits of OUI mask */
#define		MII_ID1_MODEL		0x03F0	/* model number */
#define		MII_ID1_REV		0x000F	/* model number */

/* MII NWAY Register Bits ...
   valid for the ANAR (Auto-Negotiation Advertisement) and
   ANLPAR (Auto-Negotiation Link Partner) registers */
enum mii_nway_register_bits {
	MII_NWAY_NODE_SEL = 0x001f, MII_NWAY_CSMA_CD = 0x0001,
	MII_NWAY_T	  = 0x0020, MII_NWAY_T_FDX   = 0x0040,
	MII_NWAY_TX       = 0x0080, MII_NWAY_TX_FDX  = 0x0100,
	MII_NWAY_T4       = 0x0200, MII_NWAY_PAUSE   = 0x0400,
	MII_NWAY_RF       = 0x2000, MII_NWAY_ACK     = 0x4000,
	MII_NWAY_NP       = 0x8000
};

enum mii_stsout_register_bits {
	MII_STSOUT_LINK_FAIL = 0x4000,
	MII_STSOUT_SPD       = 0x0080, MII_STSOUT_DPLX = 0x0040
};

enum mii_stsics_register_bits {
	MII_STSICS_SPD  = 0x8000, MII_STSICS_DPLX = 0x4000,
	MII_STSICS_LINKSTS = 0x0001
};

enum mii_stssum_register_bits {
	MII_STSSUM_LINK = 0x0008, MII_STSSUM_DPLX = 0x0004,
	MII_STSSUM_AUTO = 0x0002, MII_STSSUM_SPD  = 0x0001
};

enum sis900_revision_id {
	SIS630A_900_REV = 0x80,		SIS630E_900_REV = 0x81,
	SIS630S_900_REV = 0x82,		SIS630EA1_900_REV = 0x83,
	SIS630ET_900_REV = 0x84,	SIS635A_900_REV = 0x90,
	SIS96x_900_REV = 0X91,		SIS900B_900_REV = 0x03
};

enum sis630_revision_id {
	SIS630A0    = 0x00, SIS630A1      = 0x01,
	SIS630B0    = 0x10, SIS630B1      = 0x11
};

#define FDX_CAPABLE_DUPLEX_UNKNOWN      0
#define FDX_CAPABLE_HALF_SELECTED       1
#define FDX_CAPABLE_FULL_SELECTED       2

#define HW_SPEED_UNCONFIG		0
#define HW_SPEED_HOME		1
#define HW_SPEED_10_MBPS        	10
#define HW_SPEED_100_MBPS       	100
#define HW_SPEED_DEFAULT        	(HW_SPEED_100_MBPS)

#define CRC_SIZE                4
#define MAC_HEADER_SIZE         14

#define TX_BUF_SIZE     1536
#define RX_BUF_SIZE     1536

#define TX_TOTAL_SIZE	NUM_TX_DESC*sizeof(BufferDesc)
#define RX_TOTAL_SIZE	NUM_RX_DESC*sizeof(BufferDesc)

int mdio_read(struct SiS900Unit *, int, int);

void sis900func_initialize(struct SiS900Unit *);
void sis900func_deinitialize(struct SiS900Unit *);
int sis900func_open(struct SiS900Unit *);
int sis900func_close(struct SiS900Unit *);
void sis900func_set_mac(struct SiS900Unit *);
void sis900func_set_multicast(struct SiS900Unit *);

#endif

