/* rhine.c:Fast Ethernet driver for Linux. */
/*
	Adapted 09-jan-2000 by Paolo Marini (paolom@prisma-eng.it)

        originally written by Donald Becker.

	This software may be used and distributed according to the terms
	of the GNU Public License (GPL), incorporated herein by reference.
	Drivers derived from this code also fall under the GPL and must retain
	this authorship and copyright notice.

	Under no circumstances are the authors responsible for
	the proper functioning of this software, nor do the authors assume any
	responsibility for damages incurred with its use.

	This driver is designed for the VIA VT86C100A Rhine-II PCI Fast Ethernet
	controller.

*/

static const char *version = "rhine.c v1.0.0 2000-01-07\n";

/* A few user-configurable values. */

/* Size of the in-memory receive ring. */
#define RX_BUF_LEN_IDX	3	/* 0==8K, 1==16K, 2==32K, 3==64K */
#define RX_BUF_LEN (8192 << RX_BUF_LEN_IDX)

/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE	1536
#define RX_BUF_SIZE	1536

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256	/* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024. */
#define RX_FIFO_THRESH	4	/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	4	/* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST	4

/* Operational parameters that usually are not changed. */
/* Time in jiffies before concluding the transmitter is hung. */
#define TX_TIMEOUT  ((2000*HZ)/1000)

#include "etherboot.h"
#include "nic.h"
#include "pci.h"
#include "cards.h"

/* define all ioaddr */

#define byPAR0				ioaddr
#define byRCR				ioaddr + 6
#define byTCR				ioaddr + 7
#define byCR0				ioaddr + 8
#define byCR1				ioaddr + 9
#define byISR0				ioaddr + 0x0c
#define byISR1				ioaddr + 0x0d
#define byIMR0				ioaddr + 0x0e
#define byIMR1				ioaddr + 0x0f
#define byMAR0				ioaddr + 0x10
#define byMAR1				ioaddr + 0x11
#define byMAR2				ioaddr + 0x12
#define byMAR3				ioaddr + 0x13
#define byMAR4				ioaddr + 0x14
#define byMAR5				ioaddr + 0x15
#define byMAR6				ioaddr + 0x16
#define byMAR7				ioaddr + 0x17
#define dwCurrentRxDescAddr		ioaddr + 0x18
#define dwCurrentTxDescAddr		ioaddr + 0x1c
#define dwCurrentRDSE0			ioaddr + 0x20
#define dwCurrentRDSE1			ioaddr + 0x24
#define dwCurrentRDSE2			ioaddr + 0x28
#define dwCurrentRDSE3			ioaddr + 0x2c
#define dwNextRDSE0			ioaddr + 0x30
#define dwNextRDSE1			ioaddr + 0x34
#define dwNextRDSE2			ioaddr + 0x38
#define dwNextRDSE3			ioaddr + 0x3c
#define dwCurrentTDSE0			ioaddr + 0x40
#define dwCurrentTDSE1			ioaddr + 0x44
#define dwCurrentTDSE2			ioaddr + 0x48
#define dwCurrentTDSE3			ioaddr + 0x4c
#define dwNextTDSE0			ioaddr + 0x50
#define dwNextTDSE1			ioaddr + 0x54
#define dwNextTDSE2			ioaddr + 0x58
#define dwNextTDSE3			ioaddr + 0x5c
#define dwCurrRxDMAPtr			ioaddr + 0x60
#define dwCurrTxDMAPtr			ioaddr + 0x64
#define byMPHY				ioaddr + 0x6c
#define byMIISR				ioaddr + 0x6d
#define byBCR0				ioaddr + 0x6e
#define byBCR1				ioaddr + 0x6f
#define byMIICR				ioaddr + 0x70
#define byMIIAD				ioaddr + 0x71
#define wMIIDATA			ioaddr + 0x72
#define byEECSR				ioaddr + 0x74
#define byTEST				ioaddr + 0x75
#define byGPIO				ioaddr + 0x76
#define byCFGA				ioaddr + 0x78
#define byCFGB				ioaddr + 0x79
#define byCFGC				ioaddr + 0x7a
#define byCFGD				ioaddr + 0x7b
#define wTallyCntMPA			ioaddr + 0x7c
#define wTallyCntCRC			ioaddr + 0x7d
/*---------------------  Exioaddr Definitions -------------------------*/

/*
 * Bits in the RCR register
 */

#define RCR_RRFT2		0x80
#define RCR_RRFT1		0x40
#define RCR_RRFT0		0x20
#define RCR_PROM		0x10
#define RCR_AB			0x08
#define RCR_AM			0x04
#define RCR_AR			0x02
#define RCR_SEP			0x01

/*
 * Bits in the TCR register
 */

#define TCR_RTSF		0x80
#define TCR_RTFT1		0x40
#define TCR_RTFT0		0x20
#define TCR_OFSET		0x08
#define TCR_LB1			0x04	/* loopback[1] */
#define TCR_LB0			0x02	/* loopback[0] */

/*
 * Bits in the CR0 register
 */

#define CR0_RDMD		0x40	/* rx descriptor polling demand */
#define CR0_TDMD		0x20	/* tx descriptor polling demand */
#define CR0_TXON		0x10
#define CR0_RXON		0x08
#define CR0_STOP		0x04	/* stop NIC, default = 1 */
#define CR0_STRT		0x02	/* start NIC */
#define CR0_INIT		0x01	/* start init process */


