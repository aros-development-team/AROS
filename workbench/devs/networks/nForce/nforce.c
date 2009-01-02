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
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/ports.h>

#include <aros/libcall.h>
#include <aros/debug.h>
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

#include "nforce.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* A bit fixed linux stuff here :) */

#undef LIBBASE
#define LIBBASE (dev->nu_device)

#define net_device NFUnit

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

static volatile ULONG readl(APTR base)
{
    return *((ULONG*)base);
}
static volatile void writel(ULONG val, APTR base)
{
    *((ULONG*)base) = val;
}

static inline struct fe_priv *get_nvpriv(struct net_device *dev)
{
    return dev->nu_fe_priv;
}

static inline UBYTE *get_hwbase(struct net_device *dev)
{
    return (UBYTE*)dev->nu_BaseMem;
}

static inline void pci_push(UBYTE *base)
{
    /* force out pending posted writes */
    readl(base);
}

static inline ULONG nv_descr_getlength(struct ring_desc *prd, ULONG v)
{
        return AROS_LE2LONG(prd->FlagLen)
                & ((v == DESC_VER_1) ? LEN_MASK_V1 : LEN_MASK_V2);
}

static int reg_delay(struct net_device *dev, int offset, ULONG mask, ULONG target,
                                int delay, int delaymax, const char *msg)
{
    UBYTE *base = get_hwbase(dev);

    pci_push(base);
    do {
        udelay(delay);
        delaymax -= delay;
        if (delaymax < 0) {
            if (msg)
                D(bug(msg));
            return 1;
        }
    } while ((readl(base + offset) & mask) != target);
    return 0;
}

#define MII_READ        (-1)
/* mii_rw: read/write a register on the PHY.
 *
 * Caller must guarantee serialization
 */
static int mii_rw(struct net_device *dev, int addr, int miireg, int value)
{
    UBYTE *base = get_hwbase(dev);
    ULONG reg;
    int retval;

    writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);

    reg = readl(base + NvRegMIIControl);
    if (reg & NVREG_MIICTL_INUSE) {
        writel(NVREG_MIICTL_INUSE, base + NvRegMIIControl);
        udelay(NV_MIIBUSY_DELAY);
    }

    reg = (addr << NVREG_MIICTL_ADDRSHIFT) | miireg;
    if (value != MII_READ) {
        writel(value, base + NvRegMIIData);
        reg |= NVREG_MIICTL_WRITE;
    }
    writel(reg, base + NvRegMIIControl);

    if (reg_delay(dev, NvRegMIIControl, NVREG_MIICTL_INUSE, 0,
		            NV_MIIPHY_DELAY, NV_MIIPHY_DELAYMAX, NULL)) {
        D(bug("%s: mii_rw of reg %d at PHY %d timed out.\n",
                        dev->name, miireg, addr));
        retval = -1;
    } else if (value != MII_READ) {
        /* it was a write operation - fewer failures are detectable */
        D(bug("%s: mii_rw wrote 0x%x to reg %d at PHY %d\n",
                        dev->name, value, miireg, addr));
        retval = 0;
    } else if (readl(base + NvRegMIIStatus) & NVREG_MIISTAT_ERROR) {
        D(bug("%s: mii_rw of reg %d at PHY %d failed.\n",
                        dev->name, miireg, addr));
        retval = -1;
    } else {
        retval = readl(base + NvRegMIIData);
        D(bug("%s: mii_rw read from reg %d at PHY %d: 0x%x.\n",
                        dev->name, miireg, addr, retval));
    }

    return retval;
}

static int phy_reset(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    ULONG miicontrol;
    unsigned int tries = 0;

    miicontrol = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
    miicontrol |= BMCR_RESET;
    if (mii_rw(dev, np->phyaddr, MII_BMCR, miicontrol)) {
        return -1;
    }

    /* wait for 500ms */
    Delay(25);

    /* must wait till reset is deasserted */
    while (miicontrol & BMCR_RESET) {
        Delay(1);
        miicontrol = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        /* FIXME: 100 tries seem excessive */
        if (tries++ > 100)
                return -1;
    }
    return 0;
}

