/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DesignWare MAC register, mdio and PHY access.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "dwmac.h"

/*
 * Registers are reached through a plain volatile long. A struct laid
 * over the block would have to be packed to pin the offsets down, and
 * packing tells the compiler to assume nothing about alignment - on a
 * strict alignment target it then splits every access into bytes, which
 * a register does not accept.
 */
ULONG DWMAC_Read(struct dwmac_hw *hw, ULONG reg)
{
    return *(volatile ULONG *)(hw->base + reg);
}

void DWMAC_Write(struct dwmac_hw *hw, ULONG reg, ULONG val)
{
    *(volatile ULONG *)(hw->base + reg) = val;
}

/*
 * The mdio bus runs at a divided version of the clock feeding the
 * control registers, and the divider is chosen by naming the band that
 * clock falls in rather than the ratio itself.
 */
static ULONG dwmac_csrrange(ULONG hz)
{
    const ULONG mhz = hz / 1000000;

    if (mhz >= 20 && mhz < 35)
        return 2;
    if (mhz >= 35 && mhz < 60)
        return 3;
    if (mhz >= 60 && mhz < 100)
        return 0;
    if (mhz >= 100 && mhz < 150)
        return 1;
    if (mhz >= 150 && mhz < 250)
        return 4;
    if (mhz >= 250 && mhz < 300)
        return 5;
    if (mhz >= 300 && mhz < 500)
        return 6;
    if (mhz >= 500 && mhz < 800)
        return 7;

    /* Nothing sensible to go on - the slowest divider is the safe one */
    return 7;
}

/*
 * Wait for the bus to go idle. A transfer takes a handful of
 * microseconds; this spins rather than sleeping because it is also
 * wanted before the timer is available.
 */
static BOOL dwmac_mdiowait(struct dwmac_hw *hw)
{
    ULONG spins = DWMAC_MDIO_TIMEOUT;

    while (spins--)
    {
        if (!(DWMAC_Read(hw, DWMAC_MAC_MDIO_ADDR) & DWMAC_MDIO_BUSY))
            return TRUE;
    }

    D(bug("[dwmac] mdio bus stayed busy\n");)

    return FALSE;
}

static ULONG dwmac_mdioaddr(struct dwmac_hw *hw, ULONG phy, ULONG reg)
{
    return ((phy & 0x1f) << DWMAC_MDIO_ADDR_SHIFT)
         | ((reg & 0x1f) << DWMAC_MDIO_REG_SHIFT)
         | (dwmac_csrrange(hw->csrClock) << DWMAC_MDIO_CR_SHIFT)
         | DWMAC_MDIO_BUSY;
}

/* Returns the register contents, or -1 if the bus did not answer */
LONG DWMAC_MDIORead(struct dwmac_hw *hw, ULONG phy, ULONG reg)
{
    if (!dwmac_mdiowait(hw))
        return -1;

    DWMAC_Write(hw, DWMAC_MAC_MDIO_ADDR,
                dwmac_mdioaddr(hw, phy, reg) | DWMAC_MDIO_GOC_READ);

    if (!dwmac_mdiowait(hw))
        return -1;

    return (LONG)(DWMAC_Read(hw, DWMAC_MAC_MDIO_DATA) & DWMAC_MDIO_DATA_MASK);
}

BOOL DWMAC_MDIOWrite(struct dwmac_hw *hw, ULONG phy, ULONG reg, UWORD val)
{
    if (!dwmac_mdiowait(hw))
        return FALSE;

    DWMAC_Write(hw, DWMAC_MAC_MDIO_DATA, (ULONG)val);
    DWMAC_Write(hw, DWMAC_MAC_MDIO_ADDR,
                dwmac_mdioaddr(hw, phy, reg) | DWMAC_MDIO_GOC_WRITE);

    return dwmac_mdiowait(hw);
}

/*
 * Reset the whole block and set up everything that does not depend on
 * the link: bus mode, fifo thresholds, queue routing. The reset clears
 * every register, so this has to run before anything else is written.
 */