/*
 * Bits in the CR1 register
 */

#define CR1_SFRST		0x80	/* software reset */
#define CR1_RDMD1		0x40	/* RDMD1 */
#define CR1_TDMD1		0x20	/* TDMD1 */
#define CR1_KEYPAG		0x10	/* turn on par/key */
#define CR1_DPOLL		0x08	/* disable rx/tx auto polling */
#define CR1_FDX			0x04	/* full duplex mode */
#define CR1_ETEN		0x02	/* early tx mode */
#define CR1_EREN		0x01	/* early rx mode */

/*
 * Bits in the CR register
 */

#define CR_RDMD			0x0040	/* rx descriptor polling demand */
#define CR_TDMD			0x0020	/* tx descriptor polling demand */
#define CR_TXON			0x0010
#define CR_RXON			0x0008
#define CR_STOP			0x0004	/* stop NIC, default = 1 */
#define CR_STRT			0x0002	/* start NIC */
#define CR_INIT			0x0001	/* start init process */
#define CR_SFRST		0x8000	/* software reset */
#define CR_RDMD1		0x4000	/* RDMD1 */
#define CR_TDMD1		0x2000	/* TDMD1 */
#define CR_KEYPAG		0x1000	/* turn on par/key */
#define CR_DPOLL		0x0800	/* disable rx/tx auto polling */
#define CR_FDX			0x0400	/* full duplex mode */
#define CR_ETEN			0x0200	/* early tx mode */
#define CR_EREN			0x0100	/* early rx mode */

/*
 * Bits in the IMR0 register
 */

#define IMR0_CNTM		0x80
#define IMR0_BEM		0x40
#define IMR0_RUM		0x20
#define IMR0_TUM		0x10
#define IMR0_TXEM		0x08
#define IMR0_RXEM		0x04
#define IMR0_PTXM		0x02
#define IMR0_PRXM		0x01

/* define imrshadow */

#define IMRShadow		0x5AFF

/*
 * Bits in the IMR1 register
 */

#define IMR1_INITM		0x80
#define IMR1_SRCM		0x40
#define IMR1_NBFM		0x10
#define IMR1_PRAIM		0x08
#define IMR1_RES0M		0x04
#define IMR1_ETM		0x02
#define IMR1_ERM		0x01

/*
 * Bits in the ISR register
 */

#define ISR_INITI		0x8000
#define ISR_SRCI		0x4000
#define ISR_ABTI		0x2000
#define ISR_NORBF		0x1000
#define ISR_PKTRA		0x0800
#define ISR_RES0		0x0400
#define ISR_ETI			0x0200
#define ISR_ERI			0x0100
#define ISR_CNT			0x0080
#define ISR_BE			0x0040
#define ISR_RU			0x0020
#define ISR_TU			0x0010
#define ISR_TXE			0x0008
#define ISR_RXE			0x0004
#define ISR_PTX			0x0002
#define ISR_PRX			0x0001

/*
 * Bits in the ISR0 register
 */

#define ISR0_CNT		0x80
#define ISR0_BE			0x40
#define ISR0_RU			0x20
#define ISR0_TU			0x10
#define ISR0_TXE		0x08
#define ISR0_RXE		0x04
#define ISR0_PTX		0x02
#define ISR0_PRX		0x01

/*
 * Bits in the ISR1 register
 */

#define ISR1_INITI		0x80
#define ISR1_SRCI		0x40
#define ISR1_NORBF		0x10
#define ISR1_PKTRA		0x08
#define ISR1_ETI		0x02
#define ISR1_ERI		0x01

/* ISR ABNORMAL CONDITION */

#define ISR_ABNORMAL ISR_BE+ISR_RU+ISR_TU+ISR_CNT+ISR_NORBF+ISR_PKTRA

/*
 * Bits in the MIISR register
 */

#define MIISR_MIIERR		0x08
#define MIISR_MRERR		0x04
#define MIISR_LNKFL		0x02
#define MIISR_SPEED		0x01

/*
 * Bits in the MIICR register
 */

#define MIICR_MAUTO		0x80
#define MIICR_RCMD		0x40
#define MIICR_WCMD		0x20
#define MIICR_MDPM		0x10
#define MIICR_MOUT		0x08
#define MIICR_MDO		0x04
#define MIICR_MDI		0x02
#define MIICR_MDC		0x01

/*
 * Bits in the EECSR register
 */

#define EECSR_EEPR		0x80	/* eeprom programed status, 73h means programed */
#define EECSR_EMBP		0x40	/* eeprom embeded programming */
#define EECSR_AUTOLD		0x20	/* eeprom content reload */
#define EECSR_DPM		0x10	/* eeprom direct programming */
#define EECSR_CS		0x08	/* eeprom CS pin */
#define EECSR_SK		0x04	/* eeprom SK pin */
#define EECSR_DI		0x02	/* eeprom DI pin */
#define EECSR_DO		0x01	/* eeprom DO pin */

/*
 * Bits in the BCR0 register
 */

#define BCR0_CRFT2		0x20
#define BCR0_CRFT1		0x10
#define BCR0_CRFT0		0x08
#define BCR0_DMAL2		0x04
#define BCR0_DMAL1		0x02
#define BCR0_DMAL0		0x01

