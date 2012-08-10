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

#include "rtl8139.h"

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>

#include <aros/libcall.h>
#include <aros/macros.h>
#include <aros/io.h>

#include <oop/oop.h>

#include <devices/timer.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <hidd/pci.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

#include <hardware/intbits.h>

#include <stdlib.h>

#include "unit.h"
#include LC_LIBDEFS_FILE

/* A bit fixed linux stuff here :) */

#undef LIBBASE
#define LIBBASE (unit->rtl8139u_device)

#if defined(__i386__) || defined(__x86_64__)
#define TIMER_RPROK 3599597124UL

static ULONG usec2tick(ULONG usec)
{
	ULONG ret, timer_rpr = TIMER_RPROK;
	asm volatile("movl $0,%%eax; divl %2":"=a"(ret):"d"(usec),"m"(timer_rpr));
	return ret;
}

void udelay(LONG usec)
{
	int oldtick, tick;
	usec = usec2tick(usec);

	BYTEOUT(0x43, 0x80);
	oldtick = BYTEIN(0x42);
	oldtick += BYTEIN(0x42) << 8;

	while (usec > 0)
	{
		BYTEOUT(0x43, 0x80);
		tick = BYTEIN(0x42);
		tick += BYTEIN(0x42) << 8;

		usec -= (oldtick - tick);
		if (tick > oldtick) usec -= 0x10000;
		oldtick = tick;
	}
}
#else

struct timerequest timerio;
struct MsgPort *timermp;

void udelay(LONG usec)
{
	timerio.tr_node.io_Command = TR_ADDREQUEST;
	timerio.tr_time.tv_secs = usec / 1000000;
	timerio.tr_time.tv_micro = usec % 1000000;
	DoIO(&timerio.tr_node);
}

int init_timer(void)
{
	if ((timermp = CreateMsgPort())) {
		timerio.tr_node.io_Message.mn_Node.ln_Type=NT_MESSAGE;
		timerio.tr_node.io_Message.mn_ReplyPort = timermp;
		timerio.tr_node.io_Message.mn_Length=sizeof(timerio);
		if (0 == OpenDevice("timer.device", UNIT_MICROHZ, &timerio.tr_node, 0)) {
			return TRUE;
		}
	}
	return FALSE;
}
ADD2INIT(init_timer, 10);

void exit_timer(void)
{
	CloseDevice(&timerio.tr_node);
	DeleteMsgPort(timermp);
}
ADD2EXIT(exit_timer, 10);
#endif

static inline struct fe_priv *get_pcnpriv(struct net_device *unit)
{
	return unit->rtl8139u_fe_priv;
}

static inline UBYTE *get_hwbase(struct net_device *unit)
{
	return (UBYTE *)unit->rtl8139u_BaseMem;
}

static int read_eeprom(long base, int location, int addr_len)
{
  int i;
  unsigned retval = 0;
  long rtlprom_addr = base + RTLr_Cfg9346;
  int read_cmd = location | (EE_READ_CMD << addr_len);

  BYTEOUT(rtlprom_addr, EE_ENB & ~EE_CS);
  BYTEOUT(rtlprom_addr, EE_ENB);

  // Shift the read command bits out
  for (i = 4 + addr_len; i >= 0; i--) 
  {
	int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
	BYTEOUT(rtlprom_addr, EE_ENB | dataval);
	eeprom_delay(rtlprom_addr);
	BYTEOUT(rtlprom_addr, EE_ENB | dataval | EE_SHIFT_CLK);
	eeprom_delay(rtlprom_addr);
  }
  BYTEOUT(rtlprom_addr, EE_ENB);
  eeprom_delay(rtlprom_addr);

  for (i = 16; i > 0; i--) 
  {
	BYTEOUT(rtlprom_addr, EE_ENB | EE_SHIFT_CLK);
	eeprom_delay(rtlprom_addr);
	retval = (retval << 1) | ((BYTEIN(rtlprom_addr) & EE_DATA_READ) ? 1 : 0);
	BYTEOUT(rtlprom_addr, EE_ENB);
	eeprom_delay(rtlprom_addr);
  }

  // Terminate EEPROM access
  BYTEOUT(rtlprom_addr, ~EE_CS);
  eeprom_delay(rtlprom_addr);
  return retval;
}

