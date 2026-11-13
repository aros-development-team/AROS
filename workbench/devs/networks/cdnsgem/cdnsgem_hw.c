/*
 * Copyright (c) 2021-2022 Visa Hankala
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Cadence GEM register, mdio and PHY access.

    Reset and bring-up follow cad_reset()/cad_up() in OpenBSD
    sys/dev/fdt/if_cad.c. The PHY half is the BCM54xx code from bcmgenet,
    which drives the same BCM54213PE on the Pi 4.
*/

/* Bring-up diagnostics: reset, PHY and link changes only. */
#define DEBUG 1
#include <aros/debug.h>
#include <aros/macros.h>

#include <proto/exec.h>

#include "cdnsgem.h"

#define GEM_MDIO_TIMEOUT 10000

static inline void __dsb(void) { asm volatile("dsb sy" ::: "memory"); }
static inline void __dmb(void) { asm volatile("dmb sy" ::: "memory"); }

static void cdnsgem_wait(struct CDNSGEMUnit *unit, ULONG usec)
{
    struct timerequest *timer = unit->cgu_TimerReq;

    timer->tr_node.io_Command = TR_ADDREQUEST;
    timer->tr_time.tv_secs = 0;
    timer->tr_time.tv_micro = usec;

    DoIO((struct IORequest *)timer);
}

ULONG CDNSGEM_Read(struct cdnsgem_hw *hw, ULONG reg)
{
    ULONG val;

    __dmb();
    val = *(volatile ULONG *)(hw->base + reg);
    __dsb();

    return val;
}

void CDNSGEM_Write(struct cdnsgem_hw *hw, ULONG reg, ULONG val)
{
    __dsb();
    *(volatile ULONG *)(hw->base + reg) = val;
    __dmb();
}

static BOOL cdnsgem_mdio_wait(struct cdnsgem_hw *hw)
{
    ULONG spins;

    for (spins = GEM_MDIO_TIMEOUT; spins > 0; spins--)
    {
        if (CDNSGEM_Read(hw, GEM_NETSR) & GEM_NETSR_PHY_MGMT_IDLE)
            return TRUE;
    }

    return FALSE;
}

static BOOL cdnsgem_mdio_oper(struct cdnsgem_hw *hw, ULONG phy, ULONG reg, ULONG oper)
{
    oper |= (phy << GEM_PHYMNTNC_ADDR_SHIFT) & GEM_PHYMNTNC_ADDR_MASK;
    oper |= (reg << GEM_PHYMNTNC_REG_SHIFT) & GEM_PHYMNTNC_REG_MASK;
    oper |= GEM_PHYMNTNC_CLAUSE_22 | GEM_PHYMNTNC_MUST_10;

    if (!cdnsgem_mdio_wait(hw))
        return FALSE;

    CDNSGEM_Write(hw, GEM_PHYMNTNC, oper);

    return cdnsgem_mdio_wait(hw);
}

LONG CDNSGEM_MDIORead(struct cdnsgem_hw *hw, ULONG phy, ULONG reg)
{
    if (!cdnsgem_mdio_oper(hw, phy, reg, GEM_PHYMNTNC_OP_READ))
    {
        D(bug("[cdnsgem] MDIO read timeout, phy %u reg %u\n", phy, reg);)
        return -1;
    }

    return (LONG)(CDNSGEM_Read(hw, GEM_PHYMNTNC) & GEM_PHYMNTNC_DATA_MASK);
}

BOOL CDNSGEM_MDIOWrite(struct cdnsgem_hw *hw, ULONG phy, ULONG reg, UWORD val)
{
    if (!cdnsgem_mdio_oper(hw, phy, reg, GEM_PHYMNTNC_OP_WRITE | val))
    {
        D(bug("[cdnsgem] MDIO write timeout, phy %u reg %u\n", phy, reg);)
        return FALSE;
    }

    return TRUE;
}

/*
 * Everything off and every queue pointer cleared, then the MDIO bus
 * enabled. pclk is not known to a PCI-probed driver, so MDC uses the
 * largest divider (224), which keeps it under 2.5 MHz up to 560 MHz.
 */