/*
 * Bits in the BCR1 register
 */

#define BCR1_CTSF		0x20
#define BCR1_CTFT1		0x10
#define BCR1_CTFT0		0x08
#define BCR1_POT2		0x04
#define BCR1_POT1		0x02
#define BCR1_POT0		0x01

/*
 * Bits in the CFGA register
 */

#define CFGA_EELOAD		0x80	/* enable eeprom embeded and direct programming */
#define CFGA_JUMPER		0x40
#define CFGA_MTGPIO		0x08
#define CFGA_T10EN		0x02
#define CFGA_AUTO		0x01

/*
 * Bits in the CFGB register
 */

#define CFGB_PD			0x80
#define CFGB_POLEN		0x02
#define CFGB_LNKEN		0x01

/*
 * Bits in the CFGC register
 */

#define CFGC_M10TIO		0x80
#define CFGC_M10POL		0x40
#define CFGC_PHY1		0x20
#define CFGC_PHY0		0x10
#define CFGC_BTSEL		0x08
#define CFGC_BPS2		0x04	/* bootrom select[2] */
#define CFGC_BPS1		0x02	/* bootrom select[1] */
#define CFGC_BPS0		0x01	/* bootrom select[0] */

/*
 * Bits in the CFGD register
 */

#define CFGD_GPIOEN		0x80
#define CFGD_DIAG		0x40
#define CFGD_MAGIC		0x10
#define CFGD_CFDX		0x04
#define CFGD_CEREN		0x02
#define CFGD_CETEN		0x01

/* Bits in RSR */
#define RSR_RERR		0x00000001
#define RSR_CRC			0x00000002
#define RSR_FAE			0x00000004
#define RSR_FOV			0x00000008
#define RSR_LONG		0x00000010
#define RSR_RUNT		0x00000020
#define RSR_SERR		0x00000040
#define RSR_BUFF		0x00000080
#define RSR_EDP			0x00000100
#define RSR_STP			0x00000200
#define RSR_CHN			0x00000400
#define RSR_PHY			0x00000800
#define RSR_BAR			0x00001000
#define RSR_MAR			0x00002000
#define RSR_RXOK		0x00008000
#define RSR_ABNORMAL		RSR_RERR+RSR_LONG+RSR_RUNT

/* Bits in TSR */
#define TSR_NCR0		0x00000001
#define TSR_NCR1		0x00000002
#define TSR_NCR2		0x00000004
#define TSR_NCR3		0x00000008
#define TSR_COLS		0x00000010
#define TSR_CDH			0x00000080
#define TSR_ABT			0x00000100
#define TSR_OWC			0x00000200
#define TSR_CRS			0x00000400
#define TSR_UDF			0x00000800
#define TSR_TBUFF		0x00001000
#define TSR_SERR		0x00002000
#define TSR_JAB			0x00004000
#define TSR_TERR		0x00008000
#define TSR_ABNORMAL		TSR_TERR+TSR_OWC+TSR_ABT+TSR_JAB+TSR_CRS
#define TSR_OWN_BIT		0x80000000

#define CB_DELAY_LOOP_WAIT	10	/* 10ms */
/* enabled mask value of irq */

#define W_IMR_MASK_VALUE	0x1BFF	/* initial value of IMR */

/* Ethernet address filter type */
#define PKT_TYPE_DIRECTED	0x0001	/* obsolete, directed address is always accepted */
#define PKT_TYPE_MULTICAST	0x0002
#define PKT_TYPE_ALL_MULTICAST	0x0004
#define PKT_TYPE_BROADCAST	0x0008
#define PKT_TYPE_PROMISCUOUS	0x0020
#define PKT_TYPE_LONG		0x2000
#define PKT_TYPE_RUNT		0x4000
#define PKT_TYPE_ERROR		0x8000	/* accept error packets, e.g. CRC error */

/* Loopback mode */

#define NIC_LB_NONE		0x00
#define NIC_LB_INTERNAL		0x01
#define NIC_LB_PHY		0x02	/* MII or Internal-10BaseT loopback */

#define TX_RING_SIZE		2
#define RX_RING_SIZE		2
#define PKT_BUF_SZ		1536	/* Size of each temporary Rx buffer. */

/* Transmit and receive descriptors definition */

struct rhine_tx_desc
{
    union VTC_tx_status_tag
    {
	struct
	{
	    unsigned long ncro:1;
	    unsigned long ncr1:1;
	    unsigned long ncr2:1;
	    unsigned long ncr3:1;
	    unsigned long cols:1;
	    unsigned long reserve_1:2;
	    unsigned long cdh:1;
	    unsigned long abt:1;
	    unsigned long owc:1;
	    unsigned long crs:1;
	    unsigned long udf:1;
	    unsigned long tbuff:1;
	    unsigned long serr:1;
	    unsigned long jab:1;
	    unsigned long terr:1;
	    unsigned long reserve_2:15;
	    unsigned long own_bit:1;
	}
	bits;
	unsigned long lw;
    }
    tx_status;

