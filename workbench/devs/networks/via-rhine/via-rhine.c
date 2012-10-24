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
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>

#include <aros/libcall.h>
#include <aros/macros.h>
#include <aros/io.h>

#include <oop/oop.h>

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

#include <stdlib.h>

#include "via-rhine.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* A bit fixed linux stuff here :) */

#undef LIBBASE
#define LIBBASE (dev->rhineu_device)

#define net_device VIARHINEUnit

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

static inline struct fe_priv *get_pcnpriv(struct net_device *dev)
{
    return dev->rhineu_fe_priv;
}

static inline UBYTE *get_hwbase(struct net_device *dev)
{
    return (UBYTE*)dev->rhineu_BaseMem;
}

static int mdio_read(struct net_device *dev, int phy_id, int location)
{
	UBYTE *base = get_hwbase(dev);

	int i = 1024;

	while ((BYTEIN(base + VIAR_MIICmd) & 0x60) && --i > 0)
	{
	}

	BYTEOUT(base + VIAR_MIICmd, 0x00);
	BYTEOUT(base + VIAR_MIIPhyAddr, phy_id);
	BYTEOUT(base + VIAR_MIIRegAddr, location);
	BYTEOUT(base + VIAR_MIICmd, 0x40);             //Trigger the read

	i = 1024;

	while ((BYTEIN(base + VIAR_MIICmd) & 0x40) && --i > 0)
	{
	}
	
	return WORDIN(base + VIAR_MIIData);
}

#if 0
static void viarhinenic_start_rx(struct net_device *dev)
{
    // struct fe_priv *np = get_pcnpriv(dev);
    // UBYTE *base = get_hwbase(dev);

D(bug("%s: viarhinenic_start_rx\n", dev->rhineu_name));
    // Already running? Stop it.
/* TODO: Handle starting/stopping Rx */
}

static void viarhinenic_stop_rx(struct net_device *dev)
{
    // UBYTE *base = get_hwbase(dev);

D(bug("%s: viarhinenic_stop_rx\n", dev->rhineu_name));
/* TODO: Handle starting/stopping Rx */
}

static void viarhinenic_start_tx(struct net_device *dev)
{
    // UBYTE *base = get_hwbase(dev);

D(bug("%s: viarhinenic_start_tx()\n", dev->rhineu_name));
/* TODO: Handle starting/stopping Tx */
}

static void viarhinenic_stop_tx(struct net_device *dev)
{
    // UBYTE *base = get_hwbase(dev);

D(bug("%s: viarhinenic_stop_tx()\n", dev->rhineu_name));
/* TODO: Handle starting/stopping Tx */
}

static void viarhinenic_txrx_reset(struct net_device *dev)
{
    // struct fe_priv *np = get_pcnpriv(dev);
    // UBYTE *base = get_hwbase(dev);

D(bug("%s: viarhinenic_txrx_reset()\n", dev->rhineu_name));
}
#endif

/*
 * viarhinenic_set_multicast: dev->set_multicast function
 * Called with dev->xmit_lock held.
 */
static void viarhinenic_set_multicast(struct net_device *dev)
{
    // struct fe_priv *np = get_pcnpriv(dev);
    // UBYTE *base = get_hwbase(dev);
    ULONG addr[2];
    ULONG mask[2];
    // ULONG pff;

D(bug("%s: viarhinenic_set_multicast()\n", dev->rhineu_name));

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));
}

static void viarhinenic_deinitialize(struct net_device *dev)
{
//    dev->write_csr(dev->rhineu_BaseMem, 0, (1 << 2)); /* Stop the PCnet32 by setting the stop bit.. */
D(bug("%s: PCnet32 chipset STOPPED\n", dev->rhineu_name));

//    dev->write_bcr(dev->rhineu_BaseMem, 20, 4); /* Set pcnet32 into 16bit mode */
D(bug("%s: Chipset put into 16bit mode\n", dev->rhineu_name));
}