// Syncronize the MII management interface by shifting 32 one bits out
static char mii_2_8139_map[8] = 
{
  RTLr_MII_BMCR, RTLr_MII_BMSR, 0, 0, RTLr_NWayAdvert, RTLr_NWayLPAR, RTLr_NWayExpansion, 0 
};

static void mdio_sync(long base)
{
	int i;

	for (i = 32; i >= 0; i--) 
	{
		BYTEOUT(base, MDIO_WRITE1);
		mdio_delay(base);
		BYTEOUT(base, MDIO_WRITE1 | MDIO_CLK);
		mdio_delay(base);
	}
}

static int mdio_read(struct net_device *unit, int phy_id, int location)
{
	// struct fe_priv *np = get_pcnpriv(unit);
	UBYTE *base = get_hwbase(unit);
	int mii_cmd = (0xf6 << 10) | (phy_id << 5) | location;
	int retval = 0;
	int i;

	if (phy_id > 31) 
	{  
		// Really a 8139.  Use internal registers
		return location < 8 && mii_2_8139_map[location] ? WORDIN(base + mii_2_8139_map[location]) : 0;
	}

	mdio_sync((IPTR)base + RTLr_MII_SMI);

	// Shift the read command bits out
	for (i = 15; i >= 0; i--) 
	{
		int dataval = (mii_cmd & (1 << i)) ? MDIO_DATA_OUT : 0;

		BYTEOUT(base + RTLr_MII_SMI, MDIO_DIR | dataval);
		mdio_delay(base + RTLr_MII_SMI);
		BYTEOUT(base + RTLr_MII_SMI, MDIO_DIR | dataval | MDIO_CLK);
		mdio_delay(base + RTLr_MII_SMI);
	}

	// Read the two transition, 16 data, and wire-idle bits
	for (i = 19; i > 0; i--) 
	{
		BYTEOUT(base + RTLr_MII_SMI, 0);
		mdio_delay(base + RTLr_MII_SMI);
		retval = (retval << 1) | ((BYTEIN(base + RTLr_MII_SMI) & MDIO_DATA_IN) ? 1 : 0);
		BYTEOUT(base + RTLr_MII_SMI, MDIO_CLK);
		mdio_delay(base + RTLr_MII_SMI);
	}

	return (retval >> 1) & 0xffff;
}

#if 0
static void rtl8139nic_start_rx(struct net_device *unit)
{
	// struct fe_priv *np = get_pcnpriv(unit);
	// UBYTE *base = get_hwbase(unit);

RTLD(bug("[%s] rtl8139nic_start_rx\n", unit->rtl8139u_name))
	// Already running? Stop it.
/* TODO: Handle starting/stopping Rx */
}

static void rtl8139nic_stop_rx(struct net_device *unit)
{
	// UBYTE *base = get_hwbase(unit);

RTLD(bug("[%s] rtl8139nic_stop_rx\n", unit->rtl8139u_name))
/* TODO: Handle starting/stopping Rx */
}

static void rtl8139nic_start_tx(struct net_device *unit)
{
	// UBYTE *base = get_hwbase(unit);

RTLD(bug("[%s] rtl8139nic_start_tx()\n", unit->rtl8139u_name))
/* TODO: Handle starting/stopping Tx */
}

static void rtl8139nic_stop_tx(struct net_device *unit)
{
	// UBYTE *base = get_hwbase(unit);

RTLD(bug("[%s] rtl8139nic_stop_tx()\n", unit->rtl8139u_name))
/* TODO: Handle starting/stopping Tx */
}

static void rtl8139nic_txrx_reset(struct net_device *unit)
{
	// struct fe_priv *np = get_pcnpriv(unit);
	// UBYTE *base = get_hwbase(unit);

RTLD(bug("[%s] rtl8139nic_txrx_reset()\n", unit->rtl8139u_name))
}
#endif

/*
 * rtl8139nic_set_multicast: unit->set_multicast function
 * Called with unit->xmit_lock held.
 */
static void rtl8139nic_set_multicast(struct net_device *unit)
{
	// struct fe_priv *np = get_pcnpriv(unit);
	// UBYTE *base = get_hwbase(unit);
	ULONG addr[2];
	ULONG mask[2];
	// ULONG pff;

RTLD(bug("[%s] rtl8139nic_set_multicast()\n", unit->rtl8139u_name))

	memset(addr, 0, sizeof(addr));
	memset(mask, 0, sizeof(mask));
}

