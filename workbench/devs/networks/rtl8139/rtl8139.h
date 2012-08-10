#ifndef _RTL8139_H
#define _RTL8139_H

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

#include <aros/debug.h>

#if DEBUG > 0
#define RTL_DEBUG
#endif

#define RTLD(d) \
	if (unit->rtl8139u_flags & IFF_DEBUG) \
	{ \
		d; \
	}

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
#define LIBBASETYPEPTR struct RTL8139Base *

#define net_device RTL8139Unit

#define RTL8139_TASK_NAME	"%s.task"
#define RTL8139_PORT_NAME	"%s.port"

/** Operational parameters that are set at compile time **/
#define ETH_ZLEN  60 // Min. octets in frame sans FCS

// Maximum size of the in-memory receive ring (smaller if no memory)
#define RX_BUF_LEN_IDX  2     // 0=8K, 1=16K, 2=32K, 3=64K
#define RX_FIFO_THRESH  6    // Rx buffer level before first PCI xfer
#define RX_DMA_BURST    6   // Maximum PCI burst, '4' is 256 bytes

// Size of the Tx bounce buffers -- must be at least (mtu+14+4)
#define TX_BUF_SIZE 1536
#define NUM_TX_DESC 4         // Number of Tx descriptor registers
#define TX_FIFO_THRESH 256  // In bytes, rounded down to 32 byte units
#define TX_DMA_BURST    6   // Calculate as 16 << val

/** Device Driver Structures **/

extern struct Library *OOPBase;

struct RTL8139Base
{
    struct Device       rtl8139b_Device;

    OOP_Object          *rtl8139b_PCI;
    OOP_AttrBase        rtl8139b_PCIDeviceAttrBase;

    ULONG               rtl8139b_UnitCount;
    struct List         rtl8139b_Units;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (LIBBASE->rtl8139b_PCIDeviceAttrBase)

struct RTL8139Startup
{
    struct MsgPort           *rtl8139sm_SyncPort;
    struct RTL8139Unit       *rtl8139sm_Unit;
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

struct RTL8139Unit {
	struct MinNode          rtl8139u_Node;

	struct RTL8139Base    *rtl8139u_device;

	STRPTR                  rtl8139u_name;
	
	ULONG                   rtl8139u_UnitNum;
	IPTR                    rtl8139u_DriverFlags;

	OOP_Object              *rtl8139u_PCIDevice;
	OOP_Object              *rtl8139u_PCIDriver;
	IPTR                    rtl8139u_IRQ;

	int                     rtl8139u_open_count;
	struct SignalSemaphore  rtl8139u_unit_lock;

	LONG                    rtl8139u_range_count;
	struct MinList          rtl8139u_Openers;
	struct MinList          rtl8139u_multicast_ranges;
	struct MinList          rtl8139u_type_trackers;

	struct timeval          rtl8139u_toutPOLL;
	BOOL                    rtl8139u_toutNEED;

	struct MsgPort          *rtl8139u_TimerSlowPort;
	struct timerequest      *rtl8139u_TimerSlowReq;

	struct MsgPort          *rtl8139u_TimerFastPort;
	struct timerequest      *rtl8139u_TimerFastReq;

	ULONG                   rtl8139u_mtu;
	ULONG                   rtl8139u_flags;
	struct Sana2DeviceQuery  rtl8139u_Sana2Info;
	struct Sana2DeviceStats rtl8139u_stats;
	ULONG                   rtl8139u_special_stats[STAT_COUNT];

	char                    *rtl8139u_rtl_cardname;
	char                    *rtl8139u_rtl_chipname;
	ULONG                  rtl8139u_rtl_chipcapabilities;
	
	ULONG                  rtl8139u_rtl_LinkSpeed;
#define support_fdx     (1 << 0) // Supports Full Duplex
#define support_mii     (1 << 1)
#define support_fset    (1 << 2)
#define support_ltint   (1 << 3)
#define support_dxsuflo (1 << 4)
/* Card Funcs */
	void                    (*initialize)(struct RTL8139Unit *);
	void                    (*deinitialize)(struct RTL8139Unit *);
	int                     (*start)(struct RTL8139Unit *);
	int                     (*stop)(struct RTL8139Unit *);
	int                     (*alloc_rx)(struct RTL8139Unit *);
	void                    (*set_mac_address)(struct RTL8139Unit *);
	void                    (*linkchange)(struct RTL8139Unit *);
	void                    (*linkirq)(struct RTL8139Unit *);
//    ULONG                   (*descr_getlength)(struct ring_desc *prd, ULONG v);
	void                    (*set_multicast)(struct RTL8139Unit *);

	struct Process          *rtl8139u_Process;

	struct Interrupt        rtl8139u_irqhandler;
	struct Interrupt        rtl8139u_touthandler;
	IPTR	                rtl8139u_DeviceID;
	APTR                    rtl8139u_BaseMem;
	IPTR                    rtl8139u_SizeMem;
	IPTR	                  rtl8139u_BaseIO;