void CDNSGEM_HWReset(struct cdnsgem_hw *hw)
{
    ULONG i, netcfg;

    CDNSGEM_Write(hw, GEM_NETCTL, 0);
    CDNSGEM_Write(hw, GEM_IDR, ~0U);
    CDNSGEM_Write(hw, GEM_RXSR, ~0U);
    CDNSGEM_Write(hw, GEM_TXSR, ~0U);
    CDNSGEM_Write(hw, GEM_ISR, CDNSGEM_Read(hw, GEM_ISR));

    CDNSGEM_Write(hw, GEM_RXQBASEHI, 0);
    CDNSGEM_Write(hw, GEM_TXQBASEHI, 0);
    CDNSGEM_Write(hw, GEM_RXQBASE, 0);
    CDNSGEM_Write(hw, GEM_TXQBASE, 0);

    for (i = 1; i < GEM_MAX_PRIQ; i++)
    {
        if (hw->qmask & (1U << i))
        {
            if (i < 8)
                CDNSGEM_Write(hw, GEM_RXQ1BASE(i - 1), 0);
            else
                CDNSGEM_Write(hw, GEM_RXQ8BASE(i - 8), 0);
            CDNSGEM_Write(hw, GEM_TXQ1BASE(i - 1), 0);
        }
    }

    /* No screeners, so every frame goes to queue 0 */
    for (i = 0; i < hw->ntype1scr; i++)
        CDNSGEM_Write(hw, GEM_SCR_TYPE1(i), 0);
    for (i = 0; i < hw->ntype2scr; i++)
        CDNSGEM_Write(hw, GEM_SCR_TYPE2(i), 0);

    netcfg = CDNSGEM_Read(hw, GEM_NETCFG);
    netcfg &= ~GEM_NETCFG_MDCCLKDIV_MASK;
    netcfg |= 7 << GEM_NETCFG_MDCCLKDIV_SHIFT;
    CDNSGEM_Write(hw, GEM_NETCFG, netcfg);

    hw->netctl = GEM_NETCTL_MDEN;
    CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl);
}

/* The parts of cad_up() that do not depend on the rings */
void CDNSGEM_HWInit(struct cdnsgem_hw *hw)
{
    ULONG val;

    val = CDNSGEM_Read(hw, GEM_NETCFG);
    val |= GEM_NETCFG_FCSREM | GEM_NETCFG_1000 | GEM_NETCFG_100 |
           GEM_NETCFG_FDEN | GEM_NETCFG_1536RXEN;
    val &= ~(GEM_NETCFG_RXOFFS_MASK | GEM_NETCFG_BCASTDI |
             GEM_NETCFG_RXCSUMEN | GEM_NETCFG_SGMIIEN | GEM_NETCFG_PCSSEL);
    val |= GEM_RX_OFFSET << GEM_NETCFG_RXOFFS_SHIFT;
    CDNSGEM_Write(hw, GEM_NETCFG, val);

    /* 64-bit descriptors, little-endian, whole frames, 16-beat bursts */
    val = CDNSGEM_Read(hw, GEM_DMACR);
    val |= GEM_DMACR_DMA64 | GEM_DMACR_AHBDISC | GEM_DMACR_TXSIZE;
    val &= ~(GEM_DMACR_ES_DESCR | GEM_DMACR_ES_PDATA | GEM_DMACR_TXCSUMEN |
             GEM_DMACR_RXSIZE_MASK | GEM_DMACR_RXBUF_MASK | GEM_DMACR_BLEN_MASK);
    val |= GEM_DMACR_RXSIZE_8K;
    val |= (GEM_BUFSIZE / 64) << GEM_DMACR_RXBUF_SHIFT;
    val |= GEM_DMACR_BLEN_16;
    CDNSGEM_Write(hw, GEM_DMACR, val);

    CDNSGEM_Write(hw, GEM_NETCTL, hw->netctl | GEM_NETCTL_STATCLR);
}

