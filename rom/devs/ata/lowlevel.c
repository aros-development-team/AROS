/*
    Copyright (C) 2004-2026, The AROS Development Team. All rights reserved.

    Desc:
*/

/*
 * DMA requests are serialized per ATA bus. Providers with shared DMA state
 * must serialize across buses; ata_pci disables secondary DMA on simplex hardware.
 */

#include <aros/debug.h>

#include <proto/exec.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <devices/timer.h>

#include <stddef.h>
#include <string.h>

#include "ata.h"
#include "ata_bus.h"
#include "timer.h"

// use #define xxx(a) D(a) to enable particular sections.
#if DEBUG
#define DIRQ(a) D(a)
#define DIRQ_MORE(a)
#define DUMP(a) D(a)
#define DUMP_MORE(a)
#define DATA(a) D(a)
#define DATAPI(a) D(a)
#define DINIT(a) D(a)
#else
#define DIRQ(a)
#define DIRQ_MORE(a)
#define DUMP(a)
#define DUMP_MORE(a)
#define DATA(a)
#define DATAPI(a)
#define DINIT(a)
#endif
/* Errors that shouldn't happen */
#define DERROR(a) a

static BYTE ata_Identify(struct ata_Unit *unit);
static BYTE ata_ReadSector32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE ata_ReadSector64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static BYTE ata_ReadMultiple32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE ata_ReadMultiple64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static BYTE ata_ReadDMA32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE ata_ReadDMA64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static BYTE ata_WriteSector32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE ata_WriteSector64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static BYTE ata_WriteMultiple32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE ata_WriteMultiple64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static BYTE ata_WriteDMA32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE ata_WriteDMA64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static void ata_ResetBus(struct ata_Bus *bus);
static BYTE ata_Eject(struct ata_Unit *);
static BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout, BOOL irq,
    BOOL fake_irq, UBYTE *stout);

/*
 * ATAPI transport request. Filled in by the caller; results are returned
 * in the ap_Actual, ap_Status and ap_Error fields.
 */
struct atapi_Packet
{
    const UBYTE *ap_CDB;        /* SCSI command descriptor block              */
    ULONG        ap_CDBLen;     /* Length of the CDB (6 .. packet size)       */
    APTR         ap_Data;       /* Data buffer, NULL if there is no data      */
    ULONG        ap_DataLen;    /* Data phase length in bytes                 */
    BOOL         ap_Write;      /* Data phase direction is host -> device     */
    BOOL         ap_DMA;        /* Caller has prepared bus master DMA         */
    ULONG        ap_Actual;     /* Result: bytes transferred                  */
    UBYTE        ap_Status;     /* Result: ATA status register at completion  */
    UBYTE        ap_Error;      /* Result: ATA error register on CHECK        */
};

static BYTE atapi_SendPacket(struct ata_Unit *unit, struct atapi_Packet *pkt);
static BYTE atapi_DirectSCSI(struct ata_Unit *unit, struct SCSICmd *cmd);
static BYTE atapi_Read(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE atapi_Write(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE atapi_Eject(struct ata_Unit *);
static ULONG atapi_RequestSense(struct ata_Unit* unit, UBYTE* sense,
    ULONG senselen, ULONG *actual);
static void atapi_DeviceReset(struct ata_Unit *unit);

static void common_SetBestXferMode(struct ata_Unit* unit);

#define DEVHEAD_VAL 0xe0

#if DEBUG
static void dump(APTR mem, ULONG len)
{
    register int i, j = 0;

    DUMP_MORE(for (j=0; j<(len+15)>>4; ++j))
    {
        bug("[ATA  ] %06lx: ", j<<4);

        for (i=0; i<len-(j<<4); i++)
        {
            bug("%02lx ", ((unsigned char*)mem)[(j<<4)|i]);
            if (i == 15)
                break;
        }

        for (i=0; i<len-(j<<4); i++)
        {
            unsigned char c = ((unsigned char*)mem)[(j<<4)|i];

            bug("%c", c >= 0x20 ? c<=0x7f ? c : '.' : '.');
            if (i == 15)
                break;
        }
        bug("\n");
    }
}
#endif

static void ata_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    register int i = size;

    while (size--)
        str2[size ^ 1] = str1[size];

    while (i > 0 && str2[--i] <= ' ')
        str2[i] = '\0';
}

static inline struct ata_Unit* ata_GetSelectedUnit(struct ata_Bus* bus)
{
    return bus->ab_SelectedUnit;
}

/* The 400ns settle time the ATA protocol asks for after register writes */
static void ata_Wait400ns(struct ata_Bus *bus)
{
    ata_WaitNano(400, bus->ab_Base);
}

/* Sleep for the given number of microseconds using the bus timer */
static void ata_DelayMicro(struct ata_Bus *bus, ULONG micro)
{
    ata_WaitTO(bus->ab_Timer, 0, micro, 0);
}

static inline UBYTE ata_ReadStatus(struct ata_Bus *bus)
{
    return PIO_In(bus, ata_Status);
}

static inline UBYTE ata_ReadAltStatus(struct ata_Bus *bus)
{
    return PIO_InAlt(bus, ata_AltStatus);
}

static inline void ata_SelectUnit(struct ata_Unit* unit)
{
    struct ata_Bus *bus = unit->au_Bus;

    if (unit == bus->ab_SelectedUnit)
        return;

    PIO_Out(bus, unit->au_DevMask, ata_DevHead);

    do
    {
        ata_Wait400ns(bus);
        //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);
    }
    while (0 != (ATAF_BUSY & ata_ReadStatus(bus)));

    bus->ab_SelectedUnit = unit;
}

/*
 * handle IRQ; still fast and efficient, supposed to verify if this irq is for us and take adequate steps
 * part of code moved here from ata.c to reduce containment
 */
static void ata_IRQSignalTask(struct ata_Bus *bus)
{
    bus->ab_IntCnt++;
    Signal(bus->ab_Task, 1UL << bus->ab_SleepySignal);
}

static void ata_IRQSetHandler(struct ata_Unit *unit,
    void (*handler)(struct ata_Unit*, UBYTE), APTR piomem, ULONG blklen,
    ULONG piolen)
{
    if (NULL != handler)
    {
        unit->au_cmd_error = 0;
        unit->au_cmd_data = piomem;
        unit->au_cmd_length = (piolen < blklen) ? piolen : blklen;
        unit->au_cmd_total = piolen;
    }
    /*
     * A NULL handler ends the transfer. The counters are left alone so
     * that the amount of data actually moved can still be reported.
     */
    unit->au_Bus->ab_HandleIRQ = handler;
}

static void ata_IRQNoData(struct ata_Unit *unit, UBYTE status)
{
    if (status & ATAF_BUSY)
        return;

    if ((unit->au_cmd_error == 0) && (status & ATAF_ERROR))
        unit->au_cmd_error = HFERR_BadStatus;

    DIRQ(bug("[ATA%02ld] %s: NoData - done; status %02lx.\n", unit->au_UnitNum, __func__, status));
    ata_IRQSetHandler(unit, NULL, NULL, 0, 0);
    ata_IRQSignalTask(unit->au_Bus);
}

static void ata_IRQPIORead(struct ata_Unit *unit, UBYTE status)
{
    if (status & ATAF_DATAREQ)
    {
        DIRQ(bug("[ATA%02ld] %s: PIOReadData - DRQ.\n", unit->au_UnitNum, __func__));

        Unit_InS(unit, unit->au_cmd_data, unit->au_cmd_length);

        /*
         * Adjust data pointer and counter. If there's more data left for
         * this transfer, keep same handler and wait for next interrupt
         */
        unit->au_cmd_data += unit->au_cmd_length;
        unit->au_cmd_total -= unit->au_cmd_length;
        if (unit->au_cmd_total != 0)
        {
            if (unit->au_cmd_length > unit->au_cmd_total)
                unit->au_cmd_length = unit->au_cmd_total;
            return;
        }
        DIRQ(bug("[ATA%02ld] %s: PIOReadData - transfer completed.\n",
            unit->au_UnitNum, __func__));
    }
    else
        unit->au_cmd_error = HFERR_BadStatus;
    ata_IRQNoData(unit, status);
}

static void ata_PIOWriteBlk(struct ata_Unit *unit)
{
    Unit_OutS(unit, unit->au_cmd_data, unit->au_cmd_length);

    /*
     * Adjust data pointer and counter
     */
    unit->au_cmd_data += unit->au_cmd_length;
    unit->au_cmd_total -= unit->au_cmd_length;
    if (unit->au_cmd_length > unit->au_cmd_total)
        unit->au_cmd_length = unit->au_cmd_total;
}

static void ata_IRQPIOWrite(struct ata_Unit *unit, UBYTE status)
{
    /*
     * If there's more data left for this transfer, write it, keep same
     * handler and wait for next interrupt
     */
    if (status & ATAF_DATAREQ) {
        DIRQ(bug("[ATA%02ld] IRQ: PIOWriteData - DRQ.\n", unit->au_UnitNum));
        ata_PIOWriteBlk(unit);
        return;
    }
    else if (unit->au_cmd_total != 0)
        unit->au_cmd_error = HFERR_BadStatus;
    DIRQ(bug("[ATA%02ld] IRQ: PIOWriteData - done.\n", unit->au_UnitNum));
    ata_IRQNoData(unit, status);
}

static void ata_IRQDMAReadWrite(struct ata_Unit *unit, UBYTE status)
{
    struct ata_Bus *bus = unit->au_Bus;
    ULONG stat = DMA_GetResult(bus);

    DIRQ(bug("[ATA%02ld] IRQ: IO status %02lx, DMA status %02lx\n", unit->au_UnitNum, status, stat));

    if ((status & ATAF_ERROR) || stat)
    {
        /* This is turned on in order to help Phantom - Pavel Fedin <sonic_amiga@rambler.ru> */
        DERROR(bug("[ATA%02ld] IRQ: IO status %02lx, DMA status %d\n", unit->au_UnitNum, status, stat));
        DERROR(bug("[ATA%02ld] IRQ: ERROR %02lx\n", unit->au_UnitNum, PIO_In(bus, atapi_Error)));
        DERROR(bug("[ATA%02ld] IRQ: DMA Failed.\n", unit->au_UnitNum));

        unit->au_cmd_error = stat;
        ata_IRQNoData(unit, status);
    }
    else if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
    {
        DIRQ(bug("[ATA%02ld] IRQ: DMA Done.\n", unit->au_UnitNum));
        ata_IRQNoData(unit, status);
    }
}

/* Data port transfer to (write) or from the device */
static inline void Unit_XferS(struct ata_Unit *unit, APTR data, ULONG length, BOOL write)
{
    (write ? unit->au_outs : unit->au_ins)(unit->pioInterface, data, length);
}

/*
 * ATAPI PIO data phase. Called once per DRQ block; the device announces the
 * size of each block in the byte count registers and the interrupt reason
 * register tells the direction. The data register is 16 bits wide, so odd
 * lengths are handled by transferring the last byte through a pad word.
 * The final (status) interrupt has DRQ clear and ends the transfer.
 */
static void ata_IRQPIOAtapi(struct ata_Unit *unit, UBYTE status, BOOL write)
{
    struct ata_Bus *bus = unit->au_Bus;
    /* the name of the interrupt handler, for the error reports */
    CONST_STRPTR name = write ? "ata_IRQPIOWriteAtapi" : "ata_IRQPIOReadAtapi";
    ULONG size, xfer;
    UWORD tmp = 0;
    UBYTE reason;

    /* have we failed yet? */
    if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
    {
        ata_IRQNoData(unit, status);
        return;
    }
    if (status & ATAF_ERROR)
    {
        ata_IRQNoData(unit, status);
        return;
    }
    if (status & ATAF_BUSY)
        return;

    reason = PIO_In(bus, atapi_Reason);
    DIRQ(bug("[ATAPI] %s: Current status: %ld during %s\n", name, reason, write ? "WRITE" : "READ"));

    /* anything for us please? */
    if ((write ? ATAPIF_WRITE : ATAPIF_READ) != (reason & ATAPIF_MASK))
    {
        /*
         * The device wants to move data in the wrong direction. We can not
         * satisfy that; the command completes through the timeout path.
         */
        DERROR(bug("[ATAPI] %s: unexpected interrupt reason %02lx\n", name, reason));
        unit->au_cmd_error = HFERR_Phase;
        return;
    }

    size = PIO_In(bus, atapi_ByteCntH) << 8 | PIO_In(bus, atapi_ByteCntL);
    DIRQ(bug("[ATAPI] %s: data %s (%ld bytes, max: %ld bytes)\n", name,
        write ? "requested for write" : "available for read", size, unit->au_cmd_total));

    xfer = size;
    if (xfer > unit->au_cmd_total)
    {
        if (write)
            DERROR(bug("[ATAPI] %s: device requests %ld bytes, only %ld bytes of data left\n", name, size, unit->au_cmd_total));
        else
            DERROR(bug("[ATAPI] %s: device offers %ld bytes, only %ld bytes of buffer left\n", name, size, unit->au_cmd_total));
        xfer = unit->au_cmd_total;
    }

    Unit_XferS(unit, unit->au_cmd_data, xfer & ~1, write);
    if (xfer & 1)
    {
        /* the last byte goes through a pad word */
        if (write)
            ((UBYTE *)&tmp)[0] = ((UBYTE *)unit->au_cmd_data)[xfer - 1];
        Unit_XferS(unit, &tmp, 2, write);
        if (!write)
            ((UBYTE *)unit->au_cmd_data)[xfer - 1] = ((UBYTE *)&tmp)[0];
        tmp = 0;
    }

    unit->au_cmd_data = &((UBYTE*)unit->au_cmd_data)[xfer];
    unit->au_cmd_total -= xfer;

    DIRQ(bug("[ATAPI] %s: %lu bytes %s.\n", name, xfer, write ? "written" : "read"));

    /*
     * Reading: soak up whatever else the device insists on transferring in
     * this block, so that it can move on to the next phase.
     * Writing: pad the block with zeros if the device asked for more than
     * we have.
     */
    for (size = (size + 1) & ~1, xfer = (xfer + 1) & ~1; size > xfer; size -= 2)
        Unit_XferS(unit, &tmp, 2, write);

    if (unit->au_cmd_total == 0)
        ata_IRQSetHandler(unit, &ata_IRQNoData, NULL, 0, 0);
}

static void ata_IRQPIOReadAtapi(struct ata_Unit *unit, UBYTE status)
{
    ata_IRQPIOAtapi(unit, status, FALSE);
}

static void ata_IRQPIOWriteAtapi(struct ata_Unit *unit, UBYTE status)
{
    ata_IRQPIOAtapi(unit, status, TRUE);
}

/*
 * wait for timeout or drive ready
 */
static BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout, BOOL irq,
    BOOL fake_irq, UBYTE *stout)
{
    struct ata_Bus *bus = unit->au_Bus;
    UBYTE status = 0x5e; /* Mask all bits EXCEPT BSY/DF/ERR */
    ULONG step = 0;
    BOOL res = TRUE;

    DIRQ(
        bug("[ATA%02ld] %s(%s, %s)...\n",
            unit->au_UnitNum, __func__,
            (irq) ? "TRUE" :  "FALSE",
            (fake_irq) ? "TRUE" :  "FALSE");
    )

    if (bus->ab_Base->ata_Poll)
        irq = FALSE;

    DIRQ(
        bug("[ATA%02ld] %s: using IRQ mode = %s\n",
            unit->au_UnitNum, __func__,
            (irq) ? "TRUE" :  "FALSE");
        bug("[ATA%02ld] %s: timeout = %u...\n",
            unit->au_UnitNum, __func__, tout);
    )
    if (irq)
    {
        DIRQ(bug("[ATA%02ld] %s: IRQ mode\n", unit->au_UnitNum, __func__);)
        /* Do not read ata_Status in irq mode. It can cause random lost interrupts. */
        if (bus->haveAltIO)
            status = PIO_InAlt(bus, ata_AltStatus);

        /*
         * If we already have an error reported by Alt status,
         * dont wait or the IRQ will never arrive!
         */
        if ((!(status & (1 << 7))) && (status & ((1 << 5) | (1 << 0))))
        {
            fake_irq = TRUE;
            bug("[ATA%02ld] %s: %s occured, status = %02lx, ata error %lx\n",
                unit->au_UnitNum, __func__, (status & (1 << 5)) ? "drive fault" : "error", status, PIO_In(bus, ata_Error));
            DIRQ(bug("[ATA%02ld] %s: faking completion IRQ (real IRQ will not come)\n", unit->au_UnitNum, __func__);)
        }
        else
        {
            /*
             * wait for either IRQ or timeout
             */
            DIRQ(bug("[ATA%02ld] Waiting (Current status: %02lx)...\n", unit->au_UnitNum, status);)
            step = ata_WaitTO(unit->au_Bus->ab_Timer, tout, 0,
                1 << bus->ab_SleepySignal);
        }

        if (step == 0)
        {
            DERROR(bug("[ATA%02ld] Timeout while waiting for device to complete"
                " operation\n", unit->au_UnitNum));

            Disable();
            if (fake_irq)
            {
                /* fake the interrupt we expected */
                status = PIO_In(bus, ata_Status);
                while (unit->au_Bus->ab_HandleIRQ != NULL)
                    unit->au_Bus->ab_HandleIRQ(unit, status);
            }
            else
            {
                /* do nothing if the interrupt eventually arrives */
                ata_IRQSetHandler(unit, NULL, NULL, 0, 0);
                res = FALSE;
            }
            Enable();
        }
    }
    else
    {
        DIRQ(bug("[ATA%02ld] %s: no IRQ mode\n", unit->au_UnitNum, __func__);)
        status = PIO_InAlt(bus, ata_AltStatus);
        while (status & ATAF_BUSY)
        {
            ++step;
            DIRQ(bug("[ATA%02ld] %s: step #%u\n", unit->au_UnitNum, __func__, step);)
            /*
             * every 16n rounds do some extra stuff
             */
            if ((step & 15) == 0)
            {
                /*
                 * huhm. so it's been 16n rounds already. any timeout yet?
                 */
                if (step >> 4 > tout * 1000)
                {
                    DERROR(bug("[ATA%02ld] Device still busy after timeout."
                        " Aborting\n", unit->au_UnitNum));
                    res = FALSE;
                    break;
                }

                /*
                 * no timeout just yet, but it's not a good idea to keep
                 * spinning like that. let's give the system some time.
                 */
                ata_DelayMicro(unit->au_Bus, 1000);
            }
            status = PIO_InAlt(bus, ata_AltStatus);
        }
    }

    /*
     * get final status and clear any interrupt (may be neccessary if we
     * were polling, for example)
     */
    status = PIO_In(bus, ata_Status);

    /*
     * be nice to frustrated developer
     */
    DIRQ(bug("[ATA%02ld] WaitBusy status: %lx / %ld\n", unit->au_UnitNum,
        status, res));

    /*
     * release old junk
     */
    SetSignal(0, 1 << bus->ab_SleepySignal);

    /*
     * and say it went fine (i mean it)
     */
    if (stout)
        *stout = status;
    return res;
}

