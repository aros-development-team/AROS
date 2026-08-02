#define DEBUG 1
/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Running a transaction on a DesignWare i2c controller.

    The controller is a master that speaks i2c by itself: it is handed a
    target address and a queue of read/write commands, and it produces
    the START, the address byte, the acknowledge checking and the STOP.
    Nothing here toggles a wire.

    Everything is polled. That costs a busy wait, but it needs neither
    an interrupt controller nor timer.device, so a client can use the
    bus at any point in the boot - which is the whole reason a driver
    like this exists in a kickstart module. Transactions to the kind of
    chip found on an i2c bus are a few bytes long, so the wait is short.

    No locking is done here; the caller holds the controller's
    semaphore for the whole transaction.
*/

#include <aros/debug.h>

#include "dwi2c_intern.h"

#define DXFER(x)

static inline ULONG dwi2c_rd(struct dwi2c_ctrl *ctrl, ULONG reg)
{
    return *(volatile ULONG *)(ctrl->base + reg);
}

static inline void dwi2c_wr(struct dwi2c_ctrl *ctrl, ULONG reg, ULONG val)
{
    *(volatile ULONG *)(ctrl->base + reg) = val;
}

/*
 * The FIFOs are a synthesis-time choice, and IC_COMP_PARAM_1 reports
 * what was chosen: one less than the depth, in two bytes. Knowing the
 * receive depth matters because a read is queued as a command in the
 * transmit FIFO but arrives in the receive one, so queueing more reads
 * than the receive FIFO can hold overruns it.
 */
void DWI2C_HWProbeFIFO(struct dwi2c_ctrl *ctrl)
{
    ULONG param = dwi2c_rd(ctrl, IC_COMP_PARAM_1);

    ctrl->rxDepth = ((param >> 8) & 0xff) + 1;
    ctrl->txDepth = ((param >> 16) & 0xff) + 1;

    /* Not every part implements the register - it reads as zero, which
       would leave a depth of one and work, but very slowly */
    if (param == 0)
    {
        ctrl->rxDepth = DWI2C_DEFAULT_FIFO;
        ctrl->txDepth = DWI2C_DEFAULT_FIFO;
    }
}

/*
 * How many input clock cycles a stretch of SCL lasts.
 *
 * The core adds a few cycles of its own to each half of the clock -
 * three to the high period, one to the low - so those come off the
 * count that gets programmed. The fall time is added to the period the
 * bus specification asks for, because the line only counts as low once
 * it has actually got there.
 */
static ULONG dwi2c_hcnt(ULONG clkKHz, ULONG highNs, ULONG fallNs)
{
    UQUAD cycles = ((UQUAD)clkKHz * (highNs + fallNs) + 500000) / 1000000;

    if (cycles < 3 + 6)
        return 6;               /* the smallest the core accepts */

    return (ULONG)(cycles - 3);
}

static ULONG dwi2c_lcnt(ULONG clkKHz, ULONG lowNs, ULONG fallNs)
{
    UQUAD cycles = ((UQUAD)clkKHz * (lowNs + fallNs) + 500000) / 1000000;

    if (cycles < 1 + 8)
        return 8;

    return (ULONG)(cycles - 1);
}

/*
 * Work out the clock shape for both speeds from the rate the block is
 * fed at. Without that rate there is nothing to compute from, and the
 * counts are left at zero so that the transfer path knows to leave
 * whatever the firmware programmed alone.
 *
 * The times are the minimums the i2c specification lays down: 4.0/4.7us
 * high and low at 100kHz, 0.6/1.3us at 400kHz, with 300ns allowed for
 * the line to fall.
 */
void DWI2C_HWCalcTiming(struct dwi2c_ctrl *ctrl)
{
    ULONG khz = ctrl->inputClock / 1000;

    ctrl->ssHcnt = ctrl->ssLcnt = 0;
    ctrl->fsHcnt = ctrl->fsLcnt = 0;
    ctrl->sdaHold = 0;

    if (!khz)
        return;

    ctrl->ssHcnt = dwi2c_hcnt(khz, 4000, 300);
    ctrl->ssLcnt = dwi2c_lcnt(khz, 4700, 300);
    ctrl->fsHcnt = dwi2c_hcnt(khz, 600, 300);
    ctrl->fsLcnt = dwi2c_lcnt(khz, 1300, 300);

    /*
     * How long SDA is held after SCL falls. Getting this wrong is what
     * makes a bus look intermittent, so honour the tree when it says
     * something and leave the register alone when it does not.
     */
    if (ctrl->sdaHoldNs)
    {
        ULONG cycles = (ULONG)(((UQUAD)khz * ctrl->sdaHoldNs + 500000)
                               / 1000000);

        if (!cycles)
            cycles = 1;
        if (cycles > 0xffff)
            cycles = 0xffff;

        ctrl->sdaHold = cycles;
    }

    D(bug("[DWI2C:HW] %s: ss %u/%u fs %u/%u hold %u\n", ctrl->name,
          ctrl->ssHcnt, ctrl->ssLcnt, ctrl->fsHcnt, ctrl->fsLcnt,
          ctrl->sdaHold);)
}