static void viarhinenic_initialize(struct net_device *dev)
{
    struct fe_priv *np = dev->rhineu_fe_priv;
    UBYTE *base = get_hwbase(dev);
    int i;
    // int config1;

	WORDOUT(base + VIAR_ChipCmd, CmdReset);
	udelay(20000);
D(bug("%s: VIA Rhine Reset.\n", dev->rhineu_name));

	for (i = 0; i < 6; i++)
	{
		np->orig_mac[i] = BYTEIN(base + VIAR_StationAddr + i);
	}

	int phy, phy_idx = 0;
	if (dev->rhineu_chipcapabilities & RTLc_CanHaveMII)
	{
		np->mii_phys[0] = 1;
		for (phy = 0; (phy < 32) && (phy_idx < 4); phy++)
		{
			int mii_status = mdio_read(dev, phy, 1);
			if (mii_status != 0xffff && mii_status != 0x0000)
			{
				np->mii_phys[phy_idx++] = phy;
				np->mii_advertising = mdio_read(dev, phy, 4);
D(bug("%s: MII transceiver %d status 0x%4.4x advertising %4.4x\n", dev->rhineu_name,
																										  phy, mii_status, np->mii_advertising));

			}
		}
	}

    dev->rhineu_dev_addr[0] = dev->rhineu_org_addr[0] = np->orig_mac[0];
    dev->rhineu_dev_addr[1] = dev->rhineu_org_addr[1] = np->orig_mac[1];
    dev->rhineu_dev_addr[2] = dev->rhineu_org_addr[2] = np->orig_mac[2];
    dev->rhineu_dev_addr[3] = dev->rhineu_org_addr[3] = np->orig_mac[3];

    dev->rhineu_dev_addr[4] = dev->rhineu_org_addr[4] = np->orig_mac[4];
    dev->rhineu_dev_addr[5] = dev->rhineu_org_addr[5] = np->orig_mac[5];

D(bug("%s: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", dev->rhineu_name,
            dev->rhineu_dev_addr[0], dev->rhineu_dev_addr[1], dev->rhineu_dev_addr[2],
            dev->rhineu_dev_addr[3], dev->rhineu_dev_addr[4], dev->rhineu_dev_addr[5]));
}

static void viarhinenic_drain_tx(struct net_device *dev)
{
    // struct fe_priv *np = get_pcnpriv(dev);
    int i;
    for (i = 0; i < TX_BUFFERS; i++) {
/* TODO: viarhinenic_drain_tx does nothing atm. */
    }
}

static void viarhinenic_drain_rx(struct net_device *dev)
{
    // struct fe_priv *np = get_pcnpriv(dev);
    int i;
    for (i = 0; i < RX_BUFFERS; i++) {
/* TODO: viarhinenic_drain_rx does nothing atm. */
    }
}


static void drain_ring(struct net_device *dev)
{
    viarhinenic_drain_tx(dev);
    viarhinenic_drain_rx(dev);
}

static int request_irq(struct net_device *dev)
{
D(bug("%s: request_irq()\n", dev->rhineu_name));

    if (!dev->rhineu_IntsAdded)
    {
        AddIntServer(INTB_KERNEL + dev->rhineu_IRQ, &dev->rhineu_irqhandler);
        AddIntServer(INTB_VERTB, &dev->rhineu_touthandler);
        dev->rhineu_IntsAdded = TRUE;
    }

D(bug("%s: request_irq: IRQ Handlers configured\n", dev->rhineu_name));
    return 0;
}

static void free_irq(struct net_device *dev)
{
    if (dev->rhineu_IntsAdded)
    {
        RemIntServer(INTB_KERNEL + dev->rhineu_IRQ, &dev->rhineu_irqhandler);
        RemIntServer(INTB_VERTB, &dev->rhineu_touthandler);
        dev->rhineu_IntsAdded = FALSE;
    }
}