/*
 * Procedure for sending ATA command blocks
 * it appears LARGE but there's a lot of COMMENTS here :)
 * handles *all* ata commands (no data, pio and dma)
 * naturally could be split at some point in the future
 * depends if anyone believes that the change for 50 lines
 * would make slow ATA transfers any faster
 */
static BYTE ata_exec_cmd(struct ata_Unit* unit, ata_CommandBlock *block)
{
    struct ata_Bus *bus = unit->au_Bus;
    BYTE err = 0;
    APTR mem = block->buffer;
    UBYTE status;

    /*
     * Use a short timeout for Identify commands. This is because some bad
     * drives make us think that there are two devices on the bus when there
     * is really only one, so when we send an Identify command to the phantom
     * drive, we can't rely on getting an IRQ. Additionally, some real drives
     * don't produce an IRQ when the Identify command is complete, so we call
     * the interrupt handler "manually" if we time out while waiting for it.
     */
    BOOL fake_irq = ((block->command == ATA_IDENTIFY_DEVICE)
        || (block->command == ATA_IDENTIFY_ATAPI));
    UWORD timeout = (fake_irq) ? 1 : TIMEOUT;

    DATA(
        bug("[ATA%02ld] %s(0x%p, %u)\n", unit->au_UnitNum, __func__, unit, fake_irq);
        bug("[ATA%02ld] %s: bus @ 0x%p\n", unit->au_UnitNum, __func__, bus);
        bug("[ATA%02ld] %s: selecting unit..\n", unit->au_UnitNum, __func__);
    )
    ata_SelectUnit(unit);

    switch (block->type)
    {
        case CT_CHS:
        case CT_LBA28:
            if (block->sectors > 256)
            {
                DERROR(bug("[ATA%02ld] %s: ERROR: Transfer length (%ld) exceeds 256 sectors. Aborting.\n", unit->au_UnitNum, __func__, block->sectors));
                return IOERR_BADLENGTH;
            }

            /* note:
             * we want the above to fall in here!
             * we really do (checking for secmul)
             */

        case CT_LBA48:
            if (block->sectors > 65536)
            {
                DERROR(bug("[ATA%02ld] %s: ERROR: Transfer length (%ld) exceeds 65536 sectors. Aborting.\n", unit->au_UnitNum, __func__, block->sectors));
                return IOERR_BADLENGTH;
            }
            if (block->secmul == 0)
            {
                DERROR(bug("[ATA%02ld] %s: ERROR: Invalid transfer multiplier. Should be at least set to 1 (correcting)\n", unit->au_UnitNum, __func__));
                block->secmul = 1;
            }
           break;

        case CT_NoBlock:
            break;

        default:
            DERROR(bug("[ATA%02ld] %s: ERROR: Invalid command type %lx. Aborting.\n", unit->au_UnitNum, block->type, __func__));
            return IOERR_NOCMD;
    }

    block->actual = 0;
    DATA(bug("[ATA%02ld] %s: Executing command %02lx\n", unit->au_UnitNum, block->command, __func__);)

    if (block->feature != 0)
        PIO_Out(bus, block->feature, ata_Feature);

    /*
     * - set LBA and sector count
     */
    {
        /* Task file registers in the order they are written, per command type */
        static const UBYTE regs_chs[]   = { ata_DevHead, ata_Sector, ata_CylinderLow, ata_CylinderHigh, ata_Count };
        static const UBYTE regs_lba28[] = { ata_DevHead, ata_LBAHigh, ata_LBAMid, ata_LBALow, ata_Count };
        static const UBYTE regs_lba48[] = { ata_DevHead, ata_LBAHigh, ata_LBAMid, ata_LBALow,
                                            ata_LBAHigh, ata_LBAMid, ata_LBALow, ata_Count, ata_Count };
        const UBYTE *regs = regs_lba28;
        UBYTE val[9];
        UWORD i, n = 0;

        switch (block->type)
        {
            case CT_CHS:
                DATA(bug("[ATA%02ld] %s: Command uses CHS addressing (OLD)\n", unit->au_UnitNum, __func__));
                {
                    ULONG cyl, head, sector;
                    ULONG tmp = unit->au_Heads * unit->au_Sectors;
                    cyl = block->blk / tmp;
                    head = (block->blk % tmp) / unit->au_Sectors;
                    sector = (block->blk % unit->au_Sectors) + 1;

                    regs = regs_chs;
                    n = 5;
                    val[0] = ((head) & 0x0f) | unit->au_DevMask;
                    val[1] = sector;
                    val[2] = cyl & 0xff;
                    val[3] = (cyl >> 8) & 0xff;
                    val[4] = block->sectors;
                }
                break;
            case CT_LBA28:
                DATA(bug("[ATA%02ld] %s: Command uses 28bit LBA addressing (OLD)\n", unit->au_UnitNum, __func__));

                n = 5;
                val[0] = ((block->blk >> 24) & 0x0f) | DEVHEAD_VAL | unit->au_DevMask;
                val[1] = block->blk >> 16;
                val[2] = block->blk >> 8;
                val[3] = block->blk;
                val[4] = block->sectors;
                break;

            case CT_LBA48:
                DATA(bug("[ATA%02ld] %s: Command uses 48bit LBA addressing (NEW)\n", unit->au_UnitNum, __func__));

                regs = regs_lba48;
                n = 9;
                val[0] = DEVHEAD_VAL | unit->au_DevMask;
                val[1] = block->blk >> 40;
                val[2] = block->blk >> 32;
                val[3] = block->blk >> 24;

                val[4] = block->blk >> 16;
                val[5] = block->blk >> 8;
                val[6] = block->blk;

                val[7] = block->sectors >> 8;
                val[8] = block->sectors;
                break;

            case CT_NoBlock:
                DATA(bug("[ATA%02ld] %s: Command does not address any block\n", unit->au_UnitNum, __func__));
                break;
        }

        for (i = 0; i < n; i++)
            PIO_Out(bus, val[i], regs[i]);
    }

    switch (block->method)
    {
        case CM_PIOWrite:
        case CM_PIORead:
            ata_IRQSetHandler(unit, (block->method == CM_PIOWrite) ? &ata_IRQPIOWrite : &ata_IRQPIORead,
                mem, block->secmul << unit->au_SectorShift, block->length);
            break;

        case CM_DMARead:
        case CM_DMAWrite:
            if (FALSE == DMA_Setup(bus, mem, block->length, block->method == CM_DMARead))
                return IOERR_ABORTED;

            ata_IRQSetHandler(unit, &ata_IRQDMAReadWrite, NULL, 0, 0);
            DMA_Start(bus);
            break;

        case CM_NoData:
            ata_IRQSetHandler(unit, &ata_IRQNoData, NULL, 0, 0);
            break;

        default:
            return IOERR_NOCMD;
            break;
    };

    /*
     * send command now
     * let drive propagate its signals
     */
    DATA(bug("[ATA%02ld] %s: Sending command\n", unit->au_UnitNum, __func__);)

    PIO_Out(bus, block->command, ata_Command);
    DATA(bug("[ATA%02ld] %s: waiting 400ns (bus ab_Base = 0x%p) ..\n", unit->au_UnitNum, __func__, bus->ab_Base);)
    ata_Wait400ns(bus);
    //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);

    /*
     * In case of PIO write the drive won't issue an IRQ before first
     * data transfer, so we should poll the status and send the first
     * block upon request.
     */
    if (block->method == CM_PIOWrite)
    {
    DATA(bug("[ATA%02ld] %s: waiting for PIO write completion ...\n", unit->au_UnitNum, __func__);)
        if (FALSE == ata_WaitBusyTO(unit, TIMEOUT, FALSE, FALSE, &status)) {
            DERROR(bug("[ATA%02ld] %s: PIOWrite - no response from device. Status %02X\n", unit->au_UnitNum, __func__, status));
            return IOERR_UNITBUSY;
        }
        if (status & ATAF_DATAREQ) {
            DATA(bug("[ATA%02ld] %s: PIOWrite - DRQ.\n", unit->au_UnitNum, __func__));
            ata_PIOWriteBlk(unit);
        }
        else
        {
            DERROR(bug("[ATA%02ld] %s: PIOWrite - bad status: %02X\n", unit->au_UnitNum, __func__, status));
            return HFERR_BadStatus;
        }
    }

    /*
     * wait for drive to complete what it has to do
     */
    DATA(bug("[ATA%02ld] %s: waiting for drive (timeout %u)..\n", unit->au_UnitNum, __func__, timeout);)
    if (FALSE == ata_WaitBusyTO(unit, timeout, TRUE, fake_irq, &status))
    {
        DERROR(bug("[ATA%02ld] %s: Device is late - no response\n", unit->au_UnitNum, __func__));
        err = IOERR_UNITBUSY;
    }
    else
        err = unit->au_cmd_error;

    DATA(bug("[ATA%02ld] %s: Command done\n", unit->au_UnitNum, __func__);)

    /*
     * clean up DMA
     * don't use 'mem' pointer here as it's already invalid.
     */
    if ((block->method == CM_DMARead) || (block->method == CM_DMAWrite))
        DMA_End(bus, block->buffer, block->length, block->method == CM_DMARead);

    D(bug("[ATA%02ld] %s: return code %ld\n", unit->au_UnitNum, __func__, err));
    return err;
}