    union VTC_tx_ctrl_tag
    {
	struct
	{
	    unsigned long tx_buf_size:11;
	    unsigned long extend_tx_buf_size:4;
	    unsigned long chn:1;
	    unsigned long crc:1;
	    unsigned long reserve_1:4;
	    unsigned long stp:1;
	    unsigned long edp:1;
	    unsigned long ic:1;
	    unsigned long reserve_2:8;
	}
	bits;
	unsigned long lw;
    }
    tx_ctrl;

    unsigned long buf_addr_1:32;
    unsigned long buf_addr_2:32;

};

struct rhine_rx_desc
{
    union VTC_rx_status_tag
    {
	struct
	{
	    unsigned long rerr:1;
	    unsigned long crc_error:1;
	    unsigned long fae:1;
	    unsigned long fov:1;
	    unsigned long toolong:1;
	    unsigned long runt:1;
	    unsigned long serr:1;
	    unsigned long buff:1;
	    unsigned long edp:1;
	    unsigned long stp:1;
	    unsigned long chn:1;
	    unsigned long phy:1;
	    unsigned long bar:1;
	    unsigned long mar:1;
	    unsigned long reserve_1:1;
	    unsigned long rxok:1;
	    unsigned long frame_length:11;
	    unsigned long reverve_2:4;
	    unsigned long own_bit:1;
	}
	bits;
	unsigned long lw;
    }
    rx_status;

    union VTC_rx_ctrl_tag
    {
	struct
	{
	    unsigned long rx_buf_size:11;
	    unsigned long extend_rx_buf_size:4;
	    unsigned long reserved_1:17;
	}
	bits;
	unsigned long lw;
    }
    rx_ctrl;

    unsigned long buf_addr_1:32;
    unsigned long buf_addr_2:32;

};


/* The I/O extent. */
#define rhine_TOTAL_SIZE 0x80

#ifdef	HAVE_DEVLIST
struct netdev_entry rhine_drv =
    { "rhine", rhine_probe, rhine_TOTAL_SIZE, NULL };
#endif

static int rhine_debug = 1;

/*
				Theory of Operation

I. Board Compatibility

This driver is designed for the VIA 86c100A Rhine-II PCI Fast Ethernet
controller.

II. Board-specific settings

Boards with this chip are functional only in a bus-master PCI slot.

Many operational settings are loaded from the EEPROM to the Config word at
offset 0x78.  This driver assumes that they are correct.
If this driver is compiled to use PCI memory space operations the EEPROM
must be configured to enable memory ops.

III. Driver operation

IIIa. Ring buffers

This driver uses two statically allocated fixed-size descriptor lists
formed into rings by a branch from the final descriptor to the beginning of
the list.  The ring sizes are set at compile time by RX/TX_RING_SIZE.

IIIb/c. Transmit/Receive Structure

This driver attempts to use a zero-copy receive and transmit scheme.

Alas, all data buffers are required to start on a 32 bit boundary, so
the driver must often copy transmit packets into bounce buffers.

The driver allocates full frame size skbuffs for the Rx ring buffers at
open() time and passes the skb->data field to the chip as receive data
buffers.  When an incoming frame is less than RX_COPYBREAK bytes long,
a fresh skbuff is allocated and the frame is copied to the new skbuff.
When the incoming frame is larger, the skbuff is passed directly up the
protocol stack.  Buffers consumed this way are replaced by newly allocated
skbuffs in the last phase of netdev_rx().

The RX_COPYBREAK value is chosen to trade-off the memory wasted by
using a full-sized skbuff for small frames vs. the copying costs of larger
frames.  New boards are typically used in generously configured machines
and the underfilled buffers have negligible impact compared to the benefit of
a single allocation size, so the default value of zero results in never
copying packets.  When copying is done, the cost is usually mitigated by using
a combined copy/checksum routine.  Copying also preloads the cache, which is
most useful with small frames.

Since the VIA chips are only able to transfer data to buffers on 32 bit
boundaries, the the IP header at offset 14 in an ethernet frame isn't
longword aligned for further processing.  Copying these unaligned buffers
has the beneficial effect of 16-byte aligning the IP header.

IIId. Synchronization

The driver runs as two independent, single-threaded flows of control.  One
is the send-packet routine, which enforces single-threaded use by the
dev->tbusy flag.  The other thread is the interrupt handler, which is single
threaded by the hardware and interrupt handling software.

The send packet thread has partial control over the Tx ring and 'dev->tbusy'
flag.  It sets the tbusy flag whenever it's queuing a Tx packet. If the next
queue slot is empty, it clears the tbusy flag when finished otherwise it sets
the 'lp->tx_full' flag.

The interrupt handler has exclusive control over the Rx ring and records stats
from the Tx ring.  After reaping the stats, it marks the Tx queue entry as
empty by incrementing the dirty_tx mark. Iff the 'lp->tx_full' flag is set, it
clears both the tx_full and tbusy flags.

IV. Notes

IVb. References

Preliminary VT86C100A manual from http://www.via.com.tw/
http://cesdis.gsfc.nasa.gov/linux/misc/100mbps.html
http://cesdis.gsfc.nasa.gov/linux/misc/NWay.html

IVc. Errata

The VT86C100A manual is not reliable information.
The chip does not handle unaligned transmit or receive buffers, resulting
in significant performance degradation for bounce buffer copies on transmit
and unaligned IP headers on receive.
The chip does not pad to minimum transmit length.

*/

