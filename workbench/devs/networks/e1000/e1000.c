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

#include <hardware/intbits.h>

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

#include "e1000_osdep.h"
#include "e1000.h"
#include "e1000_defines.h"
#include "e1000_api.h"

#include "unit.h"
#include LC_LIBDEFS_FILE

/* A bit fixed linux stuff here :) */

#undef LIBBASE
#define LIBBASE (unit->e1ku_device)

void e1000_usec_delay(struct net_device *unit, ULONG usec)
{
    if (unit != NULL)
    {
        unit->e1ku_DelayPort.mp_SigTask = FindTask(NULL);
        unit->e1ku_DelayReq.tr_node.io_Command = TR_ADDREQUEST;
        unit->e1ku_DelayReq.tr_time.tv_micro = usec % 1000000;
        unit->e1ku_DelayReq.tr_time.tv_secs = usec / 1000000;

        DoIO((struct IORequest *)&unit->e1ku_DelayReq);
    }
}

void e1000_msec_delay(struct net_device *unit, ULONG msec)
{
    e1000_usec_delay(unit, 1000 * msec);
}

void e1000_msec_delay_irq(struct net_device *unit, ULONG msec)
{
    //e1000_usec_delay(unit, 1000 * msec);
}

static BOOL e1000func_check_64k_bound(struct net_device *unit,
                                       void *start, unsigned long len)
{
    unsigned long begin = (unsigned long) start;
    unsigned long end = begin + len;

    /* First rev 82545 and 82546 need to not allow any memory
     * write location to cross 64k boundary due to errata 23 */
    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82545 ||
        ((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82546) {
            return ((begin ^ (end - 1)) >> 16) != 0 ? FALSE : TRUE;
    }

    return TRUE;
}

void e1000func_irq_disable(struct net_device *unit)
{
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_IMC, ~0);
    E1000_WRITE_FLUSH((struct e1000_hw *)unit->e1ku_Private00);
}

void e1000func_irq_enable(struct net_device *unit)
{
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_IMS, IMS_ENABLE_MASK);
    E1000_WRITE_FLUSH((struct e1000_hw *)unit->e1ku_Private00);
}

static void e1000_clean_all_rx_rings(struct net_device *unit)
{
    D(bug("[%s]: %s(unit @ %p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit));
//    e1000func_clean_rx_ring(unit, unit->e1ku_rxRing);
}

static void e1000func_enter_82542_rst(struct net_device *unit)
{
    ULONG rctl;

    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type != e1000_82542)
        return;
    if (((struct e1000_hw *)unit->e1ku_Private00)->revision_id != E1000_REVISION_2)
        return;

    D(bug("[%s]: %s(unit @ %p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit));

    e1000_pci_clear_mwi((struct e1000_hw *)unit->e1ku_Private00);

    rctl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL);
    rctl |= E1000_RCTL_RST;
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL, rctl);
    E1000_WRITE_FLUSH((struct e1000_hw *)unit->e1ku_Private00);

    e1000_msec_delay(unit, 5);

//	if (netif_running(netdev))
		e1000_clean_all_rx_rings(unit);
}

static void e1000func_leave_82542_rst(struct net_device *unit)
{
    ULONG rctl;

    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type != e1000_82542)
        return;
    if (((struct e1000_hw *)unit->e1ku_Private00)->revision_id != E1000_REVISION_2)
        return;

    D(bug("[%s]: %s(unit @ %p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit));

    rctl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL);
    rctl &= ~E1000_RCTL_RST;
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL, rctl);
    E1000_WRITE_FLUSH((struct e1000_hw *)unit->e1ku_Private00);

    e1000_msec_delay(unit, 5);

    if (((struct e1000_hw *)unit->e1ku_Private00)->bus.pci_cmd_word & CMD_MEM_WRT_INVALIDATE)
        e1000_pci_set_mwi((struct e1000_hw *)unit->e1ku_Private00);

//	if (netif_running(netdev)) {
//		/* No need to loop, because 82542 supports only 1 queue */
//		struct e1000_rx_ring *ring = &adapter->rx_ring[0];
//		e1000_configure_rx(adapter);
//		adapter->alloc_rx_buf(adapter, ring, E1000_DESC_UNUSED(ring));
//	}
}

static void e1000func_configure_tx(struct net_device *unit)
{
    ULONG tdlen, tctl, tipg;
    ULONG ipgr1, ipgr2;
    UQUAD tdba;
    int i;

    D(bug("[%s]: %s(unit @ %p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit));

    /* Setup the HW Tx Head and Tail descriptor pointers */
    for (i = 0; i < unit->e1ku_txRing_QueueSize; i++)
    {
        D(bug("[%s] %s: Tx Queue %d @ %p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, i, &unit->e1ku_txRing[i]));
        D(bug("[%s] %s: Tx Queue count = %d)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit->e1ku_txRing[i].count));

        tdba = (IPTR)unit->e1ku_txRing[i].dma;
        tdlen = (ULONG)(unit->e1ku_txRing[i].count * sizeof(struct e1000_tx_desc));
        D(bug("[%s] %s: Tx Queue Ring Descriptor DMA @ %p [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit->e1ku_txRing[i].dma, tdlen));

        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TDBAL(i), (ULONG)(tdba & 0x00000000ffffffffULL));
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TDBAH(i), (ULONG)(tdba >> 32));
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TDLEN(i), tdlen);
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TDH(i), 0);
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TDT(i), 0);
        unit->e1ku_txRing[i].tdh = E1000_REGISTER((struct e1000_hw *)unit->e1ku_Private00, E1000_TDH(i));
        unit->e1ku_txRing[i].tdt = E1000_REGISTER((struct e1000_hw *)unit->e1ku_Private00, E1000_TDT(i));
        D(bug("[%s] %s: Tx Queue TDH=%d, TDT=%d\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit->e1ku_txRing[i].tdh, unit->e1ku_txRing[i].tdt));
    }

    /* Set the default values for the Tx Inter Packet Gap timer */
    if ((((struct e1000_hw *)unit->e1ku_Private00)->mac.type <= e1000_82547_rev_2) &&
        ((((struct e1000_hw *)unit->e1ku_Private00)->phy.media_type == e1000_media_type_fiber) ||
        (((struct e1000_hw *)unit->e1ku_Private00)->phy.media_type == e1000_media_type_internal_serdes)))
        tipg = DEFAULT_82543_TIPG_IPGT_FIBER;
    else
        tipg = DEFAULT_82543_TIPG_IPGT_COPPER;

    switch (((struct e1000_hw *)unit->e1ku_Private00)->mac.type)
    {
    case e1000_82542:
        tipg = DEFAULT_82542_TIPG_IPGT;
        ipgr1 = DEFAULT_82542_TIPG_IPGR1;
        ipgr2 = DEFAULT_82542_TIPG_IPGR2;
        break;
    default:
        ipgr1 = DEFAULT_82543_TIPG_IPGR1;
        ipgr2 = DEFAULT_82543_TIPG_IPGR2;
        break;
    }
    tipg |= ipgr1 << E1000_TIPG_IPGR1_SHIFT;
    tipg |= ipgr2 << E1000_TIPG_IPGR2_SHIFT;
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TIPG, tipg);

    /* Set the Tx Interrupt Delay register */
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TIDV, 0);
//	if (unit->flags & E1000_FLAG_HAS_INTR_MODERATION)
//		E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TADV, unit->tx_abs_int_delay);

    /* Program the Transmit Control Register */
    tctl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TCTL);
    tctl &= ~E1000_TCTL_CT;
    tctl |= E1000_TCTL_PSP | E1000_TCTL_RTLC |
            (E1000_COLLISION_THRESHOLD << E1000_CT_SHIFT);

    e1000_config_collision_dist((struct e1000_hw *)unit->e1ku_Private00);

    /* Setup Transmit Descriptor Settings for eop descriptor */
    unit->txd_cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS;

    /* only set IDE if we are delaying interrupts using the timers */
//	if (unit->tx_int_delay)
//		unit->txd_cmd |= E1000_TXD_CMD_IDE;

    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type < e1000_82543)
            unit->txd_cmd |= E1000_TXD_CMD_RPS;
    else
            unit->txd_cmd |= E1000_TXD_CMD_RS;

    /* Cache if we're 82544 running in PCI-X because we'll
     * need this to apply a workaround later in the send path. */
//	if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82544 &&
//	    ((struct e1000_hw *)unit->e1ku_Private00)->bus.type == e1000_bus_type_pcix)
//		adapter->pcix_82544 = 1;

    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_TCTL, tctl);
}