static void rtl8139nic_deinitialize(struct net_device *unit)
{

}

static void rtl8139nic_get_mac(struct net_device *unit, char * addr, BOOL fromROM)
{
	UBYTE *base = get_hwbase(unit);
	int i;

RTLD(bug("[%s] rtl8139nic_get_mac()\n",unit->rtl8139u_name))
	if (fromROM)
	{
		int addr_len = read_eeprom((IPTR)base, 0, 8) == 0x8129 ? 8 : 6;
		int mac_add = 0;
		for (i = 0; i < 3; i++)
		{
			UWORD mac_curr = read_eeprom((IPTR)base, i + 7, addr_len);
			addr[mac_add++] = mac_curr & 0xff;
			addr[mac_add++] = (mac_curr >> 8) & 0xff;
		}
	}
	else
	{
		ULONG mac_cur = 0;
		mac_cur = LONGIN(base + RTLr_MAC0 + 0);
		addr[0] = mac_cur & 0xFF;
		addr[1] = (mac_cur >> 8) & 0xFF;
		addr[2] = (mac_cur >> 16) & 0xFF;
		addr[3] = (mac_cur >> 24) & 0xFF;
		mac_cur = LONGIN(base + RTLr_MAC0 + 4);
		addr[4] = mac_cur & 0xFF;
		addr[5] = (mac_cur >> 8) & 0xFF;
	}
}

static void rtl8139nic_set_mac(struct net_device *unit)
{
	UBYTE *base = get_hwbase(unit);
	// int i,j;

RTLD(bug("[%s] rtl8139nic_set_mac()\n",unit->rtl8139u_name))

	BYTEOUT(base + RTLr_Cfg9346, 0xc0);

	LONGOUT(base + RTLr_MAC0 + 0,
		unit->rtl8139u_dev_addr[0] |
		(unit->rtl8139u_dev_addr[1] << 8) |
		(unit->rtl8139u_dev_addr[2] << 16) |
		(unit->rtl8139u_dev_addr[3] << 24));
	LONGOUT(base + RTLr_MAC0 + 4,
		unit->rtl8139u_dev_addr[4] |
		(unit->rtl8139u_dev_addr[5] << 8));

	BYTEOUT(base + RTLr_Cfg9346, 0x00);

RTLD(
	/* Read it back to be certain! */
	TEXT	newmac[6];
	rtl8139nic_get_mac(unit, newmac, FALSE);

	bug("[%s] rtl8139nic_set_mac: New MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl8139u_name,
			newmac[0], newmac[1], newmac[2],
			newmac[3], newmac[4], newmac[5]))
}