/*
 * atapi packet iface
 */

/*
 * io_Error values returned for a CHECK CONDITION, indexed by the sense key
 * reported in the ATA error register. The full sense data is fetched by
 * atapi_DirectSCSI() when the caller asked for autosense.
 */
static const BYTE ErrorMap[16] = {
    CDERR_NotSpecified,     /* NO SENSE         */
    CDERR_NoSecHdr,         /* RECOVERED ERROR  */
    CDERR_NoDisk,           /* NOT READY        */
    CDERR_NoSecHdr,         /* MEDIUM ERROR     */
    CDERR_NoSecHdr,         /* HARDWARE ERROR   */
    CDERR_NOCMD,            /* ILLEGAL REQUEST  */
    CDERR_NoDisk,           /* UNIT ATTENTION   */
    CDERR_WriteProt,        /* DATA PROTECT     */
    CDERR_NotSpecified,     /* BLANK CHECK      */
    CDERR_NotSpecified,     /* VENDOR SPECIFIC  */
    CDERR_NotSpecified,     /* COPY ABORTED     */
    CDERR_ABORTED,          /* ABORTED COMMAND  */
    CDERR_NotSpecified,     /* (obsolete)       */
    CDERR_NotSpecified,     /* VOLUME OVERFLOW  */
    CDERR_NoSecHdr,         /* MISCOMPARE       */
    CDERR_NotSpecified,     /* COMPLETED        */
};

/*
 * IDENTIFY PACKET DEVICE word 0, bits 1:0 tell the packet size:
 * 00b = 12 bytes, 01b = 16 bytes.
 */
static inline ULONG atapi_PacketSize(struct ata_Unit *unit)
{
    return ((unit->au_Drive->id_General & 0x03) == 0x01) ? 16 : 12;
}

/*
 * Only bulk data commands are sent using DMA. Everything else uses PIO,
 * which copes with devices returning less data than was asked for.
 */
static BOOL atapi_CanDMA(UBYTE opcode)
{
    static const UBYTE dmacmds[] =
    {
        SCSI_READ10,
        SCSI_READ12,
        0xbe,               /* READ CD               */
        0xb9,               /* READ CD MSF           */
        SCSI_VERIFY10,
        SCSI_WRITE10,
        SCSI_WRITE12,
        0x2e,               /* WRITE AND VERIFY (10) */
        0xad,               /* READ DVD STRUCTURE    */
        0xa4,               /* REPORT KEY            */
        0xa3                /* SEND KEY              */
    };
    UWORD i;

    for (i = 0; i < sizeof(dmacmds); i++)
    {
        if (dmacmds[i] == opcode)
            return TRUE;
    }
    return FALSE;
}

/*
 * Discard whatever the device still wants to transfer in PIO mode, so that
 * it can leave the data phase.
 */
static void atapi_DrainFIFO(struct ata_Unit *unit)
{
    struct ata_Bus *bus = unit->au_Bus;
    ULONG words = 0;
    UWORD tmp;

    while (((ata_ReadAltStatus(bus) & (ATAF_BUSY | ATAF_DATAREQ)) == ATAF_DATAREQ)
        && (words < 65536))
    {
        Unit_InS(unit, &tmp, 2);
        words++;
    }
    DERROR(bug("[ATA%02ld] %s: drained %lu words\n", unit->au_UnitNum, __func__, words));
}

/*
 * DEVICE RESET is mandatory for PACKET devices. It is used to recover a
 * device which stopped responding or violated the packet protocol. The
 * command does not generate an interrupt, so completion is polled.
 */
static void atapi_DeviceReset(struct ata_Unit *unit)
{
    struct ata_Bus *bus = unit->au_Bus;

    DERROR(bug("[ATA%02ld] %s: resetting device\n", unit->au_UnitNum, __func__));

    /* Whatever transfer was in progress is over: a late interrupt must not touch the buffer */
    Disable();
    ata_IRQSetHandler(unit, NULL, NULL, 0, 0);
    Enable();

    PIO_Out(bus, unit->au_DevMask, ata_DevHead);
    ata_Wait400ns(bus);
    PIO_Out(bus, ATA_DEVICE_RESET, ata_Command);
    ata_Wait400ns(bus);

    if (FALSE == ata_WaitBusyTO(unit, 5, FALSE, FALSE, NULL))
        DERROR(bug("[ATA%02ld] %s: device still busy after reset\n", unit->au_UnitNum, __func__));

    /* Drop a completion signal which may have arrived meanwhile */
    SetSignal(0, 1 << bus->ab_SleepySignal);
}

/*
 * Send one SCSI CDB through the PACKET command and run the data and status
 * phases. Both PIO and bus master DMA transfers are handled; for DMA the
 * caller has already called DMA_Setup() and we always call DMA_End().
 */
