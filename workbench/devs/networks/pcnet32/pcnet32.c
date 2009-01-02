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
#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

#include <stdlib.h>

#include "pcnet32.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* A bit fixed linux stuff here :) */

#undef LIBBASE
#define LIBBASE (dev->pcnu_device)

#define net_device PCN32Unit

#define TIMER_RPROK 3599597124UL

/* BYTE IO */
extern volatile UBYTE readb(APTR base);
extern volatile void writeb(UBYTE val, APTR base);

/* WORD IO */
extern volatile UWORD readw(APTR base);
extern volatile void writew(UWORD val, APTR base);

/* LONG IO */
extern volatile ULONG readl(APTR base);
extern volatile void writel(ULONG val, APTR base);

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
    return dev->pcnu_fe_priv;
}

static inline UBYTE *get_hwbase(struct net_device *dev)
{
    return (UBYTE*)dev->pcnu_BaseMem;
}

static inline void pci_push(UBYTE *base)
{
    /* force out pending posted writes */
    readl(base);
}

static void pcn32_start_rx(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);
    UBYTE *base = get_hwbase(dev);

D(bug("%s: pcn32_start_rx\n", dev->pcnu_name));
    // Already running? Stop it.
#warning "TODO: Handle starting/stopping Rx"
}

static void pcn32_stop_rx(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);

D(bug("%s: pcn32_stop_rx\n", dev->pcnu_name));
#warning "TODO: Handle starting/stopping Rx"
}

static void pcn32_start_tx(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);

D(bug("%s: pcn32_start_tx()\n", dev->pcnu_name));
#warning "TODO: Handle starting/stopping Tx"
}

static void pcn32_stop_tx(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);

D(bug("%s: pcn32_stop_tx()\n", dev->pcnu_name));
#warning "TODO: Handle starting/stopping Tx"
}

static void pcn32_txrx_reset(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);
    UBYTE *base = get_hwbase(dev);

D(bug("%s: pcn32_txrx_reset()\n", dev->pcnu_name));
}

/*
 * pcn32_set_multicast: dev->set_multicast function
 * Called with dev->xmit_lock held.
 */
static void pcn32_set_multicast(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);
    UBYTE *base = get_hwbase(dev);
    ULONG addr[2];
    ULONG mask[2];
    ULONG pff;

D(bug("%s: pcn32_set_multicast()\n", dev->pcnu_name));

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));
}

static void pcnet32_deinitialize(struct net_device *dev)
{
    dev->write_csr(dev->pcnu_BaseMem, 0, (1 << 2)); /* Stop the PCnet32 by setting the stop bit.. */
D(bug("%s: PCnet32 chipset STOPPED\n", dev->pcnu_name));

    dev->write_bcr(dev->pcnu_BaseMem, 20, 4); /* Set pcnet32 into 16bit mode */
D(bug("%s: Chipset put into 16bit mode\n", dev->pcnu_name));
}