static int phy_init(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base = get_hwbase(dev);
    ULONG phyinterface, phy_reserved, mii_status, mii_control, mii_control_1000,reg;

    /* set advertise register */
    reg = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
    reg |= (ADVERTISE_10HALF|ADVERTISE_10FULL|ADVERTISE_100HALF|ADVERTISE_100FULL|0x800|0x400);
    if (mii_rw(dev, np->phyaddr, MII_ADVERTISE, reg)) {
        D(bug("%s: phy write to advertise failed.\n", dev->name));
        return PHY_ERROR;
    }

    /* get phy interface type */
    phyinterface = readl(base + NvRegPhyInterface);

    /* see if gigabit phy */
    mii_status = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
    if (mii_status & PHY_GIGABIT) {
        np->gigabit = PHY_GIGABIT;
        mii_control_1000 = mii_rw(dev, np->phyaddr, MII_1000BT_CR, MII_READ);
        mii_control_1000 &= ~ADVERTISE_1000HALF;
        if (phyinterface & PHY_RGMII)
            mii_control_1000 |= ADVERTISE_1000FULL;
        else
            mii_control_1000 &= ~ADVERTISE_1000FULL;

        if (mii_rw(dev, np->phyaddr, MII_1000BT_CR, mii_control_1000)) {
            D(bug("%s: phy init failed.\n", dev->name));
            return PHY_ERROR;
        }
    }
    else
        np->gigabit = 0;

    /* reset the phy */
    if (phy_reset(dev)) {
        D(bug("%s: phy reset failed\n", dev->name));
        return PHY_ERROR;
    }

    /* phy vendor specific configuration */
    if ((np->phy_oui == PHY_OUI_CICADA) && (phyinterface & PHY_RGMII) ) {
        phy_reserved = mii_rw(dev, np->phyaddr, MII_RESV1, MII_READ);
        phy_reserved &= ~(PHY_INIT1 | PHY_INIT2);
        phy_reserved |= (PHY_INIT3 | PHY_INIT4);
        if (mii_rw(dev, np->phyaddr, MII_RESV1, phy_reserved)) {
            D(bug("%s: phy init failed.\n", dev->name));
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, MII_NCONFIG, MII_READ);
        phy_reserved |= PHY_INIT5;
        if (mii_rw(dev, np->phyaddr, MII_NCONFIG, phy_reserved)) {
            D(bug("%s: phy init failed.\n", dev->name));
            return PHY_ERROR;
        }
    }
    if (np->phy_oui == PHY_OUI_CICADA) {
        phy_reserved = mii_rw(dev, np->phyaddr, MII_SREVISION, MII_READ);
        phy_reserved |= PHY_INIT6;
        if (mii_rw(dev, np->phyaddr, MII_SREVISION, phy_reserved)) {
            D(bug("%s: phy init failed.\n", dev->name));
            return PHY_ERROR;
        }
    }

    /* restart auto negotiation */
    mii_control = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
    mii_control |= (BMCR_ANRESTART | BMCR_ANENABLE);
    if (mii_rw(dev, np->phyaddr, MII_BMCR, mii_control)) {
        return PHY_ERROR;
    }

    return 0;
}

static void nv_start_rx(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base = get_hwbase(dev);

    D(bug("%s: nv_start_rx\n", dev->name));
    /* Already running? Stop it. */
    if (readl(base + NvRegReceiverControl) & NVREG_RCVCTL_START) {
        writel(0, base + NvRegReceiverControl);
        pci_push(base);
    }
    writel(np->linkspeed, base + NvRegLinkSpeed);
    pci_push(base);
    writel(NVREG_RCVCTL_START, base + NvRegReceiverControl);
    D(bug("%s: nv_start_rx to duplex %d, speed 0x%08x.\n",
                        dev->name, np->duplex, np->linkspeed));
    pci_push(base);
}

static void nv_stop_rx(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);

    D(bug("%s: nv_stop_rx\n", dev->name));
    writel(0, base + NvRegReceiverControl);
    reg_delay(dev, NvRegReceiverStatus, NVREG_RCVSTAT_BUSY, 0,
                NV_RXSTOP_DELAY1, NV_RXSTOP_DELAY1MAX,
                "nv_stop_rx: ReceiverStatus remained busy");

    udelay(NV_RXSTOP_DELAY2);
    writel(0, base + NvRegLinkSpeed);
}