#define PCI_VENDOR_ID_FET		0x1106
#define PCI_DEVICE_ID_FET_3043		0x3043

/* The rest of these values should never change. */
#define NUM_TX_DESC	2	/* Number of Tx descriptor registers. */

static struct rhine_private
{
    char devname[8];		/* Used only for kernel debugging. */
    const char *product_name;
    struct rhine_rx_desc *rx_ring;
    struct rhine_tx_desc *tx_ring;
    char *rx_buffs[RX_RING_SIZE];
    char *tx_buffs[TX_RING_SIZE];

    /* temporary Rx buffers. */

    int chip_id;
    int chip_revision;
    unsigned short ioaddr;
    unsigned int cur_rx, cur_tx;	/* The next free and used entries */
    unsigned int dirty_rx, dirty_tx;
    /* The saved address of a sent-in-place packet/buffer, for skfree(). */
    struct sk_buff *tx_skbuff[TX_RING_SIZE];
    unsigned char mc_filter[8];	/* Current multicast filter. */
    char phys[4];		/* MII device addresses. */
    unsigned int tx_full:1;	/* The Tx queue is full. */
    unsigned int full_duplex:1;	/* Full-duplex operation requested. */
    unsigned int default_port:4;	/* Last dev->if_port value. */
    unsigned int media2:4;	/* Secondary monitored media port. */
    unsigned int medialock:1;	/* Don't sense media type. */
    unsigned int mediasense:1;	/* Media sensing in progress. */
}
rhine;

static struct nic *rhine_probe1 (struct nic *dev, int ioaddr,
				 int chip_id, int options);
static int QueryAuto (int);
static int ReadMII (int byMIIIndex, int);
static void WriteMII (char, char, char, int);
static void MIIDelay (void);
static void rhine_init_ring (struct nic *dev);
static void rhine_disable (struct nic *nic);
static void rhine_reset (struct nic *nic);
static int rhine_poll (struct nic *nic);
static void rhine_transmit (struct nic *nic, const char *d, unsigned int t,
			    unsigned int s, const char *p);

/* Linux support functions */
#define virt_to_bus(x) ((unsigned long)x)
#define bus_to_virt(x) ((void *)x)

/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void
rhine_init_ring (struct nic *nic)
{
    struct rhine_private *tp = (struct rhine_private *) nic->priv_data;
    int i;

    tp->tx_full = 0;
    tp->cur_rx = tp->cur_tx = 0;
    tp->dirty_rx = tp->dirty_tx = 0;

    for (i = 0; i < RX_RING_SIZE; i++)
    {

	tp->rx_ring[i].rx_status.bits.own_bit = 1;
	tp->rx_ring[i].rx_ctrl.bits.rx_buf_size = 1536;

	tp->rx_ring[i].buf_addr_1 = virt_to_bus (tp->rx_buffs[i]);
	tp->rx_ring[i].buf_addr_2 = virt_to_bus (&tp->rx_ring[i + 1]);
	/* printf("[%d]buf1=%hX,buf2=%hX",i,tp->rx_ring[i].buf_addr_1,tp->rx_ring[i].buf_addr_2); */
    }
    /* Mark the last entry as wrapping the ring. */
    /* tp->rx_ring[i-1].rx_ctrl.bits.rx_buf_size =1518; */
    tp->rx_ring[i - 1].buf_addr_2 = virt_to_bus (&tp->rx_ring[0]);
    /*printf("[%d]buf1=%hX,buf2=%hX",i-1,tp->rx_ring[i-1].buf_addr_1,tp->rx_ring[i-1].buf_addr_2); */

    /* The Tx buffer descriptor is filled in as needed, but we
       do need to clear the ownership bit. */

    for (i = 0; i < TX_RING_SIZE; i++)
    {

	tp->tx_ring[i].tx_status.lw = 0;
	tp->tx_ring[i].tx_ctrl.lw = 0x00e08000;
	tp->tx_ring[i].buf_addr_1 = virt_to_bus (tp->tx_buffs[i]);
	tp->tx_ring[i].buf_addr_2 = virt_to_bus (&tp->tx_ring[i + 1]);
	/* printf("[%d]buf1=%hX,buf2=%hX",i,tp->tx_ring[i].buf_addr_1,tp->tx_ring[i].buf_addr_2); */
    }

    tp->tx_ring[i - 1].buf_addr_2 = virt_to_bus (&tp->tx_ring[0]);
    /* printf("[%d]buf1=%hX,buf2=%hX",i,tp->tx_ring[i-1].buf_addr_1,tp->tx_ring[i-1].buf_addr_2); */
}

int
QueryAuto (int ioaddr)
{
    int byMIIIndex;
    int MIIReturn;

	int advertising,mii_reg5;
	int negociated;

    byMIIIndex = 0x04;
    MIIReturn = ReadMII (byMIIIndex, ioaddr);
	advertising=MIIReturn;

    byMIIIndex = 0x05;
    MIIReturn = ReadMII (byMIIIndex, ioaddr);
	mii_reg5=MIIReturn;

	negociated=mii_reg5 & advertising;

	if ( (negociated & 0x100) || (negociated & 0x1C0) == 0x40 )
		return 1;
	else
		return 0;

}