int viarhinenic_rx_setmode(struct net_device *dev)
{
	struct fe_priv *np = get_pcnpriv(dev);
	UBYTE *base = get_hwbase(dev);
	unsigned long mc_filter[2];         //Multicast hash filter.
	unsigned int rx_mode;

	mc_filter[1] = mc_filter[0] = 0x0;
	
	if (dev->rhineu_flags & IFF_PROMISC)
	{
D(bug("%s: viarhinenic_rx_setmode: PROMISCUOUS Mode\n", dev->rhineu_name));
		rx_mode = 0x1c;
	}
	else if (dev->rhineu_flags & IFF_ALLMULTI)
	{
D(bug("%s: viarhinenic_rx_setmode: ALL MULTICAST Enabled\n", dev->rhineu_name));
		rx_mode = 0x0c;
		mc_filter[1] = mc_filter[0] = 0xffffffff;
	}
	else
	{
D(bug("%s: viarhinenic_rx_setmode: Default Rx Mode (no Multi or Promisc)\n", dev->rhineu_name));
		rx_mode = 0x1a;
	}
	
	LONGOUT(base + VIAR_MulticastFilter0, mc_filter[0]);
	LONGOUT(base + VIAR_MulticastFilter1, mc_filter[1]);
	BYTEOUT(base + VIAR_RxConfig, np->rx_thresh | rx_mode);
	
	return 0;
}

static void viarhinenic_set_mac(struct net_device *dev)
{
   UBYTE *base = get_hwbase(dev);
   int i;

	for (i = 0; i < 6; i++)
	{
		BYTEOUT(base + VIAR_StationAddr + i, dev->rhineu_dev_addr[i]);
	}
}

static int viarhinenic_open(struct net_device *dev)
{
   struct fe_priv *np = get_pcnpriv(dev);
   UBYTE *base = get_hwbase(dev);
   int ret, oom, i;
   ULONG txb_size, rxb_size;

   txb_size = BUFFER_SIZE * TX_BUFFERS;
   rxb_size = BUFFER_SIZE * RX_BUFFERS;

   oom = 0;
   
	ret = request_irq(dev);
	if (ret)
		goto out_drain;

    np->tx_buffer = HIDD_PCIDriver_AllocPCIMem(
                        dev->rhineu_PCIDriver,
                        txb_size);

    if (np->tx_buffer == NULL)
    {
      oom = 1;
    }
    else
    {	
D(bug("%s: viarhinenic_open: Tx Ring buffers allocated @ %x\n", dev->rhineu_name, np->tx_buffer));
		for (i=0; i< TX_BUFFERS; i++)
		{
			np->tx_desc[i].tx_status = 0;
			np->tx_desc[i].desc_length = 0;
			np->tx_desc[i].addr = (IPTR)(np->tx_buffer + (i * BUFFER_SIZE));
			np->tx_desc[i].next = (IPTR)&np->tx_desc[i + 1];
D(bug("%s: viarhinenic_open: Tx Ring %d @ %x, Buffer = %x\n",dev->rhineu_name,
                        i, &np->tx_desc[i], np->tx_desc[i].addr));
		}
		np->tx_desc[i-1].next = (IPTR)&np->tx_desc[0];
	}

    np->rx_buffer = HIDD_PCIDriver_AllocPCIMem(
                        dev->rhineu_PCIDriver,
                        rxb_size);

	if (np->rx_buffer == NULL)
	{
		oom = 1;
	}
	else
	{
D(bug("%s: viarhinenic_open: Rx Ring buffers allocated @ %x\n", dev->rhineu_name, np->rx_buffer));
		for (i=0; i< RX_BUFFERS; i++)
		{
			np->rx_desc[i].rx_status = DescOwn;
			np->rx_desc[i].desc_length = MAX_FRAME_SIZE;
			np->rx_desc[i].addr = (IPTR)( np->rx_buffer + (i * BUFFER_SIZE));
			np->rx_desc[i].next = (IPTR)&np->rx_desc[i + 1];
D(bug("%s: viarhinenic_open: Rx Ring %d @ %x, Buffer = %x\n",dev->rhineu_name,
                        i, &np->rx_desc[i], np->rx_desc[i].addr));
		}
		np->rx_desc[i-1].next = (IPTR)&np->rx_desc[0];
	}

   if (oom == 0)
   {
D(bug("%s: viarhinenic_open: Allocated IO Buffers [ %d x Tx @ %x] [ %d x Rx @ %x]\n",dev->rhineu_name,
                        TX_BUFFERS, np->tx_buffer,
                        RX_BUFFERS, np->rx_buffer));
   
        viarhinenic_set_mac(dev);
D(bug("%s: viarhinenic_open: copied MAC address\n",dev->rhineu_name));

		//Set up Frame Buffer Ring Descriptors
		LONGOUT(base + VIAR_TxRingPtr, (IPTR)&np->tx_desc[0]);
		LONGOUT(base + VIAR_RxRingPtr, (IPTR)&np->rx_desc[0]);

D(bug("%s: viarhinenic_open: Frame Buffer Ring Descriptors set\n",dev->rhineu_name));
	   
		//Initialise other registers
		WORDOUT(base + VIAR_PCIBusConfig, 0x0006);

		np->tx_thresh = 0x20;
		np->rx_thresh = 0x60;
	   
		BYTEOUT(base + VIAR_TxConfig, np->tx_thresh);
		viarhinenic_rx_setmode(dev);

D(bug("%s: viarhinenic_open: Tx/Rx Configuration set\n",dev->rhineu_name));

		/* check_duplex *//*
		if (np->mii_phys[0] >= 0 || (dev->rhineu_rtl_chipcapabilities & RTLc_HAS_MII_XCVR))
		{
			unsigned short mii_reg5 = mdio_read(dev, np->mii_phys[0], 5);
			if (mii_reg5 != 0xffff)
			{
				if (((mii_reg5 & 0x0100) == 0x0100) || ((mii_reg5 & 0x00c0) == 0x0040))
				{
					np->full_duplex = 1;
				}
			}

			if ((mii_reg5 == 0 ) || !(mii_reg5 & 0x0180))
			{
			   dev->rhineu_rtl_LinkSpeed = 10000000;
			}
			else
			{
			   dev->rhineu_rtl_LinkSpeed = 100000000;				
			}
			
D(bug("%s: viarhinenic_open: Setting %s%s-duplex based on auto-neg partner ability %4.4x\n",dev->rhineu_name,
			                                                                                                                                     mii_reg5 == 0 ? "" : (mii_reg5 & 0x0180) ? "100mbps " : "10mbps ",
			                                                                                                                                     np->full_duplex ? "full" : "half", mii_reg5));
		}*/

		//Enable all known interrupts by setting the interrupt mask ..
		WORDOUT(base + VIAR_IntrEnable, (IntrRxDone | IntrRxErr | IntrRxEmpty | IntrRxOverflow| IntrRxDropped | IntrTxDone | IntrTxAbort | IntrTxUnderrun | IntrPCIErr | IntrStatsMax | IntrLinkChange | IntrMIIChange));

		np->cmd = CmdStart | CmdTxOn | CmdRxOn | CmdRxDemand;

		WORDOUT(base + VIAR_ChipCmd, np->cmd);

	   dev->rhineu_flags |= IFF_UP;
	   ReportEvents(LIBBASE, dev, S2EVENT_ONLINE);
D(bug("%s: viarhinenic_open: Device set as ONLINE\n",dev->rhineu_name));
   }
   return 0;

out_drain:
    drain_ring(dev);
    return ret;
}