BOOL DWMAC_HWInit(struct dwmac_hw *hw)
{
    ULONG spins = 1000000;
    ULONG feat1, txfifo, rxfifo;

    DWMAC_Write(hw, DWMAC_DMA_MODE, DWMAC_DMA_MODE_SWR);
    while (DWMAC_Read(hw, DWMAC_DMA_MODE) & DWMAC_DMA_MODE_SWR)
    {
        if (!spins--)
        {
            D(bug("[dwmac] reset never completed\n");)
            return FALSE;
        }
    }

    /* The counter blocks would otherwise raise interrupts of their own */
    DWMAC_Write(hw, DWMAC_MMC_RX_INT_MASK, 0xffffffff);
    DWMAC_Write(hw, DWMAC_MMC_TX_INT_MASK, 0xffffffff);
    DWMAC_Write(hw, DWMAC_MMC_RX_IPC_INT_MASK, 0xffffffff);

    /* Descriptors carry 64-bit buffer addresses; RAM sits above 2GB */
    DWMAC_Write(hw, DWMAC_DMA_SYSBUS_MODE,
                DWMAC_DMA_SYSBUS_EAME | DWMAC_DMA_SYSBUS_BLEN16 |
                DWMAC_DMA_SYSBUS_BLEN8 | DWMAC_DMA_SYSBUS_BLEN4);

    /*
     * Store and forward both ways, with the queues sized to the fifos
     * the integrator chose. The size fields count 256-byte units, less
     * one.
     */
    feat1 = DWMAC_Read(hw, DWMAC_MAC_HW_FEATURE1);
    txfifo = 128 << DWMAC_HWFEAT1_TXFIFO(feat1);
    rxfifo = 128 << DWMAC_HWFEAT1_RXFIFO(feat1);

    DWMAC_Write(hw, DWMAC_MTL_TXQ0_OPMODE,
                ((txfifo / 256 - 1) << DWMAC_MTL_TXQ_TQS_SHIFT) |
                DWMAC_MTL_TXQ_TSF | DWMAC_MTL_TXQ_EN);
    DWMAC_Write(hw, DWMAC_MTL_RXQ0_OPMODE,
                ((rxfifo / 256 - 1) << DWMAC_MTL_RXQ_RQS_SHIFT) |
                DWMAC_MTL_RXQ_RSF);

    /* Receive traffic lands in queue 0, which must be switched on */
    DWMAC_Write(hw, DWMAC_MAC_RXQ_CTRL0, DWMAC_RXQ0_ENABLE_DCB);

    DWMAC_Write(hw, DWMAC_MAC_INT_ENABLE, 0);
    DWMAC_Write(hw, DWMAC_DMA_CH0_INT_ENABLE, 0);

    return TRUE;
}

void DWMAC_SetMACAddress(struct dwmac_hw *hw, const UBYTE *addr)
{
    DWMAC_Write(hw, DWMAC_MAC_ADDR_HI(0),
                (1UL << 31) | ((ULONG)addr[5] << 8) | addr[4]);
    DWMAC_Write(hw, DWMAC_MAC_ADDR_LO(0),
                ((ULONG)addr[3] << 24) | ((ULONG)addr[2] << 16) |
                ((ULONG)addr[1] << 8) | addr[0]);
}

/*
 * Firmware may have left the address it used in the filter registers;
 * all ones or all zeroes mean nobody ever programmed them.
 */
BOOL DWMAC_GetMACAddress(struct dwmac_hw *hw, UBYTE *addr)
{
    ULONG hi = DWMAC_Read(hw, DWMAC_MAC_ADDR_HI(0));
    ULONG lo = DWMAC_Read(hw, DWMAC_MAC_ADDR_LO(0));
    ULONG i, all0 = 0, all1 = 0xff;

    addr[0] = lo & 0xff;
    addr[1] = (lo >> 8) & 0xff;
    addr[2] = (lo >> 16) & 0xff;
    addr[3] = (lo >> 24) & 0xff;
    addr[4] = hi & 0xff;
    addr[5] = (hi >> 8) & 0xff;

    for (i = 0; i < ETH_ADDRESSSIZE; i++)
    {
        all0 |= addr[i];
        all1 &= addr[i];
    }

    return (all0 != 0) && (all1 != 0xff);
}