static void e1000func_setup_rctl(struct net_device *unit)
{
    ULONG rctl;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    rctl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL);

    rctl &= ~(3 << E1000_RCTL_MO_SHIFT);

    rctl |= E1000_RCTL_BAM | E1000_RCTL_LBM_NO | E1000_RCTL_RDMTS_HALF |
        (((struct e1000_hw *)unit->e1ku_Private00)->mac.mc_filter_type << E1000_RCTL_MO_SHIFT);

    /* disable the stripping of CRC because it breaks
     * BMC firmware connected over SMBUS
    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type > e1000_82543)
            rctl |= E1000_RCTL_SECRC;
    */

    if (e1000_tbi_sbp_enabled_82543((struct e1000_hw *)unit->e1ku_Private00))
        rctl |= E1000_RCTL_SBP;
    else
        rctl &= ~E1000_RCTL_SBP;

    if (unit->e1ku_mtu <= ETH_DATA_LEN)
        rctl &= ~E1000_RCTL_LPE;
    else
        rctl |= E1000_RCTL_LPE;

    /* Setup buffer sizes */
    rctl &= ~E1000_RCTL_SZ_4096;
    rctl |= E1000_RCTL_BSEX;
    switch (unit->rx_buffer_len)
    {
    case E1000_RXBUFFER_256:
        rctl |= E1000_RCTL_SZ_256;
        rctl &= ~E1000_RCTL_BSEX;
        break;
    case E1000_RXBUFFER_512:
        rctl |= E1000_RCTL_SZ_512;
        rctl &= ~E1000_RCTL_BSEX;
        break;
    case E1000_RXBUFFER_1024:
        rctl |= E1000_RCTL_SZ_1024;
        rctl &= ~E1000_RCTL_BSEX;
        break;
    case E1000_RXBUFFER_2048:
    default:
        rctl |= E1000_RCTL_SZ_2048;
        rctl &= ~E1000_RCTL_BSEX;
        break;
    case E1000_RXBUFFER_4096:
        rctl |= E1000_RCTL_SZ_4096;
        break;
    case E1000_RXBUFFER_8192:
        rctl |= E1000_RCTL_SZ_8192;
        break;
    case E1000_RXBUFFER_16384:
        rctl |= E1000_RCTL_SZ_16384;
        break;
    }

    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL, rctl);
}

static void e1000func_configure_rx(struct net_device *unit)
{
    ULONG rdlen, rctl, rxcsum;
    UQUAD rdba;
    int i;

    D(bug("[%s]: %s(0x%p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit));

    /* disable receivers while setting up the descriptors */
    rctl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL);
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL, rctl & ~E1000_RCTL_EN);
    E1000_WRITE_FLUSH((struct e1000_hw *)unit->e1ku_Private00);

    e1000_msec_delay(unit, 10);

    /* set the Receive Delay Timer Register */
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RDTR, 0);

