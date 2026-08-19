/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2711 GENET Ethernet — Hardware access
    Based on U-Boot drivers/net/bcmgenet.c
    Copyright (C) 2019 Amit Singh Tomar (GPL-2.0+)
    Adapted for AROS SANA-II.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/macros.h>

#include "genet.h"

/* Register access */
static inline ULONG genet_rd(struct GENETUnit *unit, ULONG off)
{
    return AROS_LE2LONG(*(volatile ULONG *)(unit->gn_RegBase + off));
}

static inline void genet_wr(struct GENETUnit *unit, ULONG off, ULONG val)
{
    *(volatile ULONG *)(unit->gn_RegBase + off) = AROS_LONG2LE(val);
}

static inline void genet_set(struct GENETUnit *unit, ULONG off, ULONG bits)
{
    genet_wr(unit, off, genet_rd(unit, off) | bits);
}

static inline void genet_clr(struct GENETUnit *unit, ULONG off, ULONG bits)
{
    genet_wr(unit, off, genet_rd(unit, off) & ~bits);
}

#define udelay genet_udelay

/* ============================================================
 * UMAC Reset
 * ============================================================ */

static void genet_umac_reset(struct GENETUnit *unit)
{
    ULONG reg;

    /* Flush RX */
    reg = genet_rd(unit, SYS_RBUF_FLUSH_CTRL);
    reg |= (1 << 1);
    genet_wr(unit, SYS_RBUF_FLUSH_CTRL, reg);
    udelay(10);
    reg &= ~(1 << 1);
    genet_wr(unit, SYS_RBUF_FLUSH_CTRL, reg);
    udelay(10);
    genet_wr(unit, SYS_RBUF_FLUSH_CTRL, 0);
    udelay(10);

    /* Reset MAC */
    genet_wr(unit, UMAC_CMD, 0);
    genet_wr(unit, UMAC_CMD, CMD_SW_RESET | CMD_LCL_LOOP_EN);
    udelay(2);
    genet_wr(unit, UMAC_CMD, 0);

    /* Clear MIB counters */
    genet_wr(unit, UMAC_MIB_CTRL, MIB_RESET_RX | MIB_RESET_TX | MIB_RESET_RUNT);
    genet_wr(unit, UMAC_MIB_CTRL, 0);

    /* Max frame length */
    genet_wr(unit, UMAC_MAX_FRAME_LEN, ETH_MAXPACKETSIZE);

    /* Enable 2-byte alignment for IP headers */
    genet_set(unit, RBUF_CTRL, RBUF_ALIGN_2B);
    genet_wr(unit, RBUF_TBUF_SIZE_CTRL, 1);
}

/* ============================================================
 * MAC Address
 * ============================================================ */

void genet_HW_SetMAC(struct GENETUnit *unit, UBYTE *addr)
{
    ULONG reg;

    reg = (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3];
    genet_wr(unit, UMAC_MAC0, reg);

    reg = (addr[4] << 8) | addr[5];
    genet_wr(unit, UMAC_MAC1, reg);
}

/* ============================================================
 * DMA Ring Setup
 * ============================================================ */

static void genet_disable_dma(struct GENETUnit *unit)
{
    genet_clr(unit, TDMA_REG_BASE + DMA_CTRL, DMA_EN);
    genet_clr(unit, RDMA_REG_BASE + DMA_CTRL, DMA_EN);
    genet_wr(unit, UMAC_TX_FLUSH, 1);
    udelay(10);
    genet_wr(unit, UMAC_TX_FLUSH, 0);
}

static void genet_enable_dma(struct GENETUnit *unit)
{
    ULONG dma_ctrl = (1 << (DEFAULT_Q + DMA_RING_BUF_EN_SHIFT)) | DMA_EN;

    genet_wr(unit, TDMA_REG_BASE + DMA_CTRL, dma_ctrl);
    genet_set(unit, RDMA_REG_BASE + DMA_CTRL, dma_ctrl);
}

static void genet_rx_descs_init(struct GENETUnit *unit)
{
    ULONG i;
    ULONG len_stat = (RX_BUF_LENGTH << DMA_BUFLENGTH_SHIFT) | DMA_OWN;

    for (i = 0; i < RX_DESCS; i++) {
        IPTR buf = (IPTR)unit->gn_RxBuffer + i * RX_BUF_LENGTH;
        IPTR desc = unit->gn_RxDescBase + i * DMA_DESC_SIZE;

        *(volatile ULONG *)(desc + DMA_DESC_ADDRESS_LO) = AROS_LONG2LE((ULONG)(buf & 0xFFFFFFFF));
        *(volatile ULONG *)(desc + DMA_DESC_ADDRESS_HI) = AROS_LONG2LE((ULONG)(buf >> 32));
        *(volatile ULONG *)(desc + DMA_DESC_LENGTH_STATUS) = AROS_LONG2LE(len_stat);
    }
}