static void pcnet32_initialize(struct net_device *dev)
{
    struct fe_priv *np = dev->pcnu_fe_priv;
    UBYTE *base = get_hwbase(dev);
    int i;

   dev->reset(base); /* Cause the PCnet Chipset to reset */
D(bug("%s: Chipset RESET\n", dev->pcnu_name));

   pcnet32_deinitialize(dev);    // Stop the chipset and set it in 16bit-mode

    np->ring_addr = HIDD_PCIDriver_AllocPCIMem(
                    dev->pcnu_PCIDriver,
                    sizeof(struct rx_ring_desc) * (RX_RING_SIZE + TX_RING_SIZE));

    np->fep_pcnet_init_block->rx_ring = AROS_LONG2LE(np->ring_addr);
    np->fep_pcnet_init_block->tx_ring = AROS_LONG2LE(&((struct rx_ring_desc *)np->ring_addr)[RX_RING_SIZE]);

D(bug("%s: Allocated IO Rings [%d x Tx @ %x : %x] [%d x Rx @ %x : %x]\n",
  dev->pcnu_name,
  TX_RING_SIZE, np->ring_addr + (RX_RING_SIZE * sizeof(struct rx_ring_desc)), np->fep_pcnet_init_block->tx_ring,
  RX_RING_SIZE, np->ring_addr, np->fep_pcnet_init_block->rx_ring));

    dev->write_bcr(dev->pcnu_BaseMem, 20, 2); /* Set pcnet32 into 32bit mode */
D(bug("%s: PCnet Chipset put into 32bit mode\n", dev->pcnu_name));
            
    np->orig_mac[0] = ( (readb(base + 0)  << 24) | (readb(base + 1)  << 16) | (readb(base + 2)  << 8) | readb(base + 3) );
    np->orig_mac[1] = ( (readb(base + 4)  << 8) | readb(base + 5) );

    dev->pcnu_dev_addr[0] = dev->pcnu_org_addr[0] = (np->orig_mac[0] >> 24) & 0xff;
    dev->pcnu_dev_addr[1] = dev->pcnu_org_addr[1] = (np->orig_mac[0] >> 16) & 0xff;
    dev->pcnu_dev_addr[2] = dev->pcnu_org_addr[2] = (np->orig_mac[0] >>  8) & 0xff;
    dev->pcnu_dev_addr[3] = dev->pcnu_org_addr[3] = (np->orig_mac[0] >>  0) & 0xff;

    dev->pcnu_dev_addr[4] = dev->pcnu_org_addr[4] = (np->orig_mac[1] >>  8) & 0xff;
    dev->pcnu_dev_addr[5] = dev->pcnu_org_addr[5] = (np->orig_mac[1] >>  0) & 0xff;

D(bug("%s: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", dev->pcnu_name,
            dev->pcnu_dev_addr[0], dev->pcnu_dev_addr[1], dev->pcnu_dev_addr[2],
            dev->pcnu_dev_addr[3], dev->pcnu_dev_addr[4], dev->pcnu_dev_addr[5]));
}

static void pcn32_drain_tx(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);
    int i;
    for (i = 0; i < TX_RING_SIZE; i++) {
#warning "TODO: pcn32_drain_tx does nothing atm."
//        np->fep_pcnet_init_block->tx_ring[i].FlagLen = 0;
    }
}

static void pcn32_drain_rx(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);
    int i;
    for (i = 0; i < RX_RING_SIZE; i++) {
#warning "TODO: pcn32_drain_rx does nothing atm."
//        np->fep_pcnet_init_block->rx_ring[i].FlagLen = 0;
    }
}


static void drain_ring(struct net_device *dev)
{
    pcn32_drain_tx(dev);
    pcn32_drain_rx(dev);
}

static int request_irq(struct net_device *dev)
{
    OOP_Object *irq = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
    BOOL ret;

D(bug("%s: request_irq()\n", dev->pcnu_name));

    if (irq)
    {
        ret = HIDD_IRQ_AddHandler(irq, dev->pcnu_irqhandler, dev->pcnu_IRQ);
        HIDD_IRQ_AddHandler(irq, dev->pcnu_touthandler, vHidd_IRQ_Timer);

D(bug("%s: request_irq: IRQ Handlers configured\n", dev->pcnu_name));

        OOP_DisposeObject(irq);

        if (ret)
        {
            return 0;
        }
    }
    return 1;
}

static void free_irq(struct net_device *dev)
{
    OOP_Object *irq = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
    if (irq)
    {
        HIDD_IRQ_RemHandler(irq, dev->pcnu_irqhandler);
        HIDD_IRQ_RemHandler(irq, dev->pcnu_touthandler);
        OOP_DisposeObject(irq);
    }
}

static void pcnet32_set_mac(struct net_device *dev)
{
   UBYTE *base = get_hwbase(dev);
   int i;

   for (i = 0; i < 6; i++) // Copy MAC Address to init block
   {
      dev->pcnu_fe_priv->fep_pcnet_init_block->phys_addr[i] = dev->pcnu_dev_addr[i];    
   }
}