//	if (adapter->flags & E1000_FLAG_HAS_INTR_MODERATION) {
//		E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RADV, adapter->rx_abs_int_delay);
//		if (adapter->itr_setting != 0)
//			E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_ITR,
//				1000000000 / (adapter->itr * 256));
//	}

    /* Setup the HW Rx Head and Tail Descriptor Pointers and
     * the Base and Length of the Rx Descriptor Ring */
    for (i = 0; i < unit->e1ku_rxRing_QueueSize; i++)
    {
        D(bug("[%s] %s: Rx Queue %d @ %p\n", unit->e1ku_name, __PRETTY_FUNCTION__, i, &unit->e1ku_rxRing[i]));
        D(bug("[%s] %s:        count = %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit->e1ku_rxRing[i].count));

        rdlen = (ULONG)(unit->e1ku_rxRing[i].count * sizeof(struct e1000_rx_desc));
        rdba = (IPTR)unit->e1ku_rxRing[i].dma;
        D(bug("[%s] %s:        Ring Descriptor DMA @ %p, [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit->e1ku_rxRing[i].dma, rdlen));

        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RDBAL(i), (ULONG)(rdba & 0x00000000ffffffffULL));
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RDBAH(i), (ULONG)(rdba >> 32));
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RDLEN(i), rdlen);
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RDH(i), 0);
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RDT(i), 0);
        unit->e1ku_rxRing[i].rdh = E1000_REGISTER((struct e1000_hw *)unit->e1ku_Private00, E1000_RDH(i));
        unit->e1ku_rxRing[i].rdt = E1000_REGISTER((struct e1000_hw *)unit->e1ku_Private00, E1000_RDT(i));
        D(bug("[%s] %s:        RDH Reg %d, RDT Reg %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit->e1ku_rxRing[i].rdh, unit->e1ku_rxRing[i].rdt));
        D(bug("[%s] %s:        RDH = %d, RDT = %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, readl((APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + unit->e1ku_rxRing[i].rdh)), readl((APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + unit->e1ku_rxRing[i].rdt))));
    }

    D(bug("[%s] %s: Configuring checksum Offload..\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type >= e1000_82543)
    {
        /* Enable 82543 Receive Checksum Offload for TCP and UDP */
        rxcsum = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RXCSUM);
//		if (unit->rx_csum == TRUE) {
//			rxcsum |= E1000_RXCSUM_TUOFL;
//		} else {
            rxcsum &= ~E1000_RXCSUM_TUOFL;
                /* don't need to clear IPPCSE as it defaults to 0 */
//		}
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RXCSUM, rxcsum);
    }

    /* Enable Receivers */
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL, rctl);
}

void e1000func_reset(struct net_device *unit)
{
    struct e1000_mac_info *mac = &((struct e1000_hw *)unit->e1ku_Private00)->mac;
    struct e1000_fc_info *fc = &((struct e1000_hw *)unit->e1ku_Private00)->fc;
    u32 pba = 0, tx_space, min_tx_space, min_rx_space;
    bool legacy_pba_adjust = FALSE;
    u16 hwm;

    D(bug("[%s]: %s(0x%p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit);)

    /* Repartition Pba for greater than 9k mtu
       To take effect CTRL.RST is required.     */

    switch (mac->type)
    {
    case e1000_82542:
    case e1000_82543:
    case e1000_82544:
    case e1000_82540:
    case e1000_82541:
    case e1000_82541_rev_2:
        legacy_pba_adjust = TRUE;
        pba = E1000_PBA_48K;
        break;
    case e1000_82545:
    case e1000_82545_rev_3:
    case e1000_82546:
    case e1000_82546_rev_3:
        pba = E1000_PBA_48K;
        break;
    case e1000_82547:
    case e1000_82547_rev_2:
        legacy_pba_adjust = TRUE;
        pba = E1000_PBA_30K;
        break;
    case e1000_undefined:
    case e1000_num_macs:
        break;
    }

    if (legacy_pba_adjust == TRUE) {
        if (unit->e1ku_frame_max > E1000_RXBUFFER_8192)
                pba -= 8; /* allocate more FIFO for Tx */

        if (mac->type == e1000_82547) {
            unit->e1ku_tx_fifo_head = 0;
            unit->e1ku_tx_head_addr = pba << E1000_TX_HEAD_ADDR_SHIFT;
            unit->e1ku_tx_fifo_size = (E1000_PBA_40K - pba) << E1000_PBA_BYTES_SHIFT;
//			atomic_set(&unit->tx_fifo_stall, 0);
        }
    } else if (unit->e1ku_frame_max > ETH_MAXPACKETSIZE) {
        /* adjust PBA for jumbo frames */
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_PBA, pba);

        /* To maintain wire speed transmits, the Tx FIFO should be
         * large enough to accommodate two full transmit packets,
         * rounded up to the next 1KB and expressed in KB.  Likewise,
         * the Rx FIFO should be large enough to accommodate at least
         * one full receive packet and is similarly rounded up and
         * expressed in KB. */
        pba = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_PBA);
        /* upper 16 bits has Tx packet buffer allocation size in KB */
        tx_space = pba >> 16;
        /* lower 16 bits has Rx packet buffer allocation size in KB */
        pba &= 0xffff;
        /* the tx fifo also stores 16 bytes of information about the tx
         * but don't include ethernet FCS because hardware appends it */
        min_tx_space = (unit->e1ku_frame_max + sizeof(struct e1000_tx_desc) - ETH_CRCSIZE) * 2;
        min_tx_space = ALIGN(min_tx_space, 1024);
        min_tx_space >>= 10;
        /* software strips receive CRC, so leave room for it */
        min_rx_space = unit->e1ku_frame_max;
        min_rx_space = ALIGN(min_rx_space, 1024);
        min_rx_space >>= 10;

        /* If current Tx allocation is less than the min Tx FIFO size,
         * and the min Tx FIFO size is less than the current Rx FIFO
         * allocation, take space away from current Rx allocation */
        if ((tx_space < min_tx_space) &&
            ((min_tx_space - tx_space) < pba))
        {
            pba = pba - (min_tx_space - tx_space);

            /* PCI/PCIx hardware has PBA alignment constraints */
            switch (mac->type)
            {
            case e1000_82545 ... e1000_82546_rev_3:
                pba &= ~(E1000_PBA_8K - 1);
                break;
            default:
                break;
            }

            /* if short on rx space, rx wins and must trump tx
             * adjustment or use Early Receive if available */
            if (pba < min_rx_space)
            {
                pba = min_rx_space;
            }
        }
    }

    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_PBA, pba);
    D(bug("[%s]: %s: pba = %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, pba);)

    /* flow control settings */
    /* The high water mark must be low enough to fit one full frame
     * (or the size used for early receive) above it in the Rx FIFO.
     * Set it to the lower of:
     * - 90% of the Rx FIFO size, and
     * - the full Rx FIFO size minus the early receive size (for parts
     *   with ERT support assuming ERT set to E1000_ERT_2048), or
     * - the full Rx FIFO size minus one full frame */
    hwm = min(((pba << 10) * 9 / 10), ((pba << 10) - unit->e1ku_frame_max));

    fc->high_water = hwm & 0xFFF8;	/* 8-byte granularity */
    fc->low_water = fc->high_water - 8;

    fc->pause_time = E1000_FC_PAUSE_TIME;
    fc->send_xon = 1;
    fc->current_mode = fc->requested_mode;

    /* Allow time for pending master requests to run */
    e1000_reset_hw((struct e1000_hw *)unit->e1ku_Private00);

    if (mac->type >= e1000_82544)
    {
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_WUC, 0);
    }

    if (e1000_init_hw((struct e1000_hw *)unit->e1ku_Private00))
    {
        D(bug("[%s] %s: Hardware Error\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
    }
    /* if (unit->hwflags & HWFLAGS_PHY_PWR_BIT) { */
    if ((mac->type >= e1000_82544) &&
        (mac->type <= e1000_82547_rev_2) &&
        (mac->autoneg == 1) &&
        (((struct e1000_hw *)unit->e1ku_Private00)->phy.autoneg_advertised == ADVERTISE_1000_FULL))
    {
        u32 ctrl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_CTRL);
        /* clear phy power management bit if we are in gig only mode,
         * which if enabled will attempt negotiation to 100Mb, which
         * can cause a loss of link at power off or driver unload */
        ctrl &= ~E1000_CTRL_SWDPIN3;
        E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_CTRL, ctrl);
    }

    /* Enable h/w to recognize an 802.1Q VLAN Ethernet packet */
    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_VET, ETHERNET_IEEE_VLAN_TYPE);

    e1000_reset_adaptive((struct e1000_hw *)unit->e1ku_Private00);
    e1000_get_phy_info((struct e1000_hw *)unit->e1ku_Private00);
}

int e1000func_set_mac(struct net_device *unit)
{
    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__);)

    /* 82542 2.0 needs to be in reset to write receive address registers */
    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82542)
    {
        e1000func_enter_82542_rst(unit);
    }

    memcpy(((struct e1000_hw *)unit->e1ku_Private00)->mac.addr, unit->e1ku_dev_addr, ETH_ADDRESSSIZE);

    e1000_rar_set((struct e1000_hw *)unit->e1ku_Private00, ((struct e1000_hw *)unit->e1ku_Private00)->mac.addr, 0);

    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82542)
    {
        e1000func_leave_82542_rst(unit);
    }

    return 0;
}