	BYTE                    rtl8139u_signal_0;
	BYTE                    rtl8139u_signal_1;
	BYTE                    rtl8139u_signal_2;
	BYTE                    rtl8139u_signal_3;

	struct MsgPort          *rtl8139u_input_port;

	struct MsgPort          *rtl8139u_request_ports[REQUEST_QUEUE_COUNT];

	struct Interrupt        rtl8139u_rx_int;
	struct Interrupt        rtl8139u_tx_int;

	ULONG                   rtl8139u_state;
	APTR                    rtl8139u_mc_list;
	UBYTE                   rtl8139u_dev_addr[6];
	UBYTE                   rtl8139u_org_addr[6];
	struct fe_priv          *rtl8139u_fe_priv;
};

void handle_request(LIBBASETYPEPTR, struct IOSana2Req *);

/* Media selection options. */
enum
{
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
	struct RTL8139Unit      *pci_dev;
	int                     in_shutdown;
	ULONG                   linkspeed;
	int                     duplex;
	int                     autoneg;
	int                     fixed_mode;
	int                     phyaddr;
	int                     wolenabled;
	unsigned int            phy_oui;
	UWORD                   gigabit;
	ULONG                   desc_ver;
	struct SignalSemaphore  lock;

	IPTR                    ring_addr;

/* Start - rtl new */
	int                     full_duplex;

	char                    mii_phys[4]; //MII device address
	unsigned short          advertising;  //NWay media advertising

	unsigned int            rx_config;
	UBYTE                   *rx_buffer;
	unsigned int            rx_buf_len;
	unsigned int            rx_current;

	ULONG                   tx_flag;
	UBYTE                   *tx_buffer;
	unsigned char           *tx_pbuf[NUM_TX_DESC];
	unsigned char           *tx_buf[NUM_TX_DESC];
	unsigned int            tx_dirty;
	unsigned int            tx_current;
/* End - rtl new */
	
	unsigned short          cur_rx;
	ULONG                   refill_rx;

	ULONG                   next_tx, nic_tx;
	ULONG                   tx_flags;

	ULONG                   irqmask;
	ULONG                   need_linktimer;
	struct  timeval         link_timeout;
	UBYTE                   orig_mac[6];
};

#define pci_name(unit)  (unit->rtl8139u_name)

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

void rtl8139nic_get_functions(struct RTL8139Unit *Unit);

/* ***************************** */
/*       RTL8139 DEFINES         */
/* ***************************** */

enum rtl_boardcapabilities 
{
  RTLc_HAS_MII_XCVR    = 0x01, 
  RTLc_HAS_CHIP_XCVR   = 0x02,
  RTLc_HAS_LNK_CHNG    = 0x04, 
  RTLc_HAS_DESC        = 0x08
};

// Symbolic offsets to registers

enum rtl_registers 
{
  RTLr_MAC0             = 0x00,       // Ethernet hardware address
  RTLr_MAR0             = 0x08,       // Multicast filter
  RTLr_TxStatus0        = 0x10,       // Transmit status (Four 32bit registers)
  RTLr_TxStatus1		= 0x14,
  RTLr_TxStatus2		= 0x18,
  RTLr_TxStatus3		= 0x1C,
  RTLr_TxAddr0          = 0x20,       // Tx descriptors (also four 32bit)
  RTLr_TxAddr1			= 0x24,
  RTLr_TxAddr2			= 0x28,
  RTLr_TxAddr3			= 0x2c,
  RTLr_RxBuf            = 0x30, 
  RTLr_RxEarlyCnt       = 0x34, 
  RTLr_RxEarlyStatus    = 0x36,
  RTLr_ChipCmd          = 0x37,
  RTLr_RxBufPtr         = 0x38,
  RTLr_RxBufAddr        = 0x3A,
  RTLr_IntrMask         = 0x3C,
  RTLr_IntrStatus       = 0x3E,
  RTLr_TxConfig         = 0x40,
  RTLr_RxConfig         = 0x44,
  RTLr_Timer            = 0x48,        // A general-purpose counter
  RTLr_RxMissed         = 0x4C,        // 24 bits valid, write clears
  RTLr_Cfg9346          = 0x50, 
  RTLr_Config0          = 0x51, 
  RTLr_Config1          = 0x52,
  RTLr_FlashReg         = 0x54, 	   // Timer Interrupt Register ?
  RTLr_GPPinData        = 0x58, 	   // Media status register ?
  RTLr_GPPinDir         = 0x59, 	   // Configuration register 3
  RTLr_MII_SMI          = 0x5A, 	   // Configuration register 4
  RTLr_HltClk           = 0x5B,		   // Reserved
  RTLr_MultiIntr        = 0x5C,
  RTLr_RERID			= 0x5E,
  RTLr_TxSummary        = 0x60,		   // Transmit status of all descriptors
  RTLr_MII_BMCR         = 0x62, 	   // Basic mode control register
  RTLr_MII_BMSR         = 0x64, 	   // Basic mode status register
  RTLr_NWayAdvert       = 0x66, 	   // Auto negociation expansion register
  RTLr_NWayLPAR         = 0x68,		   // Auto negociation link partner register
  RTLr_NWayExpansion    = 0x6A,		   // Auto negociation expansion register
  RTLr_DIS				= 0x6C,		   // Disconnect counter
  RTLr_FCSC				= 0x6E,		   // False carrier sense counter
  