/*
 * Lay both rings out afresh and point the engine at them. Only with RX and
 * TX disabled: clearing TXEN rewinds the engine to the ring base, so the
 * software indices have to start over with it.
 */
void CDNSGEM_InitRings(struct CDNSGEMUnit *unit)
{
    struct cdnsgem_hw *hw = unit->cgu_HW;
    struct cdnsgem_ring *tx = &unit->cgu_TX;
    struct cdnsgem_ring *rx = &unit->cgu_RX;
    UQUAD txbase, rxbase;
    ULONG i;

    for (i = 0; i < CDNSGEM_NDESC; i++)
    {
        tx->desc[i].addrlo = (ULONG)tx->dma[i];
        tx->desc[i].addrhi = (ULONG)(tx->dma[i] >> 32);
        tx->desc[i].status = GEM_TXD_USED;
    }
    tx->desc[CDNSGEM_NDESC - 1].status |= GEM_TXD_WRAP;
    tx->desc[CDNSGEM_NDESC].addrlo = 0;
    tx->desc[CDNSGEM_NDESC].addrhi = 0;
    tx->desc[CDNSGEM_NDESC].status = GEM_TXD_USED | GEM_TXD_WRAP;

    for (i = 0; i < CDNSGEM_NDESC; i++)
    {
        rx->desc[i].addrhi = (ULONG)(rx->dma[i] >> 32);
        rx->desc[i].status = 0;
        rx->desc[i].addrlo = (ULONG)rx->dma[i];
    }
    rx->desc[CDNSGEM_NDESC - 1].addrlo |= GEM_RXD_ADDR_WRAP;
    rx->desc[CDNSGEM_NDESC].addrhi = 0;
    rx->desc[CDNSGEM_NDESC].addrlo = GEM_RXD_ADDR_USED | GEM_RXD_ADDR_WRAP;

    tx->pidx = tx->cidx = 0;
    rx->pidx = rx->cidx = 0;

    __dsb();

    txbase = unit->cgu_DescDMA;
    rxbase = txbase + (CDNSGEM_NDESC + 1) * sizeof(struct gem_desc);

    CDNSGEM_Write(hw, GEM_TXQBASEHI, (ULONG)(txbase >> 32));
    CDNSGEM_Write(hw, GEM_TXQBASE, (ULONG)txbase);
    CDNSGEM_Write(hw, GEM_RXQBASEHI, (ULONG)(rxbase >> 32));
    CDNSGEM_Write(hw, GEM_RXQBASE, (ULONG)rxbase);

    /* Unused priority queues get the dummies, disabled where possible */
    txbase += CDNSGEM_NDESC * sizeof(struct gem_desc);
    rxbase += CDNSGEM_NDESC * sizeof(struct gem_desc);
    for (i = 1; i < GEM_MAX_PRIQ; i++)
    {
        if (hw->qmask & (1U << i))
        {
            CDNSGEM_Write(hw, GEM_TXQ1BASE(i - 1),
                          (ULONG)txbase | GEM_TXQ1BASE_DISABLE);
            if (i < 8)
                CDNSGEM_Write(hw, GEM_RXQ1BASE(i - 1),
                              (ULONG)rxbase | GEM_RXQ1BASE_DISABLE);
            else
                CDNSGEM_Write(hw, GEM_RXQ8BASE(i - 8),
                              (ULONG)rxbase | GEM_RXQ8BASE_DISABLE);
        }
    }
}

void CDNSGEM_SetMACAddress(struct cdnsgem_hw *hw, const UBYTE *addr)
{
    ULONG i;

    CDNSGEM_Write(hw, GEM_LADDRL(0),
                  addr[0] | ((ULONG)addr[1] << 8) |
                  ((ULONG)addr[2] << 16) | ((ULONG)addr[3] << 24));
    CDNSGEM_Write(hw, GEM_LADDRH(0), addr[4] | ((ULONG)addr[5] << 8));

    for (i = 1; i < GEM_LADDRNUM; i++)
    {
        CDNSGEM_Write(hw, GEM_LADDRL(i), 0);
        CDNSGEM_Write(hw, GEM_LADDRH(i), 0);
    }
}