static void rtl8139nic_initialize(struct net_device *unit)
{
	struct fe_priv *np = unit->rtl8139u_fe_priv;
	UBYTE *base = get_hwbase(unit);
	int config1;
	// int i;

	config1 = BYTEIN(base + RTLr_Config1);
	if (unit->rtl8139u_rtl_chipcapabilities & RTLc_HAS_MII_XCVR)
	{
		// 8129
		BYTEOUT(base + RTLr_Config1, config1 & ~0x03);
	}
RTLD(bug("[%s] Chipset brought out of low power mode.\n", unit->rtl8139u_name))

	rtl8139nic_get_mac(unit, np->orig_mac, TRUE);

	int phy, phy_idx = 0;
	if (unit->rtl8139u_rtl_chipcapabilities & RTLc_HAS_MII_XCVR)
	{
		for (phy = 0; phy < 32 && phy_idx < sizeof(np->mii_phys); phy++)
		{
			int mii_status = mdio_read(unit, phy, 1);
			if (mii_status != 0xffff && mii_status != 0x0000)
			{
				np->mii_phys[phy_idx++] = phy;
				np->advertising = mdio_read(unit, phy, 4);
RTLD(bug("[%s] MII transceiver %d status 0x%4.4x advertising %4.4x\n", unit->rtl8139u_name,
																		phy, mii_status, np->advertising))
			}
		}
	}

	if (phy_idx == 0)
	{
RTLD(bug("[%s] No MII transceiver found, Assuming SYM transceiver\n", unit->rtl8139u_name))
		np->mii_phys[0] = 32;
	}
		
	unit->rtl8139u_dev_addr[0] = unit->rtl8139u_org_addr[0] = np->orig_mac[0];
	unit->rtl8139u_dev_addr[1] = unit->rtl8139u_org_addr[1] = np->orig_mac[1];
	unit->rtl8139u_dev_addr[2] = unit->rtl8139u_org_addr[2] = np->orig_mac[2];
	unit->rtl8139u_dev_addr[3] = unit->rtl8139u_org_addr[3] = np->orig_mac[3];
	unit->rtl8139u_dev_addr[4] = unit->rtl8139u_org_addr[4] = np->orig_mac[4];
	unit->rtl8139u_dev_addr[5] = unit->rtl8139u_org_addr[5] = np->orig_mac[5];

RTLD(bug("[%s] MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", unit->rtl8139u_name,
			unit->rtl8139u_dev_addr[0], unit->rtl8139u_dev_addr[1], unit->rtl8139u_dev_addr[2],
			unit->rtl8139u_dev_addr[3], unit->rtl8139u_dev_addr[4], unit->rtl8139u_dev_addr[5]))
  
	BYTEOUT(base + RTLr_Cfg9346, 0xc0);
	if (unit->rtl8139u_rtl_chipcapabilities & RTLc_HAS_MII_XCVR)
	{
		// 8129
		BYTEOUT(base + RTLr_Config1, 0x03);
	}
	BYTEOUT(base + RTLr_HltClk, 'H'); //Disable the chips clock ('R' enables)
RTLD(bug("[%s] Chipset put into low power mode.\n", unit->rtl8139u_name))
}

static void rtl8139nic_drain_tx(struct net_device *unit)
{
	// struct fe_priv *np = get_pcnpriv(unit);
	// int i;

//	for (i = 0; i < NUM_TX_DESC; i++)
//	{
/* TODO: rtl8139nic_drain_tx does nothing atm. */
//	}
}

static void rtl8139nic_drain_rx(struct net_device *unit)
{
	// struct fe_priv *np = get_pcnpriv(unit);
	// int i;

//	for (i = 0; i < RX_RING_SIZE; i++)
//	{
/* TODO: rtl8139nic_drain_rx does nothing atm. */
//	}
}


static void drain_ring(struct net_device *unit)
{
	rtl8139nic_drain_tx(unit);
	rtl8139nic_drain_rx(unit);
}

static int request_irq(struct net_device *unit)
{
RTLD(bug("[%s] request_irq()\n", unit->rtl8139u_name))

    AddIntServer(INTB_KERNEL + unit->rtl8139u_IRQ, &unit->rtl8139u_irqhandler);
    AddIntServer(INTB_VERTB, &unit->rtl8139u_touthandler);

RTLD(bug("[%s] request_irq: IRQ Handlers configured\n", unit->rtl8139u_name))
	return 1;
}

static void free_irq(struct net_device *unit)
{
    RemIntServer(INTB_KERNEL + unit->rtl8139u_IRQ, &unit->rtl8139u_irqhandler);
    RemIntServer(INTB_VERTB, &unit->rtl8139u_touthandler);
}