static BYTE atapi_SendPacket(struct ata_Unit *unit, struct atapi_Packet *pkt)
{
    struct ata_Bus *bus = unit->au_Bus;
    ULONG plen = atapi_PacketSize(unit);
    ULONG bytecount = pkt->ap_DataLen;
    UBYTE cmd[16];
    UBYTE status;
    BYTE err = 0;
    ULONG i;

    pkt->ap_Actual = 0;
    pkt->ap_Status = 0;
    pkt->ap_Error  = 0;

    if ((pkt->ap_CDBLen == 0) || (pkt->ap_CDBLen > plen))
    {
        err = IOERR_NOCMD;
        goto done;
    }

    /* The packet always has the size announced by the device; unused bytes are zero */
    for (i = 0; i < sizeof(cmd); i++)
        cmd[i] = (i < pkt->ap_CDBLen) ? pkt->ap_CDB[i] : 0;

    /*
     * Byte count limit: the most the device may transfer per DRQ assertion
     * in PIO mode. The data phase handlers deal with any number of DRQ
     * blocks, so large transfers are simply split by the device. Values of
     * 0 and 0xFFFF as well as odd values are not handled by all devices.
     */
    if (bytecount > ATAPI_MAX_BYTECOUNT)
        bytecount = ATAPI_MAX_BYTECOUNT;
    if (bytecount & 1)
        bytecount++;

    DATAPI({
        bug("[ATA%02lx] %s: Sending %s ATA packet (%lu bytes, %lu data bytes): ", unit->au_UnitNum, __func__,
            pkt->ap_DMA ? "DMA" : "PIO", plen, pkt->ap_DataLen);
        for (i = 0; i < plen; i++)
            bug("%02lx ", cmd[i]);
        bug("\n");
    });

    ata_SelectUnit(unit);

    /*
     * FEATURES: bit 0 selects DMA for the data phase, bit 2 (DMADIR) tells
     * devices which need it that the data moves from device to host.
     */
    PIO_Out(bus,
            (pkt->ap_DMA ? 1 : 0) | (((unit->au_Drive->id_DMADir & 0x8000) && !pkt->ap_Write) ? 4 : 0),
            atapi_Features);
    PIO_Out(bus, bytecount & 0xff, atapi_ByteCntL);
    PIO_Out(bus, (bytecount >> 8) & 0xff, atapi_ByteCntH);

    /*
     * Issue PACKET and wait for the device to request the command bytes.
     * Devices announcing "interrupt DRQ" (IDENTIFY word 0 bits 6:5 = 01b)
     * raise INTRQ when ready; the others set DRQ within 3 ms (or 50 us)
     * and are polled. Either way we must give 400 ns for BSY to be set.
     */
    DATAPI(bug("[ATAPI] Issuing ATA_PACKET command.\n"));
    ata_IRQSetHandler(unit, &ata_IRQNoData, NULL, 0, 0);
    PIO_Out(bus, ATA_PACKET, atapi_Command);
    ata_Wait400ns(bus);

    if (FALSE == ata_WaitBusyTO(unit, TIMEOUT, (unit->au_Drive->id_General & 0x60) == 0x20,
        FALSE, &status))
    {
        DERROR(bug("[ATA%02ld] %s: device did not accept PACKET command\n", unit->au_UnitNum, __func__));
        atapi_DeviceReset(unit);
        err = IOERR_UNITBUSY;
        goto done;
    }

    pkt->ap_Status = status;
    if (status & ATAF_ERROR)
    {
        /* The command was rejected before the packet was transferred */
        pkt->ap_Error = PIO_In(bus, atapi_Error);
        DATAPI(bug("[ATAPI] PACKET rejected, error %02lx\n", pkt->ap_Error));
        err = ErrorMap[pkt->ap_Error >> 4];
        goto done;
    }
    if (0 == (status & ATAF_DATAREQ))
    {
        DERROR(bug("[ATA%02ld] %s: no DRQ for packet, status %02lx\n", unit->au_UnitNum, __func__, status));
        err = HFERR_BadStatus;
        goto done;
    }

    /*
     * setup appropriate hooks
     */
    if (pkt->ap_DataLen == 0)
        ata_IRQSetHandler(unit, &ata_IRQNoData, NULL, 0, 0);
    else if (pkt->ap_DMA)
        ata_IRQSetHandler(unit, &ata_IRQDMAReadWrite, NULL, 0, 0);
    else if (pkt->ap_Write)
        ata_IRQSetHandler(unit, &ata_IRQPIOWriteAtapi, pkt->ap_Data, 0, pkt->ap_DataLen);
    else
        ata_IRQSetHandler(unit, &ata_IRQPIOReadAtapi, pkt->ap_Data, 0, pkt->ap_DataLen);

    if (pkt->ap_DMA)
    {
        DATAPI(bug("[ATAPI] Starting DMA\n"));
        DMA_Start(bus);
    }

    DATAPI(bug("[ATAPI] Sending packet\n"));
    Unit_OutS(unit, cmd, plen);
    ata_Wait400ns(bus);

    DATAPI(bug("[ATAPI] Status after packet: %lx\n", ata_ReadAltStatus(bus)));

    /*
     * Wait for command to complete. For PIO transfers the handler is run
     * once per DRQ block and signals us from the final status interrupt.
     */
    if (ata_WaitTO(bus->ab_Timer, TIMEOUT, 0, 1 << bus->ab_SleepySignal) == 0)
    {
        DERROR(bug("[ATA%02ld] %s: packet %02lx timed out\n", unit->au_UnitNum, __func__, cmd[0]));
        atapi_DeviceReset(unit);
        err = IOERR_UNITBUSY;
        goto done;
    }

    /*
     * read alternate status register (per specs), then the status register
     */
    PIO_InAlt(bus, ata_AltStatus);
    status = PIO_In(bus, atapi_Status);
    pkt->ap_Status = status;

    DATAPI(bug("[ATA%02ld] %s: Command complete. Status: %lx\n", unit->au_UnitNum, __func__, status));

    if (!pkt->ap_DMA)
        pkt->ap_Actual = pkt->ap_DataLen - unit->au_cmd_total;

    if (status & ATAF_DATAREQ)
    {
        /*
         * The device is still in a data phase: the transfer did not match
         * what the command implied. Get it out of there.
         */
        DERROR(bug("[ATA%02ld] %s: device still in data phase after packet %02lx\n", unit->au_UnitNum, __func__, cmd[0]));
        atapi_DrainFIFO(unit);
        atapi_DeviceReset(unit);
        err = HFERR_Phase;
    }
    else if (status & ATAPIF_CHECK)
    {
        pkt->ap_Error = PIO_In(bus, atapi_Error);
        DATAPI(bug("[ATA%02ld] %s: CHECK CONDITION, sense key %lx\n", unit->au_UnitNum, __func__, pkt->ap_Error >> 4));
        err = ErrorMap[pkt->ap_Error >> 4];
    }
    else if (unit->au_cmd_error != 0)
    {
        /* DMA failure or protocol violation noticed by the interrupt handler */
        err = unit->au_cmd_error;
    }
    else if (pkt->ap_DMA)
    {
        /* Bus master hardware does not report a residual; a good status means everything was moved */
        pkt->ap_Actual = pkt->ap_DataLen;
    }

done:
    if (pkt->ap_DMA)
        DMA_End(bus, pkt->ap_Data, pkt->ap_DataLen, !pkt->ap_Write);

    DATAPI(bug("[ATA%02ld] %s: error %ld, status %02lx, %lu of %lu bytes\n", unit->au_UnitNum, __func__,
        err, pkt->ap_Status, pkt->ap_Actual, pkt->ap_DataLen));
    return err;
}

/*
 * HD_SCSICMD for PACKET devices: pass the caller's CDB through and fill in
 * the scsi_Actual/scsi_Status/scsi_SenseActual results the way scsi.device
 * does.
 */
static BYTE atapi_DirectSCSI(struct ata_Unit *unit, struct SCSICmd *cmd)
{
    struct atapi_Packet pkt;
    BYTE err;

    cmd->scsi_Actual      = 0;
    cmd->scsi_CmdActual   = 0;
    cmd->scsi_SenseActual = 0;
    cmd->scsi_Status      = SCSI_STATUS_GOOD;

    if ((cmd->scsi_Command == NULL) || (cmd->scsi_CmdLength < 6)
        || (cmd->scsi_CmdLength > atapi_PacketSize(unit)))
    {
        DATAPI(bug("[ATA%02lx] %s: invalid CDB length %ld\n", unit->au_UnitNum, __func__, cmd->scsi_CmdLength));
        return IOERR_NOCMD;
    }
    if ((cmd->scsi_Length != 0) && (cmd->scsi_Data == NULL))
        return IOERR_BADADDRESS;

    pkt.ap_CDB     = cmd->scsi_Command;
    pkt.ap_CDBLen  = cmd->scsi_CmdLength;
    pkt.ap_Data    = cmd->scsi_Data;
    pkt.ap_DataLen = cmd->scsi_Length;
    pkt.ap_Write   = (cmd->scsi_Flags & SCSIF_READ) == 0;
    pkt.ap_DMA     = FALSE;

    DATAPI(bug("[DSCSI] Sending packet!\n"));

    /*
     * Bulk data commands go through bus master DMA when the bus and the
     * drive support it. If the DMA setup fails we fall back to PIO.
     */
    if ((unit->au_Flags & AF_DMA) && (pkt.ap_DataLen != 0) && atapi_CanDMA(cmd->scsi_Command[0]))
        pkt.ap_DMA = DMA_Setup(unit->au_Bus, pkt.ap_Data, pkt.ap_DataLen, !pkt.ap_Write);

    err = atapi_SendPacket(unit, &pkt);

    cmd->scsi_CmdActual = cmd->scsi_CmdLength;
    cmd->scsi_Actual    = pkt.ap_Actual;

    DUMP({ if ((err == 0) && (cmd->scsi_Data != NULL)) dump(cmd->scsi_Data, cmd->scsi_Actual); });

    DATAPI(bug("[ATA%02lx] atapi_DirectSCSI: SCSI Flags: %02lx / Error: %ld\n", unit->au_UnitNum, cmd->scsi_Flags, err));
    if (err == 0)
        return 0;

    if (pkt.ap_Status & ATAPIF_CHECK)
    {
        cmd->scsi_Status = SCSI_STATUS_CHECK_CONDITION;

        /*
         * on check condition - grab sense data
         */
        if ((cmd->scsi_Flags & SCSIF_AUTOSENSE) && (cmd->scsi_SenseData != NULL)
            && (cmd->scsi_SenseLength != 0))
        {
            ULONG senseactual = 0;

            DATAPI(bug("[DSCSI] atapi_DirectSCSI: Packet Failed. Calling atapi_RequestSense\n"));
            atapi_RequestSense(unit, cmd->scsi_SenseData, cmd->scsi_SenseLength, &senseactual);
            cmd->scsi_SenseActual = senseactual;
            DUMP(dump(cmd->scsi_SenseData, senseactual));
        }
    }
    else if (err == IOERR_UNITBUSY)
    {
        cmd->scsi_Status = SCSI_STATUS_BUSY;
    }

    return err;
}

/*
 * chops the large transfers into set of smaller transfers
 * specifically useful when requested transfer size is >256 sectors for 28bit commands
 */
static BYTE ata_exec_blk(struct ata_Unit *unit, ata_CommandBlock *blk)
{
    BYTE err = 0;
    ULONG part;
    ULONG max=256;
    ULONG count=blk->sectors;
    APTR buffer = blk->buffer;
    APTR bounce_buffer = NULL;
    IPTR bounce_buffer_length = 0;

    if (blk->type == CT_LBA48)
        max <<= 8;

    if (((IPTR)blk->buffer > 0xffffffffULL || ((IPTR)blk->buffer + (count << unit->au_SectorShift)) > 0xffffffffULL) &&
        (blk->method == CM_DMARead || blk->method == CM_DMAWrite))
    {
        DATA(bug("[ATA%02ld] ata_exec_blk: attempt to do DMA transfer outside 32bit address space\n", unit->au_UnitNum));
        DATA(bug("[ATA%02ld] ata_exec_blk: ptr %p, length %d, %s\n", unit->au_UnitNum, blk->buffer, count << unit->au_SectorShift, blk->method == CM_DMARead ? "DMARead" : "DMAWrite"));

        bounce_buffer_length = count << unit->au_SectorShift;
        bounce_buffer = AllocPooled(unit->au_Bus->ab_BounceBufferPool, bounce_buffer_length);
        blk->buffer = bounce_buffer;
    }

    DATA(bug("[ATA%02ld] ata_exec_blk: Accessing %ld sectors starting from %x%08x\n", unit->au_UnitNum, count, (ULONG)(blk->blk >> 32), (ULONG)blk->blk));
    while ((count > 0) && (err == 0))
    {
        part = (count > max) ? max : count;
        blk->sectors = part;
        blk->length  = part << unit->au_SectorShift;

        DATA(bug("[ATA%02ld] Transfer of %ld sectors from %x%08x\n", unit->au_UnitNum, part, (ULONG)(blk->blk >> 32), (ULONG)blk->blk));
        // If bounce buffer is active,
        if (bounce_buffer && blk->method == CM_DMAWrite)
        {
            DATA(bug("[ATA%02ld] Copy %d bytes from source %p to bounce buffer %p\n", unit->au_UnitNum, blk->length, buffer, bounce_buffer));
            CopyMemQuick(buffer, bounce_buffer, blk->length);
            buffer = (APTR)((IPTR)buffer + blk->length);
        }
        err = ata_exec_cmd(unit, blk);
        DATA(bug("[ATA%02ld] ata_exec_blk: ata_exec_cmd returned %lx\n", unit->au_UnitNum, err));
        if (bounce_buffer)
        {
            if (blk->method == CM_DMARead)
            {
                DATA(bug("[ATA%02ld] Copy %d bytes from bounce buffer %p to destination %p\n", unit->au_UnitNum, blk->length, bounce_buffer, buffer));
                CopyMemQuick(bounce_buffer, buffer, part << unit->au_SectorShift);
                buffer = (APTR)((IPTR)buffer + (part << unit->au_SectorShift));
            }
        }
        else
        {
            blk->buffer  = (APTR)((IPTR)blk->buffer + (part << unit->au_SectorShift));
        }
        blk->blk    += part;
        count -= part;
    }

    if (bounce_buffer)
        FreePooled(unit->au_Bus->ab_BounceBufferPool, bounce_buffer, bounce_buffer_length);

    return err;
}

/*
 * Initial device configuration that suits *all* cases
 */
void ata_init_unit(struct ata_Bus *bus, struct ata_Unit *unit, UBYTE u)
{
    struct ataBase *ATABase = bus->ab_Base;
    OOP_Object *obj = OOP_OBJECT(ATABase->busClass, bus);

    unit->au_Bus       = bus;
    unit->pioInterface = bus->pioInterface;
    unit->au_UnitNum   = bus->ab_BusNum << 1 | u;      // b << 8 | u
    unit->au_DevMask   = 0xa0 | (u << 4);

    DINIT(bug("[ATA%02u] ata_init_unit: bus %u unit %d\n", unit->au_UnitNum, bus->ab_BusNum, u));

    /* Set PIO transfer functions, either 16 or 32 bits */
    if (ATABase->ata_32bit && OOP_GET(obj, aHidd_ATABus_Use32Bit))
        Unit_Enable32Bit(unit);
    else
        Unit_Disable32Bit(unit);
}