int
ReadMII (int byMIIIndex, int ioaddr)
{
    int ReturnMII;
    char byMIIAdrbak;
    char byMIICRbak;
    char byMIItemp;

    byMIIAdrbak = inb (byMIIAD);
    byMIICRbak = inb (byMIICR);
    outb (byMIICRbak & 0x7f, byMIICR);
    MIIDelay ();

    outb (byMIIIndex, byMIIAD);
    MIIDelay ();

    outb (inb (byMIICR) | 0x40, byMIICR);

    byMIItemp = inb (byMIICR);
    byMIItemp = byMIItemp & 0x40;

    while (byMIItemp != 0)
    {
	byMIItemp = inb (byMIICR);
	byMIItemp = byMIItemp & 0x40;
    }
    MIIDelay ();

    ReturnMII = inw (wMIIDATA);

    outb (byMIIAdrbak, byMIIAD);
    outb (byMIICRbak, byMIICR);
    MIIDelay ();

    return (ReturnMII);

}

void
WriteMII (char byMIISetByte, char byMIISetBit, char byMIIOP, int ioaddr)
{
    int ReadMIItmp;
    int MIIMask;
    char byMIIAdrbak;
    char byMIICRbak;
    char byMIItemp;


    byMIIAdrbak = inb (byMIIAD);

    byMIICRbak = inb (byMIICR);
    outb (byMIICRbak & 0x7f, byMIICR);
    MIIDelay ();
    outb (byMIISetByte, byMIIAD);
    MIIDelay ();

    outb (inb (byMIICR) | 0x40, byMIICR);

    byMIItemp = inb (byMIICR);
    byMIItemp = byMIItemp & 0x40;

    while (byMIItemp != 0)
    {
	byMIItemp = inb (byMIICR);
	byMIItemp = byMIItemp & 0x40;
    }
    MIIDelay ();

    ReadMIItmp = inw (wMIIDATA);
    MIIMask = 0x0001;
    MIIMask = MIIMask << byMIISetBit;


    if (byMIIOP == 0)
    {
	MIIMask = ~MIIMask;
	ReadMIItmp = ReadMIItmp & MIIMask;
    }
    else
    {
	ReadMIItmp = ReadMIItmp | MIIMask;

    }
    outw (ReadMIItmp, wMIIDATA);
    MIIDelay ();

    outb (inb (byMIICR) | 0x20, byMIICR);
    byMIItemp = inb (byMIICR);
    byMIItemp = byMIItemp & 0x20;

    while (byMIItemp != 0)
    {
	byMIItemp = inb (byMIICR);
	byMIItemp = byMIItemp & 0x20;
    }
    MIIDelay ();

    outb (byMIIAdrbak & 0x7f, byMIIAD);
    outb (byMIICRbak, byMIICR);
    MIIDelay ();

}

void
MIIDelay (void)
{
    int i;
    for (i = 0; i < 0x7fff; i++)
    {
	inb (0x61);
	inb (0x61);
	inb (0x61);
	inb (0x61);
    }
}

struct nic *
rhine_probe (struct nic *nic, unsigned short *probeaddrs,
	       struct pci_device *pci)
{
    if (!pci->ioaddr)
	return NULL;
    nic = rhine_probe1 (nic, pci->ioaddr, 0, -1);

    if (nic)
	adjust_pci_device(pci);
    nic->poll = rhine_poll;
    nic->transmit = rhine_transmit;
    nic->reset = rhine_reset;
    nic->disable = rhine_disable;
    rhine_reset (nic);

    return nic;
}

static struct nic *
rhine_probe1 (struct nic *nic, int ioaddr, int chip_id, int options)
{
    struct rhine_private *tp;
    static int did_version = 0;	/* Already printed version info. */
    int i;
    unsigned int timeout;
    int FDXFlag;
    int byMIIvalue, LineSpeed, MIICRbak;

    if (rhine_debug > 0 && did_version++ == 0)
	printf (version);
    /* Perhaps this should be read from the EEPROM? */
    for (i = 0; i < ETH_ALEN; i++)
	nic->node_addr[i] = inb (byPAR0 + i);
    printf ("IO address %hX Ethernet Address: %!\n", ioaddr, nic->node_addr);

    /* restart MII auto-negotiation */
    WriteMII (0, 9, 1, ioaddr);
    printf ("Analyzing Media type,this will take several seconds........");
    for (i = 0; i < 5; i++)
    {
	/* need to wait 1 millisecond - we will round it up to 50-100ms */
	timeout = currticks() + 2;
	for (timeout = currticks() + 2; currticks() < timeout;)
	    /* nothing */;
	if (ReadMII (1, ioaddr) & 0x0020)
	    break;
    }
    printf ("OK\n");

#if	0
	/* JJM : for Debug */
	printf("MII : Address %hhX ",inb(ioaddr+0x6c));
	{
	 unsigned char st1,st2,adv1,adv2,l1,l2;
	
	 st1=ReadMII(1,ioaddr)>>8;
	 st2=ReadMII(1,ioaddr)&0xFF;
	 adv1=ReadMII(4,ioaddr)>>8;
	 adv2=ReadMII(4,ioaddr)&0xFF;
	 l1=ReadMII(5,ioaddr)>>8;
	 l2=ReadMII(5,ioaddr)&0xFF;
	 printf(" status 0x%hhX%hhX, advertising 0x%hhX%hhX, link 0x%hhX%hhX\n", st1,st2,adv1,adv2,l1,l2);
	}
#endif

    /* query MII to know LineSpeed,duplex mode */
    byMIIvalue = inb (ioaddr + 0x6d);
    LineSpeed = byMIIvalue & MIISR_SPEED;
    if (LineSpeed != 0)						//JJM
    {
	printf ("Linespeed=10Mbs");
    }
    else
    {
	printf ("Linespeed=100Mbs");
    }
	
    FDXFlag = QueryAuto (ioaddr);
    if (FDXFlag == 1)
    {
	printf (" Fullduplex\n");
	outw (CR_FDX, byCR0);
    }
    else
    {
	printf (" Halfduplex\n");
    }


    /* set MII 10 FULL ON */
    WriteMII (17, 1, 1, ioaddr);

    /* turn on MII link change */
    MIICRbak = inb (byMIICR);
    outb (MIICRbak & 0x7F, byMIICR);
    MIIDelay ();
    outb (0x41, byMIIAD);
    MIIDelay ();

    /* while((inb(byMIIAD)&0x20)==0) ; */
    outb (MIICRbak | 0x80, byMIICR);

    nic->priv_data = &rhine;
    tp = &rhine;
    tp->chip_id = chip_id;
    tp->ioaddr = ioaddr;
    tp->phys[0] = -1;

    /* The lower four bits are the media type. */
    if (options > 0)
    {
	tp->full_duplex = (options & 16) ? 1 : 0;
	tp->default_port = options & 15;
	if (tp->default_port)
	    tp->medialock = 1;
    }
    return nic;
}