static void nv_start_tx(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);

    D(bug("%s: nv_start_tx\n", dev->name));
    writel(NVREG_XMITCTL_START, base + NvRegTransmitterControl);
    pci_push(base);
}

static void nv_stop_tx(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);

    D(bug("%s: nv_stop_tx\n", dev->name));
    writel(0, base + NvRegTransmitterControl);
    reg_delay(dev, NvRegTransmitterStatus, NVREG_XMITSTAT_BUSY, 0,
                NV_TXSTOP_DELAY1, NV_TXSTOP_DELAY1MAX,
                "nv_stop_tx: TransmitterStatus remained busy");

    udelay(NV_TXSTOP_DELAY2);
    writel(0, base + NvRegUnknownTransmitterReg);
}

static void nv_txrx_reset(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base = get_hwbase(dev);

    D(bug("%s: nv_txrx_reset\n", dev->name));
    writel(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET | np->desc_ver, base + NvRegTxRxControl);
    pci_push(base);
    udelay(NV_TXRX_RESET_DELAY);
    writel(NVREG_TXRXCTL_BIT2 | np->desc_ver, base + NvRegTxRxControl);
    pci_push(base);
}

/*
 * nv_set_multicast: dev->set_multicast function
 * Called with dev->xmit_lock held.
 */
static void nv_set_multicast(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base = get_hwbase(dev);
    ULONG addr[2];
    ULONG mask[2];
    ULONG pff;

    memset(addr, 0, sizeof(addr));
    memset(mask, 0, sizeof(mask));

    if (dev->flags & IFF_PROMISC) {
        D(bug("%s: Promiscuous mode enabled.\n", dev->name));
        pff = NVREG_PFF_PROMISC;
    } else {
        pff = NVREG_PFF_MYADDR;

        if (dev->flags & IFF_ALLMULTI || dev->mc_list) {
            ULONG alwaysOff[2];
            ULONG alwaysOn[2];

            alwaysOn[0] = alwaysOn[1] = alwaysOff[0] = alwaysOff[1] = 0xffffffff;
            if (dev->flags & IFF_ALLMULTI) {
                alwaysOn[0] = alwaysOn[1] = alwaysOff[0] = alwaysOff[1] = 0;
            } else {
                struct dev_mc_list *walk;

                walk = dev->mc_list;
                while (walk != NULL) {
                    ULONG a, b;
                    a = AROS_LE2LONG(*(ULONG *) walk->dmi_addr);
                    b = AROS_LE2WORD(*(UWORD *) (&walk->dmi_addr[4]));
                    alwaysOn[0] &= a;
                    alwaysOff[0] &= ~a;
                    alwaysOn[1] &= b;
                    alwaysOff[1] &= ~b;
                    walk = walk->next;
                }
            }
            addr[0] = alwaysOn[0];
            addr[1] = alwaysOn[1];
            mask[0] = alwaysOn[0] | alwaysOff[0];
            mask[1] = alwaysOn[1] | alwaysOff[1];
        }
    }
    addr[0] |= NVREG_MCASTADDRA_FORCE;
    pff |= NVREG_PFF_ALWAYS;
    ObtainSemaphore(&np->lock);
    nv_stop_rx(dev);
    writel(addr[0], base + NvRegMulticastAddrA);
    writel(addr[1], base + NvRegMulticastAddrB);
    writel(mask[0], base + NvRegMulticastMaskA);
    writel(mask[1], base + NvRegMulticastMaskB);
    writel(pff, base + NvRegPacketFilterFlags);
    D(bug("%s: reconfiguration for multicast lists.\n",
        dev->name));
    nv_start_rx(dev);
    ReleaseSemaphore(&np->lock);
}