BOOL ata_setup_unit(struct ata_Bus *bus, struct ata_Unit *unit)
{
    /*
     * this stuff always goes along the same way
     * WARNING: NO INTERRUPTS AT THIS POINT!
     */
    UBYTE u = (unit->au_UnitNum & 1), status;

    DINIT(
        bug("[ATA%02ld] %s()\n", unit->au_UnitNum, __func__);
        bug("[ATA%02ld] %s: sending select unit\n", unit->au_UnitNum, __func__);
    )
    ata_SelectUnit(unit);

    if (FALSE == ata_WaitBusyTO(unit, 1, FALSE, FALSE, NULL))
    {
        bug("[ATA%02ld] %s: ERROR: Drive not ready for use. Keeping functions stubbed\n", unit->au_UnitNum, __func__);
        return FALSE;
    }

    status = PIO_InAlt(bus, ata_AltStatus);
    if (!(status & ((1 << 0) | (1 << 5))))
    {

        DINIT(bug("[ATA%02ld] %s: device type %u\n", unit->au_UnitNum, __func__, bus->ab_Dev[u]);)

        switch (bus->ab_Dev[u])
        {
            /*
             * safe fallback settings
             */
            case DEV_SATAPI:
            case DEV_ATAPI:
            case DEV_SATA:
            case DEV_ATA:
                DINIT(bug("[ATA%02ld] %s:     * ATA\n", unit->au_UnitNum, __func__);)
                unit->au_Identify = ata_Identify;
                break;

            default:
                DINIT(bug("[ATA%02ld] %s: Unsupported device %lx. All functions will remain stubbed.\n", unit->au_UnitNum, __func__, bus->ab_Dev[u]));
                return FALSE;
        }

        status = PIO_InAlt(bus, ata_AltStatus);
        DINIT(bug("[ATA%02ld] %s: status before enabling IRQs = %lx\n", unit->au_UnitNum, __func__, status);)

        PIO_OutAlt(bus, 0x0, ata_AltControl);

        status = PIO_InAlt(bus, ata_AltStatus);
        if (!(status & ((1 << 0) | (1 << 5))))
        {
            /*
             * now make unit self diagnose
             */
            DINIT(bug("[ATA%02ld] %s: performing self identify (func @ 0x%p)\n", unit->au_UnitNum, __func__, unit->au_Identify);)
            if ((status = unit->au_Identify(unit)) == 0)
                return TRUE;
            
            bug("[ATA%02ld] %s: identify failed, status = %02lx (ata error %lx)\n", unit->au_UnitNum, __func__, status, PIO_In(bus, ata_Error));
        }
        else
        {
            bug("[ATA%02ld] %s: error/fault after enabling IRQ's (status = %lx)\n", unit->au_UnitNum, __func__, status);
        }
        
    }
    else
    {
        bug("[ATA%02ld] %s: error/fault after selecting unit (status = %lx)\n", unit->au_UnitNum, __func__, status);
    }
    bug("[ATA%02ld] %s: setup failed\n", unit->au_UnitNum, __func__);

    /* flag that there is no unit attached */
    bus->ab_Dev[u] = DEV_NONE;
    unit->au_Identify = NULL;

    return FALSE;
}

/*
 * ata[pi] identify
 */
static void common_SetXferMode(struct ata_Unit* unit, ata_XferMode mode)
{
    struct ata_Bus *bus = unit->au_Bus;
    BOOL dma = FALSE;
/*
 * We can't set drive modes unless we also set the controller's timing registers
 * FIXME:   Implement aoHodd_ATABus_CanSetXferMode and moHidd_ATABus_SetXferMode
            support.
 * CHECKME: Current code lives with what machine's firmware has set for us. Looks
 *          like all firmwares set up the best DMA mode. But what if the firmware
 *          didn't set it up for some reason (the add-on controller which has been
 *          ignored by it
 *          for example) ? Shouldn't we check unit->au_UseModes here ?
 */
#if 0
    struct ataBase *ATABase = bus->ab_Base;
    OOP_Object *obj = OOP_OBJECT(ATABase->busClass, bus);
    UBYTE type=0;
    ata_CommandBlock acb =
    {
        ATA_SET_FEATURES,
        0x03,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        CM_NoData,
        CT_LBA28
    };
#endif
    DINIT(bug("[ATA%02ld] common_SetXferMode: Trying to set mode %d\n", unit->au_UnitNum, mode));

    /* CHECKME: This condition should be not needed. */
    if ((!bus->dmaVectors) && (mode >= AB_XFER_MDMA0))
    {
        DINIT(bug("[ATA%02ld] common_SetXferMode: This controller does not own DMA port! Will set best PIO\n", unit->au_UnitNum));
        common_SetBestXferMode(unit);
        return;
    }

    /*
     * first, ONLY for ATA devices, set new commands
     */
    if (0 == (unit->au_XferModes & AF_XFER_PACKET))
    {
        if ((mode >= AB_XFER_MDMA0) && (mode <= AB_XFER_UDMA6))
        {
            /* DMA, both multiword and Ultra */
            unit->au_Read32  = ata_ReadDMA32;
            unit->au_Write32 = ata_WriteDMA32;
            if (unit->au_XferModes & AF_XFER_48BIT)
            {
                unit->au_UseModes |= AF_XFER_48BIT;
                unit->au_Read64    = ata_ReadDMA64;
                unit->au_Write64   = ata_WriteDMA64;
            }
        }
        else if ((!unit->au_Bus->ab_Base->ata_NoMulti) && (unit->au_XferModes & AF_XFER_RWMULTI))
        {
            /* Multisector PIO */
            ata_IRQSetHandler(unit, ata_IRQNoData, NULL, 0, 0);
            PIO_Out(bus, unit->au_Drive->id_RWMultipleSize & 0xFF, ata_Count);
            PIO_Out(bus, ATA_SET_MULTIPLE, ata_Command);
            ata_WaitBusyTO(unit, -1, TRUE, FALSE, NULL);

            unit->au_UseModes |= AF_XFER_RWMULTI;
            unit->au_Read32    = ata_ReadMultiple32;
            unit->au_Write32   = ata_WriteMultiple32;
            if (unit->au_XferModes & AF_XFER_48BIT)
            {
                unit->au_UseModes |= AF_XFER_48BIT;
                unit->au_Read64    = ata_ReadMultiple64;
                unit->au_Write64   = ata_WriteMultiple64;
            }
        }
        else
        {
            /* 1-sector PIO */
            unit->au_Read32  = ata_ReadSector32;
            unit->au_Write32 = ata_WriteSector32;
            if (unit->au_XferModes & AF_XFER_48BIT)
            {
                unit->au_UseModes |= AF_XFER_48BIT;
                unit->au_Read64    = ata_ReadSector64;
                unit->au_Write64   = ata_WriteSector64;
            }
        }
    }

#if 0 // We can't set drive modes unless we also set the controller's timing registers
    if ((mode >= AB_XFER_PIO0) && (mode <= AB_XFER_PIO4))
    {
        type = 8 + (mode - AB_XFER_PIO0);
    }
    else if ((mode >= AB_XFER_MDMA0) && (mode <= AB_XFER_MDMA2))
    {
        type = 32 + (mode - AB_XFER_MDMA0);
        dma=TRUE;
    }
    else if ((mode >= AB_XFER_UDMA0) && (mode <= AB_XFER_UDMA6))
    {
        type = 64 + (mode - AB_XFER_UDMA0);
        dma=TRUE;
    }
    else
    {
        type = 0;
    }

    acb.sectors = type;
    if (0 != ata_exec_cmd(unit, &acb))
    {
        DINIT(bug("[ATA%02ld] common_SetXferMode: ERROR: Failed to apply new xfer mode.\n", unit->au_UnitNum));
    }

    if (!HIDD_ATABus_SetXferMode(obj, mode))
    {
        /*
         * DMA mode setup failed.
         * FIXME: Should completely revert back to PIO protocol, or try lower mode.
         */
        dma = FALSE;
    }
#else
    if (mode >= AB_XFER_MDMA0)
        dma = TRUE;
#endif

    if (dma)
    {
        unit->au_Flags |= AF_DMA; /* This flag is used by ATAPI protocol */
    }
    else
    {
        unit->au_UseModes &= ~AF_XFER_DMA_MASK;
        unit->au_Flags    &= ~AF_DMA;
    }
}

static void common_SetBestXferMode(struct ata_Unit* unit)
{
    struct ata_Bus *bus = unit->au_Bus;
    struct ataBase *ATABase = bus->ab_Base;
    OOP_Object *obj = OOP_OBJECT(ATABase->busClass, bus);
    int iter;
    int max = AB_XFER_UDMA6;

    if ((!bus->dmaInterface)
        || (   !(unit->au_Drive->id_MWDMASupport & 0x0700)
            && !(unit->au_Drive->id_UDMASupport  & 0x7f00)))
    {
        /*
         * make sure you reduce scan search to pio here!
         * otherwise this and above function will fall into infinite loop
         */
        DINIT(bug("[ATA%02ld] common_SetBestXferMode: DMA is disabled for"
            " this drive.\n", unit->au_UnitNum));
        max = AB_XFER_PIO4;
    }
    else if (!OOP_GET(obj, aHidd_ATABus_Use80Wire))
    {
        DINIT(bug("[ATA%02ld] common_SetBestXferMode: "
            "An 80-wire cable has not been detected for this drive. "
            "Disabling modes above UDMA2.\n", unit->au_UnitNum));
        max = AB_XFER_UDMA2;
    }

    for (iter=max; iter>=AB_XFER_PIO0; --iter)
    {
        if (unit->au_XferModes & (1<<iter))
        {
            common_SetXferMode(unit, iter);
            return;
        }
    }
    bug("[ATA%02ld] common_SetBestXferMode: ERROR: device never reported any valid xfer modes. will continue at default\n", unit->au_UnitNum);
    common_SetXferMode(unit, AB_XFER_PIO0);
}