static void
rhine_disable (struct nic *nic)
{
    struct rhine_private *tp = (struct rhine_private *) nic->priv_data;
    int ioaddr = tp->ioaddr;

    printf ("rhine disable\n");
    /* Switch to loopback mode to avoid hardware races. */
    writeb(0x60 | 0x01, byTCR);
    /* Stop the chip's Tx and Rx processes. */
    writew(CR_STOP, byCR0);
}

/**************************************************************************
ETH_RESET - Reset adapter
***************************************************************************/
static void
rhine_reset (struct nic *nic)
{
    struct rhine_private *tp = (struct rhine_private *) nic->priv_data;
    int ioaddr = tp->ioaddr;
    int i, j;
    int FDXFlag, CRbak;
    int rx_ring_tmp, rx_ring_tmp1;
    int tx_ring_tmp, tx_ring_tmp1;
    int rx_bufs_tmp, rx_bufs_tmp1;
    int tx_bufs_tmp, tx_bufs_tmp1;

#ifdef	USE_LOWMEM_BUFFER
#define buf1 (0x10000 - (RX_RING_SIZE * PKT_BUF_SZ + 32))
#define buf2 (buf1 - (RX_RING_SIZE * PKT_BUF_SZ + 32))
#define desc1 (buf2 - (TX_RING_SIZE * sizeof (struct rhine_tx_desc) + 32))
#define desc2 (desc1 - (TX_RING_SIZE * sizeof (struct rhine_tx_desc) + 32))
#else
    static char buf1[RX_RING_SIZE * PKT_BUF_SZ + 32];
    static char buf2[RX_RING_SIZE * PKT_BUF_SZ + 32];
    static char desc1[TX_RING_SIZE * sizeof (struct rhine_tx_desc) + 32];
    static char desc2[TX_RING_SIZE * sizeof (struct rhine_tx_desc) + 32];
#endif

    /* printf ("rhine_reset\n"); */
    /* Soft reset the chip. */
    /*outb(CmdReset, ioaddr + ChipCmd); */

    tx_bufs_tmp = (int) buf1;
    tx_ring_tmp = (int) desc1;
    rx_bufs_tmp = (int) buf2;
    rx_ring_tmp = (int) desc2;

    /* tune RD TD 32 byte alignment */
    rx_ring_tmp1 = (int) virt_to_bus ((char *) rx_ring_tmp);
    j = (rx_ring_tmp1 + 32) & (~0x1f);
    /* printf ("txring[%d]", j); */
    tp->rx_ring = (struct rhine_rx_desc *) bus_to_virt (j);

    tx_ring_tmp1 = (int) virt_to_bus ((char *) tx_ring_tmp);
    j = (tx_ring_tmp1 + 32) & (~0x1f);
    tp->tx_ring = (struct rhine_tx_desc *) bus_to_virt (j);
    /* printf ("rxring[%X]", j); */


    tx_bufs_tmp1 = (int) virt_to_bus ((char *) tx_bufs_tmp);
    j = (int) (tx_bufs_tmp1 + 32) & (~0x1f);
    tx_bufs_tmp = (int) bus_to_virt (j);
    /* printf ("txb[%X]", j); */

    rx_bufs_tmp1 = (int) virt_to_bus ((char *) rx_bufs_tmp);
    j = (int) (rx_bufs_tmp1 + 32) & (~0x1f);
    rx_bufs_tmp = (int) bus_to_virt (j);
    /* printf ("rxb[%X][%X]", rx_bufs_tmp1, j); */

    for (i = 0; i < RX_RING_SIZE; i++)
    {
	tp->rx_buffs[i] = (char *) rx_bufs_tmp;
	/* printf("r[%X]",tp->rx_buffs[i]); */
	rx_bufs_tmp += 1536;
    }

    for (i = 0; i < TX_RING_SIZE; i++)
    {
	tp->tx_buffs[i] = (char *) tx_bufs_tmp;
	/* printf("t[%X]",tp->tx_buffs[i]);  */
	tx_bufs_tmp += 1536;
    }

    /* software reset */
    outb (CR1_SFRST, byCR1);
    MIIDelay ();

    /* printf ("init ring"); */
    rhine_init_ring (nic);
    /*write TD RD Descriptor to MAC */
    outl (virt_to_bus (tp->rx_ring), dwCurrentRxDescAddr);
    outl (virt_to_bus (tp->tx_ring), dwCurrentTxDescAddr);

    /* close IMR */
    outw (0x0000, byIMR0);

    /* set TCR RCR threshold */
    outb (0x06, byBCR0);
    outb (0x00, byBCR1);
    outb (0x2c, byRCR);
    outb (0x60, byTCR);
    /* Set Fulldupex */
    FDXFlag = QueryAuto (ioaddr);
    if (FDXFlag == 1)
    {
	outb (CFGD_CFDX, byCFGD);
	outw (CR_FDX, byCR0);
    }

    /* KICK NIC to WORK */
    CRbak = inw (byCR0);
    CRbak = CRbak & 0xFFFB;	/* not CR_STOP */
    outw ((CRbak | CR_STRT | CR_TXON | CR_RXON | CR_DPOLL), byCR0);

    /*set IMR to work */
    outw (IMRShadow, byIMR0);
}