/*
 * Enabling and disabling is a handshake: the write to IC_ENABLE is not
 * in effect until IC_ENABLE_STATUS agrees. Disabling in the middle of a
 * transaction is also the documented way to abandon one, which is what
 * a failed transfer does on its way out.
 */
static BOOL dwi2c_enable(struct dwi2c_ctrl *ctrl, BOOL on)
{
    ULONG want = on ? 1 : 0;
    ULONG i;

    for (i = 0; i < DWI2C_ENABLE_TRIES; i++)
    {
        dwi2c_wr(ctrl, IC_ENABLE, want);

        if ((dwi2c_rd(ctrl, IC_ENABLE_STATUS) & 1) == want)
            return TRUE;
    }

    D(bug("[DWI2C:HW] %s: stuck %s\n", ctrl->name,
          on ? "disabled" : "enabled");)

    return FALSE;
}

/*
 * Did the controller give up on the transaction? Reading
 * IC_CLR_TX_ABRT is what clears the condition and lets the FIFOs be
 * used again, so this both reports and resets.
 *
 * Returns 0 when all is well, otherwise the source register - or a
 * stand-in, since an abort with no reason recorded is still an abort.
 */
static ULONG dwi2c_abort(struct dwi2c_ctrl *ctrl)
{
    ULONG src;

    if (!(dwi2c_rd(ctrl, IC_RAW_INTR_STAT) & IC_INTR_TX_ABRT))
        return 0;

    src = dwi2c_rd(ctrl, IC_TX_ABRT_SOURCE);
    (void)dwi2c_rd(ctrl, IC_CLR_TX_ABRT);

    return src ? src : DWI2C_ABRT_UNKNOWN;
}

/*
 * Point the controller at a slave and switch it on.
 *
 * The target address, the speed and the clock shape can only be changed
 * while the controller is disabled, so every transaction starts from
 * there rather than trying to track what is already programmed.
 */
static BOOL dwi2c_program(struct dwi2c_ctrl *ctrl, UWORD address)
{
    ULONG con;

    if (!dwi2c_enable(ctrl, FALSE))
        return FALSE;

    /*
     * Master only. Leaving the slave side enabled would have the
     * controller answer its own transactions if the target address ever
     * matched the one it was synthesised with.
     */
    con = IC_CON_MASTER | IC_CON_SLAVE_DISABLE | IC_CON_RESTART_EN;

    /*
     * A repeated START is only possible in fast mode or above on some
     * builds, but RESTART_EN above is what actually permits it; the
     * speed here only picks which pair of count registers is used.
     */
    if (ctrl->busSpeed > 100000)
    {
        con |= IC_CON_SPEED_FAST;
        if (ctrl->fsHcnt)
        {
            dwi2c_wr(ctrl, IC_FS_SCL_HCNT, ctrl->fsHcnt);
            dwi2c_wr(ctrl, IC_FS_SCL_LCNT, ctrl->fsLcnt);
        }
    }
    else
    {
        con |= IC_CON_SPEED_STD;
        if (ctrl->ssHcnt)
        {
            dwi2c_wr(ctrl, IC_SS_SCL_HCNT, ctrl->ssHcnt);
            dwi2c_wr(ctrl, IC_SS_SCL_LCNT, ctrl->ssLcnt);
        }
    }

    dwi2c_wr(ctrl, IC_CON, con);

    if (ctrl->sdaHold)
        dwi2c_wr(ctrl, IC_SDA_HOLD, ctrl->sdaHold);

    /* Nothing is interrupt driven here, and a stale mask left by the
       firmware would have the controller raise lines nobody handles */
    dwi2c_wr(ctrl, IC_INTR_MASK, 0);

    /* Thresholds only matter to the interrupt path; keep them at the
       point where the status bits mean "anything at all" */
    dwi2c_wr(ctrl, IC_TX_TL, 0);
    dwi2c_wr(ctrl, IC_RX_TL, 0);

    /* Seven bit addressing: IC_CON is left without 10BITADDR_MASTER, so
       only the low seven bits of IC_TAR are used */
    dwi2c_wr(ctrl, IC_TAR, address & 0x7f);

    if (!dwi2c_enable(ctrl, TRUE))
        return FALSE;

    /* Anything the last transaction left behind is not ours */
    (void)dwi2c_rd(ctrl, IC_CLR_INTR);

    return TRUE;
}

/*
 * Push the write half. The last byte carries the STOP unless a read is
 * to follow, in which case the read half issues a repeated START.
 */