static void common_DetectXferModes(struct ata_Unit* unit)
{
    int iter;

    DINIT(bug("[ATA%02ld] common_DetectXferModes: Supports\n", unit->au_UnitNum));

    if (unit->au_Drive->id_Commands4 & (1 << 4))
    {
        DINIT(bug("[ATA%02ld] common_DetectXferModes: - Packet interface\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_PACKET;
        unit->au_DirectSCSI     = atapi_DirectSCSI;
    }
    else if (unit->au_Drive->id_Commands5 & (1 << 10))
    {
        /* ATAPI devices do not use this bit. */
        DINIT(bug("[ATA%02ld] common_DetectXferModes: - 48bit I/O\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_48BIT;
    }

    if ((unit->au_XferModes & AF_XFER_PACKET) || (unit->au_Drive->id_Capabilities & (1<< 9)))
    {
        DINIT(bug("[ATA%02ld] common_DetectXferModes: - LBA Addressing\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_LBA;
        unit->au_UseModes      |= AF_XFER_LBA;
    }
    else
    {
        DINIT(bug("[ATA%02ld] common_DetectXferModes: - DEVICE DOES NOT SUPPORT LBA ADDRESSING >> THIS IS A POTENTIAL PROBLEM <<\n", unit->au_UnitNum));
        unit->au_Flags |= AF_CHSOnly;
    }

    if (unit->au_Drive->id_RWMultipleSize & 0xff)
    {
        DINIT(bug("[ATA%02ld] common_DetectXferModes: - R/W Multiple (%ld sectors per xfer)\n", unit->au_UnitNum, unit->au_Drive->id_RWMultipleSize & 0xff));
        unit->au_XferModes     |= AF_XFER_RWMULTI;
    }

    DINIT(bug("[ATA%02ld] common_DetectXferModes: - PIO0 PIO1 PIO2 ",
        unit->au_UnitNum));
    unit->au_XferModes |= AF_XFER_PIO(0) | AF_XFER_PIO(1) | AF_XFER_PIO(2);
    if (unit->au_Drive->id_ConfigAvailable & (1 << 1))
    {
        for (iter = 0; iter < 2; iter++)
        {
            if (unit->au_Drive->id_PIOSupport & (1 << iter))
            {
                DINIT(bug("PIO%ld ", 3 + iter));
                unit->au_XferModes |= AF_XFER_PIO(3 + iter);
            }
        }
        DINIT(bug("\n"));
    }

    if ((unit->au_Drive->id_ConfigAvailable & (1 << 1)) &&
        (unit->au_Drive->id_Capabilities & (1<<8)))
    {
        DINIT(bug("[ATA%02ld] common_DetectXferModes: DMA:\n", unit->au_UnitNum));
        if (unit->au_Drive->id_MWDMASupport & 0xff)
        {
            DINIT(bug("[ATA%02ld] common_DetectXferModes: - ", unit->au_UnitNum));
            for (iter = 0; iter < 3; iter++)
            {
                if (unit->au_Drive->id_MWDMASupport & (1 << iter))
                {
                    unit->au_XferModes |= AF_XFER_MDMA(iter);
                    if (unit->au_Drive->id_MWDMASupport & (256 << iter))
                    {
                        unit->au_UseModes |= AF_XFER_MDMA(iter);
                        DINIT(bug("[MDMA%ld] ", iter));
                    }
                        DINIT(else bug("MDMA%ld ", iter);)
                }
            }
            DINIT(bug("\n"));
        }

        if (unit->au_Drive->id_UDMASupport & 0xff)
        {
            DINIT(bug("[ATA%02ld] common_DetectXferModes: - ", unit->au_UnitNum));
            for (iter = 0; iter < 7; iter++)
            {
                if (unit->au_Drive->id_UDMASupport & (1 << iter))
                {
                    unit->au_XferModes |= AF_XFER_UDMA(iter);
                    if (unit->au_Drive->id_UDMASupport & (256 << iter))
                    {
                        unit->au_UseModes |= AF_XFER_UDMA(iter);
                        DINIT(bug("[UDMA%ld] ", iter));
                    }
                        DINIT(else bug("UDMA%ld ", iter);)
                }
            }
            DINIT(bug("\n"));
        }
    }
}

#if (AROS_BIG_ENDIAN != 0)
/*
 * IDENTIFY data is a block of 256 little endian words. The tables below list
 * the fields which are converted to host byte order after the block has been
 * read, each by the number of its first word.
 */
#define ID_WORD(field)  (offsetof(struct DriveIdent, field) / sizeof(UWORD))

static const UBYTE ata_IdentWords[] =
{
    ID_WORD(id_General),
    ID_WORD(id_OldCylinders),
    ID_WORD(id_SpecificConfig),
    ID_WORD(id_OldHeads),
    ID_WORD(id_OldSectors),
    ID_WORD(id_RWMultipleSize),
    ID_WORD(id_Capabilities),
    ID_WORD(id_OldCaps),
    ID_WORD(id_OldPIO),
    ID_WORD(id_ConfigAvailable),
    ID_WORD(id_OldLCylinders),
    ID_WORD(id_OldLHeads),
    ID_WORD(id_OldLSectors),
    ID_WORD(id_RWMultipleTrans),
    ID_WORD(id_DMADir),
    ID_WORD(id_MWDMASupport),
    ID_WORD(id_PIOSupport),
    ID_WORD(id_MWDMA_MinCycleTime),
    ID_WORD(id_MWDMA_DefCycleTime),
    ID_WORD(id_PIO_MinCycleTime),
    ID_WORD(id_PIO_MinCycleTimeIORDY),
    ID_WORD(id_QueueDepth),
    ID_WORD(id_ATAVersion),
    ID_WORD(id_ATARevision),
    ID_WORD(id_Commands1),
    ID_WORD(id_Commands2),
    ID_WORD(id_Commands3),
    ID_WORD(id_Commands4),
    ID_WORD(id_Commands5),
    ID_WORD(id_Commands6),
    ID_WORD(id_UDMASupport),
    ID_WORD(id_SecurityEraseTime),
    ID_WORD(id_ESecurityEraseTime),
    ID_WORD(id_CurrentAdvPowerMode),
    ID_WORD(id_MasterPwdRevision),
    ID_WORD(id_HWResetResult),
    ID_WORD(id_AcousticManagement),
    ID_WORD(id_StreamMinimunReqSize),
    ID_WORD(id_StreamingTimeDMA),
    ID_WORD(id_StreamingLatency),
    ID_WORD(id_StreamingTimePIO),
    ID_WORD(id_PhysSectorSize),
    ID_WORD(id_RemMediaStatusNotificationFeatures),
    ID_WORD(id_SecurityStatus),
    ID_WORD(id_NominalMediaRotationRate)
};

static const UBYTE ata_IdentLongs[] =
{
    ID_WORD(id_WordsPerLogicalSector),
    ID_WORD(id_LBASectors),
    ID_WORD(id_StreamingGranularity)
};

static const UBYTE ata_IdentQuads[] =
{
    ID_WORD(id_LBA48Sectors)
};

/* Reverse the byte order of the listed fields; size is the size of one field in bytes */
static void ata_SwapIdentFields(struct DriveIdent *id, const UBYTE *fields,
    UWORD count, UWORD size)
{
    while (count--)
    {
        UBYTE *p = (UBYTE *)id + *fields++ * sizeof(UWORD);
        UBYTE *q = p + size;

        while (p < q)
        {
            UBYTE t = *p;

            *p++ = *--q;
            *q = t;
        }
    }
}
#endif

static BYTE ata_Identify(struct ata_Unit *unit)
{
    BOOL atapi = unit->au_Bus->ab_Dev[unit->au_UnitNum & 1] & 0x80;
    BOOL supportLBA, supportLBA48;
    ata_CommandBlock acb =
    {
        atapi ? ATA_IDENTIFY_ATAPI : ATA_IDENTIFY_DEVICE,
        0,
        1,
        0,
        0,
        0,
        unit->au_Drive,
        sizeof(struct DriveIdent),
        0,
        CM_PIORead,
        CT_NoBlock
    };
    UWORD n = 0, *p, *limit;
    BYTE status;

    /* If the right command fails, try the wrong one. If both fail, abort */
    DINIT(
        bug("[ATA%02ld] %s: Executing ATA_IDENTIFY_%s command\n",
        unit->au_UnitNum, __func__, atapi ? "ATAPI" : "DEVICE");
    )
    if ((status = ata_exec_cmd(unit, &acb)) != 0)
    {
        acb.command = atapi ? ATA_IDENTIFY_DEVICE : ATA_IDENTIFY_ATAPI;
        DINIT(
        bug("[ATA%02ld] %s: failed (status = %lx)\n", unit->au_UnitNum, __func__, status);
        //ata_ResetBus(bus);
        bug("[ATA%02ld] %s: Executing ATA_IDENTIFY_%s command"
            " instead\n", unit->au_UnitNum, __func__, atapi ? "DEVICE" : "ATAPI");
        )
        if ((status = ata_exec_cmd(unit, &acb)) != 0)
        {
            DINIT(
            bug("[ATA%02ld] %s: failed (status = %lx)\n", unit->au_UnitNum, __func__, status);
            bug("[ATA%02ld] %s: Both command variants failed."
                " Discarding drive.\n",
                unit->au_UnitNum, __func__);
            )
            //ata_ResetBus(bus);
            return IOERR_OPENFAIL;
        }
        unit->au_Bus->ab_Dev[unit->au_UnitNum & 1] ^= 0x82;
        atapi = unit->au_Bus->ab_Dev[unit->au_UnitNum & 1] & 0x80;
        DINIT(
        bug("[ATA%02ld] %s:"
            " Incorrect device signature detected."
            " Switching device type to %lx.\n",
            unit->au_UnitNum, __func__,
            unit->au_Bus->ab_Dev[unit->au_UnitNum & 1]);
        )
    }
    DINIT(
        bug("[ATA%02ld] %s: IDENTIFY done\n",
        unit->au_UnitNum, __func__);
    )

    /*
     * If every second word is zero with 32-bit reads, switch to 16-bit
     * accesses for this drive and try again
     */
    if (unit->au_Bus->ab_Base->ata_32bit)
    {
        for (p = (UWORD *)unit->au_Drive, limit = p + 256; p < limit; p++)
            n |= *++p;

        if (n == 0)
        {
            DINIT(
            bug("[ATA%02ld] %s: Identify data was invalid with 32-bit reads."
                " Switching to 16-bit mode.\n", unit->au_UnitNum, __func__);
            )

            Unit_Disable32Bit(unit);

            if (ata_exec_cmd(unit, &acb))
                return IOERR_OPENFAIL;
        }
    }

#if (AROS_BIG_ENDIAN != 0)
    ata_SwapIdentFields(unit->au_Drive, ata_IdentWords, sizeof(ata_IdentWords), sizeof(UWORD));
    ata_SwapIdentFields(unit->au_Drive, ata_IdentLongs, sizeof(ata_IdentLongs), sizeof(ULONG));
    ata_SwapIdentFields(unit->au_Drive, ata_IdentQuads, sizeof(ata_IdentQuads), sizeof(UQUAD));
#endif

    DUMP(dump(unit->au_Drive, sizeof(struct DriveIdent)));

    if (atapi)
    {
        unit->au_SectorShift    = 11;
        unit->au_Read32         = atapi_Read;
        unit->au_Write32        = atapi_Write;
        unit->au_DirectSCSI     = atapi_DirectSCSI;
        unit->au_Eject          = atapi_Eject;
        unit->au_DevType        = (unit->au_Drive->id_General >>8) & 0x1f;
        unit->au_XferModes      = AF_XFER_PACKET;
        unit->au_UseModes      |= AF_XFER_PACKET; /* OR because this field may already contain AF_XFER_PIO32 */
    }
    else
    {
        unit->au_SectorShift    = 9;
        unit->au_DevType        = DG_DIRECT_ACCESS;
        unit->au_Read32         = ata_ReadSector32;
        unit->au_Write32        = ata_WriteSector32;
        unit->au_Eject          = ata_Eject;
        unit->au_XferModes      = 0;
        unit->au_Flags         |= AF_DiscPresent | AF_DiscChanged;
    }

    DINIT(bug("[ATA%02ld] %s: copying unit strings ..\n", unit->au_UnitNum, __func__);)

    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

    DINIT(bug("[ATA%02ld] %s: Unit info: %s / %s / %s\n", unit->au_UnitNum, __func__, unit->au_Model, unit->au_SerialNumber, unit->au_FirmwareRev);)
    common_DetectXferModes(unit);
    common_SetBestXferMode(unit);

    if (unit->au_Drive->id_General & 0x80)
    {
        DINIT(bug("[ATA%02ld] %s: Device is removable.\n", unit->au_UnitNum, __func__));
        unit->au_Flags |= AF_Removable;
    }

    supportLBA = (unit->au_Drive->id_Capabilities & (1 << 9)) != 0;
    supportLBA48 = supportLBA && (unit->au_Drive->id_Commands5 & (1 << 10)) != 0;

    unit->au_Capacity = unit->au_Drive->id_LBASectors;
    if (supportLBA48)
        unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;
    else
        unit->au_Capacity48 = unit->au_Capacity;

    DINIT(bug("[ATA%02ld] %s: Unit LBA%d: %07lx 28bit / %04lx:%08lx 48bit addressable blocks\n",
        unit->au_UnitNum, __func__, supportLBA48 ? 48 : (supportLBA ? 28 : 0),
        unit->au_Capacity, (ULONG)(unit->au_Capacity48 >> 32), (ULONG)(unit->au_Capacity48 & 0xfffffffful)));

    if (atapi)
    {
        /*
         * ok, this is not very original, but quite compatible :P
         */
        switch (unit->au_DevType)
        {
            case DG_CDROM:
            case DG_WORM:
            case DG_OPTICAL_DISK:
                unit->au_SectorShift    = 11;
                unit->au_Heads          = 1;
                unit->au_Sectors        = 75;
                unit->au_Cylinders      = 4440;
                break;

            case DG_DIRECT_ACCESS:
                unit->au_SectorShift = 9;
                if (!strcmp("LS-120", &unit->au_Model[0]))
                {
                    unit->au_Heads      = 2;
                    unit->au_Sectors    = 18;
                    unit->au_Cylinders  = 6848;
                }
                else if (!strcmp("ZIP 100 ", &unit->au_Model[8]))
                {
                    unit->au_Heads      = 1;
                    unit->au_Sectors    = 64;
                    unit->au_Cylinders  = 3072;
                }
                break;
        }

        {
            UWORD poll = 0;

            /*
             * Establish the initial media state before publishing the
             * unit. A drive still spinning up after the bus reset reports
             * BECOMING READY; recording that as "no disc" would make the
             * first daemon poll announce the boot disc as a change, so give
             * the drive a bounded time to settle.
             */
            while ((atapi_TestUnitOK(unit) == ATAPI_MEDIA_BECOMING_READY)
                && (++poll < ATAPI_SPINUP_POLLS))
            {
                ata_DelayMicro(unit->au_Bus, ATAPI_SPINUP_DELAY);
            }
        }

        /* Only subsequent changes should notify filesystem handlers. */
        unit->au_Flags &= ~AF_DiscChanged;
    }
    else
    {
        /*
           For drive capacities > 8.3GB assume maximal possible layout.
           It really doesn't matter here, as BIOS will not handle them in
           CHS way anyway :)
           i guess this just solves that weirdo div-by-zero crash, if nothing
           else...
           */
        if (supportLBA && ((unit->au_Drive->id_LBA48Sectors > (63 * 255 * 1024)) ||
            (unit->au_Drive->id_LBASectors > (63 * 255 * 1024))))
        {
            ULONG div = 1;
            /*
             * TODO: this shouldn't be casted down here.
             */
            ULONG sec = unit->au_Capacity48;

            if (sec < unit->au_Capacity48)
                sec = ~((ULONG)0);

            if (sec < unit->au_Capacity)
                sec = unit->au_Capacity;

            unit->au_Sectors = 63;
            sec /= 63;
            /*
             * keep dividing by 2
             */
            do
            {
                if (((sec >> 1) << 1) != sec)
                    break;
                if ((div << 1) > 255)
                    break;
                div <<= 1;
                sec >>= 1;
            } while (1);

            do
            {
                if (((sec / 3) * 3) != sec)
                    break;
                if ((div * 3) > 255)
                    break;
                div *= 3;
                sec /= 3;
            } while (1);

            unit->au_Cylinders  = sec;
            unit->au_Heads      = div;
        }
        else
        {
            unit->au_Cylinders  = unit->au_Drive->id_OldLCylinders;
            unit->au_Heads      = unit->au_Drive->id_OldLHeads;
            unit->au_Sectors    = unit->au_Drive->id_OldLSectors;
            if (!supportLBA) {
                unit->au_Capacity   = unit->au_Cylinders * unit->au_Heads * unit->au_Sectors;
                unit->au_Capacity48 = unit->au_Capacity;
            }
        }
    }

    DINIT(bug("[ATA%02ld] ata_Identify: Unit CHS: %d/%d/%d\n", unit->au_UnitNum, unit->au_Cylinders, unit->au_Heads, unit->au_Sectors));

    return 0;
}

/*
 * ata read and write commands
 *
 * The access functions a unit can be given differ in the command they send,
 * the transfer method, the sector multiplier and the addressing. ATA_RW()
 * packs that description into one word for ata_ReadWrite(), which builds the
 * command block and runs it.
 */
#define RWF_MULTI       0x0100  /* multiplier from IDENTIFY (READ/WRITE MULTIPLE) */
#define RWF_LBA48       0x0200  /* 48 bit command; otherwise LBA28, or CHS on old drives */
#define RW_METHODSHIFT  12      /* the CM_xxx transfer method goes in the top four bits */
#define ATA_RW(command, method, flags) \
    ((command) | ((method) << RW_METHODSHIFT) | (flags))

_Static_assert(CM_DMAWrite < (1 << (16 - RW_METHODSHIFT)),
               "transfer methods must fit the ATA_RW() method field");

static BYTE ata_ReadWrite(struct ata_Unit *unit, UQUAD block, ULONG count,
    APTR buffer, ULONG *act, UWORD mode)
{
    ata_CommandBlock acb =
    {
        mode & 0xff,
        0,
        (mode & RWF_MULTI) ? (unit->au_Drive->id_RWMultipleSize & 0xff) : 1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        mode >> RW_METHODSHIFT,
        (mode & RWF_LBA48) ? CT_LBA48 :
            ((unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28),
    };
    BYTE err;

    D(bug("[ATA%02ld] %s(%04lx)\n", unit->au_UnitNum, __func__, mode));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

/*
 * ata read32 commands
 */
static BYTE ata_ReadSector32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_READ, CM_PIORead, 0));
}

static BYTE ata_ReadMultiple32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_READ_MULTIPLE, CM_PIORead, RWF_MULTI));
}

static BYTE ata_ReadDMA32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_READ_DMA, CM_DMARead, 0));
}