static int nv_update_linkspeed(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base = get_hwbase(dev);
    int adv, lpa;
    int newls = np->linkspeed;
    int newdup = np->duplex;
    int mii_status;
    int retval = 0;
    ULONG control_1000, status_1000, phyreg;

    /* BMSR_LSTATUS is latched, read it twice:
     * we want the current value.
     */
    mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
    mii_status = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);

    if (!(mii_status & BMSR_LSTATUS)) {
        D(bug("%s: no link detected by phy - falling back to 10HD.\n",
                        dev->name));
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
        retval = 0;
        goto set_speed;
    }

    if (np->autoneg == 0) {
        D(bug("%s: nv_update_linkspeed: autoneg off, PHY set to 0x%04x.\n",
                    dev->name, np->fixed_mode));
        if (np->fixed_mode & LPA_100FULL) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
            newdup = 1;
        } else if (np->fixed_mode & LPA_100HALF) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
            newdup = 0;
        } else if (np->fixed_mode & LPA_10FULL) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 1;
        } else {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
        }
        retval = 1;
        goto set_speed;
    }
    /* check auto negotiation is complete */
    if (!(mii_status & BMSR_ANEGCOMPLETE)) {
        /* still in autonegotiation - configure nic for 10 MBit HD and wait. */
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
        retval = 0;
        D(bug("%s: autoneg not completed - falling back to 10HD.\n", dev->name));
        goto set_speed;
    }

    retval = 1;
    if (np->gigabit == PHY_GIGABIT) {
        control_1000 = mii_rw(dev, np->phyaddr, MII_1000BT_CR, MII_READ);
        status_1000 = mii_rw(dev, np->phyaddr, MII_1000BT_SR, MII_READ);

        if ((control_1000 & ADVERTISE_1000FULL) &&
            (status_1000 & LPA_1000FULL)) {
            D(bug("%s: nv_update_linkspeed: GBit ethernet detected.\n",
                dev->name));
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_1000;
            newdup = 1;
            goto set_speed;
        }
    }

    adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
    lpa = mii_rw(dev, np->phyaddr, MII_LPA, MII_READ);
    D(bug("%s: nv_update_linkspeed: PHY advertises 0x%04x, lpa 0x%04x.\n",
                        dev->name, adv, lpa));
    /* FIXME: handle parallel detection properly */
    lpa = lpa & adv;
    if (lpa & LPA_100FULL) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
        newdup = 1;
    } else if (lpa & LPA_100HALF) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
        newdup = 0;
    } else if (lpa & LPA_10FULL) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 1;
    } else if (lpa & LPA_10HALF) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
    } else {
        D(bug("%s: bad ability %04x - falling back to 10HD.\n", dev->name, lpa));
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
    }

set_speed:
    if (np->duplex == newdup && np->linkspeed == newls)
        return retval;

    D(bug("%s: changing link setting from %d/%d to %d/%d.\n",
        dev->name, np->linkspeed, np->duplex, newls, newdup));

    np->duplex = newdup;
    np->linkspeed = newls;
    if (np->gigabit == PHY_GIGABIT) {
        phyreg = readl(base + NvRegRandomSeed);
        phyreg &= ~(0x3FF00);
        if ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_10)
            phyreg |= NVREG_RNDSEED_FORCE3;
        else if ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_100)
            phyreg |= NVREG_RNDSEED_FORCE2;
        else if ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_1000)
            phyreg |= NVREG_RNDSEED_FORCE;
        writel(phyreg, base + NvRegRandomSeed);
    }

    phyreg = readl(base + NvRegPhyInterface);
    phyreg &= ~(PHY_HALF|PHY_100|PHY_1000);
    if (np->duplex == 0)
        phyreg |= PHY_HALF;
    if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_100)
        phyreg |= PHY_100;
    else if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000)
        phyreg |= PHY_1000;
    writel(phyreg, base + NvRegPhyInterface);

    writel(NVREG_MISC1_FORCE | ( np->duplex ? 0 : NVREG_MISC1_HD),
        base + NvRegMisc1);
    pci_push(base);
    writel(np->linkspeed, base + NvRegLinkSpeed);
    pci_push(base);

    return retval;
}