  // Undocumented registers, but required for proper operation
  RTLr_FIFOTMS          = 0x70,        // FIFO Control and test (N-way tezt register)
  RTLr_CSCR             = 0x74,        // Chip Status and Configuration Register
  RTLr_PARA78           = 0x78, 	   // PHY parameter 1
  RTLr_PARA7c           = 0x7c,        // Magic transceiver parameter register (Twister parameter)
};

enum rtl_chipclearbitmasks
{
  MultiIntrClear = 0xf000,
  CmdClear  = 0xe2,
  ConfigClear = (1 << 7) | (1 << 6) | (1 << 3) | (1 << 2) | (1 << 1)
};

enum rtl_chipcmdbits 
{
  RxBufEmpty = 0x01,
  CmdTxEnb   = 0x04,
  CmdRxEnb   = 0x08,
  CmdReset   = 0x10,
};

// Interrupt register bits

enum rtl_intrstatusbits 
{
  RxOK       = 0x0001,
  RxErr      = 0x0002, 
  TxOK       = 0x0004, 
  TxErr      = 0x0008,
  RxOverflow = 0x0010,
  RxUnderrun = 0x0020, 
  RxFIFOOver = 0x0040, 
  PCSTimeout = 0x4000,
  PCIErr     = 0x8000, 
};

#define RxAckBits (RxFIFOOver | RxOverflow | RxOK)

enum rtl_txstatusbits 
{
  TxHostOwns    = 0x00002000,
  TxUnderrun    = 0x00004000,
  TxStatOK      = 0x00008000,
  TxOutOfWindow = 0x20000000,
  TxAborted     = 0x40000000,
  TxCarrierLost = 0x80000000,
};

enum rtl_rxstatusbits 
{
  RxStatusOK  = 0x0001,
  RxBadAlign  = 0x0002, 
  RxCRCErr    = 0x0004,
  RxTooLong   = 0x0008, 
  RxRunt      = 0x0010, 
  RxBadSymbol = 0x0020, 
  RxBroadcast = 0x2000,
  RxPhysical  = 0x4000, 
  RxMulticast = 0x8000, 
};

// Bits in RxConfig

enum rtl_rxconfigbits
{
  AcceptAllPhys   = 0x01,
  AcceptMyPhys    = 0x02, 
  AcceptMulticast = 0x04, 
  AcceptRunt      = 0x10, 
  AcceptErr       = 0x20, 
  AcceptBroadcast = 0x08,
};

enum rtl_txconfigbits
{
	/* Interframe Gap Time. Only TxIFG96 doesnt violate IEEE 802.3 */
	TxIFGShift = 24,
	TxIFG84 = (0<<TxIFGShift), /* 8.4us / 840ns */
	TxIFG88 = (1<<TxIFGShift), /* 8.8us / 880ns */
	TxIFG92 = (2<<TxIFGShift), /* 9.2us / 920ns */
	TxIFG96 = (3<<TxIFGShift), /* 9.6us / 960ns */

	TxLoopBack = (1<<18) | (1<<17),  /* Enable loopback test mode */
	TxCRC          = (1<<16),                  /* Disable appending CRC to end of Tx Packet */
	TxClearAbt   = (1<<0),                    /* Clear abort (WO) */
	TxDMAShift = 8,                               /* DMA burst value (0-7) is shifted this many bits */
	TxRetryShift = 4,                              /* TXRR value (0-15) is shifted this many bits */
	
	TxVersionMask = 0x7c800000        /* Mask out version bits 30-26, 23 */
};


enum rtl_config1bits
{
	Cfg1_PM_Enable    = 0x01,
	Cfg1_VPD_Enable  = 0x02,
	Cfg1_PIO               = 0x04,
	Cfg1_MMIO           = 0x08,
	LWAKE                  = 0x10,               /* Not on 8139/8139A */
	Cfg1_Driver_Load = 0x20,
	Cfg1_LED0            = 0x40,
	Cfg1_LED1            = 0x80
};

enum rtl_cscrbits 
{
  CSCR_LinkOKBit      = 0x00400, 
  CSCR_LinkDownOffCmd = 0x003c0,
  CSCR_LinkChangeBit  = 0x00800,
  CSCR_LinkStatusBits = 0x0f000, 
  CSCR_LinkDownCmd    = 0x0f3c0,
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

int rtl8139nic_set_rxmode(struct net_device *dev);

#endif
