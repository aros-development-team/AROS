/*-
 * BCM2835 SDHOST controller bus implementation.
 * Derived from NetBSD's sys/arch/arm/broadcom/bcm2835_sdhost.c.
 *
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * AROS port additions:
 *     Copyright (C) 2026, The AROS Development Team.
 */

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dma.h>
#include <proto/utility.h>

#include <hardware/mmc.h>
#include <hardware/sdhc.h>
#include <hardware/bcm2708.h>

#include "sdcard_sdhost_intern.h"
#include "sdcard_bus.h"
#include "sdcard_unit.h"
#include "timer.h"

/*
 * Stub for FNAME_SDCBUS(GetClockDiv) — referenced by the SDHCI SetClock
 * code in sdcard_bus.c which is linked but unused when SDHOST is active.
 */
ULONG FNAME_SDCBUS(GetClockDiv)(ULONG speed, struct sdcard_Bus *bus)
{
    (void)speed; (void)bus;
    return 0;
}

static inline void sdhost_dsb(void) { asm volatile("dsb sy" ::: "memory"); }

/* Translate a virtual address to its bus-addressable form for the DMA
 * engine.  On Pi 3 the kernel identity-maps low memory and adds an
 * offset for the high kernel mapping; KrnVirtualToPhysical handles
 * both.  The 0xC0000000 alias selects the uncached view of SDRAM. */
static inline ULONG sdhost_bus_addr(struct sdcard_Bus *bus, APTR virt)
{
    struct SDCardBase *SDCardBase = bus->sdcb_DeviceBase;
    return BCM2708_DMA_BUS_ADDR((ULONG)(IPTR)KrnVirtualToPhysical(virt));
}

/* Fast bounce-buffer copy: 64 bytes/iteration via NEON for the bulk,
 * then a word-aligned tail.  Both src and dst are always at least
 * 4-byte aligned (they come from ULONG pointers), which is the only
 * alignment vldm/vstm strictly require. */
static inline void sdhost_neon_copy(void *dst, const void *src, ULONG bytes)
{
#if defined(__arm__)
    ULONG chunks = bytes >> 6;          /* 64-byte chunks */
    ULONG tail   = (bytes & 0x3f) >> 2; /* trailing whole words */
    ULONG *wsrc, *wdst;

    if (chunks)
    {
        asm volatile (
            "1: vldm %1!, {q0-q3}    \n"
            "   vstm %0!, {q0-q3}    \n"
            "   subs %2, %2, #1      \n"
            "   bne 1b               \n"
            : "+r" (dst), "+r" (src), "+r" (chunks)
            :
            : "memory", "q0", "q1", "q2", "q3", "cc"
        );
    }

    wsrc = (ULONG *)src;
    wdst = (ULONG *)dst;
    while (tail--)
        *wdst++ = *wsrc++;
#else
    /* Non-ARM: plain word copy (clang auto-vectorises). Covers bytes
     * rounded down to a whole word, matching the ARM path. */
    ULONG *wsrc = (ULONG *)src, *wdst = (ULONG *)dst;
    ULONG words = bytes >> 2;
    while (words--)
        *wdst++ = *wsrc++;
#endif
}

/* ---------------------------------------------------------------------- */
/* Wait for SDCMD_NEW to clear (NetBSD sdhost_wait_idle).                  */
/* ---------------------------------------------------------------------- */

static int sdhost_wait_cmd_idle(struct sdcard_Bus *bus, int timeout_us)
{
    /* Wait for the controller to clear SDCMD_NEW */
    while (timeout_us-- > 0)
    {
        if ((sdhost_read(bus, SDCMD) & SDCMD_NEW) == 0)
            return 0;
        sdcard_Udelay(1);
    }
    return -1;
}

/* Drain any residual FIFO data left over from a previous command. */
static void sdhost_flush_fifo(struct sdcard_Bus *bus)
{
    ULONG edm = sdhost_read(bus, SDEDM);
    ULONG level = (edm & SDEDM_FIFO_LEVEL_MASK) >> SDEDM_FIFO_LEVEL_SHIFT;
    int   guard = 32;

    while (level > 0 && guard-- > 0)
    {
        (void)sdhost_read(bus, SDDATA);
        edm = sdhost_read(bus, SDEDM);
        level = (edm & SDEDM_FIFO_LEVEL_MASK) >> SDEDM_FIFO_LEVEL_SHIFT;
    }
}