static void nv_linkchange(struct net_device *dev)
{
    if (nv_update_linkspeed(dev)) {
        if (netif_carrier_ok(dev)) {
            nv_stop_rx(dev);
        } else {
            netif_carrier_on(dev);
            D(bug("%s: link up.\n", dev->name));
        }
        nv_start_rx(dev);
    } else {
        if (netif_carrier_ok(dev)) {
            netif_carrier_off(dev);
            D(bug("%s: link down.\n", dev->name));
            nv_stop_rx(dev);
        }
    }
}

static void nv_deinitialize(struct net_device *dev)
{
}

static void nv_initialize(struct net_device *dev)
{
    struct fe_priv *np = dev->nu_fe_priv;
    UBYTE *base = get_hwbase(dev);
    int i;

    np->rx_ring = HIDD_PCIDriver_AllocPCIMem(
                    dev->nu_PCIDriver,
                    sizeof(struct ring_desc) * (RX_RING + TX_RING));

    np->ring_addr = (IPTR)np->rx_ring;
    np->tx_ring = &np->rx_ring[RX_RING];

    np->orig_mac[0] = readl(base + NvRegMacAddrA);
    np->orig_mac[1] = readl(base + NvRegMacAddrB);

    dev->dev_addr[0] = dev->org_addr[0] = (np->orig_mac[1] >>  8) & 0xff;
    dev->dev_addr[1] = dev->org_addr[1] = (np->orig_mac[1] >>  0) & 0xff;
    dev->dev_addr[2] = dev->org_addr[2] = (np->orig_mac[0] >> 24) & 0xff;
    dev->dev_addr[3] = dev->org_addr[3] = (np->orig_mac[0] >> 16) & 0xff;
    dev->dev_addr[4] = dev->org_addr[4] = (np->orig_mac[0] >>  8) & 0xff;
    dev->dev_addr[5] = dev->org_addr[5] = (np->orig_mac[0] >>  0) & 0xff;

    D(bug("%s: MAC Address %02x:%02x:%02x:%02x:%02x:%02x\n", dev->name,
            dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
            dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]));

    /* disable WOL */
    writel(0, base + NvRegWakeUpFlags);
    np->wolenabled = 0;

    if (np->desc_ver == DESC_VER_1) {
        np->tx_flags = NV_TX_LASTPACKET|NV_TX_VALID;
        if (dev->nu_DriverFlags & DEV_NEED_LASTPACKET1)
            np->tx_flags |= NV_TX_LASTPACKET1;
    } else {
        np->tx_flags = NV_TX2_LASTPACKET|NV_TX2_VALID;
        if (dev->nu_DriverFlags & DEV_NEED_LASTPACKET1)
            np->tx_flags |= NV_TX2_LASTPACKET1;
    }
    if (dev->nu_DriverFlags & DEV_IRQMASK_1)
        np->irqmask = NVREG_IRQMASK_WANTED_1;
    if (dev->nu_DriverFlags & DEV_IRQMASK_2)
        np->irqmask = NVREG_IRQMASK_WANTED_2;
    if (dev->nu_DriverFlags & DEV_NEED_TIMERIRQ)
        np->irqmask |= NVREG_IRQ_TIMER;
    if (dev->nu_DriverFlags & DEV_NEED_LINKTIMER) {
        D(bug("%s: link timer on.\n", pci_name(dev)));
        np->need_linktimer = 1;
        np->link_timeout.tv_micro = LINK_TIMEOUT % 1000000;
        np->link_timeout.tv_secs = LINK_TIMEOUT / 1000000;
    } else {
        D(bug("%s: link timer off.\n", pci_name(dev)));
        np->need_linktimer = 0;
    }

    /* find a suitable phy */
    for (i = 1; i < 32; i++) {
        int id1, id2;

        ObtainSemaphore(&np->lock);
        id1 = mii_rw(dev, i, MII_PHYSID1, MII_READ);
        ReleaseSemaphore(&np->lock);
        if (id1 < 0 || id1 == 0xffff)
            continue;
        ObtainSemaphore(&np->lock);
        id2 = mii_rw(dev, i, MII_PHYSID2, MII_READ);
        ReleaseSemaphore(&np->lock);
        if (id2 < 0 || id2 == 0xffff)
            continue;

        id1 = (id1 & PHYID1_OUI_MASK) << PHYID1_OUI_SHFT;
        id2 = (id2 & PHYID2_OUI_MASK) >> PHYID2_OUI_SHFT;
        D(bug("%s: open: Found PHY %04x:%04x at address %d.\n",
                    pci_name(dev), id1, id2, i));
        np->phyaddr = i;
        np->phy_oui = id1 | id2;
        break;
    }

    if (i == 32) {
        /* PHY in isolate mode? No phy attached and user wants to
         * test loopback? Very odd, but can be correct.
         */
        D(bug("%s: open: Could not find a valid PHY.\n",
                    pci_name(dev)));
    }

    if (i != 32) {
        /* reset it */
        phy_init(dev);
    }

    /* set default link speed settings */
    np->linkspeed = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
    np->duplex = 0;
    np->autoneg = 1;
}

