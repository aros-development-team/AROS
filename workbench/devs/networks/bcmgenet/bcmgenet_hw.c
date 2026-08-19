/*
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' ...
 */

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom GENETv5 register, mdio and PHY access.

    Port target: OpenBSD sys/dev/ic/bcmgenet.c (3-clause BSD). Function
    names below match the OpenBSD ones they replace, to make porting
    mechanical; the register/bit names come from bcmgenetreg.h, folded
    into bcmgenet.h in the plain shift/mask style the rest of this tree
    uses instead of the __BIT()/__BITS() macros OpenBSD/NetBSD define.
*/

#include "exec/memory.h"
#define DEBUG 1
#include <aros/debug.h>
#include <memory.h>
#include <aros/macros.h>

#define ARM_PERIIOBASE BCM2711_PERIIOBASE
#include <hardware/bcm2708.h>

#include <proto/exec.h>

#include "bcmgenet.h"

#define GENET_MDIO_TIMEOUT 100000

static inline void __dsb(void) { asm volatile("dsb sy" ::: "memory"); }
static inline void __dmb(void) { asm volatile("dmb sy" ::: "memory"); }

static void bcmgenet_wait(struct BCMGENETUnit *unit, ULONG usec) {
    struct timerequest *timer = unit->bgu_TimerReq;

    timer->tr_node.io_Command = TR_ADDREQUEST;
    timer->tr_time.tv_secs = 0;
    timer->tr_time.tv_micro = usec;

    DoIO((struct IORequest *)timer);
}

ULONG BCMGENET_Read(struct bcmgenet_hw *hw, ULONG reg)
{
    ULONG val;

    __dmb();
    val = *(volatile ULONG *)(hw->base + reg);
    __dsb();

    return val;
}

void BCMGENET_Write(struct bcmgenet_hw *hw, ULONG reg, ULONG val)
{
    __dsb();
    *(volatile ULONG *)(hw->base + reg) = val;
    __dmb();
}

LONG BCMGENET_MDIORead(struct bcmgenet_hw *hw, ULONG phy, ULONG reg)
{
    ULONG val;
    ULONG spins = GENET_MDIO_TIMEOUT;

    val = GENET_MDIO_READ | GENET_MDIO_START_BUSY |
          ((phy << GENET_MDIO_PMD_SHIFT) & GENET_MDIO_PMD_MASK) |
          ((reg << GENET_MDIO_REG_SHIFT) & GENET_MDIO_REG_MASK);

    BCMGENET_Write(hw, GENET_MDIO_CMD, val);

    while (spins--)
    {
        val = BCMGENET_Read(hw, GENET_MDIO_CMD);
        if ((val & GENET_MDIO_START_BUSY) == 0)
            return (LONG)(val & 0xffff);
    }

    D(bug("[bcmgenet] MDIO read timeout, phy %lu reg %lu\n", phy, reg);)
    return -1;
}

BOOL BCMGENET_MDIOWrite(struct bcmgenet_hw *hw, ULONG phy, ULONG reg, UWORD val)
{
    ULONG cmd;
    ULONG spins = GENET_MDIO_TIMEOUT;

    cmd = GENET_MDIO_WRITE | GENET_MDIO_START_BUSY | (ULONG)val |
          ((phy << GENET_MDIO_PMD_SHIFT) & GENET_MDIO_PMD_MASK) |
          ((reg << GENET_MDIO_REG_SHIFT) & GENET_MDIO_REG_MASK);

    BCMGENET_Write(hw, GENET_MDIO_CMD, cmd);

    while (spins--)
    {
        if ((BCMGENET_Read(hw, GENET_MDIO_CMD) & GENET_MDIO_START_BUSY) == 0)
            return TRUE;
    }

    D(bug("[bcmgenet] MDIO write timeout, phy %lu reg %lu\n", phy, reg);)
    return FALSE;
}