/* Whatever firmware left in the first non-empty address filter */
BOOL CDNSGEM_GetMACAddress(struct cdnsgem_hw *hw, UBYTE *addr)
{
    ULONG i, lo, hi;

    for (i = 0; i < GEM_LADDRNUM; i++)
    {
        lo = CDNSGEM_Read(hw, GEM_LADDRL(i));
        hi = CDNSGEM_Read(hw, GEM_LADDRH(i));
        if (lo != 0 || hi != 0)
        {
            addr[0] = lo;
            addr[1] = lo >> 8;
            addr[2] = lo >> 16;
            addr[3] = lo >> 24;
            addr[4] = hi;
            addr[5] = hi >> 8;
            return TRUE;
        }
    }

    return FALSE;
}

/* Bit index in the 64-bit hash: the address XOR-folded in 6-bit pieces */
UBYTE CDNSGEM_HashMAC(const UBYTE *addr)
{
    UQUAD val = 0;
    UBYTE hash = 0;
    LONG i;

    for (i = ETH_ADDRESSSIZE - 1; i >= 0; i--)
        val = (val << 8) | addr[i];

    for (i = 0; i < 8; i++)
    {
        hash ^= val;
        val >>= 6;
    }

    return hash & 0x3f;
}

static LONG cdnsgem_auxctl_read(struct cdnsgem_hw *hw, UWORD sel)
{
    if (!CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_BCM54XX_AUX_CTL,
            BCM54XX_AUXCTL_SHDWSEL_MASK |
            (sel << BCM54XX_AUXCTL_SHDWSEL_READ_SHIFT)))
        return -1;

    return CDNSGEM_MDIORead(hw, hw->phyAddr, MII_BCM54XX_AUX_CTL);
}

static LONG cdnsgem_shadow_read(struct cdnsgem_hw *hw, UWORD sel)
{
    LONG val;

    if (!CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_BCM54XX_SHD,
            (UWORD)(sel << BCM54XX_SHD_SEL_SHIFT)))
        return -1;

    val = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_BCM54XX_SHD);

    return (val < 0) ? val : (val & BCM54XX_SHD_DATA_MASK);
}

/*
 * Hand the PHY the RGMII clock delays the board's phy-mode calls for. The
 * Pi 5 says "rgmii-id": the PHY supplies both, the MAC neither.
 */
static void cdnsgem_phy_config_delays(struct cdnsgem_hw *hw)
{
    BOOL rxdelay = (hw->phyMode == GEM_PHY_MODE_RGMII_ID ||
                    hw->phyMode == GEM_PHY_MODE_RGMII_RXID);
    BOOL txdelay = (hw->phyMode == GEM_PHY_MODE_RGMII_ID ||
                    hw->phyMode == GEM_PHY_MODE_RGMII_TXID);
    LONG val;

    val = cdnsgem_auxctl_read(hw, BCM54XX_AUXCTL_SHDWSEL_MISC);
    if (val >= 0)
    {
        UWORD misc = (UWORD)val | BCM54XX_AUXCTL_MISC_WREN;

        if (rxdelay)
            misc |= BCM54XX_AUXCTL_MISC_RGMII_SKEW_EN;
        else
            misc &= ~BCM54XX_AUXCTL_MISC_RGMII_SKEW_EN;

        CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_BCM54XX_AUX_CTL,
                          BCM54XX_AUXCTL_SHDWSEL_MISC | misc);
    }

    val = cdnsgem_shadow_read(hw, BCM54XX_SHD_CLK_CTL);
    if (val >= 0)
    {
        UWORD clk = (UWORD)val;

        if (txdelay)
            clk |= BCM54XX_SHD_CLK_CTL_GTXCLK_EN;
        else
            clk &= ~BCM54XX_SHD_CLK_CTL_GTXCLK_EN;

        CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_BCM54XX_SHD,
            BCM54XX_SHD_WRITE |
            (BCM54XX_SHD_CLK_CTL << BCM54XX_SHD_SEL_SHIFT) |
            (clk & BCM54XX_SHD_DATA_MASK));
    }

    D(bug("[cdnsgem] PHY RX delay %s, TX delay %s\n",
          rxdelay ? "on" : "off", txdelay ? "on" : "off");)
}