int rtl8139nic_set_rxmode(struct net_device *unit)
{
	struct fe_priv *np = get_pcnpriv(unit);
	UBYTE *base = get_hwbase(unit);
	ULONG mc_filter[2];         //Multicast hash filter.
	ULONG rx_mode;

RTLD(bug("[%s] rtl8139nic_set_rxmode(flags %x)\n", unit->rtl8139u_name, unit->rtl8139u_flags))

/* TODO: Fix set_rxmode.. doesnt load multicast list atm .. */
	if (unit->rtl8139u_flags & IFF_PROMISC)
	{
RTLD(bug("[%s] rtl8139nic_set_rxmode: Mode: PROMISC\n",unit->rtl8139u_name))
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys | AcceptAllPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	}
	else if (unit->rtl8139u_flags & IFF_ALLMULTI)
	{
RTLD(bug("[%s] rtl8139nic_set_rxmode: Mode: ALLMULTI\n",unit->rtl8139u_name))
		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	}
	else
	{
//		struct mclist *mclist;
//		int i;
RTLD(bug("[%s] rtl8139nic_set_rxmode: Mode: DEFAULT\n",unit->rtl8139u_name))

		rx_mode = AcceptBroadcast | AcceptMulticast | AcceptMyPhys;
		/*if (unit->mc_count < multicast_filter_limit)
		{*/
			mc_filter[1] = mc_filter[0] = 0;
			/*for (i = 0, mclist = dev->netif->mclist; mclist && i < dev->netif->mccount; i++, mclist = mclist->next)
			{
		  		set_bit(mc_filter, ether_crc(6, (unsigned char *) &mclist->hwaddr) >> 26);
			}
		}
		else
		{
			mc_filter[1] = mc_filter[0] = 0xffffffff;
		}*/
	}

	LONGOUT(base + RTLr_RxConfig, np->rx_config | rx_mode);
	LONGOUT(base + RTLr_MAR0 + 0, mc_filter[0]);
	LONGOUT(base + RTLr_MAR0 + 4, mc_filter[1]);

	return 0;
}

static int rtl8139nic_open(struct net_device *unit)
{
	struct fe_priv *np = get_pcnpriv(unit);
	UBYTE *base = get_hwbase(unit);
	int ret, i, rx_buf_len_idx;

	rx_buf_len_idx = RX_BUF_LEN_IDX;

	ret = request_irq(unit);
	if (ret)
	{
		goto out_drain;
	}

	np->rx_buf_len = 8192 << rx_buf_len_idx;
		
	np->rx_buffer = HIDD_PCIDriver_AllocPCIMem(
						unit->rtl8139u_PCIDriver,
						(np->rx_buf_len + 16 + (TX_BUF_SIZE * NUM_TX_DESC))
					);

	if (np->rx_buffer != NULL)
	{
		np->tx_buffer = HIDD_PCIDriver_AllocPCIMem(
							unit->rtl8139u_PCIDriver,
							(np->rx_buf_len + 16 + (TX_BUF_SIZE * NUM_TX_DESC))
						);
	}

	if ((np->rx_buffer != NULL) && (np->tx_buffer != NULL))
	{
RTLD(bug("[%s] rtl8139nic_open: Allocated IO Buffers [ %d x Tx @ %x] [ Rx @ %x, %d bytes]\n",
                        unit->rtl8139u_name,
						NUM_TX_DESC, np->tx_buffer,
						np->rx_buffer,  np->rx_buf_len))
   
		np->tx_dirty = np->tx_current = 0;

		for (i = 0; i < NUM_TX_DESC; i++)
		{
			np->tx_pbuf[i] = NULL;
			np->tx_buf[i]  = np->tx_buffer + (i * TX_BUF_SIZE);
		}
RTLD(bug("[%s] rtl8139nic_open: TX Buffers initialised\n",unit->rtl8139u_name))

		// Early Tx threshold: 256 bytes
		np->tx_flag = (TX_FIFO_THRESH << 11) & 0x003f0000;
		np->rx_config = (RX_FIFO_THRESH << 13) | (rx_buf_len_idx << 11) | (RX_DMA_BURST << 8);
		
		BYTEOUT(base + RTLr_ChipCmd,  (BYTEIN(base + RTLr_ChipCmd) & CmdClear) | CmdReset);
		udelay(100);
		for (i = 1000; i > 0; i--)
		{
			if ((BYTEIN(base + RTLr_ChipCmd) & CmdReset) == 0)
			{
				break;
			}
		}
RTLD(bug("[%s] rtl8139nic_open: NIC Reset\n",unit->rtl8139u_name))

		rtl8139nic_set_mac(unit);
RTLD(bug("[%s] rtl8139nic_open: copied MAC address\n",unit->rtl8139u_name))

		np->rx_current = 0;

		// Unlock
		BYTEOUT(base + RTLr_Cfg9346, 0xc0);

		//Enable Tx/Rx so we can set the config(s)
		BYTEOUT(base + RTLr_ChipCmd, (BYTEIN(base + RTLr_ChipCmd) & CmdClear) |
									 CmdRxEnb | CmdTxEnb);
		LONGOUT(base + RTLr_RxConfig, np->rx_config);
		LONGOUT(base + RTLr_TxConfig, TX_DMA_BURST << 8);

RTLD(bug("[%s] rtl8139nic_open: Enabled Tx/Rx\n",unit->rtl8139u_name))

		/* check_duplex */
		if (np->mii_phys[0] >= 0 || (unit->rtl8139u_rtl_chipcapabilities & RTLc_HAS_MII_XCVR))
		{
			UWORD mii_reg5 = mdio_read(unit, np->mii_phys[0], 5);
			if (mii_reg5 != 0xffff)
			{
				if (((mii_reg5 & 0x0100) == 0x0100) || ((mii_reg5 & 0x00c0) == 0x0040))
				{
					np->full_duplex = 1;
				}
			}

			if ((mii_reg5 == 0 ) || !(mii_reg5 & 0x0180))
			{
			   unit->rtl8139u_rtl_LinkSpeed = 10000000;
			}
			else
			{
			   unit->rtl8139u_rtl_LinkSpeed = 100000000;				
			}
			
RTLD(bug("[%s] rtl8139nic_open: Setting %s%s-duplex based on auto-neg partner ability %4.4x\n",unit->rtl8139u_name,
																							 mii_reg5 == 0 ? "" : (mii_reg5 & 0x0180) ? "100mbps " : "10mbps ",
																							 np->full_duplex ? "full" : "half", mii_reg5))
		}

		if (unit->rtl8139u_rtl_chipcapabilities & RTLc_HAS_MII_XCVR)
		{
			// 8129
			BYTEOUT(base + RTLr_Config1, np->full_duplex ? 0x60 : 0x20);
		}
		// Lock
		BYTEOUT(base + RTLr_Cfg9346, 0x00);
		udelay(10);
		LONGOUT(base + RTLr_RxBuf, (IPTR)np->rx_buffer);

		//Start the chips Tx/Rx processes
		LONGOUT(base + RTLr_RxMissed, 0);

		rtl8139nic_set_rxmode(unit);

		WORDOUT(base + RTLr_MultiIntr, WORDIN(RTLr_MultiIntr) & MultiIntrClear);

		BYTEOUT(base + RTLr_ChipCmd, (BYTEIN(base + RTLr_ChipCmd) & CmdClear) |
		                             CmdRxEnb | CmdTxEnb);

		// Enable all known interrupts by setting the interrupt mask ..
		WORDOUT(base + RTLr_IntrMask, PCIErr |
									  PCSTimeout |
									  RxUnderrun |
									  RxOverflow |
									  RxFIFOOver |
									  TxErr |
									  TxOK |
									  RxErr |
									  RxOK);
		
	   unit->rtl8139u_flags |= IFF_UP;
	   ReportEvents(LIBBASE, unit, S2EVENT_ONLINE);
RTLD(bug("[%s] rtl8139nic_open: Device set as ONLINE\n",unit->rtl8139u_name))
	}
	return 0;

out_drain:
	drain_ring(unit);
	return ret;
}