/*
 * Reset the PHY, advertise everything it can do, and set negotiation
 * going. Nobody waits here for the link - that takes seconds, and the
 * unit's process polls for the outcome.
 */
BOOL DWMAC_PHYInit(struct dwmac_hw *hw)
{
    ULONG phy = hw->phyAddr;
    ULONG spins = 100000;
    LONG bmcr, bmsr;

    if (!DWMAC_MDIOWrite(hw, phy, MII_BMCR, BMCR_RESET))
        return FALSE;

    do
    {
        bmcr = DWMAC_MDIORead(hw, phy, MII_BMCR);
        if (bmcr < 0)
            return FALSE;
        if (!spins--)
        {
            D(bug("[dwmac] phy stuck in reset\n");)
            return FALSE;
        }
    } while (bmcr & BMCR_RESET);

    DWMAC_MDIOWrite(hw, phy, MII_ANAR,
                    ANAR_CSMA | ANAR_10HD | ANAR_10FD |
                    ANAR_100HD | ANAR_100FD);

    bmsr = DWMAC_MDIORead(hw, phy, MII_BMSR);
    if (bmsr >= 0 && (bmsr & BMSR_ESTATEN))
        DWMAC_MDIOWrite(hw, phy, MII_GBCR, GBCR_1000FD | GBCR_1000HD);

    return DWMAC_MDIOWrite(hw, phy, MII_BMCR,
                           BMCR_ANENABLE | BMCR_ANRESTART);
}

/*
 * What did negotiation arrive at? The answer is the highest mode both
 * ends advertised. Returns the link state; speed and duplex are only
 * meaningful when it is up.
 */
BOOL DWMAC_PHYGetLink(struct dwmac_hw *hw, ULONG *mbps, BOOL *fullduplex)
{
    ULONG phy = hw->phyAddr;
    LONG bmsr, anar, anlpar, gbcr, gbsr;
    ULONG both;

    /* Link status latches low, so a stale down reading needs rereading */
    bmsr = DWMAC_MDIORead(hw, phy, MII_BMSR);
    if (bmsr >= 0 && !(bmsr & BMSR_LSTATUS))
        bmsr = DWMAC_MDIORead(hw, phy, MII_BMSR);

    if (bmsr < 0 || !(bmsr & BMSR_LSTATUS))
        return FALSE;

    *mbps = 10;
    *fullduplex = FALSE;

    if (!(bmsr & BMSR_ANEGCOMPLETE))
        return TRUE;

    if (bmsr & BMSR_ESTATEN)
    {
        gbcr = DWMAC_MDIORead(hw, phy, MII_GBCR);
        gbsr = DWMAC_MDIORead(hw, phy, MII_GBSR);

        if (gbcr >= 0 && gbsr >= 0)
        {
            if ((gbcr & GBCR_1000FD) && (gbsr & GBSR_LP1000FD))
            {
                *mbps = 1000;
                *fullduplex = TRUE;
                return TRUE;
            }
            if ((gbcr & GBCR_1000HD) && (gbsr & GBSR_LP1000HD))
            {
                *mbps = 1000;
                return TRUE;
            }
        }
    }

    anar = DWMAC_MDIORead(hw, phy, MII_ANAR);
    anlpar = DWMAC_MDIORead(hw, phy, MII_ANLPAR);
    if (anar < 0 || anlpar < 0)
        return TRUE;

    both = anar & anlpar;

    if (both & ANAR_100FD)
    {
        *mbps = 100;
        *fullduplex = TRUE;
    }
    else if (both & ANAR_100HD)
    {
        *mbps = 100;
    }
    else if (both & ANAR_10FD)
    {
        *fullduplex = TRUE;
    }

    return TRUE;
}