static void bcmgenet_disable_dma(struct BCMGENETUnit *unit)
{
    ULONG cmd;
    struct bcmgenet_hw *hw = unit->bgu_HW;

    D(bug("[bcmgenet] Turn off UniMAC RX\n");)
    /* Turn off UniMAC RX */
    D(bug("[bcmgenet] read UMAC_CMD\n");)
    cmd = BCMGENET_Read(hw, GENET_UMAC_CMD);
    D(bug("[bcmgenet] UMAC_CMD = %08lx\n", cmd);)

    cmd &= ~GENET_UMAC_CMD_RXEN;

    D(bug("[bcmgenet] write UMAC_CMD = %08lx\n", cmd);)
    BCMGENET_Write(hw, GENET_UMAC_CMD, cmd);

    /* Diagnostic only: make a deferred MMIO write fault appear here. */
    asm volatile("dsb sy" ::: "memory");

    D(bug("[bcmgenet] UMAC_CMD write completed\n");)

    D(bug("[bcmgenet] Turn off RX DMA and queue 16\n");)
    /* Turn off RX DMA and queue 16 */
    cmd = BCMGENET_Read(hw, GENET_RX_DMA_CTRL);
    cmd &= ~GENET_RX_DMA_CTRL_EN;
    cmd &= ~GENET_RX_DMA_CTRL_RBUF_EN(GENET_DMA_DEFAULT_QUEUE);
    BCMGENET_Write(hw, GENET_RX_DMA_CTRL, cmd);

    D(bug("[bcmgenet] Turn off TX DMA and queue 16\n");)
    /* Turn off TX DMA and queue 16 */
    cmd = BCMGENET_Read(hw, GENET_TX_DMA_CTRL);
    cmd &= ~GENET_TX_DMA_CTRL_EN;
    cmd &= ~GENET_TX_DMA_CTRL_RBUF_EN(GENET_DMA_DEFAULT_QUEUE);
    BCMGENET_Write(hw, GENET_TX_DMA_CTRL, cmd);

    D(bug("[bcmgenet] Flush TX FIFO\n");)
    /* Flush TX FIFO */
    BCMGENET_Write(hw, GENET_UMAC_TX_FLUSH, 1);
    bcmgenet_wait(unit, 10);
    BCMGENET_Write(hw, GENET_UMAC_TX_FLUSH, 0);

    cmd = BCMGENET_Read(hw, GENET_UMAC_CMD);
    cmd &= ~GENET_UMAC_CMD_TXEN;
    BCMGENET_Write(hw, GENET_UMAC_CMD, cmd);
}

BOOL BCMGENET_HWReset(struct BCMGENETUnit *unit)
{
    struct bcmgenet_hw *hw = unit->bgu_HW;
    ULONG cmd;
    /*
     * The RBUF block is flushed and reset before UniMAC is touched at all:
     * on the BCM2711 a read of GENET_UMAC_CMD before this point takes an
     * external abort (SError, GISB slave error), so the OpenBSD order -
     * genet_disable_dma() first - cannot be used here. Nothing is running
     * yet at this point anyway, so there is no DMA to stop.
    */

    /* Request that GENET flush and reset the receive-buffer block (RBUF) */
    cmd = BCMGENET_Read(hw, GENET_SYS_RBUF_FLUSH_CTRL);
    cmd |= GENET_SYS_RBUF_FLUSH_RESET;
    BCMGENET_Write(hw, GENET_SYS_RBUF_FLUSH_CTRL, cmd);
    bcmgenet_wait(unit, 10);

    /* Deassert RBUF reset */
    cmd &= ~GENET_SYS_RBUF_FLUSH_RESET;
    BCMGENET_Write(hw, GENET_SYS_RBUF_FLUSH_CTRL, cmd);
    bcmgenet_wait(unit, 10);

    /* Leave the flush control register in a known empty state */
    BCMGENET_Write(hw, GENET_SYS_RBUF_FLUSH_CTRL, 0);
    bcmgenet_wait(unit, 10);

    /* Disable UniMAC completely before its software reset. This clears
        * any previous RX, TX, or link configuration */
    BCMGENET_Write(hw, GENET_UMAC_CMD, 0);

    /* Start the UniMAC software reset */
    BCMGENET_Write(hw, GENET_UMAC_CMD,
        GENET_UMAC_CMD_LCL_LOOP_EN |
        GENET_UMAC_CMD_SW_RESET);
    bcmgenet_wait(unit, 10);

    /* Take UniMAC out of reset while leaving RX and TX disabled */
    BCMGENET_Write(hw, GENET_UMAC_CMD, 0);

    /* Reset the hardware MIB statistics: TX, RX, and runt-frame counters */
    BCMGENET_Write(hw, GENET_UMAC_MIB_CTRL,
        GENET_UMAC_MIB_RESET_RUNT |
        GENET_UMAC_MIB_RESET_RX |
        GENET_UMAC_MIB_RESET_TX);

    /* Deassert the MIB reset bits */
    BCMGENET_Write(hw, GENET_UMAC_MIB_CTRL, 0);

    /* Set the largest frame size accepted by GENET's RX/TX buffering */
    BCMGENET_Write(hw, GENET_UMAC_MAX_FRAME_LEN, GENET_BUFSIZE);

    /* Request a two-byte offset for received data. This provides the
        * desired alignment for Ethernet frames in receive buffers */
    cmd = BCMGENET_Read(hw, GENET_RBUF_CTRL);
    cmd |= GENET_RBUF_ALIGN_2B;
    BCMGENET_Write(hw, GENET_RBUF_CTRL, cmd);

    /* Select GENET's expected transmit-buffer size mode */
    BCMGENET_Write(hw, GENET_RBUF_TBUF_SIZE_CTRL, 1);

    /* just for diag */
    BCMGENET_Read(hw, GENET_UMAC_CMD);
    return TRUE;
}