static int rtl8139nic_close(struct net_device *unit)
{
	struct fe_priv *np = get_pcnpriv(unit);

	unit->rtl8139u_flags &= ~IFF_UP;

	ObtainSemaphore(&np->lock);
	np->in_shutdown = 1;
	ReleaseSemaphore(&np->lock);

	unit->rtl8139u_toutNEED = FALSE;

	ObtainSemaphore(&np->lock);

	rtl8139nic_deinitialize(unit);    // Stop the chipset and set it in 16bit-mode

	ReleaseSemaphore(&np->lock);

	free_irq(unit);

	drain_ring(unit);

	HIDD_PCIDriver_FreePCIMem(unit->rtl8139u_PCIDriver, np->rx_buffer);
	HIDD_PCIDriver_FreePCIMem(unit->rtl8139u_PCIDriver, np->tx_buffer);

	ReportEvents(LIBBASE, unit, S2EVENT_OFFLINE);

	return 0;
}


void rtl8139nic_get_functions(struct net_device *Unit)
{
	Unit->initialize = rtl8139nic_initialize;
	Unit->deinitialize = rtl8139nic_deinitialize;
	Unit->start = rtl8139nic_open;
	Unit->stop = rtl8139nic_close;
	Unit->set_mac_address = rtl8139nic_set_mac;
	Unit->set_multicast = rtl8139nic_set_multicast;
}