/* Write-1-to-clear any sticky error bits before issuing a new command. */
static void sdhost_clear_errors(struct sdcard_Bus *bus)
{
    ULONG hsts = sdhost_read(bus, SDHSTS);
    if (hsts & SDHSTS_ERROR_MASK)
        sdhost_write(bus, SDHSTS, hsts & SDHSTS_ERROR_MASK);
}

/* ---------------------------------------------------------------------- */
/* DMA: control-block setup, kick, wait.                                   */
/* ---------------------------------------------------------------------- */

static void sdhost_dma_setup(struct sdcard_Bus *bus, APTR buf,
                              ULONG len, BOOL is_read)
{
    struct sdhost_private *priv = SDHOST_PRIV(bus);
    struct BCM2708DMACB *cb = priv->dma_cb;
    ULONG buf_bus = sdhost_bus_addr(bus, buf);
    ULONG ti;
    ULONG i;

    /* dma.resource hands out a "lite" DMA channel — 32-bit transfers
     * only.  Lite channels do not support the 128-bit WIDTH bit, so use
     * NO_WIDE_BURSTS and a 4-beat burst length instead.  WAIT_RESP is
     * set for both directions to keep DMA in step with peripheral DREQ
     * pacing. */
    if (is_read)
    {
        ti = DMA_TI_INTEN |
             DMA_TI_NO_WIDE_BURSTS |
             DMA_TI_WAIT_RESP |
             DMA_TI_PERMAP(SDHOST_DMA_DREQ) |
             DMA_TI_SRC_DREQ |
             DMA_TI_DEST_INC |
             DMA_TI_BURST_LENGTH(4);
        cb->source_ad = AROS_LONG2LE(SDHOST_SDDATA_DMA_ADDR);
        cb->dest_ad   = AROS_LONG2LE(buf_bus);
    }
    else
    {
        ti = DMA_TI_INTEN |
             DMA_TI_NO_WIDE_BURSTS |
             DMA_TI_WAIT_RESP |
             DMA_TI_PERMAP(SDHOST_DMA_DREQ) |
             DMA_TI_DEST_DREQ |
             DMA_TI_SRC_INC |
             DMA_TI_BURST_LENGTH(4);
        cb->source_ad = AROS_LONG2LE(buf_bus);
        cb->dest_ad   = AROS_LONG2LE(SDHOST_SDDATA_DMA_ADDR);
    }

    cb->ti        = AROS_LONG2LE(ti);
    cb->txfr_len  = AROS_LONG2LE(len);
    cb->stride    = 0;
    cb->nextconbk = 0;

    /* Clean+invalidate the data buffer before DMA in both directions.
     *  - Writes: flushes dirty CPU writes to RAM so DMA reads fresh data.
     *  - Reads:  evicts any dirty lines so they cannot be written back
     *            over data DMA has just placed in RAM, and invalidates
     *            stale clean lines.
     * Without the read-side pre-flush, direct DMA into a previously
     * dirtied user buffer corrupts the result. */
    CacheClearE(buf, len, CACRF_ClearD);
    /* Flush the control block to RAM. */
    CacheClearE(cb, sizeof(*cb), CACRF_ClearD);
    sdhost_dsb();

    /* Reset DMA channel; loading a fresh CB after reset is the safest
     * way to clear any residual ACTIVE/INT/END state. */
    *(volatile ULONG *)DMA_CS(priv->dma_channel) = AROS_LONG2LE(DMA_CS_RESET);
    sdhost_dsb();
    for (i = 0; i < 100; i++)
    {
        if (!(AROS_LE2LONG(*(volatile ULONG *)DMA_CS(priv->dma_channel)) & DMA_CS_RESET))
            break;
    }
    *(volatile ULONG *)DMA_CS(priv->dma_channel) =
        AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
    *(volatile ULONG *)DMA_CONBLK_AD(priv->dma_channel) =
        AROS_LONG2LE(BCM2708_DMA_BUS_ADDR((ULONG)cb));
    sdhost_dsb();
}