static BOOL dwi2c_write(struct dwi2c_ctrl *ctrl, const UBYTE *buf,
                        ULONG len, BOOL stop)
{
    ULONG i, spins, abrt;

    for (i = 0; i < len; i++)
    {
        ULONG cmd = buf[i];

        if (stop && i == len - 1)
            cmd |= IC_DATA_CMD_STOP;

        for (spins = 0; !(dwi2c_rd(ctrl, IC_STATUS) & IC_STATUS_TFNF);
             spins++)
        {
            if ((abrt = dwi2c_abort(ctrl)) != 0)
            {
                D(bug("[DWI2C:HW] %s: abort %08x writing byte %u\n",
                      ctrl->name, abrt, i);)
                return FALSE;
            }

            if (spins >= DWI2C_SPIN_LIMIT)
            {
                D(bug("[DWI2C:HW] %s: tx FIFO never drained\n", ctrl->name);)
                return FALSE;
            }
        }

        DXFER(bug("[DWI2C:HW] %s: W %02x\n", ctrl->name, buf[i]);)

        dwi2c_wr(ctrl, IC_DATA_CMD, cmd);
    }

    return TRUE;
}

/*
 * Pull the read half.
 *
 * Each byte to be read is a command that has to be queued, and the
 * bytes come back in a separate FIFO, so the two have to be kept in
 * step: queue as much as the receive FIFO can still hold, drain what
 * has arrived, repeat. Doing it in one pass would work only for reads
 * shorter than the FIFO.
 */
static BOOL dwi2c_read(struct dwi2c_ctrl *ctrl, UBYTE *buf, ULONG len,
                       BOOL restart)
{
    ULONG queued = 0, got = 0, spins = 0, abrt;

    while (got < len)
    {
        BOOL progress = FALSE;

        while (queued < len && (queued - got) < ctrl->rxDepth &&
               (dwi2c_rd(ctrl, IC_STATUS) & IC_STATUS_TFNF))
        {
            ULONG cmd = IC_DATA_CMD_READ;

            if (queued == 0 && restart)
                cmd |= IC_DATA_CMD_RESTART;
            if (queued == len - 1)
                cmd |= IC_DATA_CMD_STOP;

            dwi2c_wr(ctrl, IC_DATA_CMD, cmd);
            queued++;
            progress = TRUE;
        }

        while (got < len && dwi2c_rd(ctrl, IC_RXFLR) > 0)
        {
            buf[got] = (UBYTE)(dwi2c_rd(ctrl, IC_DATA_CMD) & IC_DATA_CMD_DAT);
            DXFER(bug("[DWI2C:HW] %s: R %02x\n", ctrl->name, buf[got]);)
            got++;
            progress = TRUE;
        }

        if ((abrt = dwi2c_abort(ctrl)) != 0)
        {
            D(bug("[DWI2C:HW] %s: abort %08x after %u of %u bytes read\n",
                  ctrl->name, abrt, got, len);)
            return FALSE;
        }

        if (progress)
            spins = 0;
        else if (++spins >= DWI2C_SPIN_LIMIT)
        {
            D(bug("[DWI2C:HW] %s: read stalled after %u of %u bytes\n",
                  ctrl->name, got, len);)
            return FALSE;
        }
    }

    return TRUE;
}

/* The STOP has been queued; wait for it to actually go out */
static BOOL dwi2c_waitstop(struct dwi2c_ctrl *ctrl)
{
    ULONG spins, abrt;

    for (spins = 0; spins < DWI2C_SPIN_LIMIT; spins++)
    {
        if (dwi2c_rd(ctrl, IC_RAW_INTR_STAT) & IC_INTR_STOP_DET)
            return TRUE;

        if ((abrt = dwi2c_abort(ctrl)) != 0)
        {
            D(bug("[DWI2C:HW] %s: abort %08x waiting for stop\n",
                  ctrl->name, abrt);)
            return FALSE;
        }
    }

    D(bug("[DWI2C:HW] %s: no stop condition\n", ctrl->name);)

    return FALSE;
}

/*
 * One transaction: an optional write, an optional read after a repeated
 * START, and a STOP at the end of whichever came last.
 */
BOOL DWI2C_HWTransfer(struct dwi2c_ctrl *ctrl, UWORD address,
                      CONST_APTR writeBuffer, ULONG writeLength,
                      APTR readBuffer, ULONG readLength)
{
    BOOL ok;

    if (!ctrl || !ctrl->base)
        return FALSE;

    if (!writeBuffer)
        writeLength = 0;
    if (!readBuffer)
        readLength = 0;

    /* There is no such thing as a transaction with no bytes in it: the
       controller only starts one when a command is queued */
    if (!writeLength && !readLength)
        return FALSE;

    if (address > 0x7f)
    {
        D(bug("[DWI2C:HW] %s: %04x is not a 7 bit address\n", ctrl->name,
              address);)
        return FALSE;
    }

    if (!dwi2c_program(ctrl, address))
        return FALSE;

    ok = dwi2c_write(ctrl, (const UBYTE *)writeBuffer, writeLength,
                     readLength == 0);

    if (ok && readLength)
        ok = dwi2c_read(ctrl, (UBYTE *)readBuffer, readLength,
                        writeLength != 0);

    if (ok)
        ok = dwi2c_waitstop(ctrl);

    (void)dwi2c_rd(ctrl, IC_CLR_INTR);

    /*
     * Disabling is both how the controller is parked between
     * transactions - the target address cannot be changed otherwise -
     * and how a transaction that went wrong is abandoned, so it happens
     * either way.
     */
    dwi2c_enable(ctrl, FALSE);

    return ok;
}