static int pcnet32_open(struct net_device *dev)
{
   struct fe_priv *np = get_pcnpriv(dev);
   UBYTE *base = get_hwbase(dev);
   int ret, oom, i;

   oom = 0;

   pcnet32_deinitialize(dev);    // Stop the chipset and set it in 16bit-mode

   np->rx_buffer = HIDD_PCIDriver_AllocPCIMem(
                        dev->pcnu_PCIDriver,
                        RX_RING_SIZE * RXTX_ALLOC_BUFSIZE);

   if (np->rx_buffer == NULL)
      oom = 1;

   np->tx_buffer = HIDD_PCIDriver_AllocPCIMem(
                        dev->pcnu_PCIDriver,
                        TX_RING_SIZE * RXTX_ALLOC_BUFSIZE);

   if (np->tx_buffer == NULL)
      oom = 1;

D(bug("%s: pcnet32_open: begin\n",dev->pcnu_name));

   if (oom == 0)
   {
D(bug("%s: pcnet32_open: Allocated IO Buffers [ %d x Tx @ %x] [ %d x Rx @ %x]\n",dev->pcnu_name,
                        TX_RING_SIZE, np->tx_buffer,
                        RX_RING_SIZE, np->rx_buffer ));   
   
 
   np->fep_pcnet_init_block->mode = AROS_WORD2LE(0x0003); /* Disable Rx and Tx */
   np->fep_pcnet_init_block->tlen_rlen = AROS_WORD2LE(TX_RING_LEN_BITS | RX_RING_LEN_BITS);   

D(bug("%s: pcnet32_open: Interrupts disabled\n",dev->pcnu_name));

    pcnet32_set_mac(dev);

D(bug("%s: pcnet32_open: copied MAC address\n",dev->pcnu_name));

   np->fep_pcnet_init_block->filter[0] = 0xffffffff;
   np->fep_pcnet_init_block->filter[1] = 0xffffffff;

D(bug("%s: pcnet32_open: reset filter\n"));

    ret = request_irq(dev);
    if (ret)
        goto out_drain;

   for (i=0; i < TX_RING_SIZE; i++)
   {
      ((struct tx_ring_desc *)np->ring_addr)[i + RX_RING_SIZE].PacketBuffer = 0;
      ((struct tx_ring_desc *)np->ring_addr)[i + RX_RING_SIZE].BufferStatus = 0;
   }

D(bug("%s: pcnet32_open: Tx Ring initialised\n",dev->pcnu_name));

   for (i=0; i < RX_RING_SIZE; i++)
   {
      ((struct rx_ring_desc *)np->ring_addr)[i].PacketBuffer = AROS_LONG2LE((IPTR)&np->rx_buffer[i]);
      ((struct rx_ring_desc *)np->ring_addr)[i].BufferLength = AROS_WORD2LE(-RXTX_ALLOC_BUFSIZE);
      ((struct rx_ring_desc *)np->ring_addr)[i].BufferStatus = AROS_WORD2LE((1 << 8)|(1 << 9)|(1 << 15));
   }

D(bug("%s: pcnet32_open: Rx Ring initialised\n",dev->pcnu_name));

    dev->write_bcr(dev->pcnu_BaseMem, 20, 2); /* Set pcnet32 into 32bit mode */
D(bug("%s: PCnet Chipset put into 32bit mode\n", dev->pcnu_name));

   dev->write_csr(dev->pcnu_BaseMem, 1, (AROS_LONG2LE((IPTR)np->fep_pcnet_init_block) & 0xffff)); /* Store the pointer to the pcnet32_init_block */
   dev->write_csr(dev->pcnu_BaseMem, 2, (AROS_LONG2LE((IPTR)np->fep_pcnet_init_block) >> 16));

D(bug("%s: pcnet32_open: set init_block (@ %x)\n",dev->pcnu_name,np->fep_pcnet_init_block));

   dev->write_csr(dev->pcnu_BaseMem, 0, ((1 << 6)|(1 << 0))); /* Trigger an initialisation for the interrupt (INEA|INIT)*/

D(bug("%s: pcnet32_open: triggered init int\n",dev->pcnu_name));

   pcnet32_deinitialize(dev);    // Stop the chipset and set it in 16bit-mode 

    dev->write_bcr(dev->pcnu_BaseMem, 20, 2); /* Set pcnet32 into 32bit mode */
D(bug("%s: PCnet Chipset put into 32bit mode\n", dev->pcnu_name));

   dev->write_bcr(dev->pcnu_BaseMem, 2, ((dev->read_bcr(dev->pcnu_BaseMem, 2) & ~2) | 2)); /* Set autoselect bit */

D(bug("%s: pcnet32_open: autoselect bit set\n",dev->pcnu_name));

/** Handle Link setup **/

   if ((dev->pcnu_pcnet_supported & support_mii) \
// && (!(option & enable_autoneg))
      )
   {
      dev->write_bcr(dev->pcnu_BaseMem, 32, ((dev->read_bcr(dev->pcnu_BaseMem, 32) & ~0x38) | 0x10 | 0x08 ));
D(bug("%s: pcnet32_open: Chipset set into AutoNeg:OFF FullDuplex:ON LinkSpeed:100\n",dev->pcnu_name));
   }
   else
   {
      dev->write_bcr(dev->pcnu_BaseMem, 32, ((dev->read_bcr(dev->pcnu_BaseMem, 32) & ~0x98) | 0x20 ));
D(bug("%s: pcnet32_open: Chipset set into AutoNeg:ON\n",dev->pcnu_name));
   }

/** Set GPSI bit in test register **/

/** Handle Transmit stop on underflow **/
   if (dev->pcnu_pcnet_supported & support_dxsuflo)
   {
      dev->write_csr(dev->pcnu_BaseMem, 3, (dev->read_csr(dev->pcnu_BaseMem, 3) | 0x40));
D(bug("%s: pcnet32_open: Enabled Tx_STOP_ON_UNDERFLOW\n",dev->pcnu_name));
   }

/** Enable TxDone intr inhibitor **/
   if (dev->pcnu_pcnet_supported & support_ltint)
   {
      dev->write_csr(dev->pcnu_BaseMem, 5, (dev->read_csr(dev->pcnu_BaseMem, 5) | (1 << 14)));
D(bug("%s: pcnet32_open: Enabled Tx_DONE_INTR_INHIBITOR\n",dev->pcnu_name));
   }

   np->fep_pcnet_init_block->mode = 0x0000; /* Enable Rx and Tx */

D(bug("%s: pcnet32_open: Enable Rx & Tx\n",dev->pcnu_name));

   dev->write_csr(dev->pcnu_BaseMem, 1, (AROS_LONG2LE((IPTR)np->fep_pcnet_init_block) & 0xffff)); /* Re initialise the PCnet chipset */
   dev->write_csr(dev->pcnu_BaseMem, 2, (AROS_LONG2LE((IPTR)np->fep_pcnet_init_block) >> 16));

D(bug("%s: pcnet32_open: re-initialised chipset\n",dev->pcnu_name));

   dev->write_csr(dev->pcnu_BaseMem, 4, 0x0915); /* ? */
   dev->write_csr(dev->pcnu_BaseMem, 0, ((1 << 6)|(1 << 1)|(1 << 0)));

   i = 0;
   while (i++ < 100)
      if (dev->read_csr(dev->pcnu_BaseMem, 0) & 0x0100)
         break;

D(bug("%s: pcnet32_open: chipset ENABLED, csr[0] = %x\n",dev->pcnu_name, dev->read_csr(dev->pcnu_BaseMem, 0)));

   dev->pcnu_flags |= IFF_UP;
   ReportEvents(LIBBASE, dev, S2EVENT_ONLINE);
D(bug("%s: pcnet32_open: Device set as ONLINE\n",dev->pcnu_name));
   }
   return 0;

out_drain:
    drain_ring(dev);
    return ret;
}