BOOL CDNSGEM_PHYInit(struct CDNSGEMUnit *unit)
{
    struct cdnsgem_hw *hw = unit->cgu_HW;
    LONG val;
    ULONG spins;

    if (!CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_BMCR, BMCR_RESET))
        return FALSE;

    for (spins = 50000; spins > 0; spins--)
    {
        val = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_BMCR);
        if (val < 0)
            return FALSE;

        if ((val & BMCR_RESET) == 0)
            break;

        cdnsgem_wait(unit, 10);
    }

    if (spins == 0)
    {
        D(bug("[cdnsgem] PHY reset timeout\n");)
        return FALSE;
    }

    /* The reset above returned the clock delays to their strap defaults */
    cdnsgem_phy_config_delays(hw);

    if (!CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_ANAR,
            ANAR_CSMA |
            ANAR_10HD | ANAR_10FD |
            ANAR_100HD | ANAR_100FD))
        return FALSE;

    /* The MAC cannot do gigabit half duplex */
    if (!CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_GBCR, GBCR_1000FD))
        return FALSE;

    return CDNSGEM_MDIOWrite(hw, hw->phyAddr, MII_BMCR,
                             BMCR_ANENABLE | BMCR_ANRESTART);
}

/*
 * Ask the PHY what was negotiated and set the MAC to match. RP1 derives
 * the RGMII TX clock from the speed bits itself, so there is no clock to
 * reprogram here (sc_hw_tx_freq in if_cad.c).
 */
BOOL CDNSGEM_PHYGetLink(struct cdnsgem_hw *hw, ULONG *mbps, BOOL *fullduplex)
{
    LONG bmsr, gbsr, anlpar;
    ULONG netcfg, val;

    *mbps = 0;
    *fullduplex = FALSE;

    /* BMSR link status latches low; the second read is the current state */
    if (CDNSGEM_MDIORead(hw, hw->phyAddr, MII_BMSR) < 0)
        return FALSE;

    bmsr = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_BMSR);
    if (bmsr < 0)
        return FALSE;

    if ((bmsr & (BMSR_LSTATUS | BMSR_ANEGCOMPLETE)) !=
        (BMSR_LSTATUS | BMSR_ANEGCOMPLETE))
        return FALSE;

    gbsr = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_GBSR);
    anlpar = CDNSGEM_MDIORead(hw, hw->phyAddr, MII_ANLPAR);
    if (gbsr < 0 || anlpar < 0)
        return FALSE;

    if (gbsr & GBSR_LP1000FD)
    {
        *mbps = 1000;
        *fullduplex = TRUE;
    }
    else if (anlpar & (ANAR_100FD | ANAR_100HD))
    {
        *mbps = 100;
        *fullduplex = (anlpar & ANAR_100FD) ? TRUE : FALSE;
    }
    else if (anlpar & (ANAR_10FD | ANAR_10HD))
    {
        *mbps = 10;
        *fullduplex = (anlpar & ANAR_10FD) ? TRUE : FALSE;
    }
    else
        return FALSE;

    netcfg = CDNSGEM_Read(hw, GEM_NETCFG);
    val = netcfg & ~(GEM_NETCFG_FDEN | GEM_NETCFG_100 | GEM_NETCFG_1000);
    if (*fullduplex)
        val |= GEM_NETCFG_FDEN;
    if (*mbps == 1000)
        val |= GEM_NETCFG_100 | GEM_NETCFG_1000;
    else if (*mbps == 100)
        val |= GEM_NETCFG_100;

    if (val != netcfg)
        CDNSGEM_Write(hw, GEM_NETCFG, val);

    return TRUE;
}
