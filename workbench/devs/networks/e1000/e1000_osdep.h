/*
 * $Id$
 */
/*******************************************************************************

  Intel PRO/1000 Linux driver
  Copyright(c) 1999 - 2008 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/


/* glue for the OS-dependent part of e1000
 * includes register access macros
 */

#ifndef _E1000_OSDEP_H_
#define _E1000_OSDEP_H_

#include "e1000.h"

#define ADVERTISED_1000baseT_Full           (1 << 5)
#define ADVERTISED_Autoneg                  (1 << 6)
#define ADVERTISED_TP                       (1 << 7)
#define ADVERTISED_FIBRE                    (1 << 10)

#define AUTONEG_DISABLE                     0x00
#define AUTONEG_ENABLE                      0x01

#define E1000_FLAG_HAS_SMBUS                (1 << 0)
#define E1000_FLAG_HAS_INTR_MODERATION      (1 << 4)
#define E1000_FLAG_BAD_TX_CARRIER_STATS_FD  (1 << 6)
#define E1000_FLAG_QUAD_PORT_A              (1 << 8)
#define E1000_FLAG_SMART_POWER_DOWN         (1 << 9)

#define E1000_TX_FLAGS_CSUM		            0x00000001
#define E1000_TX_FLAGS_VLAN		            0x00000002
#define E1000_TX_FLAGS_TSO		            0x00000004
#define E1000_TX_FLAGS_IPV4		            0x00000008
#define E1000_TX_FLAGS_VLAN_MASK	        0xffff0000
#define E1000_TX_FLAGS_VLAN_SHIFT	        16

#define ALIGN(x,a) (((x)+(a)-1L)&~((a)-1L))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define max(a, b)  (((a) > (b)) ? (a) : (b))

#define E1000_MAX_INTR                      10

/* TX/RX descriptor defines */
#define E1000_DEFAULT_TXD                  256
#define E1000_MAX_TXD                      256
#define E1000_MIN_TXD                       80
#define E1000_MAX_82544_TXD               4096

#define E1000_DEFAULT_RXD                  256
#define E1000_MAX_RXD                      256

#define E1000_MIN_RXD                       80
#define E1000_MAX_82544_RXD               4096

#define E1000_MIN_ITR_USECS                 10 /* 100000 irq/sec */
#define E1000_MAX_ITR_USECS              10000 /* 100    irq/sec */


/* this is the size past which hardware will drop packets when setting LPE=0 */
#define MAXIMUM_ETHERNET_VLAN_SIZE          1522

/* Supported Rx Buffer Sizes */
#define E1000_RXBUFFER_128                  128
#define E1000_RXBUFFER_256                  256
#define E1000_RXBUFFER_512                  512
#define E1000_RXBUFFER_1024                 1024
#define E1000_RXBUFFER_2048                 2048
#define E1000_RXBUFFER_4096                 4096
#define E1000_RXBUFFER_8192                 8192
#define E1000_RXBUFFER_16384                16384

/* SmartSpeed delimiters */
#define E1000_SMARTSPEED_DOWNSHIFT 3
#define E1000_SMARTSPEED_MAX       15

/* Packet Buffer allocations */
#define E1000_PBA_BYTES_SHIFT 0xA
#define E1000_TX_HEAD_ADDR_SHIFT 7
#define E1000_PBA_TX_MASK 0xFFFF0000

/* Early Receive defines */
#define E1000_ERT_2048 0x100

#define E1000_FC_PAUSE_TIME 0x0680 /* 858 usec */

/* How many Tx Descriptors do we need to call netif_wake_queue ? */
#define E1000_TX_QUEUE_WAKE	16
/* How many Rx Buffers do we bundle into one write to the hardware ? */
#define E1000_RX_BUFFER_WRITE	16	/* Must be power of 2 */

#define AUTO_ALL_MODES            0
#define E1000_EEPROM_82544_APM    0x0004
#define E1000_EEPROM_APME         0x0400

#ifndef E1000_MASTER_SLAVE
/* Switch to override PHY master/slave setting */
#define E1000_MASTER_SLAVE	e1000_ms_hw_default
#endif

/* standard definitions .. */
#ifndef PCIBAR_TYPE_IO
#define PCIBAR_TYPE_IO		0x01
#endif
#ifndef PCICS_COMMAND
#define PCICS_COMMAND		0x04	/* 16 bits */
#endif
#ifndef PCICMB_INVALIDATE
#define PCICMB_INVALIDATE	4	    /* Use memory write and invalidate */
#endif

#define PCI_COMMAND_REGISTER   PCICS_COMMAND
#define CMD_MEM_WRT_INVALIDATE (1 << PCICMB_INVALIDATE)

#ifdef __BIG_ENDIAN
#define E1000_BIG_ENDIAN __BIG_ENDIAN
#endif

#if DEBUG > 0
#if defined(intel_debug)
#define E1K_INTELDEBUG
#endif
#endif