static void nv_drain_tx(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    int i;
    for (i = 0; i < TX_RING; i++) {
        np->tx_ring[i].FlagLen = 0;
    }
}

static void nv_drain_rx(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    int i;
    for (i = 0; i < RX_RING; i++) {
        np->rx_ring[i].FlagLen = 0;
    }
}


static void drain_ring(struct net_device *dev)
{
    nv_drain_tx(dev);
    nv_drain_rx(dev);
}

static int request_irq(struct net_device *dev)
{
    OOP_Object *irq = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
    BOOL ret;

    if (irq)
    {
        ret = HIDD_IRQ_AddHandler(irq, dev->nu_irqhandler, dev->nu_IRQ);
        HIDD_IRQ_AddHandler(irq, dev->nu_touthandler, vHidd_IRQ_Timer);

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
        HIDD_IRQ_RemHandler(irq, dev->nu_irqhandler);
        HIDD_IRQ_RemHandler(irq, dev->nu_touthandler);
        OOP_DisposeObject(irq);
    }
}

static void nv_set_mac(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);
    ULONG mac[2];

    mac[0] = (dev->dev_addr[0] << 0)  + (dev->dev_addr[1] << 8) +
             (dev->dev_addr[2] << 16) + (dev->dev_addr[3] << 24);
    mac[1] = (dev->dev_addr[4] << 0)  + (dev->dev_addr[5] << 8);

    writel(mac[0], base + NvRegMacAddrA);
    writel(mac[1], base + NvRegMacAddrB);
}

/*
 * nv_alloc_rx: fill rx ring entries.
 * Return 1 if the allocations for the skbs failed and the
 * rx engine is without Available descriptors
 */
static int nv_alloc_rx(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    unsigned int refill_rx = np->refill_rx;
    int nr;

    while (np->cur_rx != refill_rx) {
        nr = refill_rx % RX_RING;

        np->rx_ring[nr].FlagLen = AROS_LONG2LE(RX_NIC_BUFSIZE | NV_RX_AVAIL);
        D(bug("%s: nv_alloc_rx: Packet %d marked as Available\n", dev->name, refill_rx));
        refill_rx++;
    }
    np->refill_rx = refill_rx;
    if (np->cur_rx - refill_rx == RX_RING)
        return 1;
    return 0;
}


static int nv_init_ring(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    int i;

    np->next_tx = np->nic_tx = 0;
    for (i = 0; i < TX_RING; i++)
        np->tx_ring[i].FlagLen = 0;

    np->cur_rx = RX_RING;
    np->refill_rx = 0;
    for (i = 0; i < RX_RING; i++)
        np->rx_ring[i].FlagLen = 0;
    return nv_alloc_rx(dev);
}

