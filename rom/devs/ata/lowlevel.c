/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*
 * TODO:
 * - put a critical section around DMA transfers (shared dma channels)
 */
 
// use #define xxx(a) D(a) to enable particular sections.
#if DEBUG
#define DIRQ(a) D(a)
#define DIRQ_MORE(a)
#define DUMP(a) D(a)
#define DUMP_MORE(a)
#define DATA(a) D(a)
#define DATAPI(a) D(a)
#define DINIT(a) (a)
#else
#define DIRQ(a)      do { } while (0)
#define DIRQ_MORE(a) do { } while (0)
#define DUMP(a)      do { } while (0)
#define DUMP_MORE(a) do { } while (0)
#define DATA(a)      do { } while (0)
#define DATAPI(a)    do { } while (0)
#define DINIT(a)
#endif
/* Errors that shouldn't happen */
#define DERROR(a) a

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <devices/timer.h>

#include "ata.h"
#include "ata_bus.h"
#include "timer.h"

/*
    Prototypes of static functions from lowlevel.c. I do not want to make them
    non-static as I'd like to remove as much symbols from global table as possible.
    Besides some of this functions could conflict with old ide.device or any other
    device.
*/
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
static BYTE ata_Eject(struct ata_Unit *);
static BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout, BOOL irq, UBYTE *stout);

static BYTE atapi_EndCmd(struct ata_Unit *unit);