void e1000func_set_multi(struct net_device *unit)
{
    struct AddressRange *range;
    UBYTE  *mta_list;
    ULONG rctl, mc_count;
    int i = 0;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__);)

    /* Check for Promiscuous and All Multicast modes */

    rctl = E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL);

    if (unit->e1ku_ifflags & IFF_PROMISC) {
        rctl |= (E1000_RCTL_UPE | E1000_RCTL_MPE);
    } else if (unit->e1ku_ifflags & IFF_ALLMULTI) {
        rctl |= E1000_RCTL_MPE;
        rctl &= ~E1000_RCTL_UPE;
    } else {
        rctl &= ~(E1000_RCTL_UPE | E1000_RCTL_MPE);
    }

    E1000_WRITE_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_RCTL, rctl);

    /* 82542 2.0 needs to be in reset to write receive address registers */

    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82542)
        e1000func_enter_82542_rst(unit);

    ListLength(&unit->e1ku_multicast_ranges, mc_count);
    
    if (mc_count > 0)
    {
        mta_list = AllocMem(mc_count * ETH_ADDRESSSIZE, MEMF_PUBLIC | MEMF_CLEAR );
        if (!mta_list)
            return;

        /* The shared function expects a packed array of only addresses. */
        ForeachNode(&unit->e1ku_multicast_ranges, range) {
            memcpy(mta_list + (i*ETH_ADDRESSSIZE), &range->lower_bound, ETH_ADDRESSSIZE);
            i++;
        }

        e1000_update_mc_addr_list((struct e1000_hw *)unit->e1ku_Private00, mta_list, i);

        FreeMem(mta_list, mc_count * ETH_ADDRESSSIZE);
    }
    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type == e1000_82542)
        e1000func_leave_82542_rst(unit);
}

// static void e1000func_deinitialize(struct net_device *unit)
// {
// }

int request_irq(struct net_device *unit)
{
    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__);)

    AddIntServer(INTB_KERNEL | unit->e1ku_IRQ, &unit->e1ku_irqhandler);
    AddIntServer(INTB_VERTB, &unit->e1ku_touthandler);

    D(bug("[%s] %s: IRQ Handlers configured\n", unit->e1ku_name, __PRETTY_FUNCTION__);)

    return 0;
}

#if 0
static void free_irq(struct net_device *unit)
{
    RemIntServer(INTB_KERNEL | unit->e1ku_IRQ, unit->e1ku_irqhandler);
    RemIntServer(INTB_VERTB, unit->e1ku_touthandler);
}
#endif