static int nv_open(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base = get_hwbase(dev);
    int ret, oom, i;

    oom = 0;

    np->rx_buffer = HIDD_PCIDriver_AllocPCIMem(
                        dev->nu_PCIDriver,
                        RX_RING * sizeof(struct eth_frame));

    if (np->rx_buffer == NULL)
        oom = 1;

    np->tx_buffer = HIDD_PCIDriver_AllocPCIMem(
                        dev->nu_PCIDriver,
                        TX_RING * sizeof(struct eth_frame));

    if (np->tx_buffer == NULL)
        oom = 1;

    for (i=0; i < TX_RING; i++)
        np->tx_ring[i].PacketBuffer = (IPTR)&np->tx_buffer[i];

    for (i=0; i < RX_RING; i++)
        np->rx_ring[i].PacketBuffer = (IPTR)&np->rx_buffer[i];

    D(bug("nv_open: begin\n"));

    /* 1) erase previous misconfiguration */
    /* 4.1-1: stop adapter: ignored, 4.3 seems to be overkill */
    writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
    writel(0, base + NvRegMulticastAddrB);
    writel(0, base + NvRegMulticastMaskA);
    writel(0, base + NvRegMulticastMaskB);
    writel(0, base + NvRegPacketFilterFlags);

    writel(0, base + NvRegTransmitterControl);
    writel(0, base + NvRegReceiverControl);

    writel(0, base + NvRegAdapterControl);

    /* 2) initialize descriptor rings */
    oom = nv_init_ring(dev);

    writel(0, base + NvRegLinkSpeed);
    writel(0, base + NvRegUnknownTransmitterReg);
    nv_txrx_reset(dev);
    writel(0, base + NvRegUnknownSetupReg6);

    np->in_shutdown = 0;

    /* 3) set mac address */
    nv_set_mac(dev);

    /* 4) give hw rings */
    writel((ULONG) np->ring_addr, base + NvRegRxRingPhysAddr);
    writel((ULONG) (np->ring_addr + RX_RING*sizeof(struct ring_desc)), base + NvRegTxRingPhysAddr);
    writel( ((RX_RING-1) << NVREG_RINGSZ_RXSHIFT) + ((TX_RING-1) << NVREG_RINGSZ_TXSHIFT),
        base + NvRegRingSizes);

    /* 5) continue setup */
    writel(np->linkspeed, base + NvRegLinkSpeed);
    writel(NVREG_UNKSETUP3_VAL1, base + NvRegUnknownSetupReg3);
    writel(np->desc_ver, base + NvRegTxRxControl);
    pci_push(base);
    writel(NVREG_TXRXCTL_BIT1|np->desc_ver, base + NvRegTxRxControl);
    reg_delay(dev, NvRegUnknownSetupReg5, NVREG_UNKSETUP5_BIT31, NVREG_UNKSETUP5_BIT31,
                NV_SETUP5_DELAY, NV_SETUP5_DELAYMAX,
                "open: SetupReg5, Bit 31 remained off\n");

    writel(0, base + NvRegUnknownSetupReg4);
    writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
    writel(NVREG_MIISTAT_MASK2, base + NvRegMIIStatus);

    /* 6) continue setup */
    writel(NVREG_MISC1_FORCE | NVREG_MISC1_HD, base + NvRegMisc1);
    writel(readl(base + NvRegTransmitterStatus), base + NvRegTransmitterStatus);
    writel(NVREG_PFF_ALWAYS, base + NvRegPacketFilterFlags);
    writel(NVREG_OFFLOAD_NORMAL, base + NvRegOffloadConfig);

    writel(readl(base + NvRegReceiverStatus), base + NvRegReceiverStatus);
    i = random();
    writel(NVREG_RNDSEED_FORCE | (i&NVREG_RNDSEED_MASK), base + NvRegRandomSeed);
    writel(NVREG_UNKSETUP1_VAL, base + NvRegUnknownSetupReg1);
    writel(NVREG_UNKSETUP2_VAL, base + NvRegUnknownSetupReg2);
    writel(NVREG_POLL_DEFAULT, base + NvRegPollingInterval);
    writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);
    writel((np->phyaddr << NVREG_ADAPTCTL_PHYSHIFT)|NVREG_ADAPTCTL_PHYVALID|NVREG_ADAPTCTL_RUNNING,
            base + NvRegAdapterControl);
    writel(NVREG_MIISPEED_BIT8|NVREG_MIIDELAY, base + NvRegMIISpeed);
    writel(NVREG_UNKSETUP4_VAL, base + NvRegUnknownSetupReg4);
    writel(NVREG_WAKEUPFLAGS_VAL, base + NvRegWakeUpFlags);

    i = readl(base + NvRegPowerState);
    if ( (i & NVREG_POWERSTATE_POWEREDUP) == 0)
        writel(NVREG_POWERSTATE_POWEREDUP|i, base + NvRegPowerState);

    pci_push(base);
    udelay(10);
    writel(readl(base + NvRegPowerState) | NVREG_POWERSTATE_VALID, base + NvRegPowerState);

    writel(0, base + NvRegIrqMask);
    pci_push(base);
    writel(NVREG_MIISTAT_MASK2, base + NvRegMIIStatus);
    writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
    pci_push(base);

    ret = request_irq(dev);
    if (ret)
        goto out_drain;

    /* ask for interrupts */
    writel(np->irqmask, base + NvRegIrqMask);

    ObtainSemaphore(&np->lock);
    writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
    writel(0, base + NvRegMulticastAddrB);
    writel(0, base + NvRegMulticastMaskA);
    writel(0, base + NvRegMulticastMaskB);
    writel(NVREG_PFF_ALWAYS|NVREG_PFF_MYADDR, base + NvRegPacketFilterFlags);
    /* One manual link speed update: Interrupts are enabled, future link
     * speed changes cause interrupts and are handled by nv_link_irq().
     */
    {
        ULONG miistat;
        miistat = readl(base + NvRegMIIStatus);
        writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);
        D(bug("startup: got 0x%08x.\n", miistat));
    }
    ret = nv_update_linkspeed(dev);
    nv_start_rx(dev);
    nv_start_tx(dev);
    netif_start_queue(dev);
    if (ret) {
        netif_carrier_on(dev);
    } else {
        bug("%s: no link during initialization.\n", dev->name);
        netif_carrier_off(dev);
    }
    if (oom) {
        bug("%s: Out Of Memory. PANIC!\n", dev->name);
        ret = 1;
        goto out_drain;
    }

    ReleaseSemaphore(&np->lock);
    dev->flags |= IFF_UP;
    ReportEvents(LIBBASE, dev, S2EVENT_ONLINE);

    return 0;