BOOL bcmgenet_fill_rx_ring(struct BCMGENETUnit *unit, ULONG qid)
{
    struct bcmgenet_ring *rx = &unit->bgu_RX;
    struct bcmgenet_hw *hw = unit->bgu_HW;

    for (ULONG i = 0; i < GENET_DMA_DESC_COUNT; i++) {
        /* Cleans the buffer out of the D-cache and yields the address the
         * engine has to be given. Identity on aarch64 today, but this is
         * the only place that stays correct if that ever changes. */
        ULONG len = GENET_BUFSIZE;
        UQUAD dma = (UQUAD)(IPTR)CachePreDMA(rx->buf[i], &len, 0);

        BCMGENET_Write(hw, GENET_RX_DESC_ADDRESS_LO(i), (ULONG)dma);
        BCMGENET_Write(hw, GENET_RX_DESC_ADDRESS_HI(i), (ULONG)(dma >> 32));
    }

    rx->cidx = GENET_DMA_DESC_COUNT;
    BCMGENET_Write(hw, GENET_RX_DMA_CONS_INDEX(qid), rx->cidx);

    return TRUE;
}

static BOOL bcmgenet_init_rings(struct BCMGENETUnit *unit, ULONG qid) {
    struct bcmgenet_hw *hw = unit->bgu_HW;

    unit->bgu_TX.next = 0;
    unit->bgu_TX.cidx = 0;
    unit->bgu_TX.pidx = 0;

    BCMGENET_Write(hw, GENET_TX_SCB_BURST_SIZE, 0x08);
   	BCMGENET_Write(hw, GENET_TX_DMA_READ_PTR_LO(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_READ_PTR_HI(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_CONS_INDEX(qid), unit->bgu_TX.cidx);
	BCMGENET_Write(hw, GENET_TX_DMA_PROD_INDEX(qid), unit->bgu_TX.pidx);
	BCMGENET_Write(hw, GENET_TX_DMA_RING_BUF_SIZE(qid),
	((GENET_DMA_DESC_COUNT << GENET_DMA_RING_BUF_SIZE_DESC_COUNT_SHIFT) &
      GENET_DMA_RING_BUF_SIZE_DESC_COUNT_MASK) |
	((GENET_BUFSIZE << GENET_DMA_RING_BUF_SIZE_BUF_LENGTH_SHIFT) &
      GENET_DMA_RING_BUF_SIZE_BUF_LENGTH_MASK));
	BCMGENET_Write(hw, GENET_TX_DMA_START_ADDR_LO(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_START_ADDR_HI(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_END_ADDR_LO(qid),
	    GENET_DMA_DESC_COUNT * GENET_DMA_DESC_SIZE / 4 - 1);
	BCMGENET_Write(hw, GENET_TX_DMA_END_ADDR_HI(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_MBUF_DONE_THRES(qid), 1);
	BCMGENET_Write(hw, GENET_TX_DMA_FLOW_PERIOD(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_WRITE_PTR_LO(qid), 0);
	BCMGENET_Write(hw, GENET_TX_DMA_WRITE_PTR_HI(qid), 0);

	BCMGENET_Write(hw, GENET_TX_DMA_RING_CFG, 1UL << qid);

	ULONG cmd = BCMGENET_Read(hw, GENET_TX_DMA_CTRL);
	cmd |= GENET_TX_DMA_CTRL_EN;
	cmd |= GENET_TX_DMA_CTRL_RBUF_EN(qid);
	BCMGENET_Write(hw,  GENET_TX_DMA_CTRL, cmd);

	unit->bgu_RX.next = 0;
    unit->bgu_RX.cidx = 0;
    unit->bgu_RX.pidx = GENET_DMA_DESC_COUNT;

	BCMGENET_Write(hw,  GENET_RX_SCB_BURST_SIZE, 0x08);

	BCMGENET_Write(hw,  GENET_RX_DMA_WRITE_PTR_LO(qid), 0);
	BCMGENET_Write(hw, GENET_RX_DMA_WRITE_PTR_HI(qid), 0);
	BCMGENET_Write(hw,  GENET_RX_DMA_PROD_INDEX(qid), unit->bgu_RX.pidx);
	BCMGENET_Write(hw,  GENET_RX_DMA_CONS_INDEX(qid), unit->bgu_RX.cidx);

	BCMGENET_Write(hw, GENET_RX_DMA_RING_BUF_SIZE(qid),
    ((GENET_DMA_DESC_COUNT << GENET_DMA_RING_BUF_SIZE_DESC_COUNT_SHIFT) &
     GENET_DMA_RING_BUF_SIZE_DESC_COUNT_MASK) |
    (GENET_BUFSIZE & GENET_DMA_RING_BUF_SIZE_BUF_LENGTH_MASK));

	BCMGENET_Write(hw,  GENET_RX_DMA_START_ADDR_LO(qid), 0);
	BCMGENET_Write(hw,  GENET_RX_DMA_START_ADDR_HI(qid), 0);
	BCMGENET_Write(hw, GENET_RX_DMA_END_ADDR_LO(qid),
	    GENET_DMA_DESC_COUNT * GENET_DMA_DESC_SIZE / 4 - 1);
	BCMGENET_Write(hw,  GENET_RX_DMA_END_ADDR_HI(qid), 0);

	BCMGENET_Write(hw, GENET_RX_DMA_XON_XOFF_THRES(qid),
    ((5 << GENET_RX_DMA_XON_XOFF_THRES_LO_SHIFT) &
     GENET_RX_DMA_XON_XOFF_THRES_LO_MASK) |
    (((GENET_DMA_DESC_COUNT >> 4) <<
      GENET_RX_DMA_XON_XOFF_THRES_HI_SHIFT) &
     GENET_RX_DMA_XON_XOFF_THRES_HI_MASK));

	BCMGENET_Write(hw,  GENET_RX_DMA_READ_PTR_LO(qid), 0);
	BCMGENET_Write(hw, GENET_RX_DMA_READ_PTR_HI(qid), 0);

	BCMGENET_Write(hw,  GENET_RX_DMA_RING_CFG, 1UL << qid);

	bcmgenet_fill_rx_ring(unit, qid);

	cmd = BCMGENET_Read(hw, GENET_RX_DMA_CTRL);
	cmd |= GENET_RX_DMA_CTRL_EN;
	cmd |= GENET_RX_DMA_CTRL_RBUF_EN(qid);
	BCMGENET_Write(hw,  GENET_RX_DMA_CTRL, cmd);

	return TRUE;
}

BOOL bcmgenet_setup_rxbuf(struct bcmgenet_hw *hw, ULONG index)
{
    /*
	int error;

	error = bus_dmamap_load_mbuf(sc->sc_rx.buf_tag,
	    sc->sc_rx.buf_map[index].map, m, BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error != 0)
		return error;

	bus_dmamap_sync(sc->sc_rx.buf_tag, sc->sc_rx.buf_map[index].map,
	    0, sc->sc_rx.buf_map[index].map->dm_mapsize,
	    BUS_DMASYNC_PREREAD);

	sc->sc_rx.buf_map[index].mbuf = m;
	genet_setup_rxdesc(sc, index,
	    sc->sc_rx.buf_map[index].map->dm_segs[0].ds_addr,
	    sc->sc_rx.buf_map[index].map->dm_segs[0].ds_len);
*/
	return 0;
}

BOOL BCMGENET_HWInit(struct BCMGENETUnit *unit)
{
    ULONG qid = GENET_DMA_DEFAULT_QUEUE;

    if (!bcmgenet_init_rings(unit, qid))
        return FALSE;

    return TRUE;
}

static void bcmgenet_setup_rxfilter_mdf(struct bcmgenet_hw *hw, ULONG n,
                                        const UBYTE *addr)
{
    BCMGENET_Write(hw, GENET_UMAC_MDF_ADDR0(n),
                   ((ULONG)addr[0] << 8) | addr[1]);
    BCMGENET_Write(hw, GENET_UMAC_MDF_ADDR1(n),
                   ((ULONG)addr[2] << 24) | ((ULONG)addr[3] << 16) |
                   ((ULONG)addr[4] << 8) | addr[5]);
}

void BCMGENET_SetMACAddress(struct bcmgenet_hw *hw, const UBYTE *addr)
{
    static const UBYTE broadcast[ETH_ADDRESSSIZE] =
        { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    BCMGENET_Write(hw, GENET_UMAC_MAC0,
                   ((ULONG)addr[0] << 24) | ((ULONG)addr[1] << 16) |
                   ((ULONG)addr[2] << 8) | addr[3]);
    BCMGENET_Write(hw, GENET_UMAC_MAC1,
                   ((ULONG)addr[4] << 8) | addr[5]);

    bcmgenet_setup_rxfilter_mdf(hw, 0, broadcast);
    bcmgenet_setup_rxfilter_mdf(hw, 1, addr);
    BCMGENET_Write(hw, GENET_UMAC_MDF_CTRL, GENET_MDF_CTRL_ENABLE(2));
}

BOOL BCMGENET_GetMACAddress(struct bcmgenet_hw *hw, UBYTE *addr)
{
    ULONG maclo, machi;

    maclo = BCMGENET_Read(hw, GENET_UMAC_MAC0);
    machi = BCMGENET_Read(hw, GENET_UMAC_MAC1);

    addr[0] = (maclo >> 24) & 0xff;
    addr[1] = (maclo >> 16) & 0xff;
    addr[2] = (maclo >> 8) & 0xff;
    addr[3] = (maclo >> 0) & 0xff;
    addr[4] = (machi >> 8) & 0xff;
    addr[5] = (machi >> 0) & 0xff;

    return TRUE;
}

BOOL BCMGENET_PHYInit(struct BCMGENETUnit *unit)
{
    struct bcmgenet_hw *hw = unit->bgu_HW;
    LONG id1, id2, val;
    ULONG spins;

    id1 = BCMGENET_MDIORead(hw, hw->phyAddr, MII_PHYSID1);
    id2 = BCMGENET_MDIORead(hw, hw->phyAddr, MII_PHYSID2);

    if (id1 < 0 || id2 < 0)
        return FALSE;

    D(bug("[bcmgenet] PHY %lu: id %04lx:%04lx\n", hw->phyAddr, (ULONG)id1, (ULONG)id2);)

    if (!BCMGENET_MDIOWrite(hw, hw->phyAddr, MII_BMCR, BMCR_RESET))
        return FALSE;

    for (spins = 50000; spins > 0; spins--)
    {
        val = BCMGENET_MDIORead(hw, hw->phyAddr, MII_BMCR);
        if (val < 0)
            return FALSE;

        if ((val & BMCR_RESET) == 0)
            break;

        bcmgenet_wait(unit, 10);
    }

    if (spins == 0)
    {
        D(bug("[bcmgenet] PHY reset timeout\n");)
        return FALSE;
    }

    /* Announce support for 10, 100 Mbit, half and full duplex */
    if (!BCMGENET_MDIOWrite(hw, hw->phyAddr, MII_ANAR,
            ANAR_CSMA |
            ANAR_10HD | ANAR_10FD |
            ANAR_100HD | ANAR_100FD))
        return FALSE;

    /* Announce support for 1000 Mbit, half and full duplex */
    if (!BCMGENET_MDIOWrite(hw, hw->phyAddr, MII_GBCR,
            GBCR_1000HD | GBCR_1000FD))
        return FALSE;

    if (!BCMGENET_MDIOWrite(hw, hw->phyAddr, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART))
        return FALSE;

    return TRUE;
}

static void bcmgenet_set_link_down(struct bcmgenet_hw *hw)
{
    ULONG val = BCMGENET_Read(hw, GENET_EXT_RGMII_OOB_CTRL);

    val &= ~GENET_EXT_RGMII_OOB_RGMII_LINK;
    BCMGENET_Write(hw, GENET_EXT_RGMII_OOB_CTRL, val);
}


BOOL BCMGENET_PHYGetLink(struct bcmgenet_hw *hw, ULONG *mbps, BOOL *fullduplex)
{
    LONG bmsr, gbsr, anlpar;
    ULONG speed, val;

    *mbps = 0;
    *fullduplex = FALSE;

    /*
     * BMSR link status latches low: the first read tells whether the link
     * has been down at any point since it was last read, the second what
     * the state is now. Only the second answers the question asked here.
     */
    if (BCMGENET_MDIORead(hw, hw->phyAddr, MII_BMSR) < 0)
        return FALSE;

    bmsr = BCMGENET_MDIORead(hw, hw->phyAddr, MII_BMSR);
    if (bmsr < 0)
        return FALSE;

    if ((bmsr & (BMSR_LSTATUS | BMSR_ANEGCOMPLETE)) !=
        (BMSR_LSTATUS | BMSR_ANEGCOMPLETE)) {
        bcmgenet_set_link_down(hw);
        return FALSE;
    }

    gbsr = BCMGENET_MDIORead(hw, hw->phyAddr, MII_GBSR);
    anlpar = BCMGENET_MDIORead(hw, hw->phyAddr, MII_ANLPAR);
    if (gbsr < 0 || anlpar < 0)
        return FALSE;

    /*
     * Highest mode both ends offered wins. BCMGENET_PHYInit() advertises
     * every mode, so what the link partner announced settles it on its
     * own. ANLPAR carries the same bit layout as ANAR, which is why the
     * ANAR_* masks are used against it.
     */
    if (gbsr & (GBSR_LP1000FD | GBSR_LP1000HD))
    {
        *mbps = 1000;
        *fullduplex = (gbsr & GBSR_LP1000FD) ? TRUE : FALSE;
        speed = GENET_UMAC_CMD_SPEED_1000;
    }
    else if (anlpar & (ANAR_100FD | ANAR_100HD))
    {
        *mbps = 100;
        *fullduplex = (anlpar & ANAR_100FD) ? TRUE : FALSE;
        speed = GENET_UMAC_CMD_SPEED_100;
    }
    else if (anlpar & (ANAR_10FD | ANAR_10HD))
    {
        *mbps = 10;
        *fullduplex = (anlpar & ANAR_10FD) ? TRUE : FALSE;
        speed = GENET_UMAC_CMD_SPEED_10;
    }
    else {
        bcmgenet_set_link_down(hw);
        return FALSE;
    }

    /*
     * UniMAC does not follow the PHY on its own. Tell the RGMII block the
     * link is up and hand UniMAC the speed. ID_MODE_DISABLE turns off the
     * internal clock delay and is wanted only for plain "rgmii" - the
     * Pi 4B straps RXID, so there the delay has to stay on.
     */
    val = BCMGENET_Read(hw, GENET_EXT_RGMII_OOB_CTRL);
    val &= ~GENET_EXT_RGMII_OOB_OOB_DISABLE;
    val |= GENET_EXT_RGMII_OOB_RGMII_LINK;
    val |= GENET_EXT_RGMII_OOB_RGMII_MODE_EN;
    if (hw->phyMode == GENET_PHY_MODE_RGMII)
        val |= GENET_EXT_RGMII_OOB_ID_MODE_DISABLE;
    else
        val &= ~GENET_EXT_RGMII_OOB_ID_MODE_DISABLE;
    BCMGENET_Write(hw, GENET_EXT_RGMII_OOB_CTRL, val);

    val = BCMGENET_Read(hw, GENET_UMAC_CMD);
    val &= ~GENET_UMAC_CMD_SPEED_MASK;
    val |= (speed << GENET_UMAC_CMD_SPEED_SHIFT) & GENET_UMAC_CMD_SPEED_MASK;
    BCMGENET_Write(hw, GENET_UMAC_CMD, val);

    D(bug("[bcmgenet] link up: %lu Mbit, %s duplex\n", *mbps,
          *fullduplex ? "full" : "half");)

    return TRUE;
}