static int
rhine_poll (struct nic *nic)
{
    struct rhine_private *tp = (struct rhine_private *) nic->priv_data;
    int rxstatus, good = 0;;

    if (tp->rx_ring[tp->cur_rx].rx_status.bits.own_bit == 0)
    {
	rxstatus = tp->rx_ring[tp->cur_rx].rx_status.lw;
	if ((rxstatus & 0x0300) != 0x0300)
	{
	    printf("rhine_poll: bad status\n");
	}
	else if (rxstatus & (RSR_ABNORMAL))
	{
	    printf ("rxerr[%X]\n", rxstatus);
	}
	else
	    good = 1;

	if (good)
	{
	    nic->packetlen = tp->rx_ring[tp->cur_rx].rx_status.bits.frame_length;
	    memcpy (nic->packet, tp->rx_buffs[tp->cur_rx], nic->packetlen);
	    /* printf ("Packet RXed\n"); */
	}
	tp->rx_ring[tp->cur_rx].rx_status.bits.own_bit = 1;
	tp->cur_rx++;
	tp->cur_rx = tp->cur_rx % RX_RING_SIZE;
    }
    return good;
}

static void
rhine_transmit (struct nic *nic,
		const char *d, unsigned int t, unsigned int s, const char *p)
{
    struct rhine_private *tp = (struct rhine_private *) nic->priv_data;
    int ioaddr = tp->ioaddr;
    int entry;
    unsigned char CR1bak;

    /*printf ("rhine_transmit\n"); */
    /* setup ethernet header */


    /* Calculate the next Tx descriptor entry. */
    entry = tp->cur_tx % TX_RING_SIZE;

    memcpy (tp->tx_buffs[entry], d, ETH_ALEN);	/* dst */
    memcpy (tp->tx_buffs[entry] + ETH_ALEN, nic->node_addr, ETH_ALEN);	/* src */
    *((char *) tp->tx_buffs[entry] + 12) = t >> 8;	/* type */
    *((char *) tp->tx_buffs[entry] + 13) = t;
    memcpy (tp->tx_buffs[entry] + ETH_HLEN, p, s);
    s += ETH_HLEN;
    while (s < ETH_ZLEN)
	*((char *) tp->tx_buffs[entry] + ETH_HLEN + (s++)) = 0;

    tp->tx_ring[entry].tx_ctrl.bits.tx_buf_size = ETH_HLEN + s;

    tp->tx_ring[entry].tx_status.bits.own_bit = 1;


    CR1bak = inb (byCR1);

    CR1bak = CR1bak | CR1_TDMD1;
    /*printf("tdsw=[%X]",tp->tx_ring[entry].tx_status.lw); */
    /*printf("tdcw=[%X]",tp->tx_ring[entry].tx_ctrl.lw); */
    /*printf("tdbuf1=[%X]",tp->tx_ring[entry].buf_addr_1); */
    /*printf("tdbuf2=[%X]",tp->tx_ring[entry].buf_addr_2); */
    /*printf("td1=[%X]",inl(dwCurrentTDSE0)); */
    /*printf("td2=[%X]",inl(dwCurrentTDSE1)); */
    /*printf("td3=[%X]",inl(dwCurrentTDSE2)); */
    /*printf("td4=[%X]",inl(dwCurrentTDSE3)); */

    outb (CR1bak, byCR1);
    tp->cur_tx++;

    /*outw(IMRShadow,byIMR0); */
    /*dev_kfree_skb(tp->tx_skbuff[entry], FREE_WRITE); */
    /*tp->tx_skbuff[entry] = 0; */
}

/* EOF via-rhine.c */
