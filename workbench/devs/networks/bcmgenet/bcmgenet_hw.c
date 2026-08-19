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

/*
 * On real aarch64 Raspberry Pi hardware (not just under QEMU, which
 * hides the difference) a plain volatile access to Device-mapped MMIO
 * still needs an explicit barrier around it, or accesses can be
 * reordered past whatever the caller does next. See RPiHDMI/RPiPWM's
 * rd32le()/wr32le() for the precedent this mirrors.
 */
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

/*
 * Write PMD/REG/READ into GENET_MDIO_CMD | GENET_MDIO_START_BUSY,
 * poll START_BUSY clear, return the low 16 bits.
 */
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

/*
 * Same as above with GENET_MDIO_WRITE and the value to write in the low 16 bits.
 */
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

/*
 * TODO: port genet_reset() (bcmgenet.c:469). UMAC_CMD_SW_RESET, flush
 * ctrl on RX/TX buffers, MIB reset, wait for the bits to self-clear.
 */

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

/*
 * TODO: port genet_init_rings() + genet_setup_dma() (bcmgenet.c:508,
 * 887) for the default queue (GENET_DMA_DEFAULT_QUEUE): ring buffer
 * size/desc count, start/end address, enable RX_DMA_CTRL / TX_DMA_CTRL.
 * Called once from BCMGENET_CreateUnit() after the rings are allocated.
 */
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

/*
 * Port of genet_setup_rxfilter_mdf() (bcmgenet.c:379). The exact-match
 * filter splits an address the opposite way from UMAC_MAC0/MAC1: the
 * first two bytes go in ADDR0, the last four in ADDR1.
 */
static void bcmgenet_setup_rxfilter_mdf(struct bcmgenet_hw *hw, ULONG n,
                                        const UBYTE *addr)
{
    BCMGENET_Write(hw, GENET_UMAC_MDF_ADDR0(n),
                   ((ULONG)addr[0] << 8) | addr[1]);
    BCMGENET_Write(hw, GENET_UMAC_MDF_ADDR1(n),
                   ((ULONG)addr[2] << 24) | ((ULONG)addr[3] << 16) |
                   ((ULONG)addr[4] << 8) | addr[5]);
}

/*
 * Port of the station-address half of genet_init() (bcmgenet.c:601) plus
 * the two fixed filter entries genet_setup_rxfilter() (bcmgenet.c:389)
 * always programs: broadcast in entry 0, our own address in entry 1.
 * Multicast groups start at entry 2 and are not handled here - see the
 * note in BCMGENET_AddMulticastRange().
 */
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

/*
 * Port of genet_lladdr_read() (bcmgenet.c:996): the bootloader/firmware
 * programs the station address into UMAC_MAC0/MAC1 before the OS ever
 * runs, so reading it back is normally enough - but do this *before*
 * BCMGENET_HWReset(), which may clear it. Prefer hw->macAddr from the
 * device tree ("local-mac-address") when BCMGENET_Discover() found one.
 */
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

/*
 * TODO: port the mii_attach()/genet_mii_statchg() side of genet_attach()
 * (bcmgenet.c:922, 183) without the generic mii(4) layer: probe
 * hw->phyAddr with MII_PHYSID1/2, then bring it up with BMCR_RESET and
 * BMCR_ANENABLE|BMCR_ANRESTART.
 */
BOOL BCMGENET_PHYInit(struct bcmgenet_hw *hw)
{
    return FALSE;
}

/*
 * TODO: port genet_update_link() (bcmgenet.c:152). Read BMSR for
 * link/autoneg-done, GBSR/ANLPAR for the negotiated speed/duplex, and
 * push EXT_RGMII_OOB_CTRL + UMAC_CMD's speed/duplex bits to match -
 * the MAC does not follow the PHY automatically.
 */
BOOL BCMGENET_PHYGetLink(struct bcmgenet_hw *hw, ULONG *mbps, BOOL *fullduplex)
{
    *mbps = 0;
    *fullduplex = FALSE;

    return FALSE;
}