out_drain:
    drain_ring(dev);
    return ret;
}

static int nv_close(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);
    UBYTE *base;

    dev->flags &= ~IFF_UP;

    ObtainSemaphore(&np->lock);
    np->in_shutdown = 1;
    ReleaseSemaphore(&np->lock);

    dev->nu_toutNEED = FALSE;

    netif_stop_queue(dev);
    ObtainSemaphore(&np->lock);
    nv_stop_tx(dev);
    nv_stop_rx(dev);
    nv_txrx_reset(dev);

    /* disable interrupts on the nic or we will lock up */
    base = get_hwbase(dev);
    writel(0, base + NvRegIrqMask);
    pci_push(base);
    D(bug("%s: Irqmask is zero again\n", dev->name));

    ReleaseSemaphore(&np->lock);

    free_irq(dev);

    drain_ring(dev);

    HIDD_PCIDriver_FreePCIMem(dev->nu_PCIDriver, np->rx_buffer);
    HIDD_PCIDriver_FreePCIMem(dev->nu_PCIDriver, np->tx_buffer);

    if (np->wolenabled)
        nv_start_rx(dev);

    /* FIXME: power down nic */

    ReportEvents(LIBBASE, dev, S2EVENT_OFFLINE);

    return 0;
}

static void nv_link_irq(struct net_device *dev)
{
    UBYTE *base = get_hwbase(dev);
    ULONG miistat;

    miistat = readl(base + NvRegMIIStatus);
    writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);
    D(bug("%s: link change irq, status 0x%x.\n", dev->name, miistat));

    if (miistat & (NVREG_MIISTAT_LINKCHANGE))
        dev->linkchange(dev);
    D(bug("%s: link change notification done.\n", dev->name));
}

void nv_get_functions(struct net_device *Unit)
{
    Unit->initialize = nv_initialize;
    Unit->deinitialize = nv_deinitialize;
    Unit->start = nv_open;
    Unit->stop = nv_close;
    Unit->set_mac_address = nv_set_mac;
    Unit->linkchange = nv_linkchange;
    Unit->linkirq = nv_link_irq;
    Unit->descr_getlength = nv_descr_getlength;
    Unit->alloc_rx = nv_alloc_rx;
    Unit->set_multicast = nv_set_multicast;
}