static BYTE atapi_Read(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE atapi_Write(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static BYTE atapi_Eject(struct ata_Unit *);

static void common_SetBestXferMode(struct ata_Unit* unit);

#define DEVHEAD_VAL 0x40

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

static inline UBYTE ata_ReadStatus(struct ata_Bus *bus)
{
    return PIO_In(bus, ata_Status);
}

static inline UBYTE ata_ReadAltStatus(struct ata_Bus *bus)
{
    return PIO_InAlt(bus, ata_AltStatus);
}

static inline BOOL ata_SelectUnit(struct ata_Unit* unit)
{
    struct ata_Bus *bus = unit->au_Bus;

    if (unit == bus->ab_SelectedUnit)
        return TRUE;

    PIO_Out(bus, unit->au_DevMask, ata_DevHead);

    do
    {
        ata_WaitNano(400, bus->ab_Base);
        //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);
    }
    while (0 != (ATAF_BUSY & ata_ReadStatus(bus)));

    bus->ab_SelectedUnit = unit;

    return TRUE;
}

/*
 * handle IRQ; still fast and efficient, supposed to verify if this irq is for us and take adequate steps
 * part of code moved here from ata.c to reduce containment
 */
void ata_IRQSignalTask(struct ata_Bus *bus)
{
    bus->ab_IntCnt++;
    Signal(bus->ab_Task, 1UL << bus->ab_SleepySignal);
}

void ata_IRQSetHandler(struct ata_Unit *unit, void (*handler)(struct ata_Unit*, UBYTE), APTR piomem, ULONG blklen, ULONG piolen)
{
    if (NULL != handler)
        unit->au_cmd_error = 0;

    unit->au_cmd_data = piomem;
    unit->au_cmd_length = (piolen < blklen) ? piolen : blklen;
    unit->au_cmd_total = piolen;
    unit->au_Bus->ab_HandleIRQ = handler;
}

void ata_IRQNoData(struct ata_Unit *unit, UBYTE status)
{
    if (status & ATAF_BUSY)
        return;

    if ((unit->au_cmd_error == 0) && (status & ATAF_ERROR))
        unit->au_cmd_error = HFERR_BadStatus;

    DIRQ(bug("[ATA%02ld] IRQ: NoData - done; status %02lx.\n", unit->au_UnitNum, status));
    ata_IRQSetHandler(unit, NULL, NULL, 0, 0);
    ata_IRQSignalTask(unit->au_Bus);
}

void ata_IRQPIORead(struct ata_Unit *unit, UBYTE status)
{
    if (status & ATAF_DATAREQ)
    {
	DIRQ(bug("[ATA  ] IRQ: PIOReadData - DRQ.\n"));

        Unit_InS(unit, unit->au_cmd_data, unit->au_cmd_length);

	/*
	 * indicate it's all done here
	 */
	unit->au_cmd_data += unit->au_cmd_length;
	unit->au_cmd_total -= unit->au_cmd_length;
	if (unit->au_cmd_total) {
	    if (unit->au_cmd_length > unit->au_cmd_total)
		unit->au_cmd_length = unit->au_cmd_total;
	    return;
	}
	DIRQ(bug("[ATA  ] IRQ: PIOReadData - transfer completed.\n"));
    }
    ata_IRQNoData(unit, status);
}

void ata_PIOWriteBlk(struct ata_Unit *unit)
{
    Unit_OutS(unit, unit->au_cmd_data, unit->au_cmd_length);

    /*
     * indicate it's all done here
     */
    unit->au_cmd_data += unit->au_cmd_length;
    unit->au_cmd_total -= unit->au_cmd_length;
    if (unit->au_cmd_length > unit->au_cmd_total)
        unit->au_cmd_length = unit->au_cmd_total;
}

void ata_IRQPIOWrite(struct ata_Unit *unit, UBYTE status)
{
    if (status & ATAF_DATAREQ) {
	DIRQ(bug("[ATA  ] IRQ: PIOWriteData - DRQ.\n"));
	ata_PIOWriteBlk(unit);
	return;
    }
    DIRQ(bug("[ATA  ] IRQ: PIOWriteData - done.\n"));
    ata_IRQNoData(unit, status);
}

void ata_IRQDMAReadWrite(struct ata_Unit *unit, UBYTE status)
{
    struct ata_Bus *bus = unit->au_Bus;
    ULONG stat = DMA_GetResult(bus);

    DIRQ(bug("[ATA%02ld] IRQ: IO status %02lx, DMA status %02lx\n", unit->au_UnitNum, status, stat));

    if ((status & ATAF_ERROR) || stat)
    {
        /* This is turned on in order to help Phantom - Pavel Fedin <sonic_amiga@rambler.ru> */
        DERROR(bug("[ATA%02ld] IRQ: IO status %02lx, DMA status %d\n", unit->au_UnitNum, status, stat));
        DERROR(bug("[ATA%02ld] IRQ: ERROR %02lx\n", unit->au_UnitNum, PIO_In(bus, atapi_Error)));
        DERROR(bug("[ATA  ] IRQ: DMA Failed.\n"));

        unit->au_cmd_error = stat;
        ata_IRQNoData(unit, status);
    }
    else if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
    {
        DIRQ(bug("[ATA  ] IRQ: DMA Done.\n"));
        ata_IRQNoData(unit, status);
    }
}

void ata_IRQPIOReadAtapi(struct ata_Unit *unit, UBYTE status)
{
    struct ata_Bus *bus = unit->au_Bus;
    ULONG size = 0;
    LONG remainder = 0;
    UBYTE reason = PIO_In(bus, atapi_Reason);
    DIRQ(bug("[DSCSI] Current status: %ld during READ\n", reason));

    /* have we failed yet? */
    if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
        ata_IRQNoData(unit, status);
    if (status & ATAF_ERROR)
    {
        ata_IRQNoData(unit, status);
        return;
    }

    /* anything for us please? */
    if (ATAPIF_READ != (reason & ATAPIF_MASK))
        return;

    size = PIO_In(bus, atapi_ByteCntH) << 8 | PIO_In(bus, atapi_ByteCntL);
    DIRQ(bug("[ATAPI] IRQ: data available for read (%ld bytes, max: %ld bytes)\n", size, unit->au_cmd_total));

    if (size > unit->au_cmd_total)
    {
        DERROR(bug("[ATAPI] IRQ: CRITICAL! MORE DATA OFFERED THAN STORAGE CAN TAKE: %ld bytes vs %ld bytes left!\n", size, unit->au_cmd_total));
        remainder = size - unit->au_cmd_total;
        size = unit->au_cmd_total;
    }

    Unit_InS(unit, unit->au_cmd_data, size);

    unit->au_cmd_data = &((UBYTE*)unit->au_cmd_data)[size];
    unit->au_cmd_total -= size;

    DIRQ(bug("[ATAPI] IRQ: %lu bytes read.\n", size));

    /*
     * Soak up excess bytes.
     */
    for (; remainder > 0; remainder -= 2)
        Unit_InS(unit, &size, 2);

    if (unit->au_cmd_total == 0)
        ata_IRQSetHandler(unit, &ata_IRQNoData, NULL, 0, 0);
}

void ata_IRQPIOWriteAtapi(struct ata_Unit *unit, UBYTE status)
{
    struct ata_Bus *bus = unit->au_Bus;
    ULONG size = 0;
    UBYTE reason = PIO_In(bus, atapi_Reason);

    DIRQ(bug("[ATAPI] IRQ: Current status: %ld during WRITE\n", reason));

    /*
     * have we failed yet?
     * CHECKME: This sequence actually can trigger ata_IRQNoData() twice.
     * Is this correct ?
     */
    if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
        ata_IRQNoData(unit, status);
    if (status & ATAF_ERROR)
    {
        ata_IRQNoData(unit, status);
        return;
    }

    /* anything for us please? */
    if (ATAPIF_WRITE != (reason & ATAPIF_MASK))
        return;

    size = PIO_In(bus, atapi_ByteCntH) << 8 | PIO_In(bus, atapi_ByteCntL);
    DIRQ(bug("[ATAPI] IRQ: data requested for write (%ld bytes, max: %ld bytes)\n", size, unit->au_cmd_total));

    if (size > unit->au_cmd_total)
    {
        DERROR(bug("[ATAPI] IRQ: CRITICAL! MORE DATA REQUESTED THAN STORAGE CAN GIVE: %ld bytes vs %ld bytes left!\n", size, unit->au_cmd_total));
        size = unit->au_cmd_total;
    }

    Unit_OutS(unit, unit->au_cmd_data, size);
    unit->au_cmd_data = &((UBYTE*)unit->au_cmd_data)[size];
    unit->au_cmd_total -= size;

    DIRQ(bug("[ATAPI] IRQ: %lu bytes written.\n", size));

    if (unit->au_cmd_total == 0)
        ata_IRQSetHandler(unit, &ata_IRQNoData, NULL, 0, 0);
}

/*
 * wait for timeout or drive ready
 */
BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout, BOOL irq, UBYTE *stout)
{
    struct ata_Bus *bus = unit->au_Bus;
    UBYTE status;
    ULONG sigs = SIGBREAKF_CTRL_C;
    ULONG step = 0;
    BOOL res = TRUE;

    if (bus->ab_Base->ata_Poll)
        irq = FALSE;

    /* FIXME: This shouldn't happen but it could explain reported random -6 (IOERR_UNITBUSY) problem */
    if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    	DERROR(bug("[ATA%02ld] SIGBREAKF_CTRL_C was already set!?\n", unit->au_UnitNum));
 
    /*
     * set up bus timeout
     */
    bus->ab_Timeout = tout;

    sigs |= (irq ? (1 << bus->ab_SleepySignal) : 0);
    status = PIO_InAlt(bus, ata_AltStatus);

    if (irq)
    {
        /*
         * wait for either IRQ or TIMEOUT (unless device seems to be a
         * phantom SATAPI drive, in which case we fake a timeout)
         */
        DIRQ(bug("[ATA%02ld] Waiting (Current status: %02lx)...\n",
            unit->au_UnitNum, status));
        if (status != 0)
            step = Wait(sigs);
        else
            step = SIGBREAKF_CTRL_C;

        /*
         * now if we did reach timeout, then there's no point in going ahead.
         */
        if (SIGBREAKF_CTRL_C & step)
        {
            DERROR(bug("[ATA%02ld] Timeout while waiting for device to complete"
                " operation\n", unit->au_UnitNum));
            res = FALSE;

            /* do nothing if the interrupt eventually arrives */
            Disable();
            ata_IRQSetHandler(unit, NULL, NULL, 0, 0);
            Enable();
        }
    }
    else
    {
        while (status & ATAF_BUSY)
        {
            ++step;

            /*
             * every 16n rounds do some extra stuff
             */
            if ((step & 15) == 0)
            {
                /*
                 * huhm. so it's been 16n rounds already. any timeout yet?
                 */
                if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
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
                ata_WaitNano(400, bus->ab_Base);
            }

            status = PIO_InAlt(bus, ata_AltStatus);
        }
    }

    /*
     * clear up all our expectations
     */
    bus->ab_Timeout = -1;

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
    SetSignal(0, sigs);

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

    if (FALSE == ata_SelectUnit(unit))
        return IOERR_UNITBUSY;

    switch (block->type)
    {
        case CT_CHS:
        case CT_LBA28:
            if (block->sectors > 256)
            {
                DERROR(bug("[ATA%02ld] ata_exec_cmd: ERROR: Transfer length (%ld) exceeds 256 sectors. Aborting.\n", unit->au_UnitNum, block->sectors));
                return IOERR_BADLENGTH;
            }

            /* note:
             * we want the above to fall in here!
             * we really do (checking for secmul)
             */

        case CT_LBA48:
            if (block->sectors > 65536)
            {
                DERROR(bug("[ATA%02ld] ata_exec_cmd: ERROR: Transfer length (%ld) exceeds 65536 sectors. Aborting.\n", unit->au_UnitNum, block->sectors));
                return IOERR_BADLENGTH;
            }
            if (block->secmul == 0)
            {
                DERROR(bug("[ATA%02ld] ata_exec_cmd: ERROR: Invalid transfer multiplier. Should be at least set to 1 (correcting)\n", unit->au_UnitNum));
                block->secmul = 1;
            }
           break;

        case CT_NoBlock:
            break;

        default:
            DERROR(bug("[ATA%02ld] ata_exec_cmd: ERROR: Invalid command type %lx. Aborting.\n", unit->au_UnitNum, block->type));
            return IOERR_NOCMD;
    }

    block->actual = 0;
    D(bug("[ATA%02ld] ata_exec_cmd: Executing command %02lx\n", unit->au_UnitNum, block->command));

    if (block->feature != 0)
        PIO_Out(bus, block->feature, ata_Feature);

    /*
     * - set LBA and sector count
     */
    switch (block->type)
    {
        case CT_CHS:
            DATA(bug("[ATA%02ld] ata_exec_cmd: Command uses CHS addressing (OLD)\n", unit->au_UnitNum));
            {
                ULONG cyl, head, sector;
                ULONG tmp = unit->au_Heads * unit->au_Sectors;
                cyl = block->blk / tmp;
                head = (block->blk % tmp) / unit->au_Sectors;
                sector = (block->blk % unit->au_Sectors) + 1;

                PIO_Out(bus, ((head) & 0x0f) | unit->au_DevMask, ata_DevHead);
                PIO_Out(bus, sector, ata_Sector);
                PIO_Out(bus, cyl & 0xff, ata_CylinderLow);
                PIO_Out(bus, (cyl >> 8) & 0xff, ata_CylinderHigh);
                PIO_Out(bus, block->sectors, ata_Count);
            }
            break;
        case CT_LBA28:
            DATA(bug("[ATA%02ld] ata_exec_cmd: Command uses 28bit LBA addressing (OLD)\n", unit->au_UnitNum));

            PIO_Out(bus, ((block->blk >> 24) & 0x0f) | DEVHEAD_VAL | unit->au_DevMask, ata_DevHead);
            PIO_Out(bus, block->blk >> 16, ata_LBAHigh);
            PIO_Out(bus, block->blk >> 8, ata_LBAMid);
            PIO_Out(bus, block->blk, ata_LBALow);
            PIO_Out(bus, block->sectors, ata_Count);
            break;

        case CT_LBA48:
            DATA(bug("[ATA%02ld] ata_exec_cmd: Command uses 48bit LBA addressing (NEW)\n", unit->au_UnitNum));

            PIO_Out(bus, 0x40 | unit->au_DevMask, ata_DevHead);
            PIO_Out(bus, block->blk >> 40, ata_LBAHigh);
            PIO_Out(bus, block->blk >> 32, ata_LBAMid);
            PIO_Out(bus, block->blk >> 24, ata_LBALow);

            PIO_Out(bus, block->blk >> 16, ata_LBAHigh);
            PIO_Out(bus, block->blk >> 8, ata_LBAMid);
            PIO_Out(bus, block->blk, ata_LBALow);

            PIO_Out(bus, block->sectors >> 8, ata_Count);
            PIO_Out(bus, block->sectors, ata_Count);
            break;

        case CT_NoBlock:
            DATA(bug("[ATA%02ld] ata_exec_cmd: Command does not address any block\n", unit->au_UnitNum));
            break;
    }

    switch (block->method)
    {
        case CM_PIOWrite:
            ata_IRQSetHandler(unit, &ata_IRQPIOWrite, mem, block->secmul << unit->au_SectorShift, block->length);
            break;

        case CM_PIORead:
            ata_IRQSetHandler(unit, &ata_IRQPIORead, mem, block->secmul << unit->au_SectorShift, block->length);
            break;

        case CM_DMARead:
            if (FALSE == DMA_Setup(bus, mem, block->length, TRUE))
                return IOERR_ABORTED;

            ata_IRQSetHandler(unit, &ata_IRQDMAReadWrite, NULL, 0, 0);
            DMA_Start(bus);
            break;

        case CM_DMAWrite:
            if (FALSE == DMA_Setup(bus, mem, block->length, FALSE))
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
    DATA(bug("[ATA%02ld] ata_exec_cmd: Sending command\n", unit->au_UnitNum));

    PIO_Out(bus, block->command, ata_Command);
    ata_WaitNano(400, bus->ab_Base);
    //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);

    /*
     * In case of PIO write the drive won't issue an IRQ before first
     * data transfer, so we should poll the status and send the first
     * block upon request.
     */
    if (block->method == CM_PIOWrite)
    {
	if (FALSE == ata_WaitBusyTO(unit, TIMEOUT, FALSE, &status)) {
	    DERROR(bug("[ATA%02ld] ata_exec_cmd: PIOWrite - no response from device. Status %02X\n", unit->au_UnitNum, status));
	    return IOERR_UNITBUSY;
	}
	if (status & ATAF_DATAREQ) {
	    DATA(bug("[ATA%02ld] ata_exec_cmd: PIOWrite - DRQ.\n", unit->au_UnitNum));
	    ata_PIOWriteBlk(unit);
	}
	else
	{
	    DERROR(bug("[ATA%02ld] ata_exec_cmd: PIOWrite - bad status: %02X\n", status));
	    return HFERR_BadStatus;
	}
    }

    /*
     * wait for drive to complete what it has to do
     */
    if (FALSE == ata_WaitBusyTO(unit, TIMEOUT, TRUE, NULL))
    {
        DERROR(bug("[ATA%02ld] ata_exec_cmd: Device is late - no response\n", unit->au_UnitNum));
        err = IOERR_UNITBUSY;
    }
    else
        err = unit->au_cmd_error;

    DATA(bug("[ATA%02ld] ata_exec_cmd: Command done\n", unit->au_UnitNum));

    /*
     * clean up DMA
     * don't use 'mem' pointer here as it's already invalid.
     */
    switch (block->method)
    {
    case CM_DMARead:
        DMA_End(bus, block->buffer, block->length, TRUE);
        break;

    case CM_DMAWrite:
        DMA_End(bus, block->buffer, block->length, FALSE);
        break;
    
    default:
        break; /* Shut up the compiler */
    }

    D(bug("[ATA%02ld] ata_exec_cmd: return code %ld\n", unit->au_UnitNum, err));
    return err;
}

/*
 * atapi packet iface
 */
BYTE atapi_SendPacket(struct ata_Unit *unit, APTR packet, APTR data, LONG datalen, BOOL *dma, BOOL write)
{
    struct ata_Bus *bus = unit->au_Bus;
    *dma = *dma && (unit->au_Flags & AF_DMA) ? TRUE : FALSE;
    LONG err = 0;

    UBYTE cmd[12] = {
        0
    };
    register int t=5,l=0;

    if (((UBYTE*)packet)[0] > 0x1f)
        t+= 4;
    if (((UBYTE*)packet)[0] > 0x5f)
        t+= 2;

    switch (((UBYTE*)packet)[0])
    {
        case 0x28:  // read10
        case 0xa8:  // read12
        case 0xbe:  // readcd
        case 0xb9:  // readcdmsf
        case 0x2f:  // verify
        case 0x2a:  // write
        case 0xaa:  // write12
        case 0x2e:  // writeverify
        case 0xad:  // readdvdstructure
        case 0xa4:  // reportkey
        case 0xa3:  // sendkey
            break;
        default:
            *dma = FALSE;
    }

    while (l<=t)
    {
        cmd[l] = ((UBYTE*)packet)[l];
        ++l;
    }

    DATAPI({
        bug("[ATA%02lx] Sending %s ATA packet: ", unit->au_UnitNum, (*dma) ? "DMA" : "PIO");
        l=0;
        while (l<=t)
        {
            bug("%02lx ", ((UBYTE*)cmd)[l]);
            ++l;
        }
        bug("\n");

        if (datalen & 1)
            bug("[ATAPI] ERROR: DATA LENGTH NOT EVEN! Rounding Up! (%ld bytes requested)\n", datalen);
    });

    datalen = (datalen+1)&~1;

    if (FALSE == ata_SelectUnit(unit))
    {
        DATAPI(bug("[ATAPI] WaitBusy failed at first check\n"));
        return IOERR_UNITBUSY;
    }

    /*
     * tell device whether we want to read or write and if we want a dma transfer
     */
    PIO_Out(bus,
            ((*dma) ? 1 : 0) | (((unit->au_Drive->id_DMADir & 0x8000) && !write) ? 4 : 0),
            atapi_Features);
    PIO_Out(bus, (datalen & 0xff), atapi_ByteCntL);
    PIO_Out(bus, (datalen >> 8) & 0xff, atapi_ByteCntH);

    /*
     * once we're done with that, we can go ahead and inform device that we're about to send atapi packet
     * after command is dispatched, we are obliged to give 400ns for the unit to parse command and set status
     */
    DATAPI(bug("[ATAPI] Issuing ATA_PACKET command.\n"));
    ata_IRQSetHandler(unit, &ata_IRQNoData, 0, 0, 0);
    PIO_Out(bus, ATA_PACKET, atapi_Command);
    ata_WaitNano(400, bus->ab_Base);
    //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);

    ata_WaitBusyTO(unit, TIMEOUT, (unit->au_Drive->id_General & 0x60) == 0x20,
        NULL);
    if (0 == (ata_ReadStatus(bus) & ATAF_DATAREQ))
        return HFERR_BadStatus;

    /*
     * setup appropriate hooks
     */
    if (datalen == 0)
        ata_IRQSetHandler(unit, &ata_IRQNoData, 0, 0, 0);
    else if (*dma)
        ata_IRQSetHandler(unit, &ata_IRQDMAReadWrite, NULL, 0, 0);
    else if (write)
        ata_IRQSetHandler(unit, &ata_IRQPIOWriteAtapi, data, 0, datalen);
    else
        ata_IRQSetHandler(unit, &ata_IRQPIOReadAtapi, data, 0, datalen);

    if (*dma)
    {
        DATAPI(bug("[ATAPI] Starting DMA\n"));
        DMA_Start(bus);
    }

    DATAPI(bug("[ATAPI] Sending packet\n"));
    Unit_OutS(unit, cmd, 12);
    ata_WaitNano(400, bus->ab_Base);

    DATAPI(bug("[ATAPI] Status after packet: %lx\n", ata_ReadAltStatus(bus)));

    /*
     * Wait for command to complete. Note that two interrupts will occur
     * before we wake up if this is a PIO data transfer
     */
    if (ata_WaitTO(unit->au_Bus->ab_Timer, TIMEOUT, 0,
        1 << unit->au_Bus->ab_SleepySignal) == 0)
    {
        DATAPI(bug("[DSCSI] Command timed out.\n"));
        err = IOERR_UNITBUSY;
    }
    else
        err = atapi_EndCmd(unit);

    if (*dma)
    {
        DMA_End(bus, data, datalen, !write);
    }

    DATAPI(bug("[ATAPI] IO error code %ld\n", err));
    return err;
}

BYTE atapi_DirectSCSI(struct ata_Unit *unit, struct SCSICmd *cmd)
{
    APTR buffer = cmd->scsi_Data;
    ULONG length = cmd->scsi_Length;
    BYTE err = 0;
    BOOL dma = FALSE;

    cmd->scsi_Actual = 0;

    DATAPI(bug("[DSCSI] Sending packet!\n"));

    /*
     * setup DMA & push command
     * it does not really mean we will use dma here btw
     */
    if ((unit->au_Flags & AF_DMA) && (length !=0) && (buffer != 0))
    {
        dma = DMA_Setup(unit->au_Bus, buffer, length,
                        cmd->scsi_Flags & SCSIF_READ);
    }

    err = atapi_SendPacket(unit, cmd->scsi_Command, cmd->scsi_Data, cmd->scsi_Length, &dma, (cmd->scsi_Flags & SCSIF_READ) == 0);

    DUMP({ if (cmd->scsi_Data != 0) dump(cmd->scsi_Data, cmd->scsi_Length); });

    /*
     * on check condition - grab sense data
     */
    DATAPI(bug("[ATA%02lx] atapi_DirectSCSI: SCSI Flags: %02lx / Error: %ld\n", unit->au_UnitNum, cmd->scsi_Flags, err));
    if ((err != 0) && (cmd->scsi_Flags & SCSIF_AUTOSENSE))
    {
        DATAPI(bug("[DSCSI] atapi_DirectSCSI: Packet Failed. Calling atapi_RequestSense\n"));
        atapi_RequestSense(unit, cmd->scsi_SenseData, cmd->scsi_SenseLength);
        DUMP(dump(cmd->scsi_SenseData, cmd->scsi_SenseLength));
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

    if (blk->type == CT_LBA48)
        max <<= 8;

    DATA(bug("[ATA%02ld] ata_exec_blk: Accessing %ld sectors starting from %x%08x\n", unit->au_UnitNum, count, (ULONG)(blk->blk >> 32), (ULONG)blk->blk));
    while ((count > 0) && (err == 0))
    {
        part = (count > max) ? max : count;
        blk->sectors = part;
        blk->length  = part << unit->au_SectorShift;

        DATA(bug("[ATA%02ld] Transfer of %ld sectors from %x%08x\n", unit->au_UnitNum, part, (ULONG)(blk->blk >> 32), (ULONG)blk->blk));
        err = ata_exec_cmd(unit, blk);
        DATA(bug("[ATA%02ld] ata_exec_blk: ata_exec_cmd returned %lx\n", unit->au_UnitNum, err));

        blk->blk    += part;
        blk->buffer  = &((char*)blk->buffer)[part << unit->au_SectorShift];
        count -= part;
    }
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
    UBYTE u;

    DINIT(bug("[ATA  ] ata_setup_unit(%d)\n", unit->au_UnitNum));
    ata_SelectUnit(unit);

    if (FALSE == ata_WaitBusyTO(unit, 1, FALSE, NULL))
    {
        DINIT(bug("[ATA%02ld] ata_setup_unit: ERROR: Drive not ready for use. Keeping functions stubbed\n", unit->au_UnitNum));
        return FALSE;
    }

    u = unit->au_UnitNum & 1;
    switch (bus->ab_Dev[u])
    {
        /*
         * safe fallback settings
         */
        case DEV_SATAPI:
        case DEV_ATAPI:
        case DEV_SATA:
        case DEV_ATA:
            unit->au_Identify = ata_Identify;
            break;

        default:
            DINIT(bug("[ATA%02ld] ata_setup_unit: Unsupported device %lx. All functions will remain stubbed.\n", unit->au_UnitNum, bus->ab_Dev[u]));
            return FALSE;
    }

    DINIT(bug("[ATA  ] ata_setup_unit: Enabling IRQs\n"));
    PIO_OutAlt(bus, 0x0, ata_AltControl);

    /*
     * now make unit self diagnose
     */
    if (unit->au_Identify(unit) != 0)
    {
        return FALSE;
    }

    return TRUE;
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
            ata_WaitBusyTO(unit, -1, TRUE, NULL);

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

void common_DetectXferModes(struct ata_Unit* unit)
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

#define SWAP_LE_WORD(x) (x) = AROS_LE2WORD((x))
#define SWAP_LE_LONG(x) (x) = AROS_LE2LONG((x))
#define SWAP_LE_QUAD(x) (x) = AROS_LE2LONG((x) >> 32) | (((QUAD)(AROS_LE2LONG((x) & 0xffffffff))) << 32)

BYTE ata_Identify(struct ata_Unit* unit)
{
    BOOL atapi = unit->au_Bus->ab_Dev[unit->au_UnitNum & 1] & 0x80;
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

    /* If the right command fails, try the wrong one. If both fail, abort */
    DINIT(bug("[ATA%02ld] ata_Identify: Executing ATA_IDENTIFY_%s command\n",
        unit->au_UnitNum, atapi ? "ATAPI" : "DEVICE"));
    if (ata_exec_cmd(unit, &acb))
    {
        acb.command = atapi ? ATA_IDENTIFY_DEVICE : ATA_IDENTIFY_ATAPI;
        DINIT(bug("[ATA%02ld] ata_Identify: Executing ATA_IDENTIFY_%s command"
            " instead\n", unit->au_UnitNum, atapi ? "DEVICE" : "ATAPI"));
        if (ata_exec_cmd(unit, &acb))
        {
            DINIT(bug("[ATA%02ld] ata_Identify: Both command variants failed\n",
                unit->au_UnitNum));
            return IOERR_OPENFAIL;
        }
        unit->au_Bus->ab_Dev[unit->au_UnitNum & 1] ^= 0x82;
        atapi = unit->au_Bus->ab_Dev[unit->au_UnitNum & 1] & 0x80;
        DINIT(bug("[ATA%02ld] ata_Identify:"
            " Incorrect device signature detected."
            " Switching device type to %lx.\n", unit->au_UnitNum,
            unit->au_Bus->ab_Dev[unit->au_UnitNum & 1]));
    }

    /*
     * If every second word is zero with 32-bit reads, switch to 16-bit
     * accesses for this drive and try again
     */
    if (unit->au_Bus->ab_Base->ata_32bit)
    {
        UWORD n = 0, *p, *limit;

        for (p = (UWORD *)unit->au_Drive, limit = p + 256; p < limit; p++)
            n |= *++p;

        if (n == 0)
        {
            DINIT(bug("[ATA%02ld] Identify data was invalid with 32-bit reads."
                " Switching to 16-bit mode.\n", unit->au_UnitNum));

            Unit_Disable32Bit(unit);

            if (ata_exec_cmd(unit, &acb))
                return IOERR_OPENFAIL;
        }
    }

#if (AROS_BIG_ENDIAN != 0)
    SWAP_LE_WORD(unit->au_Drive->id_General);
    SWAP_LE_WORD(unit->au_Drive->id_OldCylinders);
    SWAP_LE_WORD(unit->au_Drive->id_SpecificConfig);
    SWAP_LE_WORD(unit->au_Drive->id_OldHeads);
    SWAP_LE_WORD(unit->au_Drive->id_OldSectors);
    SWAP_LE_WORD(unit->au_Drive->id_RWMultipleSize);
    SWAP_LE_WORD(unit->au_Drive->id_Capabilities);
    SWAP_LE_WORD(unit->au_Drive->id_OldCaps);
    SWAP_LE_WORD(unit->au_Drive->id_OldPIO);
    SWAP_LE_WORD(unit->au_Drive->id_ConfigAvailable);
    SWAP_LE_WORD(unit->au_Drive->id_OldLCylinders);
    SWAP_LE_WORD(unit->au_Drive->id_OldLHeads);
    SWAP_LE_WORD(unit->au_Drive->id_OldLSectors);
    SWAP_LE_WORD(unit->au_Drive->id_RWMultipleTrans);
    SWAP_LE_WORD(unit->au_Drive->id_MWDMASupport);
    SWAP_LE_WORD(unit->au_Drive->id_PIOSupport);
    SWAP_LE_WORD(unit->au_Drive->id_MWDMA_MinCycleTime);
    SWAP_LE_WORD(unit->au_Drive->id_MWDMA_DefCycleTime);
    SWAP_LE_WORD(unit->au_Drive->id_PIO_MinCycleTime);
    SWAP_LE_WORD(unit->au_Drive->id_PIO_MinCycleTimeIORDY);
    SWAP_LE_WORD(unit->au_Drive->id_QueueDepth);
    SWAP_LE_WORD(unit->au_Drive->id_ATAVersion);
    SWAP_LE_WORD(unit->au_Drive->id_ATARevision);
    SWAP_LE_WORD(unit->au_Drive->id_Commands1);
    SWAP_LE_WORD(unit->au_Drive->id_Commands2);
    SWAP_LE_WORD(unit->au_Drive->id_Commands3);
    SWAP_LE_WORD(unit->au_Drive->id_Commands4);
    SWAP_LE_WORD(unit->au_Drive->id_Commands5);
    SWAP_LE_WORD(unit->au_Drive->id_Commands6);
    SWAP_LE_WORD(unit->au_Drive->id_UDMASupport);
    SWAP_LE_WORD(unit->au_Drive->id_SecurityEraseTime);
    SWAP_LE_WORD(unit->au_Drive->id_ESecurityEraseTime);
    SWAP_LE_WORD(unit->au_Drive->id_CurrentAdvPowerMode);
    SWAP_LE_WORD(unit->au_Drive->id_MasterPwdRevision);
    SWAP_LE_WORD(unit->au_Drive->id_HWResetResult);
    SWAP_LE_WORD(unit->au_Drive->id_AcousticManagement);
    SWAP_LE_WORD(unit->au_Drive->id_StreamMinimunReqSize);
    SWAP_LE_WORD(unit->au_Drive->id_StreamingTimeDMA);
    SWAP_LE_WORD(unit->au_Drive->id_StreamingLatency);
    SWAP_LE_WORD(unit->au_Drive->id_StreamingTimePIO);
    SWAP_LE_WORD(unit->au_Drive->id_PhysSectorSize);
    SWAP_LE_WORD(unit->au_Drive->id_RemMediaStatusNotificationFeatures);
    SWAP_LE_WORD(unit->au_Drive->id_SecurityStatus);

    SWAP_LE_LONG(unit->au_Drive->id_WordsPerLogicalSector);
    SWAP_LE_LONG(unit->au_Drive->id_LBASectors);
    SWAP_LE_LONG(unit->au_Drive->id_StreamingGranularity);

    SWAP_LE_QUAD(unit->au_Drive->id_LBA48Sectors);
#endif

    DUMP(dump(unit->au_Drive, sizeof(struct DriveIdent)));

    if (atapi)
    {
        unit->au_SectorShift    = 11;
        unit->au_Read32         = atapi_Read;
        unit->au_Write32        = atapi_Write;
        unit->au_DirectSCSI     = atapi_DirectSCSI;
        unit->au_Eject          = atapi_Eject;
        unit->au_Flags         |= AF_DiscChanged;
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

    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

    bug("[ATA%02ld] ata_Identify: Unit info: %s / %s / %s\n", unit->au_UnitNum, unit->au_Model, unit->au_SerialNumber, unit->au_FirmwareRev);
    common_DetectXferModes(unit);
    common_SetBestXferMode(unit);

    if (unit->au_Drive->id_General & 0x80)
    {
        DINIT(bug("[ATA%02ld] ata_Identify: Device is removable.\n", unit->au_UnitNum));
        unit->au_Flags |= AF_Removable;
    }

    unit->au_Capacity   = unit->au_Drive->id_LBASectors;
    unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;
    DINIT(bug("[ATA%02ld] ata_Identify: Unit LBA: %07lx 28bit / %04lx:%08lx 48bit addressable blocks\n", unit->au_UnitNum, unit->au_Capacity, (ULONG)(unit->au_Capacity48 >> 32), (ULONG)(unit->au_Capacity48 & 0xfffffffful)));

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

        atapi_TestUnitOK(unit);
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
        if ((unit->au_Drive->id_LBA48Sectors > (63 * 255 * 1024)) ||
            (unit->au_Drive->id_LBASectors > (63 * 255 * 1024)))
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
            unit->au_Capacity   = unit->au_Cylinders * unit->au_Heads * unit->au_Sectors;
        }
    }

    DINIT(bug("[ATA%02ld] ata_Identify: Unit CHS: %d/%d/%d\n", unit->au_UnitNum, unit->au_Cylinders, unit->au_Heads, unit->au_Sectors));

    return 0;
}

/*
 * ata read32 commands
 */
static BYTE ata_ReadSector32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_READ,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIORead,
        (unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28,
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_ReadSector32()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_ReadMultiple32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_READ_MULTIPLE,
        0,
        unit->au_Drive->id_RWMultipleSize & 0xff,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIORead,
        (unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28,
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_ReadMultiple32()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_ReadDMA32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    BYTE err;
    ata_CommandBlock acb =
    {
        ATA_READ_DMA,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_DMARead,
        (unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28,
    };

    D(bug("[ATA%02ld] ata_ReadDMA32()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

/*
 * ata read64 commands
 */
static BYTE ata_ReadSector64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_READ64,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIORead,
        CT_LBA48
    };
    BYTE err = 0;

    D(bug("[ATA%02ld] ata_ReadSector64()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_ReadMultiple64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_READ_MULTIPLE64,
        0,
        unit->au_Drive->id_RWMultipleSize & 0xff,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIORead,
        CT_LBA48
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_ReadMultiple64()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_ReadDMA64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_READ_DMA64,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_DMARead,
        CT_LBA48
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_ReadDMA64()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

/*
 * ata write32 commands
 */
static BYTE ata_WriteSector32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_WRITE,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIOWrite,
        (unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28,
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_WriteSector32()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_WriteMultiple32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_WRITE_MULTIPLE,
        0,
        unit->au_Drive->id_RWMultipleSize & 0xff,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIOWrite,
        (unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28,
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_WriteMultiple32()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_WriteDMA32(struct ata_Unit *unit, ULONG block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_WRITE_DMA,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_DMAWrite,
        (unit->au_Flags & AF_CHSOnly) ? CT_CHS : CT_LBA28,
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_WriteDMA32()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

/*
 * ata write64 commands
 */
static BYTE ata_WriteSector64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_WRITE64,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIOWrite,
        CT_LBA48
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_WriteSector64()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_WriteMultiple64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_WRITE_MULTIPLE64,
        0,
        unit->au_Drive->id_RWMultipleSize & 0xff,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_PIOWrite,
        CT_LBA48
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_WriteMultiple64()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static BYTE ata_WriteDMA64(struct ata_Unit *unit, UQUAD block,
    ULONG count, APTR buffer, ULONG *act)
{
    ata_CommandBlock acb =
    {
        ATA_WRITE_DMA64,
        0,
        1,
        0,
        block,
        count,
        buffer,
        count << unit->au_SectorShift,
        0,
        CM_DMAWrite,
        CT_LBA48
    };
    BYTE err;

    D(bug("[ATA%02ld] ata_WriteDMA64()\n", unit->au_UnitNum));

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
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

    D(bug("[ATA%02ld] atapi_TestUnitOK()\n", unit->au_UnitNum));

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_SenseData = (void*)&sense;
    sc.scsi_SenseLength = sizeof(sense);
    sc.scsi_Flags = SCSIF_AUTOSENSE;

    DATAPI(bug("[ATA%02ld] atapi_TestUnitOK: Testing Unit Ready sense...\n", unit->au_UnitNum));
    unit->au_DirectSCSI(unit, &sc);
    unit->au_SenseKey = sense[2];

    /*
     * we may have just lost the disc...?
     */
    /*
     * per MMC, drives are expected to return 02-3a-0# status, when disc is not present
     * that would translate into following code:
     *    int p1 = ((sense[2] == 2) && (sense[12] == 0x3a)) ? 1 : 0;
     * unfortunately, it's what MMC says, not what vendors code.
     */
    int p1 = (sense[2] == 2) ? 1 : 0;
    int p2 = (0 != (AF_DiscPresent & unit->au_Flags)) ? 1 : 0;

    if (p1 == p2)
    {
        //unit->au_Flags ^= AF_DiscPresent;
        if (p1 == 0)
            unit->au_Flags |= AF_DiscPresent;
        else
            unit->au_Flags &= ~AF_DiscPresent;

        unit->au_Flags |= AF_DiscChanged;
    }

    DATAPI(bug("[ATA%02ld] atapi_TestUnitOK: Test Unit Ready sense: %02lx, Media %s\n", unit->au_UnitNum, sense[2], unit->au_Flags & AF_DiscPresent ? "PRESENT" : "ABSENT"));
    return sense[2];
}

static BYTE atapi_Read(struct ata_Unit *unit, ULONG block, ULONG count,
    APTR buffer, ULONG *act)
{
    UBYTE cmd[] = {
       SCSI_READ10, 0, block>>24, block>>16, block>>8, block, 0, count>>8, count, 0
    };
    struct SCSICmd sc = {
       0
    };

    D(bug("[ATA%02ld] atapi_Read()\n", unit->au_UnitNum));

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_Data = buffer;
    sc.scsi_Length = count << unit->au_SectorShift;
    sc.scsi_Flags = SCSIF_READ;

    return unit->au_DirectSCSI(unit, &sc);
}

static BYTE atapi_Write(struct ata_Unit *unit, ULONG block, ULONG count,
    APTR buffer, ULONG *act)
{
    UBYTE cmd[] = {
       SCSI_WRITE10, 0, block>>24, block>>16, block>>8, block, 0, count>>8, count, 0
    };
    struct SCSICmd sc = {
       0
    };

    D(bug("[ATA%02ld] atapi_Write()\n", unit->au_UnitNum));

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_Data = buffer;
    sc.scsi_Length = count << unit->au_SectorShift;
    sc.scsi_Flags = SCSIF_WRITE;

    return unit->au_DirectSCSI(unit, &sc);
}

static BYTE atapi_Eject(struct ata_Unit *unit)
{
    struct atapi_StartStop cmd = {
        command: SCSI_STARTSTOP,
        immediate: 1,
        flags: ATAPI_SS_EJECT,
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

ULONG atapi_RequestSense(struct ata_Unit* unit, UBYTE* sense, ULONG senselen)
{
    UBYTE cmd[] = {
       3, 0, 0, 0, senselen & 0xfe, 0
    };
    struct SCSICmd sc = {
       0
    };

    D(bug("[ATA%02ld] atapi_RequestSense()\n", unit->au_UnitNum));

    if ((senselen == 0) || (sense == 0))
    {
       return 0;
    }
    sc.scsi_Data = (void*)sense;
    sc.scsi_Length = senselen & 0xfe;
    sc.scsi_Command = (void*)&cmd;
    sc.scsi_CmdLength = 6;
    sc.scsi_Flags = SCSIF_READ;

    unit->au_DirectSCSI(unit, &sc);

    DATAPI(dump(sense, senselen));
    DATAPI(bug("[SENSE] atapi_RequestSense: sensed data: %lx %lx %lx\n", sense[2]&0xf, sense[12], sense[13]));
    return ((sense[2]&0xf)<<16) | (sense[12]<<8) | (sense[13]);
}

ULONG ata_ReadSignature(struct ata_Bus *bus, int unit, BOOL *DiagExecuted)
{
    UBYTE tmp1, tmp2;

    D(bug("[ATA  ] ata_ReadSignature(%02ld)\n", unit));

    PIO_Out(bus, DEVHEAD_VAL | (unit << 4), ata_DevHead);
    ata_WaitNano(400, bus->ab_Base);
    //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

    DINIT(bug("[ATA  ] ata_ReadSignature: Status %02lx Device %02lx\n",
          ata_ReadStatus(bus), PIO_In(bus, ata_DevHead)));

    /* Ok, ATA/ATAPI device. Get detailed signature */
    DINIT(bug("[ATA  ] ata_ReadSignature: ATA[PI] device present. Attempting to detect specific subtype\n"));

    tmp1 = PIO_In(bus, ata_LBAMid);
    tmp2 = PIO_In(bus, ata_LBAHigh);

    DINIT(bug("[ATA  ] ata_ReadSignature: Subtype check returned %02lx:%02lx (%04lx)\n", tmp1, tmp2, (tmp1 << 8) | tmp2));

    switch ((tmp1 << 8) | tmp2)
    {
        case 0x14eb:
            DINIT(bug("[ATA  ] ata_ReadSignature: Found signature for ATAPI device\n"));
            return DEV_ATAPI;

        case 0x3cc3:
            DINIT(bug("[ATA  ] ata_ReadSignature: Found signature for SATA device\n"));
            return DEV_SATA;

        case 0x6996:
            DINIT(bug("[ATA  ] ata_ReadSignature: Found signature for SATAPI device\n"));
            return DEV_SATAPI;

        default:
            if (0 == (ata_ReadStatus(bus) & 0xfe)) {
                DINIT(bug("[ATA  ] ata_ReadSignature: Found NONE\n"));
                return DEV_NONE;
            }
            /* ATA_EXECUTE_DIAG is executed by both devices, do it only once */
            if (!*DiagExecuted)
            {
                DINIT(bug("[ATA  ] ata_ReadSignature: ATA_EXECUTE_DIAG\n"));
                PIO_Out(bus, ATA_EXECUTE_DIAG, ata_Command);
                *DiagExecuted = TRUE;
            }

            ata_WaitTO(bus->ab_Timer, 0, 2000, 0);
            while (ata_ReadStatus(bus) & ATAF_BUSY)
                ata_WaitNano(400, bus->ab_Base);
                //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

            DINIT(bug("[ATA  ] ata_ReadSignature: ATAF_BUSY wait finished\n"));

            PIO_Out(bus, DEVHEAD_VAL | (unit << 4), ata_DevHead);
            do
            {
                ata_WaitNano(400, bus->ab_Base);
                //ata_WaitTO(unit->au_Bus->ab_Timer, 0, 1, 0);
            }
            while (0 != (ATAF_BUSY & ata_ReadStatus(bus)));
            DINIT(bug("[ATA  ] ata_ReadSignature: Further validating ATA signature: %lx & 0x7f = 1, %lx & 0x10 = unit\n",
                      PIO_In(bus, ata_Error), PIO_In(bus, ata_DevHead)));

            if ((PIO_In(bus, ata_Error) & 0x7f) == 1)
            {
                DINIT(bug("[ATA  ] ata_ReadSignature: Found *valid* signature for ATA device\n"));
                /* this might still be an (S)ATAPI device, but we correct that in ata_Identify */
                return DEV_ATA;
            }
            DERROR(bug("[ATA  ] ata_ReadSignature: Found signature for ATA "
                "device, but further validation failed\n"));
            return DEV_NONE;
    }
}

void ata_ResetBus(struct ata_Bus *bus)
{
    struct ataBase *ATABase = bus->ab_Base;
    OOP_Object *obj = OOP_OBJECT(ATABase->busClass, bus);
    ULONG TimeOut;
    BOOL  DiagExecuted = FALSE;
    IPTR haveAltIO;

    /*
     * Set and then reset the soft reset bit in the Device Control
     * register.  This causes device 0 be selected.
     */
    DINIT(bug("[ATA  ] ata_ResetBus()\n"));

    PIO_Out(bus, DEVHEAD_VAL, ata_DevHead); /* Select it never the less */
    ata_WaitNano(400, ATABase);
    //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

    OOP_GetAttr(obj, aHidd_ATABus_UseIOAlt, &haveAltIO);
    if (haveAltIO)
    {
        PIO_OutAlt(bus, ATACTLF_RESET | ATACTLF_INT_DISABLE, ata_AltControl);
        ata_WaitTO(bus->ab_Timer, 0, 10, 0);    /* sleep 10us; min: 5us */

        PIO_OutAlt(bus, ATACTLF_INT_DISABLE, ata_AltControl);
    }
    else
    {
        PIO_Out(bus, ATA_EXECUTE_DIAG, ata_Command);
    }
    ata_WaitTO(bus->ab_Timer, 0, 20000, 0); /* sleep 20ms; min: 2ms */

    /* If there is a device 0, wait for device 0 to clear BSY */
    if (DEV_NONE != bus->ab_Dev[0])
    {
        DINIT(bug("[ATA  ] ata_ResetBus: Wait for master to clear BSY\n"));
        TimeOut = 1000;     /* Timeout 1s (1ms x 1000) */

        while ( 1 )
        {
            if ((ata_ReadStatus(bus) & ATAF_BUSY) == 0)
                break;
            ata_WaitTO(bus->ab_Timer, 0, 1000, 0);
            if (!(--TimeOut)) {
                DINIT(bug("[ATA%02ld] ata_ResetBus: Master device Timed Out!\n"));
                bus->ab_Dev[0] = DEV_NONE;
                break;
            }
        }
        DINIT(bug("[ATA  ] ata_ResetBus: Wait left after %d ms\n", 1000 - TimeOut));
    }

    /* If there is a device 1, wait some time until device 1 allows
     * register access, but fail only if BSY isn't cleared */
    if (DEV_NONE != bus->ab_Dev[1])
    {
        DINIT(bug("[ATA  ] ata_ResetBus: Wait DEV1 to allow access\n"));
        PIO_Out(bus, DEVHEAD_VAL | (1 << 4), ata_DevHead);
        ata_WaitNano(400, bus->ab_Base);
        //ata_WaitTO(bus->ab_Timer, 0, 1, 0);

        TimeOut = 50;     /* Timeout 50ms (1ms x 50) */
        while ( 1 )
        {
            if ((PIO_In(bus, ata_Count) == 0x01) && (PIO_In(bus, ata_LBALow) == 0x01))
                break;
            ata_WaitTO(bus->ab_Timer, 0, 1000, 0);
            if (!(--TimeOut))
            {
                DINIT(bug("[ATA  ] ata_ResetBus: DEV1 1/2 TimeOut!\n"));
                break;
            }
        }
        DINIT(bug("[ATA  ] ata_ResetBus: DEV1 1/2 Wait left after %d ms\n", 1000 - TimeOut));

        if (DEV_NONE != bus->ab_Dev[1])
        {
            DINIT(bug("[ATA  ] ata_ResetBus: Wait for slave to clear BSY\n"));
            TimeOut = 1000;     /* Timeout 1s (1ms x 1000) */
            while ( 1 )
            {
                if ((ata_ReadStatus(bus) & ATAF_BUSY) == 0)
                    break;
                ata_WaitTO(bus->ab_Timer, 0, 1000, 0);
                if (!(--TimeOut)) {
                    DINIT(bug("[ATA  ] ata_ResetBus: Slave device Timed Out!\n"));
                    bus->ab_Dev[1] = DEV_NONE;
                    break;
                }
            }
            DINIT(bug("[ATA  ] ata_ResetBus: Wait left after %d ms\n", 1000 - TimeOut));
        }
    }

    if (DEV_NONE != bus->ab_Dev[0])
        bus->ab_Dev[0] = ata_ReadSignature(bus, 0, &DiagExecuted);
    if (DEV_NONE != bus->ab_Dev[1])
        bus->ab_Dev[1] = ata_ReadSignature(bus, 1, &DiagExecuted);
}

void ata_InitBus(struct ata_Bus *bus)
{
    UBYTE tmp1, tmp2;
    UWORD i;

    /*
     * initialize timer for the sake of scanning
     */
    bus->ab_Timer = ata_OpenTimer(bus->ab_Base);

    DINIT(bug("[ATA  ] ata_InitBus(%d)\n", bus->ab_BusNum));

    bus->ab_Dev[0] = DEV_NONE;
    bus->ab_Dev[1] = DEV_NONE;

    for (i = 0; i < MAX_BUSUNITS; i++)
    {
        /* Select device and disable IRQs */
        PIO_Out(bus, DEVHEAD_VAL | (i << 4), ata_DevHead);
        ata_WaitTO(bus->ab_Timer, 0, 100, 0);
        PIO_OutAlt(bus, ATACTLF_INT_DISABLE, ata_AltControl);

        /* Write some pattern to registers */
        PIO_Out(bus, 0x55, ata_Count);
        PIO_Out(bus, 0xaa, ata_LBALow);
        PIO_Out(bus, 0xaa, ata_Count);
        PIO_Out(bus, 0x55, ata_LBALow);
        PIO_Out(bus, 0x55, ata_Count);
        PIO_Out(bus, 0xaa, ata_LBALow);

        tmp1 = PIO_In(bus, ata_Count);
        tmp2 = PIO_In(bus, ata_LBALow);
        DB2(bug("[ATA  ] ata_InitBus: Reply 0x%02X 0x%02X\n", tmp1, tmp2));

        if ((tmp1 == 0x55) && (tmp2 == 0xaa))
            bus->ab_Dev[i] = DEV_UNKNOWN;
        DINIT(bug("[ATA  ] ata_InitBus: Device type = 0x%02X\n", bus->ab_Dev[i]));
    }

    ata_ResetBus(bus);
    ata_CloseTimer(bus->ab_Timer);
    DINIT(bug("[ATA  ] ata_InitBus: Finished\n"));
}

/*
 * not really sure what this is meant to be - TO BE REPLACED
 */
static const ULONG ErrorMap[] = {
    CDERR_NotSpecified,
    CDERR_NoSecHdr,
    CDERR_NoDisk,
    CDERR_NoSecHdr,
    CDERR_NoSecHdr,
    CDERR_NOCMD,
    CDERR_NoDisk,
    CDERR_WriteProt,
    CDERR_NotSpecified,
    CDERR_NotSpecified,
    CDERR_NotSpecified,
    CDERR_ABORTED,
    CDERR_NotSpecified,
    CDERR_NotSpecified,
    CDERR_NoSecHdr,
    CDERR_NotSpecified,
};

static BYTE atapi_EndCmd(struct ata_Unit *unit)
{
    struct ata_Bus *bus = unit->au_Bus;
    UBYTE status;

    DATAPI(bug("[ATA%02ld] atapi_EndCmd()\n", unit->au_UnitNum));

    /*
     * read alternate status register (per specs)
     */
    status = PIO_InAlt(bus, ata_AltStatus);
    DATAPI(bug("[ATA%02ld] atapi_EndCmd: Alternate status: %lx\n", unit->au_UnitNum, status));

    status = PIO_In(bus, atapi_Status);

    DATAPI(bug("[ATA%02ld] atapi_EndCmd: Command complete. Status: %lx\n",
        unit->au_UnitNum, status));

    if (!(status & ATAPIF_CHECK))
    {
        return 0;
    }
    else
    {
       status = PIO_In(bus, atapi_Error);
       DATAPI(bug("[ATA%02ld] atapi_EndCmd: Error code 0x%lx\n", unit->au_UnitNum, status >> 4));
       return ErrorMap[status >> 4];
    }
}

/*
 * vim: ts=4 et sw=4 fdm=marker fmr={,}
 */