static int viarhinenic_close(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);

    dev->rhineu_flags &= ~IFF_UP;

    ObtainSemaphore(&np->lock);
    np->in_shutdown = 1;
    ReleaseSemaphore(&np->lock);

    dev->rhineu_toutNEED = FALSE;

    netif_stop_queue(dev);
    ObtainSemaphore(&np->lock);
    
    viarhinenic_deinitialize(dev);    // Stop the chipset and set it in 16bit-mode

    ReleaseSemaphore(&np->lock);

    free_irq(dev);

    drain_ring(dev);

    HIDD_PCIDriver_FreePCIMem(dev->rhineu_PCIDriver, np->rx_buffer);
    HIDD_PCIDriver_FreePCIMem(dev->rhineu_PCIDriver, np->tx_buffer);

    ReportEvents(LIBBASE, dev, S2EVENT_OFFLINE);

    return 0;
}


void viarhinenic_get_functions(struct net_device *Unit)
{
    Unit->initialize = viarhinenic_initialize;
    Unit->deinitialize = viarhinenic_deinitialize;
    Unit->start = viarhinenic_open;
    Unit->stop = viarhinenic_close;
    Unit->set_mac_address = viarhinenic_set_mac;
    Unit->set_multicast = viarhinenic_set_multicast;
}