static inline void sdhost_dma_kick(struct sdcard_Bus *bus)
{
    struct sdhost_private *priv = SDHOST_PRIV(bus);

    *(volatile ULONG *)DMA_CS(priv->dma_channel) = AROS_LONG2LE(
        DMA_CS_WAIT_FOR_WRITES |
        DMA_CS_PANIC_PRI(15) |
        DMA_CS_PRI(8) |
        DMA_CS_ACTIVE);
    priv->dma_active = TRUE;
}

static int sdhost_dma_wait(struct sdcard_Bus *bus, ULONG timeout_us)
{
    /* Deliberately a bounded poll, NOT DMAWaitChannel: sleeping here
     * lets other tasks run mid-transfer, which both (a) speculatively
     * refetches destination cache lines during direct-to-buffer reads
     * and (b) lets shared edge lines go dirty, so the post-DMA ClearD
     * writes stale data over the DMA result — random disk corruption.
     * The tight poll keeps the window closed, as it always did. */
    struct sdhost_private *priv = SDHOST_PRIV(bus);
    volatile ULONG *dma_cs = (volatile ULONG *)DMA_CS(priv->dma_channel);
    const ULONG poll_step = 50;

    while (timeout_us > 0)
    {
        ULONG cs = AROS_LE2LONG(*dma_cs);
        if (cs & DMA_CS_END)
        {
            *dma_cs = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
            sdhost_dsb();
            return 0;
        }
        if (!(cs & DMA_CS_ACTIVE))
        {
            bug("[SDHost%02u] DMA stopped early: CS=%08x SDHSTS=%08x\n",
                bus->sdcb_BusNum, cs, sdhost_read(bus, SDHSTS));
            *dma_cs = AROS_LONG2LE(DMA_CS_RESET);
            sdhost_dsb();
            return -1;
        }
        sdcard_Udelay(poll_step);
        timeout_us = (timeout_us > poll_step) ? timeout_us - poll_step : 0;
    }
    bug("[SDHost%02u] DMA timeout: SDHSTS=%08x\n",
        bus->sdcb_BusNum, sdhost_read(bus, SDHSTS));
    *dma_cs = AROS_LONG2LE(DMA_CS_RESET);
    sdhost_dsb();
    return -1;
}


/* ---------------------------------------------------------------------- */
/* SoftReset — bring registers to a known state and power up.              */
/* Mirrors NetBSD's sdhost_host_reset.                                     */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOSTBUS(SoftReset)(UBYTE mask, struct sdcard_Bus *bus)
{
    struct sdhost_private *priv = SDHOST_PRIV(bus);
    ULONG edm;
    (void)mask;

    sdhost_write(bus, SDVDD,  0);
    sdhost_write(bus, SDCMD,  0);
    sdhost_write(bus, SDARG,  0);
    sdhost_write(bus, SDTOUT, SDTOUT_DEFAULT);
    sdhost_write(bus, SDCDIV, 0);
    sdhost_write(bus, SDHSTS, sdhost_read(bus, SDHSTS));
    sdhost_write(bus, SDHCFG, 0);
    sdhost_write(bus, SDHBCT, 0);
    sdhost_write(bus, SDHBLC, 0);

    edm = sdhost_read(bus, SDEDM);
    edm &= ~(SDEDM_RD_FIFO_MASK | SDEDM_WR_FIFO_MASK);
    edm |= (4U << SDEDM_RD_FIFO_SHIFT) | (4U << SDEDM_WR_FIFO_SHIFT);
    sdhost_write(bus, SDEDM, edm);

    sdcard_Udelay(20000);
    sdhost_write(bus, SDVDD, SDVDD_POWER);
    sdcard_Udelay(20000);

    /* Leave BUSY_EN enabled permanently — keeps the controller in a
     * consistent state across commands and removes the need to massage
     * SDHCFG inside SendCmd.  SLOW + WIDE_INT are required for proper
     * 4-bit FIFO datapath; SetBusWidth ORs in WIDE_EXT when needed. */
    priv->hcfg = SDHCFG_BUSY_EN | SDHCFG_SLOW | SDHCFG_WIDE_INT;
    sdhost_write(bus, SDHCFG, priv->hcfg);
    sdhost_write(bus, SDCDIV, SDCDIV_MASK);

    priv->cdiv = SDCDIV_MASK;
}