static void genet_rx_ring_init(struct GENETUnit *unit)
{
    genet_wr(unit, RDMA_REG_BASE + DMA_SCB_BURST_SIZE, DMA_MAX_BURST_LENGTH);
    genet_wr(unit, RDMA_RING_REG_BASE + DMA_START_ADDR, 0);
    genet_wr(unit, RDMA_READ_PTR, 0);
    genet_wr(unit, RDMA_WRITE_PTR, 0);
    genet_wr(unit, RDMA_RING_REG_BASE + DMA_END_ADDR,
             RX_DESCS * DMA_DESC_SIZE / 4 - 1);

    /* Align consumer index on producer */
    unit->gn_RxConsIdx = genet_rd(unit, RDMA_PROD_INDEX);
    genet_wr(unit, RDMA_CONS_INDEX, unit->gn_RxConsIdx);
    unit->gn_RxIdx = unit->gn_RxConsIdx & 0xFF;

    genet_wr(unit, RDMA_RING_REG_BASE + DMA_RING_BUF_SIZE,
             (RX_DESCS << DMA_RING_SIZE_SHIFT) | RX_BUF_LENGTH);
    genet_wr(unit, RDMA_XON_XOFF_THRESH,
             (DMA_FC_THRESH_LO << 16) | DMA_FC_THRESH_HI);
    genet_wr(unit, RDMA_REG_BASE + DMA_RING_CFG, 1 << DEFAULT_Q);
}

static void genet_tx_ring_init(struct GENETUnit *unit)
{
    genet_wr(unit, TDMA_REG_BASE + DMA_SCB_BURST_SIZE, DMA_MAX_BURST_LENGTH);
    genet_wr(unit, TDMA_RING_REG_BASE + DMA_START_ADDR, 0);
    genet_wr(unit, TDMA_READ_PTR, 0);
    genet_wr(unit, TDMA_WRITE_PTR, 0);
    genet_wr(unit, TDMA_RING_REG_BASE + DMA_END_ADDR,
             TX_DESCS * DMA_DESC_SIZE / 4 - 1);

    /* Align producer index on consumer */
    unit->gn_TxProdIdx = genet_rd(unit, TDMA_CONS_INDEX);
    genet_wr(unit, TDMA_PROD_INDEX, unit->gn_TxProdIdx);
    unit->gn_TxConsIdx = unit->gn_TxProdIdx;

    genet_wr(unit, TDMA_RING_REG_BASE + DMA_MBUF_DONE_THRESH, 1);
    genet_wr(unit, TDMA_FLOW_PERIOD, 0);
    genet_wr(unit, TDMA_RING_REG_BASE + DMA_RING_BUF_SIZE,
             (TX_DESCS << DMA_RING_SIZE_SHIFT) | RX_BUF_LENGTH);
    genet_wr(unit, TDMA_REG_BASE + DMA_RING_CFG, 1 << DEFAULT_Q);
}

/* ============================================================
 * Public API
 * ============================================================ */

void genet_HW_Init(struct GENETUnit *unit)
{
    unit->gn_TxDescBase = unit->gn_RegBase + GENET_TX_OFF;
    unit->gn_RxDescBase = unit->gn_RegBase + GENET_RX_OFF;

    /* Set port mode to external GPHY (RGMII) */
    genet_wr(unit, SYS_PORT_CTRL, PORT_MODE_EXT_GPHY);

    genet_umac_reset(unit);
    genet_HW_SetMAC(unit, unit->gn_DevAddr);

    genet_disable_dma(unit);

    genet_rx_ring_init(unit);
    genet_rx_descs_init(unit);
    genet_tx_ring_init(unit);

    genet_enable_dma(unit);

    /* Configure RGMII interface */
    genet_clr(unit, EXT_RGMII_OOB_CTRL, OOB_DISABLE);
    genet_set(unit, EXT_RGMII_OOB_CTRL, RGMII_LINK | RGMII_MODE_EN);

    /* Set speed (default 1000Mbps, updated after PHY negotiation) */
    genet_wr(unit, UMAC_CMD, UMAC_SPEED_1000 << CMD_SPEED_SHIFT);

    /* Enable TX and RX */
    genet_set(unit, UMAC_CMD, CMD_TX_EN | CMD_RX_EN);
}

void genet_HW_Stop(struct GENETUnit *unit)
{
    /* Disable TX/RX */
    genet_clr(unit, UMAC_CMD, CMD_TX_EN | CMD_RX_EN);
    genet_disable_dma(unit);
}

int genet_HW_Send(struct GENETUnit *unit, UBYTE *data, ULONG length)
{
    ULONG tx_idx = unit->gn_TxProdIdx & 0xFF;
    IPTR desc = unit->gn_TxDescBase + tx_idx * DMA_DESC_SIZE;
    ULONG len_stat;
    ULONG cons;
    int tries = 1000;

    /* Write descriptor */
    *(volatile ULONG *)(desc + DMA_DESC_ADDRESS_LO) = AROS_LONG2LE((ULONG)((IPTR)data & 0xFFFFFFFF));
    *(volatile ULONG *)(desc + DMA_DESC_ADDRESS_HI) = AROS_LONG2LE((ULONG)((IPTR)data >> 32));

    len_stat = (length << DMA_BUFLENGTH_SHIFT) |
               (0x3F << DMA_TX_QTAG_SHIFT) |
               DMA_TX_APPEND_CRC | DMA_SOP | DMA_EOP;
    *(volatile ULONG *)(desc + DMA_DESC_LENGTH_STATUS) = AROS_LONG2LE(len_stat);

    /* Advance producer index */
    unit->gn_TxProdIdx++;
    genet_wr(unit, TDMA_PROD_INDEX, unit->gn_TxProdIdx);

    /* Wait for completion */
    do {
        cons = genet_rd(unit, TDMA_CONS_INDEX) & 0xFFFF;
    } while (cons < unit->gn_TxProdIdx && --tries);

    if (!tries)
        return -1;

    return 0;
}