#if defined(E1K_INTELDEBUG)
#define DEBUGOUT(S) D(bug("[%s] %s:" S, ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__));
#define DEBUGOUT1(S, A) D(bug("[%s] %s:" S , ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, A));

#define DEBUGFUNC(F) D(bug("[%s]: " F "()\n", ((struct e1000Unit *)hw->back)->e1ku_name));
#define DEBUGOUT2(S, A, B) D(bug("[%s] %s:" S , ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, A, B));
#define DEBUGOUT3(S, A, B, C) D(bug("[%s] %s:" S , ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, A, B, C));
#define DEBUGOUT7(S, A, B, C, D, E, F, G) D(bug("[%s] %s:" S , ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, A, B, C, D, E, F, G));
#else
#define DEBUGOUT(S)
#define DEBUGOUT1(S, A...)

#define DEBUGFUNC(F) DEBUGOUT(F "\n")
#define DEBUGOUT2 DEBUGOUT1
#define DEBUGOUT3 DEBUGOUT2
#define DEBUGOUT7 DEBUGOUT3
#endif

#define MMIO_R8(addr)		(*((volatile UBYTE *)(addr)))
#define MMIO_R16(addr)		(*((volatile UWORD *)(addr)))
#define MMIO_R32(addr)		(*((volatile ULONG *)(addr)))

extern void MMIO_W8(APTR, UBYTE);
extern void MMIO_W16(APTR, UWORD);
extern void MMIO_W32(APTR, ULONG);

#define E1000_REGISTER(a, reg) (((a)->mac.type >= e1000_82543) ? reg : e1000_translate_register_82542(reg))

#define E1000_WRITE_REG(a, reg, value) (MMIO_W32((APTR) ((a)->hw_addr + E1000_REGISTER(a, reg)), (value)))

#define E1000_READ_REG(a, reg) (*((volatile ULONG *)((a)->hw_addr + E1000_REGISTER(a, reg))))

#define E1000_WRITE_REG_ARRAY(a, reg, offset, value) (MMIO_W32((APTR)(((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 2))), (value)))
#define E1000_READ_REG_ARRAY(a, reg, offset) (*((volatile ULONG *)((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 2))))

#define E1000_READ_REG_ARRAY_DWORD E1000_READ_REG_ARRAY
#define E1000_WRITE_REG_ARRAY_DWORD E1000_WRITE_REG_ARRAY

#define E1000_WRITE_REG_ARRAY_WORD(a, reg, offset, value) (MMIO_W16((APTR) ((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 1)), (value)))
 
#define E1000_READ_REG_ARRAY_WORD(a, reg, offset) (*((volatile UWORD *)((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 1)))))

#define E1000_WRITE_REG_ARRAY_BYTE(a, reg, offset, value) (MMIO_W8((APTR) ((a)->hw_addr + E1000_REGISTER(a, reg) + (offset)), (value)))

#define E1000_READ_REG_ARRAY_BYTE(a, reg, offset) (*((volatile UBYTE *)((a)->hw_addr + E1000_REGISTER(a, reg) + (offset)))))

#define E1000_WRITE_REG_IO(a, reg, offset) do { \
    LONGOUT(((a)->io_base), reg);                  \
    LONGOUT(((a)->io_base + 4), offset);      } while(0)

#define E1000_WRITE_FLUSH(a) E1000_READ_REG(a, E1000_STATUS)

#define E1000_WRITE_FLASH_REG(a, reg, value) (MMIO_W32((APTR) ((a)->hw_addr + E1000_REGISTER(a, reg)), (value)))

#define E1000_WRITE_FLASH_REG16(a, reg, value) (MMIO_W16((APTR) ((a)->hw_addr + E1000_REGISTER(a, reg)), (value)))

#define E1000_READ_FLASH_REG(a, reg) (*((volatile ULONG *)((a)->hw_addr + E1000_REGISTER(a, reg))))

#define E1000_READ_FLASH_REG16(a, reg) (*((volatile UWORD *)((a)->hw_addr + E1000_REGISTER(a, reg))))

#define SANA2_SPECIAL_STAT_COUNT 3

struct e1000Unit {
    struct Node             e1ku_Node;

    struct e1000Base        *e1ku_device;                     /* Pointer to our device base */

    STRPTR                  e1ku_name;
    
    ULONG                   e1ku_UnitNum;
    IPTR                    e1ku_DriverFlags;

    OOP_Object              *e1ku_PCIDevice;
    OOP_Object              *e1ku_PCIDriver;
    IPTR                    e1ku_IRQ;

    UWORD		    e1ku_PCIeCap;
    int                     e1ku_open_count;
    struct SignalSemaphore  e1ku_unit_lock;
    
    LONG                    e1ku_range_count;
    struct MinList          e1ku_Openers;
    struct MinList          e1ku_multicast_ranges;
    struct MinList          e1ku_type_trackers;