/* ---------------------------------------------------------------------- */
/* SetClock — recompute SDCDIV from core clock and requested speed.        */
/* SDHOST stores divider minus two; actual_clock = core / (SDCDIV + 2).    */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOSTBUS(SetClock)(ULONG speed, struct sdcard_Bus *bus)
{
    struct sdhost_private *priv = SDHOST_PRIV(bus);
    ULONG div;

    if (speed == 0)
    {
        div = SDCDIV_MASK;
    }
    else
    {
        div = priv->max_clk / speed;
        if (div < 2)
            div = 2;
        if ((priv->max_clk / div) > speed)
            div++;
        div -= 2;
        if (div > SDCDIV_MASK)
            div = SDCDIV_MASK;
    }
    sdhost_write(bus, SDCDIV, div);
    priv->cdiv = div;
}

/* ---------------------------------------------------------------------- */
/* SetPowerLevel — SDHOST only exposes on/off through SDVDD.               */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOSTBUS(SetPowerLevel)(ULONG supportedlvls, BOOL lowest,
                                     struct sdcard_Bus *bus)
{
    (void)supportedlvls;
    (void)lowest;
    sdhost_write(bus, SDVDD, SDVDD_POWER);
}

/* ---------------------------------------------------------------------- */
/* SetBusWidth — toggle SDHCFG_WIDE_EXT, always keep WIDE_INT | SLOW set.  */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOSTBUS(SetBusWidth)(UBYTE width, struct sdcard_Bus *bus)
{
    struct sdhost_private *priv = SDHOST_PRIV(bus);

    if (width == 4)
        priv->hcfg |= SDHCFG_WIDE_EXT;
    else
        priv->hcfg &= ~SDHCFG_WIDE_EXT;

    sdhost_write(bus, SDHCFG, priv->hcfg);
}

/* ---------------------------------------------------------------------- */
/* SendCmd — issue one command (with optional data transfer) and wait      */
/* for it to complete.  Synchronous, mirrors NetBSD's sdhost_exec_command. */
/* The full transaction happens here; WaitCmd / FinishCmd / FinishData     */
/* become trivial stubs below.                                             */
/* ---------------------------------------------------------------------- */