/*
 * ata read64 commands
 */
static BYTE ata_ReadSector64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_READ64, CM_PIORead, RWF_LBA48));
}

static BYTE ata_ReadMultiple64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_READ_MULTIPLE64, CM_PIORead, RWF_MULTI | RWF_LBA48));
}

static BYTE ata_ReadDMA64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_READ_DMA64, CM_DMARead, RWF_LBA48));
}

/*
 * ata write32 commands
 */
static BYTE ata_WriteSector32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_WRITE, CM_PIOWrite, 0));
}

static BYTE ata_WriteMultiple32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_WRITE_MULTIPLE, CM_PIOWrite, RWF_MULTI));
}

static BYTE ata_WriteDMA32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_WRITE_DMA, CM_DMAWrite, 0));
}

/*
 * ata write64 commands
 */
static BYTE ata_WriteSector64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_WRITE64, CM_PIOWrite, RWF_LBA48));
}

static BYTE ata_WriteMultiple64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_WRITE_MULTIPLE64, CM_PIOWrite, RWF_MULTI | RWF_LBA48));
}

static BYTE ata_WriteDMA64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    return ata_ReadWrite(unit, block, count, buffer, act,
        ATA_RW(ATA_WRITE_DMA64, CM_DMAWrite, RWF_LBA48));
}

/*
 * ata miscellaneous commands
 */
static BYTE ata_Eject(struct ata_Unit *unit)
{
    ata_CommandBlock acb =
    {
        ATA_MEDIA_EJECT,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        CM_NoData,
        CT_NoBlock
    };

    D(bug("[ATA%02ld] ata_Eject()\n", unit->au_UnitNum));

    return ata_exec_cmd(unit, &acb);
}

/*
 * atapi commands
 */

/*
 * Issue TEST UNIT READY, update AF_DiscPresent and set AF_DiscChanged
 * when the media state changed. Returns one of the ATAPI_MEDIA_* codes.
 */
int atapi_TestUnitOK(struct ata_Unit *unit)
{
    UBYTE cmd[6] = {
        0
    };
    UBYTE sense[16] = {
        0
    };
    struct SCSICmd sc = {
       0
    };
    UWORD i;
    UBYTE key = 0, asc = 0, ascq = 0;
    BOOL attention = FALSE;
    int state = ATAPI_MEDIA_UNKNOWN;

    D(bug("[ATA%02ld] atapi_TestUnitOK()\n", unit->au_UnitNum));

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_SenseData = (void*)&sense;
    sc.scsi_SenseLength = sizeof(sense);
    sc.scsi_Flags = SCSIF_AUTOSENSE;

    DATAPI(bug("[ATA%02ld] atapi_TestUnitOK: Testing Unit Ready sense...\n", unit->au_UnitNum));

    /* Send command twice, and take the second result, as some drives give
     * invalid (or at least not so useful) sense data straight after reset */
    for (i = 0; i < 2; i++)
    {
        sense[2] = sense[12] = sense[13] = 0;
        key = asc = ascq = 0;

        if (unit->au_DirectSCSI(unit, &sc) == 0)
        {
            state = ATAPI_MEDIA_PRESENT;
            continue;
        }

        /*
         * Only a CHECK CONDITION with returned sense data says anything
         * about the media. A timeout or transport error does not.
         */
        if ((sc.scsi_Status != SCSI_STATUS_CHECK_CONDITION) || (sc.scsi_SenseActual < 3))
        {
            state = ATAPI_MEDIA_UNKNOWN;
            continue;
        }

        key = sense[2] & 0x0f;
        if (sc.scsi_SenseActual > 12)
            asc = sense[12];
        if (sc.scsi_SenseActual > 13)
            ascq = sense[13];

        /*
         * UNIT ATTENTION, NOT READY TO READY CHANGE, MEDIUM MAY HAVE
         * CHANGED is reported once, by the first command after the drive
         * loaded new media, so honour it from either attempt.
         */
        if ((key == 6) && (asc == 0x28))
            attention = TRUE;

        /*
         * per MMC, drives are expected to return 02-3a-0# status, when disc is not present
         * that would translate into following code:
         *    absent = ((key == 2) && (asc == 0x3a));
         * unfortunately, it's what MMC says, not what vendors code, so any
         * NOT READY counts as absent - except BECOMING READY (02-04-01),
         * which a drive reports while it spins up a disc that is present.
         */
        if (key == 2)
            state = ((asc == 0x04) && (ascq == 0x01)) ? ATAPI_MEDIA_BECOMING_READY : ATAPI_MEDIA_ABSENT;
        else
            state = ATAPI_MEDIA_PRESENT;
    }
    unit->au_SenseKey = key;

    /*
     * Leave the recorded state alone while it cannot be determined; the
     * next poll will catch up.
     */
    if ((state == ATAPI_MEDIA_PRESENT) || (state == ATAPI_MEDIA_ABSENT))
    {
        BOOL present = (state == ATAPI_MEDIA_PRESENT);

        if (present != ((unit->au_Flags & AF_DiscPresent) != 0))
        {
            if (present)
                unit->au_Flags |= AF_DiscPresent;
            else
                unit->au_Flags &= ~AF_DiscPresent;

            unit->au_Flags |= AF_DiscChanged;
        }
    }

    /* The disc may have been swapped between two polls */
    if (attention)
        unit->au_Flags |= AF_DiscChanged;

    DATAPI(bug("[ATA%02ld] atapi_TestUnitOK: Test Unit Ready sense: %02x/%02x/%02x, state %d, Media %s%s\n", unit->au_UnitNum, (int)key, (int)asc, (int)ascq, state, unit->au_Flags & AF_DiscPresent ? "PRESENT" : "ABSENT", attention ? " (changed)" : ""));
    return state;
}

/*
 * CMD_READ / CMD_WRITE for PACKET devices are done with READ(10) and
 * WRITE(10), whose transfer length field is 16 bits wide.
 */
static BYTE atapi_ReadWrite(struct ata_Unit *unit, ULONG block, ULONG count,
    APTR buffer, ULONG *act, BOOL write)
{
    BYTE err = 0;

    D(bug("[ATA%02ld] atapi_%s()\n", unit->au_UnitNum, write ? "Write" : "Read"));

    *act = 0;
    while (count > 0)
    {
        ULONG part = (count > 0xffff) ? 0xffff : count;
        UBYTE cmd[10] = {
           write ? SCSI_WRITE10 : SCSI_READ10, 0, block>>24, block>>16, block>>8, block, 0, part>>8, part, 0
        };
        struct SCSICmd sc = {
           0
        };

        sc.scsi_Command = (void*) &cmd;
        sc.scsi_CmdLength = sizeof(cmd);
        sc.scsi_Data = buffer;
        sc.scsi_Length = part << unit->au_SectorShift;
        sc.scsi_Flags = write ? SCSIF_WRITE : SCSIF_READ;

        err = unit->au_DirectSCSI(unit, &sc);
        *act += sc.scsi_Actual;
        if (err != 0)
            break;
        if (sc.scsi_Actual != sc.scsi_Length)
        {
            /* Short transfer with good status: do not pretend the buffer is valid */
            err = TDERR_NotSpecified;
            break;
        }

        buffer = (APTR)((IPTR)buffer + sc.scsi_Length);
        block += part;
        count -= part;
    }

    return err;
}