    ULONG                   e1ku_mtu;
    ULONG                   e1ku_ifflags;
    struct Sana2DeviceQuery e1ku_Sana2Info;
    struct Sana2DeviceStats e1ku_stats;
    ULONG                   e1ku_special_stats[SANA2_SPECIAL_STAT_COUNT];

    struct Process          *e1ku_Process;

    struct MsgPort          *e1ku_input_port;

    struct MsgPort          *e1ku_request_ports[REQUEST_QUEUE_COUNT];

    struct Interrupt         e1ku_irqhandler;
    struct Interrupt         e1ku_touthandler;

    struct MsgPort          *e1ku_TimerSlowPort;
    struct timerequest      *e1ku_TimerSlowReq;

    struct MsgPort          *e1ku_TimerFastPort;
    struct timerequest      *e1ku_TimerFastReq;

    struct MsgPort          e1ku_DelayPort;
    struct timerequest      e1ku_DelayReq;

    BYTE                    e1ku_signal_0;
    BYTE                    e1ku_signal_1;
    BYTE                    e1ku_signal_2;
    BYTE                    e1ku_signal_3;

    UBYTE                   e1ku_dev_addr[6];
    UBYTE                   e1ku_org_addr[6];

    struct Interrupt        e1ku_tx_int;

/* Start : Intel e1000 specific */

	IPTR                    e1ku_Private00;                 /* Pointer to Intels driver-code hardware struct */
    ULONG                   e1ku_MMIOSize;
    ULONG                   e1ku_FlashSize;

    ULONG                   e1ku_hwflags;

    ULONG                   e1ku_txRing_QueueSize;
    ULONG                   e1ku_rxRing_QueueSize;
    struct e1000_tx_ring    *e1ku_txRing;
    struct e1000_rx_ring    *e1ku_rxRing;

    APTR                    e1ku_mc_list;

    ULONG                   rx_buffer_len;

	ULONG                   txd_cmd;

    ULONG                   e1ku_frame_max;
    ULONG                   e1ku_frame_min;

	ULONG                   e1ku_tx_fifo_head;
	ULONG                   e1ku_tx_head_addr;
	ULONG                   e1ku_tx_fifo_size;

	struct e1000_hw_stats *e1ku_hw_stats;

	BOOL                    detect_tx_hung;

/* End : Intel e1000 specific */

    struct timeval          e1ku_toutPOLL;
    BOOL                    e1ku_toutNEED;

};

#define net_device e1000Unit

#define E1000_DESC_UNUSED(R) \
	((((R)->next_to_clean > (R)->next_to_use) ? 0 : (R)->count) + \
	(R)->next_to_clean - (R)->next_to_use - 1)

#define E1000_RX_DESC_EXT(R, i)	    \
	(&(((union e1000_rx_desc_extended *)((R).desc))[i]))
#define E1000_GET_DESC(R, i, type) ((struct type *)&((R)->desc[i]))
#define E1000_RX_DESC(R, i)		&(R->desc[i])
#define E1000_TX_DESC(R, i)		&(R->desc[i])
#define E1000_CONTEXT_DESC(R, i)	E1000_GET_DESC(R, i, e1000_context_desc)

/* e1000.c definitions */
/* Timer functions */
/* Some workarounds require millisecond delays and are run during interrupt
 * context.  Most notably, when establishing link, the phy may need tweaking
 * but cannot process phy register reads/writes faster than millisecond
 * intervals...and we establish link due to a "link status change" interrupt.
 */
void e1000_msec_delay(struct net_device *, ULONG);
void e1000_msec_delay_irq(struct net_device *, ULONG);
void e1000_usec_delay(struct net_device *, ULONG);
#define msec_delay(msec)     e1000_msec_delay((struct net_device *)hw->back, msec)
#define msec_delay_irq(msec) e1000_msec_delay_irq((struct net_device *)hw->back, msec)
#define usec_delay(usec)     e1000_usec_delay((struct net_device *)hw->back, usec)
/* misc */
int request_irq(struct net_device *);
void e1000func_irq_disable(struct net_device *);
void e1000func_irq_enable(struct net_device *);
void e1000func_reset(struct net_device *);
int e1000func_setup_all_tx_resources(struct net_device *);
int e1000func_setup_all_rx_resources(struct net_device *);
void e1000func_configure(struct net_device *);
void e1000func_free_tx_resources(struct net_device *, struct e1000_tx_ring *);
void e1000func_free_rx_resources(struct net_device *, struct e1000_rx_ring *);
int e1000func_set_mac(struct net_device *);
void e1000func_set_multi(struct net_device *);
BOOL e1000func_clean_tx_irq(struct net_device *, struct e1000_tx_ring *);
BOOL e1000func_clean_rx_irq(struct net_device *, struct e1000_rx_ring *);

#endif /* _E1000_OSDEP_H_ */