static int e1000func_setup_tx_resources(struct net_device *unit,
                                    struct e1000_tx_ring *tx_ring)
{
    ULONG size;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__);)

    size = sizeof(struct e1000_buffer) * tx_ring->count;

    D(bug("[%s] %s: Configuring for %d buffers\n", unit->e1ku_name, __PRETTY_FUNCTION__, tx_ring->count));

    if ((tx_ring->buffer_info = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
    {
        D(bug("[%s] %s: Unable to allocate memory for the transmit descriptor ring\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
        return -E1000_ERR_CONFIG;
    }

    D(bug("[%s] %s: Tx Buffer Info @ %p [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, tx_ring->buffer_info, size);)

    /* round up to nearest 4K */
    tx_ring->size = tx_ring->count * sizeof(struct e1000_tx_desc);
    tx_ring->size = ALIGN(tx_ring->size, 4096);

    if ((tx_ring->desc = AllocMem(tx_ring->size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
    {
setup_tx_desc_die:
        FreeMem(tx_ring->buffer_info, size);
        D(bug("[%s] %s: Unable to allocate memory for the transmit descriptor ring\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
        return -E1000_ERR_CONFIG;
    }
    tx_ring->dma = HIDD_PCIDriver_CPUtoPCI(unit->e1ku_PCIDriver, (APTR)tx_ring->desc);

    /* Fix for errata 23, can't cross 64kB boundary */
    if (!e1000func_check_64k_bound(unit, tx_ring->desc, tx_ring->size))
    {
        void *olddesc = tx_ring->desc;
        D(bug("[%s] %s: tx_ring align check failed: %u bytes at %p\n", unit->e1ku_name, __PRETTY_FUNCTION__, tx_ring->size, tx_ring->desc);)
        /* Try again, without freeing the previous */
        if ((tx_ring->desc = AllocMem(tx_ring->size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        {
            /* Failed allocation, critical failure */
            FreeMem(olddesc, tx_ring->size);
            tx_ring->dma = NULL;
            goto setup_tx_desc_die;
        }
        tx_ring->dma = HIDD_PCIDriver_CPUtoPCI(unit->e1ku_PCIDriver, (APTR)tx_ring->desc);

        if (!e1000func_check_64k_bound(unit, tx_ring->desc, tx_ring->size))
        {
            /* give up */
            FreeMem(tx_ring->desc, tx_ring->size);
            FreeMem(olddesc, tx_ring->size);
            tx_ring->dma = NULL;
            D(bug("[%s] %s: Unable to allocate aligned memory for the transmit descriptor ring\n", unit->e1ku_name, __PRETTY_FUNCTION__);)

            FreeMem(tx_ring->buffer_info, size);
            return -E1000_ERR_CONFIG;
        } else {
            /* Free old allocation, new allocation was successful */
            FreeMem(olddesc, tx_ring->size);
        }
    }

    D(bug("[%s] %s: Tx Ring Descriptors @ %p [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, tx_ring->desc, tx_ring->size);)

    tx_ring->next_to_use = 0;
    tx_ring->next_to_clean = 0;

    return 0;
}

int e1000func_setup_all_tx_resources(struct net_device *unit)
{
    int i, err = 0;

    for (i = 0; i < unit->e1ku_txRing_QueueSize; i++)
    {
        err = e1000func_setup_tx_resources(unit, &unit->e1ku_txRing[i]);
        if (err)
        {
            D(bug("[%s] %s: Allocation for Tx Queue %u failed\n", unit->e1ku_name, __PRETTY_FUNCTION__, i);)
            for (i-- ; i >= 0; i--)
            {
                e1000func_free_tx_resources(unit, &unit->e1ku_txRing[i]);
            }
            break;
        }
    }

    return err;
}

static int e1000func_setup_rx_resources(struct net_device *unit,
                                    struct e1000_rx_ring *rx_ring)
{
    int buffer_size;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));
    
    buffer_size = sizeof(struct e1000_rx_buffer) * rx_ring->count;

    D(bug("[%s] %s: Configuring for %d buffers\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->count);)

    if ((rx_ring->buffer_info = AllocMem(buffer_size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL) {
        D(bug("[%s] %s: Unable to allocate memory for the receive ring buffers\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
        return -E1000_ERR_CONFIG;
    }

    D(bug("[%s] %s: Rx Buffer Info @ %p [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->buffer_info, buffer_size);)

    /* Round up to nearest 4K */
    rx_ring->size = rx_ring->count * sizeof(struct e1000_rx_desc);
    D(bug("[%s] %s: Wanted Size = %d bytes\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->size);)
    rx_ring->size = ALIGN(rx_ring->size, 4096);

    if ((rx_ring->desc = AllocMem(rx_ring->size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
    {
        D(bug("[%s] %s: Unable to allocate memory for the receive ring descriptors\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
setup_rx_desc_die:
        FreeMem(rx_ring->buffer_info, buffer_size);
        return -E1000_ERR_CONFIG;
    }
    rx_ring->dma = HIDD_PCIDriver_CPUtoPCI(unit->e1ku_PCIDriver, (APTR)rx_ring->desc);

    /* Fix for errata 23, can't cross 64kB boundary */
    if (!e1000func_check_64k_bound(unit, rx_ring->desc, rx_ring->size))
    {
        void *olddesc = rx_ring->desc;
        D(bug("[%s] %s: rx_ring align check failed: %u bytes at %p\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->size, rx_ring->desc);)

        /* Try again, without freeing the previous */
        if ((rx_ring->desc = AllocMem(rx_ring->size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        {
            /* Failed allocation, critical failure */
            FreeMem(olddesc, rx_ring->size);
            rx_ring->dma = NULL;
            D(bug("[%s] %s: Unable to allocate memory for the receive descriptor ring\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
            goto setup_rx_desc_die;
        }
        rx_ring->dma = HIDD_PCIDriver_CPUtoPCI(unit->e1ku_PCIDriver, (APTR)rx_ring->desc);

        if (!e1000func_check_64k_bound(unit, rx_ring->desc, rx_ring->size)) {
            /* give up */
            FreeMem(rx_ring->desc, rx_ring->size);
            FreeMem(olddesc, rx_ring->size);
            rx_ring->dma = NULL;
            D(bug("[%s] %s: Unable to allocate aligned memory for the receive descriptor ring\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
            goto setup_rx_desc_die;
        } else {
            /* Free old allocation, new allocation was successful */
            FreeMem(olddesc, rx_ring->size);
        }
    }

    D(bug("[%s] %s: Rx Ring Descriptors @ %p [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->desc, rx_ring->size);)

    /* set up ring defaults */
    rx_ring->next_to_clean = 0;
    rx_ring->next_to_use = 0;

    return 0;
}

int e1000func_setup_all_rx_resources(struct net_device *unit)
{
    int i, err = 0;

    D(bug("[%s] %s(0x%p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit);)

    for (i = 0; i < unit->e1ku_rxRing_QueueSize; i++)
    {
        err = e1000func_setup_rx_resources(unit, &unit->e1ku_rxRing[i]);
        if (err)
        {
            D(bug("[%s] %s: Allocation for Rx Queue %u failed\n", unit->e1ku_name, __PRETTY_FUNCTION__, i);)
            for (i-- ; i >= 0; i--)
            {
                e1000func_free_rx_resources(unit, &unit->e1ku_rxRing[i]);
            }
            break;
        }
    }

    return err;
}

void e1000func_unmap_and_free_tx_resource(struct net_device *unit,
                                             struct e1000_buffer *buffer_info)
{
    D(bug("[%s] %s(0x%p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit);)
    if (buffer_info->dma) {
        buffer_info->dma = NULL;
    }
    if (buffer_info->buffer) {
        FreeMem(buffer_info->buffer, ETH_MAXPACKETSIZE);
        buffer_info->buffer = NULL;
    }
    /* buffer_info must be completely set up in the transmit path */
}

void e1000func_clean_tx_ring(struct net_device *unit,
                                struct e1000_tx_ring *tx_ring)
{
    struct e1000_buffer *buffer_info;
    unsigned long size;
    unsigned int i;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    /* Free all the Tx ring buffers */
    for (i = 0; i < tx_ring->count; i++) {
            buffer_info = &tx_ring->buffer_info[i];
            e1000func_unmap_and_free_tx_resource(unit, buffer_info);
    }

    size = sizeof(struct e1000_buffer) * tx_ring->count;
    memset(tx_ring->buffer_info, 0, size);

    /* Zero out the descriptor ring */

    memset(tx_ring->desc, 0, tx_ring->size);

    tx_ring->next_to_use = 0;
    tx_ring->next_to_clean = 0;
//	tx_ring->last_tx_tso = 0;

    writel(0, (APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + tx_ring->tdh));
    writel(0, (APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + tx_ring->tdt));
}

void e1000func_free_tx_resources(struct net_device *unit,
                                struct e1000_tx_ring *tx_ring)
{
    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    e1000func_clean_tx_ring(unit, tx_ring);

    FreeMem(tx_ring->buffer_info, sizeof(struct e1000_buffer) * tx_ring->count);
    tx_ring->buffer_info = NULL;

    FreeMem(tx_ring->desc, tx_ring->size);
    tx_ring->dma =  tx_ring->desc = NULL;
}

void e1000func_clean_rx_ring(struct net_device *unit,
                                struct e1000_rx_ring *rx_ring)
{
    struct e1000_rx_buffer *buffer_info;
    unsigned long size;
    unsigned int i;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    /* Free all the Rx ring buffers */
    for (i = 0; i < rx_ring->count; i++) {
        buffer_info = (struct e1000_rx_buffer *)&rx_ring->buffer_info[i];
        if (buffer_info->dma != NULL) {
            buffer_info->dma = NULL;
        }
        if (buffer_info->buffer)
        {
            FreeMem(buffer_info->buffer, unit->rx_buffer_len);
            buffer_info->buffer = NULL;
        }
    }

    size = sizeof(struct e1000_rx_buffer) * rx_ring->count;
    memset(rx_ring->buffer_info, 0, size);

    /* Zero out the descriptor ring */
    memset(rx_ring->desc, 0, rx_ring->size);

    rx_ring->next_to_clean = 0;
    rx_ring->next_to_use = 0;

    writel(0, (APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdh));
    writel(0, (APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt));
}

void e1000func_free_rx_resources(struct net_device *unit,
                                    struct e1000_rx_ring *rx_ring)
{
    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    e1000func_clean_rx_ring(unit, rx_ring);

    FreeMem(rx_ring->buffer_info, sizeof(struct e1000_rx_buffer) * rx_ring->count);
    rx_ring->buffer_info = NULL;

    FreeMem(rx_ring->desc, rx_ring->size);
    rx_ring->dma = rx_ring->desc = NULL;
}

#if 0
static int e1000func_close(struct net_device *unit)
{
    D(bug("[%s] %s(0x%p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit);)

    unit->e1ku_ifflags &= ~IFF_UP;

//    ObtainSemaphore(&np->lock);
//    np->in_shutdown = 1;
//    ReleaseSemaphore(&np->lock);

    unit->e1ku_toutNEED = FALSE;

//    netif_stop_queue(unit);
//    ObtainSemaphore(&np->lock);
    
//    e1000func_deinitialize(unit);    // Stop the chipset and set it in 16bit-mode

//    ReleaseSemaphore(&np->lock);

    free_irq(unit);

//    drain_ring(unit);

//    HIDD_PCIDriver_FreePCIMem(unit->e1ku_PCIDriver, np->rx_buffer);
//    HIDD_PCIDriver_FreePCIMem(unit->e1ku_PCIDriver, np->tx_buffer);

    ReportEvents(LIBBASE, unit, S2EVENT_OFFLINE);

    return 0;
}
#endif

void e1000func_alloc_rx_buffers(struct net_device *unit,
                                   struct e1000_rx_ring *rx_ring,
                                   int cleaned_count)
{
    struct e1000_rx_desc *rx_desc;
    struct e1000_rx_buffer *buffer_info;
    unsigned int i;

    i = rx_ring->next_to_use;

    D(
        bug("[%s]: %s(0x%p, 0x%p, %d)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit, rx_ring, cleaned_count);
        bug("[%s]: %s: starting at %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, i);
     )

    while (cleaned_count--)
    {
        buffer_info = (struct e1000_rx_buffer *)&rx_ring->buffer_info[i];

        if ((buffer_info->buffer = AllocMem(unit->rx_buffer_len, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
        {
            D(
                bug("[%s] %s: Buffer %d Allocated @ %p [%d bytes]\n", unit->e1ku_name, __PRETTY_FUNCTION__, i, buffer_info->buffer, unit->rx_buffer_len);
                if ((buffer_info->dma = HIDD_PCIDriver_CPUtoPCI(unit->e1ku_PCIDriver, (APTR)buffer_info->buffer)) == NULL)
                {
                    bug("[%s] %s: Failed to Map Buffer %d for DMA!!\n", unit->e1ku_name, __PRETTY_FUNCTION__, i);
                }
                bug("[%s] %s: Buffer %d DMA @ %p\n", unit->e1ku_name, __PRETTY_FUNCTION__, i, buffer_info->dma);
            )

            rx_desc = E1000_RX_DESC(rx_ring, i);
    //    		rx_desc->buffer_addr = cpu_to_le64(buffer_info->dma);
            rx_desc->buffer_addr = (IPTR)buffer_info->dma;
        }

        if (++i == rx_ring->count)
            i = 0;
    }
    D(
        bug("[%s]: %s: next_to_use = %d, i = %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->next_to_use, i);
     )
    if (rx_ring->next_to_use != i) {
        rx_ring->next_to_use = i;
        if (i-- == 0)
            i = (rx_ring->count - 1);

        D(
            bug("[%s]: %s: Adjusting RDT to %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, i);
        )
        writel(i, (APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt));
    }
}

void e1000func_configure(struct net_device *unit)
{
    int i;

    D(bug("[%s]: %s(0x%p)\n", unit->e1ku_name, __PRETTY_FUNCTION__, unit));

    e1000func_set_multi(unit);

    e1000func_configure_tx(unit);
    e1000func_setup_rctl(unit);
    e1000func_configure_rx(unit);
    D(bug("[%s] %s: Tx/Rx Configured\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    /* call E1000_DESC_UNUSED which always leaves
     * at least 1 descriptor unused to make sure
     * next_to_use != next_to_clean */
    for (i = 0; i < unit->e1ku_rxRing_QueueSize; i++)
    {
        D(bug("[%s] %s: Allocating Rx Buffers for queue %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, i));
        struct e1000_rx_ring *ring = &unit->e1ku_rxRing[i];
        e1000func_alloc_rx_buffers(unit, ring, E1000_DESC_UNUSED(ring));
    }
    D(bug("[%s] %s: Finished\n", unit->e1ku_name, __PRETTY_FUNCTION__));
}

BOOL e1000func_clean_tx_irq(struct net_device *unit,
                                    struct e1000_tx_ring *tx_ring)
{
    struct e1000_tx_desc *tx_desc, *eop_desc;
    struct e1000_buffer *buffer_info;
    unsigned int i, eop;
    BOOL cleaned = FALSE;
    BOOL retval = FALSE;
    unsigned int total_tx_packets=0;

    D(bug("[%s]: %s()\n", unit->e1ku_name, __PRETTY_FUNCTION__));

    i = tx_ring->next_to_clean;
    eop = tx_ring->buffer_info[i].next_to_watch;
    eop_desc = E1000_TX_DESC(tx_ring, eop);

    D(bug("[%s] %s: starting at  %d, eop=%d, desc @ %p\n", unit->e1ku_name, __PRETTY_FUNCTION__, i, eop, eop_desc));

    while (eop_desc->upper.data & AROS_LONG2LE(E1000_TXD_STAT_DD)) {
        for (cleaned = FALSE; !cleaned; ) {
            D(bug("[%s] %s: cleaning Tx buffer %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, i));
            tx_desc = E1000_TX_DESC(tx_ring, i);
            buffer_info = &tx_ring->buffer_info[i];
            cleaned = (i == eop);

            if (cleaned) {
                retval = TRUE;
                total_tx_packets++;
            }
            e1000func_unmap_and_free_tx_resource(unit, buffer_info);
            tx_desc->upper.data = 0;

            if (++i == tx_ring->count)
                i = 0;
        }

        eop = tx_ring->buffer_info[i].next_to_watch;
        eop_desc = E1000_TX_DESC(tx_ring, eop);
    }

    tx_ring->next_to_clean = i;

#define TX_WAKE_THRESHOLD 32
//	if (cleaned && netif_carrier_ok(netdev) &&
//		     E1000_DESC_UNUSED(tx_ring) >= TX_WAKE_THRESHOLD) {
                /* Make sure that anybody stopping the queue after this
                 * sees the new next_to_clean.
                 */
//		smp_mb();

//		if (netif_queue_stopped(netdev) &&
//		    !(test_bit(__E1000_DOWN, &adapter->state))) {
//			netif_wake_queue(netdev);
//			++adapter->restart_queue;
//		}
//	}

    if (unit->detect_tx_hung) {
        /* Detect a transmit hang in hardware, this serializes the
         * check with the clearing of time_stamp and movement of i */
        unit->detect_tx_hung = FALSE;
        if (tx_ring->buffer_info[eop].dma  && !(E1000_READ_REG((struct e1000_hw *)unit->e1ku_Private00, E1000_STATUS) &  E1000_STATUS_TXOFF)) {
            /* detected Tx unit hang */
            D(
                bug("[%s] %s: Detected Tx Unit Hang -:\n", unit->e1ku_name);
                bug("[%s] %s:     Tx Queue             <%lu>\n", unit->e1ku_name, __PRETTY_FUNCTION__, (unsigned long)((tx_ring - unit->e1ku_txRing) / sizeof(struct e1000_tx_ring)));
                bug("[%s] %s:     TDH                  <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, MMIO_R32(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + tx_ring->tdh));
                bug("[%s] %s:     TDT                  <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, MMIO_R32(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + tx_ring->tdt));
                bug("[%s] %s:     next_to_use          <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, tx_ring->next_to_use);
                bug("[%s] %s:     next_to_clean        <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, tx_ring->next_to_clean);
                bug("[%s] %s:   buffer_info[next_to_clean]\n", unit->e1ku_name, __PRETTY_FUNCTION__);
                bug("[%s] %s:     next_to_watch        <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, eop);
                bug("[%s] %s:     next_to_watch.status <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, eop_desc->upper.fields.status);
            )
//			netif_stop_queue(netdev);
        }
    }
    unit->e1ku_stats.PacketsSent += total_tx_packets;
//	adapter->total_tx_packets += total_tx_packets;
    return retval;
}

static void e1000func_rx_checksum(struct net_device *unit, u32 status_err,
                              u32 csum, struct eth_frame *frame)
{
    BOOL doChecksum = TRUE;
#if (0)
    u16 status = (u16)status_err;
    u8 errors = (u8)(status_err >> 24);
    skb->ip_summed = CHECKSUM_NONE;

    /* 82543 or newer only */
    if (unlikely(adapter->hw.mac.type < e1000_82543)) return;
    /* Ignore Checksum bit is set */
    if (unlikely(status & E1000_RXD_STAT_IXSM)) return;
    /* TCP/UDP checksum error bit is set */
    if (unlikely(errors & E1000_RXD_ERR_TCPE)) {
            /* let the stack verify checksum errors */
            adapter->hw_csum_err++;
            return;
    }
    /* TCP/UDP Checksum has not been calculated */
    if (adapter->hw.mac.type <= e1000_82547_rev_2) {
            if (!(status & E1000_RXD_STAT_TCPCS))
                    return;
    } else {
            if (!(status & (E1000_RXD_STAT_TCPCS | E1000_RXD_STAT_UDPCS)))
                    return;
    }
    /* It must be a TCP or UDP packet with a valid checksum */
    if (likely(status & E1000_RXD_STAT_TCPCS)) {
            /* TCP checksum is good */
            skb->ip_summed = CHECKSUM_UNNECESSARY;
    }
    adapter->hw_csum_good++;
#else
    bug("[%s] %s: Frame (Pre)Checksum %x%x%x%x\n", unit->e1ku_name, __PRETTY_FUNCTION__, frame->eth_packet_crc[0], frame->eth_packet_crc[1], frame->eth_packet_crc[2], frame->eth_packet_crc[3]);
    if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type >= e1000_82543)
    {
        if (status_err & E1000_RXD_STAT_IXSM)
            return;

        /* Make sure TCP/UDP checksum error bit is not set */
        if (!((status_err >> 24) & E1000_RXD_ERR_TCPE)) {
            BOOL valid = TRUE;
            /* Check if TCP/UDP Checksum has been calculated */
            if (((struct e1000_hw *)unit->e1ku_Private00)->mac.type <= e1000_82547_rev_2) {
                if (!(status_err & E1000_RXD_STAT_TCPCS))
                    valid = FALSE;
            } else {
                if (!(status_err & (E1000_RXD_STAT_TCPCS | E1000_RXD_STAT_UDPCS)))
                    valid = FALSE;
            }
            /* It must be a TCP or UDP packet with a valid checksum */
            if (valid && (status_err & E1000_RXD_STAT_TCPCS)) {
                /* TCP checksum is good */
                bug("[%s] %s: Using offloaded Checksum\n", unit->e1ku_name, __PRETTY_FUNCTION__);

                doChecksum = FALSE;
                frame->eth_packet_crc[0] = (csum & 0xff000000) >> 24;
                frame->eth_packet_crc[1] = (csum & 0xff0000) >> 16;
                frame->eth_packet_crc[2] = (csum & 0xff00) >> 8;
                frame->eth_packet_crc[3] = csum & 0xff;
            }
#if (HAVE_CSUM_STATS)
            unit->e1ku_stats.hw_csum_good++;
#endif
        }
        else
        {
            /* let the stack verify checksum errors */
            bug("[%s] %s: Checksum Error\n", unit->e1ku_name, __PRETTY_FUNCTION__);
#if (HAVE_CSUM_STATS)
            unit->e1ku_stats.hw_csum_err++;
#endif
        }
    }
#endif
    if (doChecksum)
    {
        // We need to calculate the frames checksum ...
        bug("[%s] %s: Frames checksum needs calculated...\n", unit->e1ku_name, __PRETTY_FUNCTION__);
    }

    bug("[%s] %s: Frame (Post)Checksum %x%x%x%x\n", unit->e1ku_name, __PRETTY_FUNCTION__, frame->eth_packet_crc[0], frame->eth_packet_crc[1], frame->eth_packet_crc[2], frame->eth_packet_crc[3]);
}

UBYTE get_status(struct net_device *unit,
                                    UBYTE *_status, struct e1000_rx_desc *rx_desc)
{
    *_status = rx_desc->status;
    bug("[%s] %s: Status: %08x\n", unit->e1ku_name, __PRETTY_FUNCTION__, *_status);
    return *_status;
}

BOOL e1000func_clean_rx_irq(struct net_device *unit,
                                    struct e1000_rx_ring *rx_ring)
{
    struct e1000_rx_desc *rx_desc, *next_rxd;
    D(struct e1000_rx_buffer *buffer_info, *next_buffer;)
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    struct eth_frame *frame;

    unsigned int i, total_rx_bytes=0, total_rx_packets=0;
    int cleaned_count = 0;
    UBYTE status = 0;
    ULONG length;
    BOOL accepted, is_orphan, cleaned = FALSE, update = FALSE;

    i = rx_ring->next_to_clean;
    rx_desc = E1000_RX_DESC(rx_ring, i);
    D(buffer_info = (struct e1000_rx_buffer *)&rx_ring->buffer_info[i];)

    D(bug("[%s] %s: Starting at %d, Rx Desc @ %p, Buffer Info @ %p\n", unit->e1ku_name, __PRETTY_FUNCTION__, i, rx_desc, buffer_info);)

    while (get_status(unit, &status, rx_desc) & E1000_RXD_STAT_DD) {
        D(
            int buffer_no = i;
         )
#if (BROKEN_RX_QUEUE)
        // Queue stalls using this ....
        if (++i == rx_ring->count) i = 0;
#else
        // ... so for our sanity we loop at the rings tail
        if (++i >= readl((APTR)(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt)))
        {
            i = 0;
            update = TRUE;
        }
#endif
        next_rxd = E1000_RX_DESC(rx_ring, i);
#if (HAVE_PREFETCH)
        prefetch(next_rxd);
#endif

        D(next_buffer = (struct e1000_rx_buffer *)&rx_ring->buffer_info[i];);

        cleaned = TRUE;
        cleaned_count++;

        length = AROS_LE2WORD(rx_desc->length);

        /* !EOP means multiple descriptors were used to store a single
         * packet, also make sure the frame isn't just CRC only */
        if (!(status & E1000_RXD_STAT_EOP) || (length <= ETH_CRCSIZE)) {
            /* All receives must fit into a single buffer */
            D(bug("[%s] %s: Receive packet consumed multiple buffers - recyclcing\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
            /* recycle */
            goto next_desc;
        }

        frame = (struct eth_frame *)(IPTR)rx_desc->buffer_addr;

        if (rx_desc->errors & E1000_RXD_ERR_FRAME_ERR_MASK){
            UBYTE last_byte = *(frame->eth_packet_data + length - 1);
            D(bug("[%s] %s: Frame Error %d (last byte %x)\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_desc->errors, last_byte);)
            if (TBI_ACCEPT((struct e1000_hw *)unit->e1ku_Private00, status,
                          rx_desc->errors, length, last_byte,
                          unit->e1ku_frame_min,
                          unit->e1ku_frame_max))
            {
                D(bug("[%s] %s: TBI accepted\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
                e1000_tbi_adjust_stats_82543((struct e1000_hw *)unit->e1ku_Private00,
                                          unit->e1ku_hw_stats,
                                          length, frame->eth_packet_data,
                                          unit->e1ku_frame_max);

                length--;
            } else {
                /* recycle */
                D(bug("[%s] %s: TBI rejected - recyclcing\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
                goto next_desc;
            }
        }

        /* got a valid packet - forward it to the network core */
        is_orphan = TRUE;

        /* adjust length to remove Ethernet CRC, this must be
         * done after the TBI_ACCEPT workaround above */
        length -= ETH_CRCSIZE;

        /* probably a little skewed due to removing CRC */
        total_rx_bytes += length;
        total_rx_packets++;

        /* Receive Checksum Offload */
        e1000func_rx_checksum(unit,
                          (ULONG)(status) |
                          ((ULONG)(rx_desc->errors) << 24),
                          AROS_LE2WORD(rx_desc->csum), frame);

        /* Dump contents of frame if DEBUG enabled */
        D(
            int j;
            bug("[%s]: Rx Buffer %d Packet Dump -:", unit->e1ku_name, buffer_no);
            for (j=0; j<64; j++) {
                if ((j%16) == 0)
                {
                    bug("\n[%s]:     %03x:", unit->e1ku_name, j);
                }
                bug(" %02x", ((unsigned char*)frame)[j]);
            }
            bug("\n");
        )

        /* Check for address validity */
        if(AddressFilter(LIBBASE, unit, frame->eth_packet_dest))
        {
            D(bug("[%s] %s: Packet IP accepted with type = %d, checksum = %08x\n", unit->e1ku_name, __PRETTY_FUNCTION__, AROS_BE2WORD(frame->eth_packet_type), AROS_LE2LONG(*((ULONG *)frame->eth_packet_crc)));)
            /* Packet is addressed to this driver */

            opener = (APTR)unit->e1ku_Openers.mlh_Head;
            opener_tail = (APTR)&unit->e1ku_Openers.mlh_Tail;

            /* Offer packet to every opener */
            while(opener != opener_tail)
            {
                request = (APTR)opener->read_port.mp_MsgList.lh_Head;
                request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
                accepted = FALSE;

                /* Offer packet to each request until it's accepted */
                while((request != request_tail) && !accepted)
                {
                    if (request->ios2_PacketType == AROS_BE2WORD(frame->eth_packet_type))
                    {
                        D(bug("[%s] %s: copy packet for opener ..\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
                        CopyPacket(LIBBASE, unit, request, length, AROS_BE2WORD(frame->eth_packet_type), frame);
                        accepted = TRUE;
                    }
                    request = (struct IOSana2Req *)request->ios2_Req.io_Message.mn_Node.ln_Succ;
                }

                if(accepted)
                  is_orphan = FALSE;

                opener = (APTR)opener->node.mln_Succ;
            }

            /* If packet was unwanted, give it to S2_READORPHAN request */
            if(is_orphan)
            {
                unit->e1ku_stats.UnknownTypesReceived++;

                if(!IsMsgPortEmpty(unit->e1ku_request_ports[ADOPT_QUEUE]))
                {
                    CopyPacket(LIBBASE, unit,
                        (APTR)unit->e1ku_request_ports[ADOPT_QUEUE]->
                        mp_MsgList.lh_Head, length, AROS_BE2WORD(frame->eth_packet_type), frame);
                    D(bug("[%s] %s: packet copied to orphan queue\n", unit->e1ku_name, __PRETTY_FUNCTION__);)
                }
            }
        }

next_desc:
        rx_desc->status = 0;

        /* use prefetched values */
        rx_desc = next_rxd;
        D(buffer_info = next_buffer);
    }
    rx_ring->next_to_clean = i;

#if (BROKEN_RX_QUEUE)
    // Enabling this stalls the queue ...
    if ((cleaned_count = E1000_DESC_UNUSED(rx_ring)))
    {
        D(bug("[%s] %s: Updating rdt\n", unit->e1ku_name, __PRETTY_FUNCTION__));
        writel(i, ((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt);
    }
#else
    // ...but it seems we have to tell the hardware to wrap around?
    if (update == TRUE)
    {
        D(bug("[%s] %s: Adjusting RDH/RDT\n", unit->e1ku_name, __PRETTY_FUNCTION__));
        writel(readl(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt), ((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt);
        writel(i, ((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdh);
    }
#endif

    D(
        bug("[%s] %s: Next to clean = %d\n", unit->e1ku_name, __PRETTY_FUNCTION__, rx_ring->next_to_clean);
        bug("[%s] %s:     RDH                  <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, readl(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdh));
        bug("[%s] %s:     RDT                  <%x>\n", unit->e1ku_name, __PRETTY_FUNCTION__, readl(((struct e1000_hw *)unit->e1ku_Private00)->hw_addr + rx_ring->rdt));
     )

    unit->e1ku_stats.PacketsReceived += total_rx_packets;
    //adapter->total_rx_packets += total_rx_packets;
    //adapter->total_rx_bytes += total_rx_bytes;
    D(bug("[%s] %s: Received %d packets (%d bytes)\n", unit->e1ku_name, __PRETTY_FUNCTION__, total_rx_packets, total_rx_bytes);)

    return cleaned;
}

/** OS SUPPORT CALLS FOR INTEL CODE **/

void e1000_pci_clear_mwi(struct e1000_hw *hw)
{
    struct pHidd_PCIDevice_WriteConfigWord pciwritemsg;

    D(bug("[%s]: %s()\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__);)

    /* Check if the devices cache line size is set first ?*/
    pciwritemsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
    pciwritemsg.reg = 0x04;
    pciwritemsg.val = (UWORD)OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pciwritemsg) & ~0x0010;
    pciwritemsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
    OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pciwritemsg);
}

void e1000_pci_set_mwi(struct e1000_hw *hw)
{
    struct pHidd_PCIDevice_WriteConfigWord pciwritemsg;

    D(bug("[%s]: %s()\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__);)

    /* Check if the devices cache line size is set first ?*/
    pciwritemsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
    pciwritemsg.reg = 0x04;
    pciwritemsg.val = (UWORD)OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pciwritemsg) | 0x0010;
    pciwritemsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
    OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pciwritemsg);
}

LONG  e1000_read_pcie_cap_reg(struct e1000_hw *hw, ULONG reg, UWORD *value)
{
    struct pHidd_PCIDevice_ReadConfigWord pcireadmsg;

    D(bug("[%s]: %s(reg:%d)\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, reg);)

    if (((struct e1000Unit *)hw->back)->e1ku_PCIeCap)
    {
        pcireadmsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
        pcireadmsg.reg = ((struct e1000Unit *)hw->back)->e1ku_PCIeCap + reg;
        *value = (UWORD)OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pcireadmsg);
        D(bug("[%s] %s: ------> [%04x]\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, *value);)
        return (E1000_SUCCESS);
    }

    return 0;
}

void e1000_read_pci_cfg(struct e1000_hw *hw, ULONG reg, UWORD *value)
{
    struct pHidd_PCIDevice_ReadConfigWord pcireadmsg;
    D(bug("[%s]: %s(reg:%d)\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, reg);)

    pcireadmsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
    pcireadmsg.reg = reg;
    *value = (UWORD)OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pcireadmsg);
    D(bug("[%s] %s: ------> [%04x]\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, *value);)
}

void e1000_write_pci_cfg(struct e1000_hw *hw, ULONG reg, UWORD *value)
{
    struct pHidd_PCIDevice_WriteConfigWord pciwritemsg;
    D(bug("[%s]: %s(reg:%d, %04x)\n", ((struct e1000Unit *)hw->back)->e1ku_name, __PRETTY_FUNCTION__, reg, *value);)

    pciwritemsg.mID = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
    pciwritemsg.reg = reg;
    pciwritemsg.val = *value;
    OOP_DoMethod(((struct e1000Unit *)hw->back)->e1ku_PCIDevice, (OOP_Msg)&pciwritemsg);
}