static BYTE atapi_Read(struct ata_Unit *unit, ULONG block, ULONG count,
    APTR buffer, ULONG *act)
{
    return atapi_ReadWrite(unit, block, count, buffer, act, FALSE);
}

static BYTE atapi_Write(struct ata_Unit *unit, ULONG block, ULONG count,
    APTR buffer, ULONG *act)
{
    return atapi_ReadWrite(unit, block, count, buffer, act, TRUE);
}

static BYTE atapi_Eject(struct ata_Unit *unit)
{
    struct atapi_StartStop cmd = {
        .command    = SCSI_STARTSTOP,
        .immediate  = 1,
        .flags      = ATAPI_SS_EJECT,
    };

    struct SCSICmd sc = {
       0
    };

    D(bug("[ATA%02ld] atapi_Eject()\n", unit->au_UnitNum));

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_Flags = SCSIF_READ;

    return unit->au_DirectSCSI(unit, &sc);
}

/*
 * REQUEST SENSE, used for autosense. Goes straight to the transport so
 * that it can never recurse into another autosense.
 */
static ULONG atapi_RequestSense(struct ata_Unit* unit, UBYTE* sense,
    ULONG senselen, ULONG *actual)
{
    UBYTE cmd[6] = {
       SCSI_REQUESTSENSE, 0, 0, 0, 0, 0
    };
    struct atapi_Packet pkt;

    D(bug("[ATA%02ld] atapi_RequestSense()\n", unit->au_UnitNum));

    *actual = 0;
    if ((senselen == 0) || (sense == NULL))
       return 0;

    /* The allocation length is a single byte */
    if (senselen > 255)
        senselen = 255;
    cmd[4] = senselen;

    pkt.ap_CDB     = cmd;
    pkt.ap_CDBLen  = sizeof(cmd);
    pkt.ap_Data    = sense;
    pkt.ap_DataLen = senselen;
    pkt.ap_Write   = FALSE;
    pkt.ap_DMA     = FALSE;

    atapi_SendPacket(unit, &pkt);
    *actual = pkt.ap_Actual;

    DATAPI(dump(sense, pkt.ap_Actual));
    if (pkt.ap_Actual < 14)
        return 0;

    DATAPI(bug("[SENSE] atapi_RequestSense: sensed data: %lx %lx %lx\n", sense[2]&0xf, sense[12], sense[13]));
    return ((sense[2]&0xf)<<16) | (sense[12]<<8) | (sense[13]);
}

static ULONG ata_ReadSignature(struct ata_Bus *bus, int unit,
    BOOL *DiagExecuted)
{
    UBYTE tmp1, tmp2;

    D(bug("[ATA  ] %s(%02ld)\n", __func__, unit));

    PIO_Out(bus, DEVHEAD_VAL | (unit << 4), ata_DevHead);
    ata_Wait400ns(bus);
    //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

    DINIT(bug("[ATA  ] %s: Status %02lx Device %02lx\n",
          __func__, ata_ReadStatus(bus), PIO_In(bus, ata_DevHead)));

    /* Ok, ATA/ATAPI device. Get detailed signature */
    DINIT(bug("[ATA  ] %s: ATA[PI] device present. Attempting to detect specific subtype\n", __func__));

    tmp1 = PIO_In(bus, ata_LBAMid);
    tmp2 = PIO_In(bus, ata_LBAHigh);

    DINIT(bug("[ATA  ] %s: Subtype check returned %02lx:%02lx (%04lx)\n", __func__, tmp1, tmp2, (tmp1 << 8) | tmp2));

    switch ((tmp1 << 8) | tmp2)
    {
        case 0x14eb:
            DINIT(bug("[ATA  ] %s: Found signature for ATAPI device\n", __func__));
            return DEV_ATAPI;

        case 0x3cc3:
            DINIT(bug("[ATA  ] %s: Found signature for SATA device\n", __func__));
            return DEV_SATA;

        case 0x6996:
            DINIT(bug("[ATA  ] %s: Found signature for SATAPI device\n", __func__));
            return DEV_SATAPI;

        default:
            if (0 == (ata_ReadStatus(bus) & 0xfe)) {
                DINIT(bug("[ATA  ] %s: Found NONE\n", __func__));
                return DEV_NONE;
            }

            /* ATA_EXECUTE_DIAG is executed by both devices, do it only once */
            if (!*DiagExecuted)
            {
                DINIT(bug("[ATA  ] %s: ATA_EXECUTE_DIAG\n", __func__));
                PIO_Out(bus, ATA_EXECUTE_DIAG, ata_Command);
                *DiagExecuted = TRUE;
            }

            ata_DelayMicro(bus, 2000);
            while (ata_ReadStatus(bus) & ATAF_BUSY)
                ata_Wait400ns(bus);
                //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

            DINIT(bug("[ATA  ] %s: ATAF_BUSY wait finished\n", __func__));

            PIO_Out(bus, DEVHEAD_VAL | (unit << 4), ata_DevHead);
            do
            {
                ata_Wait400ns(bus);
                //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);
            }
            while (0 != (ATAF_BUSY & ata_ReadStatus(bus)));
            DINIT(bug("[ATA  ] %s: Further validating ATA signature: %lx & 0x7f = 1, %lx & 0x10 = unit\n",
                      __func__, PIO_In(bus, ata_Error), PIO_In(bus, ata_DevHead)));

            if ((PIO_In(bus, ata_Error) & 0x7f) == 1)
            {
                DINIT(bug("[ATA  ] %s: Found *valid* signature for ATA device\n", __func__));
                /* this might still be an (S)ATAPI device, but we correct that in ata_Identify */
                return DEV_ATA;
            }
            DERROR(bug("[ATA  ] %s: Found signature for ATA "
                "device, but further validation failed\n", __func__));
            return DEV_NONE;
    }
}

static void ata_ResetBus(struct ata_Bus *bus)
{
    ULONG TimeOut;
    BOOL  DiagExecuted = FALSE;
    UWORD i;

    /*
     * Set and then reset the soft reset bit in the Device Control
     * register. This causes device 0 to be selected.
     */
    DINIT(bug("[ATA  ] %s()\n", __func__));

    PIO_Out(bus, DEVHEAD_VAL, ata_DevHead); /* Select it never the less */
    ata_Wait400ns(bus);
    //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

    if (bus->haveAltIO)
    {
        PIO_OutAlt(bus, ATACTLF_RESET | ATACTLF_INT_DISABLE, ata_AltControl);
        ata_DelayMicro(bus, 10);    /* sleep 10us; min: 5us */

        PIO_OutAlt(bus, ATACTLF_INT_DISABLE, ata_AltControl);
    }
    else
    {
        PIO_Out(bus, ATA_EXECUTE_DIAG, ata_Command);
    }
    ata_DelayMicro(bus, 20000); /* sleep 20ms; min: 2ms */

    /* If there is a device 0, wait for device 0 to clear BSY */
    if (DEV_NONE != bus->ab_Dev[0])
    {
        DINIT(bug("[ATA  ] %s: Wait for master to clear BSY\n", __func__));
        TimeOut = 1000;     /* Timeout 1s (1ms x 1000) */

        while ( 1 )
        {
            if ((ata_ReadStatus(bus) & ATAF_BUSY) == 0)
                break;
            ata_DelayMicro(bus, 1000);
            if (!(--TimeOut)) {
                DINIT(bug("[ATA%02ld] %s: Master device Timed Out!\n", __func__));
                bus->ab_Dev[0] = DEV_NONE;
                break;
            }
        }
        DINIT(bug("[ATA  ] %s: Wait left after %d ms\n", __func__, 1000 - TimeOut));
    }

    /* If there is a device 1, wait some time until device 1 allows
     * register access, but fail only if BSY isn't cleared */
    if (DEV_NONE != bus->ab_Dev[1])
    {
        DINIT(bug("[ATA  ] %s: Wait DEV1 to allow access\n", __func__));
        PIO_Out(bus, DEVHEAD_VAL | (1 << 4), ata_DevHead);
        ata_Wait400ns(bus);
        //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

        TimeOut = 50;     /* Timeout 50ms (1ms x 50) */
        while ( 1 )
        {
            if ((PIO_In(bus, ata_Count) == 0x01) && (PIO_In(bus, ata_LBALow) == 0x01))
                break;
            ata_DelayMicro(bus, 1000);
            if (!(--TimeOut))
            {
                DINIT(bug("[ATA  ] %s: DEV1 1/2 TimeOut!\n", __func__));
                break;
            }
        }
        DINIT(bug("[ATA  ] %s: DEV1 1/2 Wait left after %d ms\n", __func__, 1000 - TimeOut));

        DINIT(bug("[ATA  ] %s: Wait for slave to clear BSY\n", __func__));
        TimeOut = 1000;     /* Timeout 1s (1ms x 1000) */
        while ( 1 )
        {
            if ((ata_ReadStatus(bus) & ATAF_BUSY) == 0)
                break;
            ata_DelayMicro(bus, 1000);
            if (!(--TimeOut)) {
                DINIT(bug("[ATA  ] %s: Slave device Timed Out!\n", __func__));
                bus->ab_Dev[1] = DEV_NONE;
                break;
            }
        }
        DINIT(bug("[ATA  ] %s: Wait left after %d ms\n", __func__, 1000 - TimeOut));
    }

    for (i = 0; i < MAX_BUSUNITS; i++)
    {
        if (DEV_NONE != bus->ab_Dev[i])
        {
            UBYTE sigtype = ata_ReadSignature(bus, i, &DiagExecuted);
            /* Only alter the type if we get a better type probed from the signature ..*/
            if (sigtype > bus->ab_Dev[i])
            {
                bus->ab_Dev[i] = sigtype;
            }
        }
    }
}

void ata_InitBus(struct ata_Bus *bus)
{
    struct ataBase *ATABase = bus->ab_Base;
    OOP_Object *obj = OOP_OBJECT(ATABase->busClass, bus);
    IPTR haveAltIO;
    /* written alternately to LBALow and LBAMid */
    static const UBYTE ata_ProbePattern[] = { 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa };
    UBYTE tmp1, tmp2;
    UWORD i, j;

    /*
     * initialize timer for the sake of scanning
     */
    if ((bus->ab_Timer = ata_OpenTimer(bus->ab_Base)) != NULL)
    {
        OOP_GetAttr(obj, aHidd_ATABus_UseIOAlt, &haveAltIO);
        bus->haveAltIO = haveAltIO != 0;

        DINIT(bug("[ATA  ] %s(%p)\n", __func__, bus));

        bus->ab_Dev[0] = DEV_NONE;
        bus->ab_Dev[1] = DEV_NONE;

        /* Check if device 0 and/or 1 is present on this bus. It may happen that
           a single drive answers for both device addresses, but the phantom
           drive will be filtered out later */
        for (i = 0; i < MAX_BUSUNITS; i++)
        {
            /* Select device and disable IRQs */
            PIO_Out(bus, DEVHEAD_VAL | (i << 4), ata_DevHead);
            ata_DelayMicro(bus, 400);
            PIO_OutAlt(bus, ATACTLF_INT_DISABLE, ata_AltControl);

            /* Write some pattern to registers. This is a variant of a more
               common technique, with the difference that we don't use the
               sector count register because some bad ATAPI drives disallow
               writing to it */
            for (j = 0; j < sizeof(ata_ProbePattern); j++)
                PIO_Out(bus, ata_ProbePattern[j], (j & 1) ? ata_LBAMid : ata_LBALow);

            tmp1 = PIO_In(bus, ata_LBALow);
            tmp2 = PIO_In(bus, ata_LBAMid);
            DB2(bug("[ATA  ] %s: Reply 0x%02X 0x%02X\n", __func__, tmp1, tmp2));

            if ((tmp1 == 0x55) && (tmp2 == 0xaa))
                bus->ab_Dev[i] = DEV_UNKNOWN;
            DINIT(bug("[ATA  ] %s: Device type = 0x%02X\n", __func__, bus->ab_Dev[i]));
        }

        ata_ResetBus(bus);
        ata_CloseTimer(bus->ab_Timer);
    }
    DINIT(bug("[ATA  ] %s: Finished\n", __func__));
}