static int pcn32_close(struct net_device *dev)
{
    struct fe_priv *np = get_pcnpriv(dev);
    UBYTE *base;

    dev->pcnu_flags &= ~IFF_UP;

    ObtainSemaphore(&np->lock);
    np->in_shutdown = 1;
    ReleaseSemaphore(&np->lock);

    dev->pcnu_toutNEED = FALSE;

    netif_stop_queue(dev);
    ObtainSemaphore(&np->lock);
    
    pcnet32_deinitialize(dev);    // Stop the chipset and set it in 16bit-mode

    base = get_hwbase(dev);

    ReleaseSemaphore(&np->lock);

    free_irq(dev);

    drain_ring(dev);

    HIDD_PCIDriver_FreePCIMem(dev->pcnu_PCIDriver, np->rx_buffer);
    HIDD_PCIDriver_FreePCIMem(dev->pcnu_PCIDriver, np->tx_buffer);

    ReportEvents(LIBBASE, dev, S2EVENT_OFFLINE);

    return 0;
}


void pcn32_get_functions(struct net_device *Unit)
{
    Unit->initialize = pcnet32_initialize;
    Unit->deinitialize = pcnet32_deinitialize;
    Unit->start = pcnet32_open;
    Unit->stop = pcn32_close;
    Unit->set_mac_address = pcnet32_set_mac;
    Unit->set_multicast = pcn32_set_multicast;
}