ULONG FNAME_SDHOSTBUS(SendCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    struct SDCardBase *SDCardBase = bus->sdcb_DeviceBase;
    struct sdhost_private *priv = SDHOST_PRIV(bus);
    ULONG  sdCommand   = (ULONG)GetTagData(SDCARD_TAG_CMD,      0, CmdTags);
    ULONG  sdArgument  = (ULONG)GetTagData(SDCARD_TAG_ARG,      0, CmdTags);
    ULONG  sdRspType   = (ULONG)GetTagData(SDCARD_TAG_RSPTYPE,  MMC_RSP_NONE, CmdTags);
    APTR   sdData      = (APTR)(IPTR)GetTagData(SDCARD_TAG_DATA, 0, CmdTags);
    ULONG  sdDataLen   = (ULONG)GetTagData(SDCARD_TAG_DATALEN,  0, CmdTags);
    ULONG  sdDataFlags = (ULONG)GetTagData(SDCARD_TAG_DATAFLAGS, 0, CmdTags);
    struct TagItem *RspTag = FindTagItem(SDCARD_TAG_RSP, CmdTags);
    BOOL   is_read = (sdDataFlags & MMC_DATA_READ) != 0;
    BOOL   has_data = (sdDataLen != 0);
    BOOL   used_bounce = FALSE;
    APTR   dma_buf;
    ULONG  cmdval;
    ULONG  retval = 0;

    DFUNCS(bug("[SDHost%02u] %s: CMD%u arg=%08x rsptype=%x len=%u flags=%x\n",
        bus->sdcb_BusNum, __PRETTY_FUNCTION__,
        sdCommand, sdArgument, sdRspType, sdDataLen, sdDataFlags));

    if (sdhost_wait_cmd_idle(bus, 250000) != 0)
    {
        D(bug("[SDHost%02u] %s: device busy before CMD%u\n",
            bus->sdcb_BusNum, __PRETTY_FUNCTION__, sdCommand));
        return -1;
    }

    /* Drain any leftover data and clear sticky error bits from a previous
     * command before driving SDHBCT/SDHBLC + DMA setup. */
    sdhost_flush_fifo(bus);
    sdhost_clear_errors(bus);

    cmdval = SDCMD_NEW;
    if (!(sdRspType & MMC_RSP_PRESENT))
        cmdval |= SDCMD_NORESP;
    if (sdRspType & MMC_RSP_136)
        cmdval |= SDCMD_LONGRESP;
    if (sdRspType & MMC_RSP_BUSY)
        cmdval |= SDCMD_BUSY;

    /* Program data transfer ahead of issuing the command. */
    if (has_data)
    {
        ULONG sectorSize = (1U << bus->sdcb_SectorShift);
        /* For small transfers (SCR = 8 B, SSR = 64 B, …) the controller must
         * be told the actual transfer length, not the sector size — otherwise
         * its FSM hangs in READDATA waiting for sectorSize bytes the card
         * never sends. */
        ULONG blklen = (sdDataLen > sectorSize) ? sectorSize : sdDataLen;
        ULONG nblks  = (sdDataLen + blklen - 1) / blklen;

        cmdval |= is_read ? SDCMD_READ : SDCMD_WRITE;
        sdhost_write(bus, SDHBCT, blklen);
        sdhost_write(bus, SDHBLC, nblks);

        /* Diagnostic: always route through the bounce buffer so every
         * transfer uses one well-aligned, known-good DMA address. Direct
         * DMA is only used as a fallback if the bounce buffer is too
         * small or unavailable. */
        {
            if (priv->dma_bounce && sdDataLen <= priv->dma_bounce_size)
            {
                dma_buf = priv->dma_bounce;
                used_bounce = TRUE;
                if (!is_read)
                    sdhost_neon_copy(priv->dma_bounce, sdData, sdDataLen);
            }
            else
            {
                bug("[SDHost%02u] CMD%u %s len=%u: no DMA path available\n",
                    bus->sdcb_BusNum, sdCommand,
                    is_read ? "read" : "write", sdDataLen);
                retval = -1;
                goto done;
            }
        }

        sdhost_dma_setup(bus, dma_buf, sdDataLen, is_read);

        priv->dma_data_addr = (ULONG)sdData;
        priv->dma_data_len  = sdDataLen;
        priv->dma_xfer_len  = sdDataLen;
        priv->xfer_is_dma   = TRUE;
        priv->xfer_active   = TRUE;
    }

    /* Issue the command.  For data transfers, the DMA engine must be
     * armed immediately after SDCMD with nothing in between: the card
     * begins streaming into the FIFO as soon as it processes the
     * command, and the FIFO overflows after ~2.5 µs at 50 MHz 4-bit. */
    sdhost_write(bus, SDARG, sdArgument);
    sdhost_write(bus, SDCMD, cmdval | (sdCommand & 0x3f));
    if (has_data)
        sdhost_dma_kick(bus);

    /* Wait for the DMA engine to drain the FIFO. */
    if (has_data)
    {
        if (sdhost_dma_wait(bus, 1000000) != 0)
        {
            retval = -1;
            goto done;
        }

        if (is_read)
        {
            APTR dma_dest = used_bounce ? priv->dma_bounce
                                        : (APTR)priv->dma_data_addr;
            CacheClearE(dma_dest, sdDataLen, CACRF_ClearD);
            if (used_bounce)
                sdhost_neon_copy((APTR)priv->dma_data_addr,
                                  priv->dma_bounce, sdDataLen);
        }

        priv->xfer_active = FALSE;
    }

    /* Wait for the command engine to settle. */
    if (sdhost_wait_cmd_idle(bus, 250000) != 0)
    {
        D(bug("[SDHost%02u] %s: cmd idle wait failed (SDCMD=%04x)\n",
            bus->sdcb_BusNum, __PRETTY_FUNCTION__,
            sdhost_read(bus, SDCMD)));
    }

    if (sdhost_read(bus, SDCMD) & SDCMD_FAIL)
    {
        bug("[SDHost%02u] CMD%u FAIL: SDCMD=%04x SDHSTS=%08x SDEDM=%08x\n",
            bus->sdcb_BusNum, sdCommand,
            sdhost_read(bus, SDCMD), sdhost_read(bus, SDHSTS),
            sdhost_read(bus, SDEDM));
        retval = -1;
        goto done;
    }

    /* Read response into the supplied tag.  136-bit responses get the
     * register order reversed so the resulting array is MSB-first (rsp[0]
     * holds bits 127..96), matching what FNAME_SDCBUS(Rsp136Unpack) and
     * the rest of sdcard.device expect. */
    if ((sdRspType & MMC_RSP_PRESENT) && RspTag != NULL)
    {
        if (sdRspType & MMC_RSP_136)
        {
            if (RspTag->ti_Data)
            {
                ULONG *rsp = (ULONG *)RspTag->ti_Data;
                rsp[0] = sdhost_read(bus, SDRSP3);
                rsp[1] = sdhost_read(bus, SDRSP2);
                rsp[2] = sdhost_read(bus, SDRSP1);
                rsp[3] = sdhost_read(bus, SDRSP0);
            }
        }
        else
        {
            RspTag->ti_Data = sdhost_read(bus, SDRSP0);
        }
    }

done:
    /* On data-transfer errors leave the DMA channel and FIFO in a known
     * good state so the higher-level CMD12 cleanup that follows can
     * issue cleanly. */
    if (has_data && retval != 0)
    {
        *(volatile ULONG *)DMA_CS(SDHOST_PRIV(bus)->dma_channel) = AROS_LONG2LE(DMA_CS_RESET);
        sdhost_dsb();
        sdhost_flush_fifo(bus);
    }
    /* W1C the sticky status bits so the next command sees a clean slate. */
    sdhost_write(bus, SDHSTS, sdhost_read(bus, SDHSTS));
    return retval;
}

/* ---------------------------------------------------------------------- */
/* WaitCmd / FinishCmd / FinishData — no-ops; SendCmd is synchronous.      */
/* ---------------------------------------------------------------------- */

ULONG FNAME_SDHOSTBUS(WaitCmd)(ULONG mask, ULONG timeout, struct sdcard_Bus *bus)
{
    (void)mask; (void)timeout; (void)bus;
    return 0;
}

ULONG FNAME_SDHOSTBUS(FinishCmd)(struct TagItem *CmdTags, struct sdcard_Bus *bus)
{
    (void)CmdTags; (void)bus;
    return 0;
}

ULONG FNAME_SDHOSTBUS(FinishData)(struct TagItem *DataTags, struct sdcard_Bus *bus)
{
    (void)DataTags; (void)bus;
    return 0;
}

/* ---------------------------------------------------------------------- */
/* IRQ handler — just clear SDHSTS so future polls see a fresh value.      */
/* SendCmd is fully synchronous (polls in foreground), so no signalling    */
/* is required here.                                                       */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOSTBUS(BusIRQ)(struct sdcard_Bus *bus, void *unused)
{
    ULONG hsts;
    (void)unused;

    hsts = sdhost_read(bus, SDHSTS);
    if (hsts != 0)
        sdhost_write(bus, SDHSTS, hsts);
}

/* ---------------------------------------------------------------------- */
/* BusInit — scan-time init, called from sdcard_init.c::Scan().            */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOST(BusInit)(struct sdcard_Bus *bus)
{
    D(bug("[SDHost%02u] %s()\n", bus->sdcb_BusNum, __PRETTY_FUNCTION__));

    FNAME_SDHOSTBUS(SoftReset)(0, bus);
    FNAME_SDHOSTBUS(SetClock)(bus->sdcb_ClockMin, bus);
    FNAME_SDHOSTBUS(SetPowerLevel)(bus->sdcb_Power, FALSE, bus);
    FNAME_SDHOSTBUS(SetBusWidth)(1, bus);
}

/* ---------------------------------------------------------------------- */
/* BusPostIRQInit — register the unit.  SDHOST has no card-detect line,   */
/* so we always assume the card is present.                                */
/* ---------------------------------------------------------------------- */

void FNAME_SDHOST(BusPostIRQInit)(struct sdcard_Bus *bus)
{
    D(bug("[SDHost%02u] %s()\n", bus->sdcb_BusNum, __PRETTY_FUNCTION__));
    FNAME_SDCBUS(RegisterUnit)(bus);
}
