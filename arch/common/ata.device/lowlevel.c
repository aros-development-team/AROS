/*
    Copyright � 2004-2008, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

/*
 * CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2006-12-20  T. Wiszkowski       Updated ATA Packet Interface to handle ATAPI/SCSI Commands
 * 2008-01-06  T. Wiszkowski       Corrected and completed ATA Packet Interface handling. PIO transfers fully operational.
 * 2008-01-07  T. Wiszkowski       Added initial DMA support for Direct SCSI commands. Corrected atapi
 *                                 READ and WRITE commands to pass proper transfer size to the atapi_SendPacket
 *                                 as discovered by mschulz
 * 2008-01-25  T. Wiszkowski       Rebuilt, rearranged and partially fixed 60% of the code here
 *                                 Enabled implementation to scan for other PCI IDE controllers
 *                                 Implemented ATAPI Packet Support for both read and write
 *                                 Corrected ATAPI DMA handling                            
 *                                 Fixed major IDE enumeration bugs severely handicapping transfers with more than one controller
 *                                 Compacted source and implemented major ATA support procedure
 *                                 Improved DMA and Interrupt management
 *                                 Removed obsolete code
 * 2008-01-26  T. Wiszkowski       Restored 32bit io
 *                                 Removed memory dump upon RequestSense
 * 2008-02-08  T. Wiszkowski       Fixed DMA accesses for direct scsi devices,
 *                                 Corrected IO Areas to allow ATA to talk to PCI controllers
 * 2008-03-03  T. Wiszkowski       Added drive reselection + setup delay on Init
 * 2008-03-29  T. Wiszkowski       Restored error on 64bit R/W access to non-64bit capable atapi devices
 *                                 cleared debug flag
 * 2008-03-30  T. Wiszkowski       Added workaround for interrupt collision handling; fixed SATA in LEGACY mode.
 *                                 nForce and Intel SATA chipsets should now be operational (nForce confirmed)
 * 2008-03-31  M. Schulz           The ins/outs function definitions used only in case of x86 and x86_64 architectures. 
 *                                 Otherwise, function declaratons are emitted.
 * 2008-04-01  M. Schulz           Use C functions ata_ins[wl] ata_outs[wl]
 * 2008-04-03  T. Wiszkowski       Fixed IRQ flood issue, eliminated and reduced obsolete / redundant code                                 
 * 2008-04-05  T. Wiszkowski       Improved IRQ management 
 * 2008-04-07  T. Wiszkowski       Changed bus timeout mechanism
 *                                 increased failure timeout values to cover rainy day scenarios
 * 2008-04-20  T. Wiszkowski       Corrected the flaw in drive identification routines leading to ocassional system hangups
 * 2008-05-11  T. Wiszkowski       Remade the ata trannsfers altogether, corrected the pio/irq handling 
 *                                 medium removal, device detection, bus management and much more
 * 2008-05-12  P. Fedin	           Explicitly enable multisector transfers on the drive
 * 2008-05-18  T. Wiszkowski       Added extra checks to prevent duplicating drive0 in drive0 only configs
 * 2008-05-18  T. Wiszkowski       Replaced static C/H/S with more accurate calcs, should make HDTB and other tools see right capacity
 * 2008-05-19  T. Wiszkowski       Updated ATA DMA handling and transfer wait operation to allow complete transfer before dma_StopDMA()
 * 2008-05-30  T. Wiszkowski       Corrected CHS calculation for larger disks
 * 2008-06-03  K. Smiechowicz      Added 400ns delay in ata_WaitBusyTO before read of device status.
 * 2008-06-25  P. Fedin            Added "nomulti" flag
 *                                 PIO works correctly again
 * 2008-11-28  T. Wiszkowski       updated test unit ready to suit individual taste of hw manufacturers
 * 2009-01-20  J. Koivisto         Modified bus reseting scheme
 */
/*
 * TODO: 
 * - put a critical section around DMA transfers (shared dma channels)
 */

#define DEBUG 0
// use #define xxx(a) D(a) to enable particular sections.
#define DIRQ(a)
#define DIRQ_MORE(a) 
#define DUMP(a) 
#define DATA(a)
#define DATAPI(a)
#define DINIT(a) D(a)

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <dos/bptr.h>

#include <proto/exec.h>
#include <devices/timer.h>

#include <asm/io.h>

#include "ata.h"



/*
    Prototypes of static functions from lowlevel.c. I do not want to make them
    non-static as I'd like to remove as much symbols from global table as possible.
    Besides some of this functions could conflict with old ide.device or any other
    device.
*/
static ULONG ata_ReadSector32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_ReadSector64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_ReadMultiple32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_ReadMultiple64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_ReadDMA32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_ReadDMA64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_WriteSector32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_WriteSector64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_WriteMultiple32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_WriteMultiple64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_WriteDMA32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_WriteDMA64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_Eject(struct ata_Unit *);
static BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout, BOOL irq, UBYTE *stout);

static ULONG atapi_EndCmd(struct ata_Unit *unit);

static ULONG atapi_Read(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG atapi_Write(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG atapi_Eject(struct ata_Unit *);

static void common_SetBestXferMode(struct ata_Unit* unit);

/*
    Again piece of code which shouldn't be here. Geee. After removing all this 
    asm constrictuins this ata.device will really deserve for location in 
    /arch/common
*/
/*
 * having an x86 assembly here i dare to assume that this is meant to be
 * an x86[_64] device only.
 * 
 * Not anymore.
 * 
 * This functions will stay here for some time *IF* and only if the code is compiled for
 * x86 or x86_64 architecture. Otherwise, function declarations will be emitted and the
 * one who ports the driver will be reponsible for adding the missing code.
 */

/*
 * the outsl and insl commands improperly assumed that every transfer is sized to multiple of four
 */
#if defined(__i386__) || defined(__x86_64__)

static VOID ata_insw(APTR address, UWORD port, ULONG count)
{
    insw(port, address, count >> 1);
}

static VOID ata_insl(APTR address, UWORD port, ULONG count)
{
    if (count & 2)
        insw(port, address, count >> 1);
    else
        insl(port, address, count >> 2);
}

static VOID ata_outsw(APTR address, UWORD port, ULONG count)
{
    outsw(port, address, count >> 1);
}

static VOID ata_outsl(APTR address, UWORD port, ULONG count)
{
    if (count & 2)
        outsw(port, address, count >> 1);
    else
        outsl(port, address, count >> 2);
}

/*
 * Very short delay (TM) by someone who assumes slow data ports.
 * well, glad it works anyways.
 */
void ata_400ns(ULONG port)
{
    ata_in(ata_AltControl, port);
    ata_in(ata_AltControl, port);
    ata_in(ata_AltControl, port);
    ata_in(ata_AltControl, port);
}

#else
extern VOID ata_insw(APTR address, UWORD port, ULONG count);
extern VOID ata_insl(APTR address, UWORD port, ULONG count);
extern VOID ata_outsw(APTR address, UWORD port, ULONG count);
extern VOID ata_outsl(APTR address, UWORD port, ULONG count);
extern void ata_400ns(ULONG port);
#endif

static void dump(APTR mem, ULONG len)
{
    register int i,j;
    for (j=0; j<(len+15)>>4; ++j)
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

static void ata_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    register int i = size;

    while (size--)
    {
        str2[size^1] = str1[size];
    }

    while (i--)
    {
        if (str2[i] <= ' ')
            str2[i] = 0;
    }
}


/*
 * a STUB function for commands not supported by this particular device
 */
static ULONG ata_STUB(struct ata_Unit *au)
{
    bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum);
    return CDERR_NOCMD;
}

static ULONG ata_STUB_IO32(struct ata_Unit *au, ULONG blk, ULONG len, APTR buf, ULONG* act)
{
    bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum);
    return CDERR_NOCMD;
}

static ULONG ata_STUB_IO64(struct ata_Unit *au, UQUAD blk, ULONG len, APTR buf, ULONG* act)
{
    bug("[ATA%02ld] CALLED STUB FUNCTION -- IO ACCESS TO BLOCK %08lx:%08lx, LENGTH %08lx. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum, (blk >> 32), (blk & 0xffffffff), len);
    return CDERR_NOCMD;
}

static ULONG ata_STUB_SCSI(struct ata_Unit *au, struct SCSICmd* cmd)
{
    bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum);
    return CDERR_NOCMD;
}

inline BOOL ata_SelectUnit(struct ata_Unit* unit)
{
    ata_out(unit->au_DevMask, ata_DevHead, unit->au_Bus->ab_Port);

    do 
    {
        ata_400ns(unit->au_Bus->ab_Alt);
    } 
    while (0 != (ATAF_BUSY & ata_ReadStatus(unit->au_Bus)));

    return TRUE;
}

inline struct ata_Unit* ata_GetSelectedUnit(struct ata_Bus* bus)
{
    register int id = (ata_in(ata_DevHead, bus->ab_Port) & 0x10) >> 4;
    return bus->ab_Units[id];
}

UBYTE ata_ReadStatus(struct ata_Bus *bus)
{
    return ata_in(ata_Status, bus->ab_Port);
}

/*
 * enable / disable IRQ; this manages interrupt requests more effectively in case of legacy emulation
 * as little code as there can be. and keep it that way.
 */
void ata_EnableIRQ(struct ata_Bus *bus, BOOL enable)
{
    enable &= (bus->ab_Dev[0] > DEV_UNKNOWN) || (bus->ab_Dev[1] > DEV_UNKNOWN);
    bug("[ATA  ] IRQ: Setting IRQ flag for bus %ld to %s\n", bus->ab_BusNum, enable ? "ENABLED" : "DISABLED");
    bus->ab_IRQ = enable;
    ata_out(enable ? 0x0 : 0x02, ata_AltControl, bus->ab_Alt);
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

void ata_HandleIRQ(struct ata_Bus *bus)
{
    struct ata_Unit *u = ata_GetSelectedUnit(bus);
    UBYTE status = ata_ReadStatus(bus);
    /*
     * don't waste your time on checking other devices.
     * pass irq ONLY if task is expecting one;
     */

    if (0 != bus->ab_HandleIRQ)
    {
        /*
         * ok, we have a routine to handle any form of transmission etc
         */
        DIRQ(bug("[ATA  ] IRQ: Calling dedicated handler... \n"));
        bus->ab_HandleIRQ(u, status);
        return;
    }
    
    DIRQ_MORE({
        /*
         * if we got *here* then device is most likely not expected to have an irq.
         */
        bug("[ATA%02ld] IRQ: Checking busy flag: ", u->au_UnitNum);

        if (0 == (ATAF_BUSY & status))
        {
            bug("device ready. Dumping details:\n");

            bug("[ATA  ] STATUS: %02lx\n", status);
            bug("[ATA  ] ALT STATUS: %02lx\n", ata_in(ata_AltStatus, bus->ab_Alt));
            bug("[ATA  ] ERROR: %02lx\n", ata_in(ata_Error, bus->ab_Port));
            bug("[ATA  ] IRQ: REASON: %02lx\n", ata_in(atapi_Reason, bus->ab_Port));
        }
        else
        {
            bug("device still busy. ignoring irq.\n");
        }
    });
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
    if (status & ATAF_DATAREQ) {
	DIRQ(bug("[ATA  ] IRQ: PIOReadData - DRQ.\n"));
        unit->au_ins(unit->au_cmd_data, unit->au_Bus->ab_Port, unit->au_cmd_length);

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
    unit->au_outs(unit->au_cmd_data, unit->au_Bus->ab_Port, unit->au_cmd_length);

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

void ata_IRQDMAReadWrite(struct ata_Unit *au, UBYTE status)
{
    UBYTE stat = ata_in(dma_Status, au->au_DMAPort);
    DIRQ(bug("[ATA%02ld] IRQ: IO status %02lx, DMA status %02lx\n", au->au_UnitNum, status, stat));

    if (0 == (stat & DMAF_Interrupt))
    {
        bug("[ATA  ] IRQ: Fake IRQ.\n");
    }
    else if ((status & ATAF_ERROR) || (stat & DMAF_Error))
    {
        /* This is turned on in order to help Phantom - Pavel Fedin <sonic_amiga@rambler.ru> */
        bug("[ATA%02ld] IRQ: IO status %02lx, DMA status %02lx\n", au->au_UnitNum, status, stat);
        bug("[ATA%02ld] IRQ: ERROR %02lx\n", au->au_UnitNum, ata_in(atapi_Error, au->au_Bus->ab_Port));
        bug("[ATA  ] IRQ: DMA Failed.\n");
        au->au_cmd_error = HFERR_DMA;
        ata_IRQNoData(au, status);
    }
    else if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
    {
        DIRQ(bug("[ATA  ] IRQ: DMA Done.\n"));
        ata_IRQNoData(au, status);
    }
}

void ata_IRQPIOReadAtapi(struct ata_Unit *unit, UBYTE status)
{
    ULONG port = unit->au_Bus->ab_Port;
    ULONG size = 0;
    UBYTE reason = ata_in(atapi_Reason, port);
    DIRQ(bug("[DSCSI] Current status: %ld during READ\n", reason));

    /* have we failed yet? */
    if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
        ata_IRQNoData(unit, status);
    if (status & ATAF_ERROR)
        ata_IRQNoData(unit, status);
    
    /* anything for us please? */
    if (ATAPIF_READ != (reason & ATAPIF_MASK))
        return;

    size = ata_in(atapi_ByteCntH, port) << 8 | ata_in(atapi_ByteCntL, port);
    DIRQ(bug("[ATAPI] IRQ: data available for read (%ld bytes, max: %ld bytes)\n", size, unit->au_cmd_total));

    if (size > unit->au_cmd_total)
    {
        bug("[ATAPI] IRQ: CRITICAL! MORE DATA OFFERED THAN STORAGE CAN TAKE: %ld bytes vs %ld bytes left!\n", size, unit->au_cmd_total);
        size = unit->au_cmd_total;
    }

    unit->au_ins(unit->au_cmd_data, port, size);
    unit->au_cmd_data = &((UBYTE*)unit->au_cmd_data)[size];
    unit->au_cmd_total -= size;

    DIRQ(bug("[ATAPI] IRQ: %lu bytes read.\n", size));

    if (unit->au_cmd_total == 0)
        ata_IRQNoData(unit, status);
}

void ata_IRQPIOWriteAtapi(struct ata_Unit *unit, UBYTE status)
{
    ULONG port = unit->au_Bus->ab_Port;
    ULONG size = 0;
    UBYTE reason = ata_in(atapi_Reason, port);
    DIRQ(bug("[ATAPI] IRQ: Current status: %ld during WRITE\n", reason));

    /* have we failed yet? */
    if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
        ata_IRQNoData(unit, status);
    if (status & ATAF_ERROR)
        ata_IRQNoData(unit, status);
    
    /* anything for us please? */
    if (ATAPIF_READ != (reason & ATAPIF_MASK))
        return;

    size = ata_in(atapi_ByteCntH, port) << 8 | ata_in(atapi_ByteCntL, port);
    DIRQ(bug("[ATAPI] IRQ: data requested for write (%ld bytes, max: %ld bytes)\n", size, unit->au_cmd_total));

    if (size > unit->au_cmd_total)
    {
        bug("[ATAPI] IRQ: CRITICAL! MORE DATA REQUESTED THAN STORAGE CAN GIVE: %ld bytes vs %ld bytes left!\n", size, unit->au_cmd_total);
        size = unit->au_cmd_total;
    }

    unit->au_outs(unit->au_cmd_data, port, size);
    unit->au_cmd_data = &((UBYTE*)unit->au_cmd_data)[size];
    unit->au_cmd_total -= size;

    DIRQ(bug("[ATAPI] IRQ: %lu bytes written.\n", size));

    if (unit->au_cmd_total == 0)
        ata_IRQNoData(unit, status);
}

/*
 * wait for timeout or drive ready
 * polling-in-a-loop, but it should be safe to remove this already
 */
BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout, BOOL irq, UBYTE *stout)
{
    UBYTE status;
    ULONG sigs = SIGBREAKF_CTRL_C;
    ULONG step = 0;
    BOOL res = TRUE;
   
    if (irq && !unit->au_Bus->ab_IRQ)
    {
        bug("[ATA  ] Requested IRQ wait, but IRQ is not enabled\n");
    }

    irq &= unit->au_Bus->ab_IRQ; 
    sigs |= (irq ? (1 << unit->au_Bus->ab_SleepySignal) : 0);
            
    /*
     * clear up all old signals
     */
    SetSignal(0, sigs);

    /*
     * set up bus timeout and irq
     */
    Disable();
    unit->au_Bus->ab_Timeout = tout;
    Enable();

    /*
     * this loop may experience one of two scenarios
     * 1) we get a valid irq and the drive wanted to let us know that it's ready
     * 2) we get an invalid irq due to some collissions. We may still want to go ahead and get some extra irq breaks
     *    this would reduce system load a little
     */

    do
    {

        /* 
         * delay the check - this was found needed for some hardware
         */

        ata_400ns(unit->au_Bus->ab_Alt);

        /*
         * lets check if the drive is already good
         */
        status = ata_in(ata_Status, unit->au_Bus->ab_Port);
        if (0 == (status & (ATAF_DATAREQ | ATAF_BUSY)))
            break;

        /*
         * so we're stuck in a loop?
         */
        DIRQ(bug("[ATA%02ld] Waiting (Current status: %02lx)...\n", unit->au_UnitNum, status));

        /*
         * if IRQ wait is requested then allow either timeout or irq;
         * then clear irq flag so we dont keep receiving more of these (especially when system suffers collissions)
         */
        if (irq)
        {
            /*
             * wait for either IRQ or TIMEOUT
             */
            step = Wait(sigs);

            /*
             * now if we did reach timeout, then there's no point in going ahead.
             */
            if (SIGBREAKF_CTRL_C & step)
            {
                bug("[ATA%02ld] Timeout while waiting for device to complete operation\n", unit->au_UnitNum);
                res = FALSE;
            }

            /*
             * if we get as far as this, there's no more signals to expect
             * but we still want the status
             */
            status = ata_in(ata_Status, unit->au_Bus->ab_Port);
            break;
        }
        else
        { 
            /*
             * device not ready just yet. lets set whether we want an IRQ and move on - to polling or irq wait
             */
            ++step;

            /*
             * every 16n rounds do some extra stuff
             */
            if ((step & 16) == 0)
            {
                /*
                 * huhm. so it's been 16n rounds already. any timeout yet?
                 */
                if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                {
                    DIRQ(bug("[ATA%02ld] Device still busy after timeout. Aborting\n", unit->au_UnitNum));
                    res = FALSE;
                    break;
                }

                /*
                 * no timeout just yet, but it's not a good idea to keep spinning like that.
                 * let's give the system some time.
                 */
                ata_400ns(unit->au_Bus->ab_Alt);
                // TODO: Put some delay here!
            }
        }
    } while(status & ATAF_BUSY);

    /*
     * be nice to frustrated developer
     */
    DIRQ(bug("[ATA%02ld] WaitBusy status: %lx / %ld\n", unit->au_UnitNum, status, res));

    /*
     * clear up all our expectations 
     */
    Disable();
    unit->au_Bus->ab_Timeout = -1;
    Enable();
            
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
 * just wait for timeout
 */
void ata_Wait(struct ata_Unit *unit, UWORD tout)
{
    SetSignal(0, SIGBREAKF_CTRL_C);
            
    Disable();
    unit->au_Bus->ab_Timeout = tout;
    Enable();

    Wait(SIGBREAKF_CTRL_C);
}

/*
 * Procedure for sending ATA command blocks
 * it appears LARGE but there's a lot of COMMENTS here :)
 * handles *all* ata commands (no data, pio and dma)
 * naturally could be splitted at some point in the future
 * depends if anyone believes that the change for 50 lines 
 * would make slow ATA transfers any faster 
 */
static ULONG ata_exec_cmd(struct ata_Unit* au, ata_CommandBlock *block)
{
    ULONG port = au->au_Bus->ab_Port;
    ULONG err = 0;
    APTR mem = block->buffer;
    UBYTE status;

    if (FALSE == ata_SelectUnit(au))
        return IOERR_UNITBUSY;

    switch (block->type)
    {
        case CT_LBA28:
            if (block->sectors > 256)
            {
                bug("[ATA%02ld] ERROR: Transfer length (%ld) exceeds 256 sectors. Aborting.\n", au->au_UnitNum, block->sectors);
                return IOERR_BADLENGTH;
            }

            /* note:
             * we want the above to fall in here!
             * we really do (checking for secmul)
             */

        case CT_LBA48:
            if (block->sectors > 65536)
            {
                bug("[ATA%02ld] ERROR: Transfer length (%ld) exceeds 65536 sectors. Aborting.\n", au->au_UnitNum, block->sectors);
                return IOERR_BADLENGTH;
            }
            if (block->secmul == 0)
            {
                bug("[ATA%02ld] ERROR: Invalid transfer multiplier. Should be at least set to 1 (correcting)\n", au->au_UnitNum);
                block->secmul = 1;
            }
           break;

        case CT_NoBlock:
            break;

        default:
            bug("[ATA%02ld] ERROR: Invalid command type %lx. Aborting.\n", au->au_UnitNum, block->type);
            return IOERR_NOCMD;
    }

    block->actual = 0;
    D(bug("[ATA%02ld] Executing command %02lx\n", au->au_UnitNum, block->command));

    if (block->feature != 0)
        ata_out(block->feature, ata_Feature, port);

    /*
     * - set LBA and sector count
     */
    switch (block->type)
    {
        case CT_LBA28:
            DATA(bug("[ATA%02ld] Command uses 28bit LBA addressing (OLD)\n", au->au_UnitNum));
            ata_out(((block->blk >> 24) & 0x0f) | 0x40 | au->au_DevMask, ata_DevHead, port);
            ata_out(block->blk >> 16, ata_LBAHigh, port);
            ata_out(block->blk >> 8, ata_LBAMid, port);
            ata_out(block->blk, ata_LBALow, port);
            ata_out(block->sectors, ata_Count, port);
            break;

        case CT_LBA48:
            DATA(bug("[ATA%02ld] Command uses 48bit LBA addressing (NEW)\n", au->au_UnitNum));
            ata_out(block->blk >> 40, ata_LBAHigh, port);
            ata_out(block->blk >> 32, ata_LBAMid, port);
            ata_out(block->blk >> 24, ata_LBALow, port);

            ata_out(block->blk >> 16, ata_LBAHigh, port);
            ata_out(block->blk >> 8, ata_LBAMid, port);
            ata_out(block->blk, ata_LBALow, port);
            
            ata_out(block->sectors >> 8, ata_Count, port);
            ata_out(block->sectors, ata_Count, port);
            break;

        case CT_NoBlock:
            DATA(bug("[ATA%02ld] Command does not address any block\n", au->au_UnitNum));
            break;
    }


    switch (block->method)
    {
        case CM_PIOWrite:
            ata_IRQSetHandler(au, &ata_IRQPIOWrite, mem, block->secmul << au->au_SectorShift, block->length);
            break;

        case CM_PIORead:
            ata_IRQSetHandler(au, &ata_IRQPIORead, mem, block->secmul << au->au_SectorShift, block->length);
            break;

        case CM_DMARead:
            if (FALSE == dma_SetupPRDSize(au, mem, block->length, TRUE))
                return IOERR_ABORTED;
            ata_IRQSetHandler(au, &ata_IRQDMAReadWrite, NULL, 0, 0);
            dma_StartDMA(au);
            break;

        case CM_DMAWrite:
            if (FALSE == dma_SetupPRDSize(au, mem, block->length, FALSE))
                return IOERR_ABORTED;
            ata_IRQSetHandler(au, &ata_IRQDMAReadWrite, NULL, 0, 0);
            dma_StartDMA(au);
            break;

        case CM_NoData:
            ata_IRQSetHandler(au, &ata_IRQNoData, NULL, 0, 0);
            break;

        default:
            return IOERR_NOCMD;
            break;
    };

    /*
     * send command now
     * let drive propagate its signals
     */
    DATA(bug("[ATA%02ld] Sending command\n", au->au_UnitNum));
    ata_out(block->command, ata_Command, port);
    ata_400ns(au->au_Bus->ab_Alt);
    
    /*
     * In case of PIO write the drive won't issue an IRQ before first
     * data transfer, so we should poll the status and send the firs
     * block upon request.
     */
    if (block->method == CM_PIOWrite) {
	if (FALSE == ata_WaitBusyTO(au, 300, FALSE, &status)) {
	    D(bug("[ATA%02ld] PIOWrite - no response from device\n", au->au_UnitNum));
	    return IOERR_UNITBUSY;
	}
	if (status & ATAF_DATAREQ) {
	    DATA(bug("[ATA%02ld] PIOWrite - DRQ.\n", au->au_UnitNum));
	    ata_PIOWriteBlk(au);
	}
	else
	{
	    D(bug("[ATA%02ld] PIOWrite - bad status: %02X\n", status));
	    return HFERR_BadStatus;
	}
    }

    /*
     * wait for drive to complete what it has to do
     */
    if (FALSE == ata_WaitBusyTO(au, 300, TRUE, NULL))
    {
        bug("[ATA%02ld] Device is late - no response\n", au->au_UnitNum);
        err = IOERR_UNITBUSY;
    }
    else
        err = au->au_cmd_error;

    DATA(bug("[ATA%02ld] Command done\n", au->au_UnitNum));
    /*
     * clean up DMA
     * don't use 'mem' pointer here as it's already invalid.
     */
    if (block->method == CM_DMARead)
    {
        dma_StopDMA(au);
        dma_Cleanup(block->buffer, block->length, TRUE);
    }
    else if (block->method == CM_DMAWrite)
    {
        dma_StopDMA(au);
        dma_Cleanup(block->buffer, block->length, FALSE);
    }

    D(bug("[ATA%02ld] return code %ld\n", au->au_UnitNum, err));
    return err;
}

/*
 * atapi packet iface
 */
int atapi_SendPacket(struct ata_Unit *unit, APTR packet, APTR data, LONG datalen, BOOL *dma, BOOL write)
{
    *dma &= (unit->au_XferModes & AF_XFER_DMA) ? TRUE : FALSE;
    LONG err = 0;

    UBYTE cmd[12] = {
        0
    };
    register int t=5,l=0;
    ULONG port = unit->au_Bus->ab_Port;

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
            break;
        default:
            *dma = FALSE;
    }

    while (l<=t)
    {
        cmd[l] = ((UBYTE*)packet)[l];
        ++l;
    }

    D({
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
     * tell device if whether we want to read or write and if we want a dma transfer
     */
    ata_out(((*dma) ? 1 : 0) | ((write) ? 4 : 0), atapi_Features, port);
    ata_out((datalen & 0xff), atapi_ByteCntL, port);
    ata_out((datalen >> 8) & 0xff, atapi_ByteCntH, port);

    /*
     * once we're done with that, we can go ahead and inform device that we're about to send atapi packet
     * after command is dispatched, we are obliged to give 400ns for the unit to parse command and set status
     */
    DATAPI(bug("[ATAPI] Issuing ATA_PACKET command.\n"));
    ata_out(ATA_PACKET, atapi_Command, port);
    ata_400ns(unit->au_Bus->ab_Alt);
    
    ata_WaitBusyTO(unit, 30, FALSE, NULL);
    if (0 == (ata_ReadStatus(unit->au_Bus) & ATAF_DATAREQ))
        return HFERR_BadStatus;

    /*
     * setup appropriate hooks. not really the best way.
     */
    if (datalen == 0)
        ata_IRQSetHandler(unit, &ata_IRQNoData, 0, 0, 0);
    else if (*dma)
        ata_IRQSetHandler(unit, &ata_IRQDMAReadWrite, NULL, 0, 0);
    else if (write)
        ata_IRQSetHandler(unit, &ata_IRQPIOWriteAtapi, data, 0, datalen);
    else
        ata_IRQSetHandler(unit, &ata_IRQPIOReadAtapi, data, 0, datalen);

    DATAPI(bug("[ATAPI] Sending packet\n"));
    unit->au_outs(cmd, unit->au_Bus->ab_Port, 12);
    ata_400ns(unit->au_Bus->ab_Alt);    /* give drive time to think about what we just said, then move on */
    /* how much time could it take for drive to raise DMARQ signal?? */
    
    DATAPI(bug("[ATAPI] Status after packet: %lx\n", ata_ReadStatus(unit->au_Bus)));

    if (*dma)
    {
        DATAPI(bug("[ATAPI] Preparing for DMA\n"));
        while (0 == ((t = ata_ReadStatus(unit->au_Bus)) & (ATAF_BUSY | ATAF_DATAREQ)))
        {
            if (t & ATAF_ERROR)
            {
                err = HFERR_DMA;
                break;
            }
            DATAPI(bug("[ATAPI] status %02lx\n", ata_ReadStatus(unit->au_Bus)));
            ata_400ns(unit->au_Bus->ab_Alt);
        }

        DATAPI(bug("[ATAPI] status %02lx\n", ata_ReadStatus(unit->au_Bus)));

        if (0 == err)
        {
            DATAPI(bug("[ATAPI] Starting DMA\n"));
            dma_StartDMA(unit);
        }
    }

    if ((0 == err) && (ata_WaitBusyTO(unit, 300, TRUE, NULL) == FALSE))
    {
        DATAPI(bug("[DSCSI] Command timed out.\n"));
        err = IOERR_UNITBUSY;
    }
    else
        err = atapi_EndCmd(unit);

    if (TRUE == *dma)
    {
        dma_StopDMA(unit);
        dma_Cleanup(data, datalen, !write);
    }

    D(bug("[ATAPI] Error code %ld\n", err));
    return err;
}

ULONG atapi_DirectSCSI(struct ata_Unit *unit, struct SCSICmd *cmd)
{
    APTR buffer = cmd->scsi_Data;
    ULONG length = cmd->scsi_Length;
    BOOL read = FALSE;
    UBYTE status;
    UBYTE err = 0;
    BOOL dma = FALSE;

    cmd->scsi_Actual = 0;

    DATAPI(bug("[DSCSI] Sending packet!\n"));


    /*
     * setup DMA & push command
     * it does not really mean we will use dma here btw
     */
    if ((unit->au_XferModes & AF_XFER_DMA) && (length !=0) && (buffer != 0))
    {
        dma = TRUE;
        if ((cmd->scsi_Flags & SCSIF_READ) != 0)
        {
            read = TRUE;
            if (FALSE == dma_SetupPRDSize(unit, buffer, length, TRUE))
                dma = FALSE;
        }
        else
        {
            if (FALSE == dma_SetupPRDSize(unit, buffer, length, FALSE))
                dma = FALSE;
        }
    }

    err = atapi_SendPacket(unit, cmd->scsi_Command, cmd->scsi_Data, cmd->scsi_Length, &dma, (cmd->scsi_Flags & SCSIF_READ) == 0);

    DUMP({ if (cmd->scsi_Data != 0) dump(cmd->scsi_Data, cmd->scsi_Length); });

    /*
     * on check condition - grab sense data
     */
    DATAPI(bug("[ATA%02lx] SCSI Flags: %02lx / Error: %ld\n", unit->au_UnitNum, cmd->scsi_Flags, err));
    if ((err != 0) && (cmd->scsi_Flags & SCSIF_AUTOSENSE))
    {
       atapi_RequestSense(unit, cmd->scsi_SenseData, cmd->scsi_SenseLength);
       DUMP(dump(cmd->scsi_SenseData, cmd->scsi_SenseLength));
    }

    return err;
}

/*
 * chops the large transfers into set of smaller transfers
 * specifically useful when requested transfer size is >256 sectors for 28bit commands
 */
static ULONG ata_exec_blk(struct ata_Unit *unit, ata_CommandBlock *blk)
{
    ULONG err=0;
    ULONG part;
    ULONG max=256;
    ULONG count=blk->sectors;

    if (blk->type == CT_LBA48)
        max <<= 8;

    DATA(bug("[ATA%02ld] Accessing %ld sectors starting from %lx\n", unit->au_UnitNum, count, blk->blk));
    while ((count > 0) && (err == 0))
    {
        part = (count > max) ? max : count;
        blk->sectors = part;
        blk->length  = part << unit->au_SectorShift;

        err = ata_exec_cmd(unit, blk);

        blk->blk    += part;
        blk->buffer  = &((char*)blk->buffer)[part << unit->au_SectorShift];
        count -= part;
    }
    return err;
}

/*
 * Initial device configuration that suits *all* cases
 */
BOOL ata_init_unit(struct ata_Bus *bus, UBYTE u)
{
    struct ata_Unit *unit=NULL;

    DINIT(bug("[ATA  ] Initializing unit %ld\n", u));

    unit = bus->ab_Units[u];
    if (NULL == unit)
        return FALSE;

    unit->au_Bus        = bus;
    unit->au_Drive      = AllocPooled(bus->ab_Base->ata_MemPool, sizeof(struct DriveIdent));
    unit->au_UnitNum    = bus->ab_BusNum << 1 | u;      // b << 8 | u
    unit->au_DevMask    = 0xa0 | (u << 4);
    if (bus->ab_Base->ata_32bit)
    {
        unit->au_ins        = ata_insl;
        unit->au_outs       = ata_outsl;
    }
    else
    {
        unit->au_ins        = ata_insw;
        unit->au_outs       = ata_outsw;
    }
    unit->au_SectorShift= 9;    /* this really has to be set here. */
    unit->au_Flags      = 0;
    
    NEWLIST(&unit->au_SoftList);

    /*
     * since the stack is always handled by caller
     * it's safe to stub all calls with one function
     */
    unit->au_Read32                 = ata_STUB_IO32;
    unit->au_Read64                 = ata_STUB_IO64;
    unit->au_Write32                = ata_STUB_IO32;
    unit->au_Write64                = ata_STUB_IO64;
    unit->au_Eject                  = ata_STUB;
    unit->au_DirectSCSI             = ata_STUB_SCSI;
    unit->au_Identify               = ata_STUB;
    return TRUE;
}

BOOL ata_setup_unit(struct ata_Bus *bus, UBYTE u)
{
    struct ata_Unit *unit=NULL;

    /*
     * this stuff always goes along the same way
     * WARNING: NO INTERRUPTS AT THIS POINT!
     */
    DINIT(bug("[ATA  ] setting up unit %ld\n", u));

    unit = bus->ab_Units[u];
    if (NULL == unit)
        return FALSE;

    ata_SelectUnit(unit);

    if (FALSE == ata_WaitBusyTO(unit, 1, FALSE, NULL))
    {
        DINIT(bug("[ATA%02ld] ERROR: Drive not ready for use. Keeping functions stubbed\n", unit->au_UnitNum));
        FreePooled(bus->ab_Base->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
        unit->au_Drive = 0;
        return FALSE;
    }

    switch (bus->ab_Dev[u])
    {
        /*
         * safe fallback settings
         */
        case DEV_SATAPI:
        case DEV_ATAPI:
            unit->au_Identify       = atapi_Identify;
            break;

        case DEV_SATA:
        case DEV_ATA:
            unit->au_Identify       = ata_Identify;
            break;


        default:
            DINIT(bug("[ATA%02ld] Unsupported device %lx. All functions will remain stubbed.\n", unit->au_UnitNum, bus->ab_Dev[u]));
            FreePooled(bus->ab_Base->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
            unit->au_Drive = 0;
            return FALSE;
    }

    bug("[ATA  ] Enabling Bus IRQs\n");
    ata_EnableIRQ(bus, TRUE);
    /*
     * now make unit self diagnose
     */
    if (unit->au_Identify(unit) != 0)
    {
        FreePooled(bus->ab_Base->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
        unit->au_Drive = 0;
        return FALSE;
    }

    return TRUE;
}



/*
 * ata[pi] identify
 */
static void common_SetXferMode(struct ata_Unit* unit, ata_XferMode mode)
{
    UBYTE type=0;
    BOOL dma=FALSE;
    ata_CommandBlock acb =
    {
        0xef,
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

    if ((unit->au_DMAPort == 0) && (mode > AB_XFER_PIO7))
    {
        DINIT(bug("[ATA%02ld] This controller does not own DMA port! Will set best PIO\n", unit->au_UnitNum));
        common_SetBestXferMode(unit);
        return;
    }


    /*
     * first, ONLY for ATA devices, set new commands
     */
    if (0 == (unit->au_XferModes & AF_XFER_PACKET))
    {
        if ((mode >= AB_XFER_PIO0) & (mode <= AB_XFER_PIO7))
        {
            if ((!unit->au_Bus->ab_Base->ata_NoMulti) && (unit->au_XferModes & AF_XFER_RWMULTI))
            {
                ata_out(unit->au_Drive->id_RWMultipleSize & 0xFF, ata_Count, unit->au_Bus->ab_Port);
                ata_out(ATA_SET_MULTIPLE, ata_Command, unit->au_Bus->ab_Port);
                ata_WaitBusyTO(unit, -1, FALSE, NULL);

                unit->au_Read32         = ata_ReadMultiple32;
                unit->au_Write32        = ata_WriteMultiple32;
                if (unit->au_XferModes & AF_XFER_48BIT)
                {
                    unit->au_Read64         = ata_ReadMultiple64;
                    unit->au_Write64        = ata_WriteMultiple64;
                }
            }
            else
            {
                unit->au_Read32         = ata_ReadSector32;
                unit->au_Write32        = ata_WriteSector32;
                if (unit->au_XferModes & AF_XFER_48BIT)
                {
                    unit->au_Read64         = ata_ReadSector64;
                    unit->au_Write64        = ata_WriteSector64;
                }
            }
        }
        else if ((mode >= AB_XFER_MDMA0) & (mode <= AB_XFER_MDMA7))
        {
            unit->au_Read32         = ata_ReadDMA32;
            unit->au_Write32        = ata_WriteDMA32;
            if (unit->au_XferModes & AF_XFER_48BIT)
            {
                unit->au_Read64         = ata_ReadDMA64;
                unit->au_Write64        = ata_WriteDMA64;
            }
        }
        else if ((mode >= AB_XFER_UDMA0) & (mode <= AB_XFER_UDMA7))
        {
            unit->au_Read32         = ata_ReadDMA32;
            unit->au_Write32        = ata_WriteDMA32;
            if (unit->au_XferModes & AF_XFER_48BIT)
            {
                unit->au_Read64         = ata_ReadDMA64;
                unit->au_Write64        = ata_WriteDMA64;
            }
        }
        else
        {
            unit->au_Read32         = ata_ReadSector32;
            unit->au_Write32        = ata_WriteSector32;
            if (unit->au_XferModes & AF_XFER_48BIT)
            {
                unit->au_Read64         = ata_ReadSector64;
                unit->au_Write64        = ata_WriteSector64;
            }
        }
    }

    if ((mode >= AB_XFER_PIO0) & (mode <= AB_XFER_PIO7))
    {
        type = 8 + (mode - AB_XFER_PIO0);
    }
    else if ((mode >= AB_XFER_MDMA0) & (mode <= AB_XFER_MDMA7))
    {
        type = 32 + (mode - AB_XFER_MDMA7);
        dma=TRUE;
    }
    else if ((mode >= AB_XFER_UDMA0) & (mode <= AB_XFER_UDMA7))
    {
        type = 64 + (mode - AB_XFER_MDMA7);
        dma=TRUE;
    }
    else
    {
        type = 0;
    }

    acb.sectors = type;
    if (0 != ata_exec_cmd(unit, &acb))
    {
        DINIT(bug("[ATA%02ld] ERROR: Failed to apply new xfer mode.\n", unit->au_UnitNum));
    }

    if (unit->au_DMAPort)
    {
        type = ata_in(dma_Status, unit->au_DMAPort);
        type &= 0x60;
        if (dma)
        {
            type |= 1 << (5 + (unit->au_UnitNum & 1));
        }
        else
        {
            type &= ~(1 << (5 + (unit->au_UnitNum & 1)));
        }

        DINIT(bug("[DSCSI] Trying to apply new DMA (%lx) status: %02lx (unit %ld)\n", unit->au_DMAPort, type, unit->au_UnitNum & 1));

        ata_SelectUnit(unit);
        ata_out(type, dma_Status, unit->au_DMAPort);
        if (type == (ata_in(dma_Status, unit->au_DMAPort) & 0x60))
        {
            DINIT(bug("[DSCSI] New DMA Status: %02lx\n", type));
        }
        else
        {
            DINIT(bug("[DSCSI] Failed to modify DMA state for this device\n"));
            dma = FALSE;
        }
    }

    if (dma)
        unit->au_XferModes |= AF_XFER_DMA;
    else
        unit->au_XferModes &=~AF_XFER_DMA;
}
    
static void common_SetBestXferMode(struct ata_Unit* unit)
{
    int iter;
    int max = AB_XFER_UDMA7;

    if (unit->au_DMAPort == 0)
    {
        /*
         * make sure you reduce scan search to pio here!
         * otherwise this and above function will fall into infinite loop
         */
        DINIT(bug("[ATA%02ld] This controller does not own DMA port\n", unit->au_UnitNum));
        max = AB_XFER_PIO7;
    }

    for (iter=max; iter>=AB_XFER_PIO0; --iter)
    {
        if (unit->au_XferModes & (1<<iter))
        {
            common_SetXferMode(unit, iter);
            return;
        }
    }
    bug("[ATA%02ld] ERROR: device never reported any valid xfer modes. will continue at default\n", unit->au_UnitNum);
    common_SetXferMode(unit, AB_XFER_PIO0);
}

void common_DetectXferModes(struct ata_Unit* unit)
{
    int iter;
    
    DINIT(bug("[ATA%02ld] Supports\n", unit->au_UnitNum));

    if (unit->au_Drive->id_Commands4 & (1 << 4))
    {
        DINIT(bug("[ATA%02ld] - Packet interface\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_PACKET;
        unit->au_DirectSCSI     = atapi_DirectSCSI;
    }
    else if (unit->au_Drive->id_Commands5 & (1 << 10))
    {
        /* ATAPI devices do not use this bit. */
        DINIT(bug("[ATA%02ld] - 48bit I/O\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_48BIT;
    }
    
    if ((unit->au_XferModes & AF_XFER_PACKET) || (unit->au_Drive->id_Capabilities & (1<< 9)))
    {
        DINIT(bug("[ATA%02ld] - LBA Addressing\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_LBA;
    }
    else
    {
        DINIT(bug("[ATA%02ld] - DEVICE DOES NOT SUPPORT LBA ADDRESSING >> THIS IS A POTENTIAL PROBLEM <<\n", unit->au_UnitNum));
    }

    if (unit->au_Drive->id_RWMultipleSize & 0xff)
    {
        DINIT(bug("[ATA%02ld] - R/W Multiple (%ld sectors per xfer)\n", unit->au_UnitNum, unit->au_Drive->id_RWMultipleSize & 0xff));
        unit->au_XferModes     |= AF_XFER_RWMULTI;
    }

    if (unit->au_Drive->id_PIOSupport & 0xff)
    {
        DINIT(bug("[ATA%02ld] - ", unit->au_UnitNum));
        for (iter=0; iter<8; iter++)
        {
            if (unit->au_Drive->id_MWDMASupport & (1 << iter))
            {
                DINIT(bug("PIO%ld ", iter));
                unit->au_XferModes     |= AF_XFER_PIO(iter);;
            }
        }
        DINIT(bug("\n"));
    }

    if (unit->au_Drive->id_Capabilities & (1<<8))
    {
        DINIT(bug("[ATA%02ld] DMA:\n", unit->au_UnitNum));
        if (unit->au_Drive->id_MWDMASupport & 0xff)
        {
            DINIT(bug("[ATA%02ld] - ", unit->au_UnitNum));
            for (iter=0; iter<8; iter++)
            {
                if (unit->au_Drive->id_MWDMASupport & (1 << iter))
                {
                    unit->au_XferModes     |= AF_XFER_MDMA(iter);;
                    if (unit->au_Drive->id_MWDMASupport & (256 << iter))
                    {
                        DINIT(bug("[MDMA%ld] ", iter));
                    }
                    else
                    {
                        DINIT(bug("MDMA%ld ", iter));
                    }
                }
            }
            DINIT(bug("\n"));
        }

        if (unit->au_Drive->id_UDMASupport & 0xff)
        {
            DINIT(bug("[ATA%02ld] - ", unit->au_UnitNum));
            for (iter=0; iter<8; iter++)
            {
                if (unit->au_Drive->id_UDMASupport & (1 << iter))
                {
                    unit->au_XferModes     |= AF_XFER_MDMA(iter);;
                    if (unit->au_Drive->id_UDMASupport & (256 << iter))
                    {
                        DINIT(bug("[UDMA%ld] ", iter));
                    }
                    else
                    {
                        DINIT(bug("UDMA%ld ", iter));
                    }
                }
            }
            DINIT(bug("\n"));
        }
    }
}

#define SWAP_LE_WORD(x) (x) = AROS_LE2WORD((x))
#define SWAP_LE_LONG(x) (x) = AROS_LE2LONG((x))
#define SWAP_LE_QUAD(x) (x) = AROS_LE2LONG((x)>>32) | AROS_LE2LONG((x) & 0xffffffff) << 32

ULONG atapi_Identify(struct ata_Unit* unit)
{
    ata_CommandBlock acb =
    {
        ATA_IDENTIFY_ATAPI,
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

    ata_SelectUnit(unit);

    if (ata_exec_cmd(unit, &acb))
    {
        return IOERR_OPENFAIL;
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
    SWAP_LE_WORD(unit->au_Drive->id_PIO_MinCycleTImeIORDY);
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
    SWAP_LE_WORD(unit->au_Drive->id_EnchSecurityEraseTime);
    SWAP_LE_WORD(unit->au_Drive->id_CurrentAdvowerMode);
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

    unit->au_SectorShift    = 11;
    unit->au_Read32         = atapi_Read;
    unit->au_Write32        = atapi_Write;
    unit->au_DirectSCSI     = atapi_DirectSCSI;
    unit->au_Eject          = atapi_Eject;
    unit->au_Flags          = AF_DiscChanged;
    unit->au_DevType        = (unit->au_Drive->id_General >>8) & 0x1f;
    unit->au_XferModes      = AF_XFER_PACKET;

    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

    bug("[ATA%02ld] Unit info: %s / %s / %s\n", unit->au_UnitNum, unit->au_Model, unit->au_SerialNumber, unit->au_FirmwareRev);
    common_DetectXferModes(unit);
    common_SetBestXferMode(unit);

    if (unit->au_Drive->id_General & 0x80)
    {
        DINIT(bug("[ATA%02ld] Device is removable.\n", unit->au_UnitNum));
        unit->au_Flags |= AF_Removable;
    }

    unit->au_Capacity   = unit->au_Drive->id_LBASectors;
    unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;
    bug("[ATA%02ld] Unit info: %07lx 28bit / %04lx:%08lx 48bit addressable blocks\n", unit->au_UnitNum, unit->au_Capacity, (ULONG)(unit->au_Capacity48 >> 32), (ULONG)(unit->au_Capacity48 & 0xfffffffful));

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

    return 0;
}

ULONG ata_Identify(struct ata_Unit* unit)
{
    ata_CommandBlock acb =
    {
        ATA_IDENTIFY_DEVICE,
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

    if (ata_exec_cmd(unit, &acb))
    {
        return IOERR_OPENFAIL;
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
    SWAP_LE_WORD(unit->au_Drive->id_PIO_MinCycleTImeIORDY);
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
    SWAP_LE_WORD(unit->au_Drive->id_EnchSecurityEraseTime);
    SWAP_LE_WORD(unit->au_Drive->id_CurrentAdvowerMode);
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

    unit->au_SectorShift    = 9;
    unit->au_DevType        = DG_DIRECT_ACCESS;
    unit->au_Read32         = ata_ReadSector32;
    unit->au_Write32        = ata_WriteSector32;
    unit->au_DirectSCSI     = atapi_DirectSCSI;
    unit->au_Eject          = ata_Eject;
    unit->au_XferModes      = 0;
    unit->au_Flags         |= AF_DiscPresent | AF_DiscChanged;
    unit->au_DevType        = DG_DIRECT_ACCESS;

    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

    bug("[ATA%02ld] Unit info: %s / %s / %s\n", unit->au_UnitNum, unit->au_Model, unit->au_SerialNumber, unit->au_FirmwareRev);
    common_DetectXferModes(unit);
    common_SetBestXferMode(unit);

    if (unit->au_Drive->id_General & 0x80)
    {
        DINIT(bug("[ATA%02ld] Device is removable.\n", unit->au_UnitNum));
        unit->au_Flags |= AF_Removable;
    }

    unit->au_Capacity   = unit->au_Drive->id_LBASectors;
    unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;
    bug("[ATA%02ld] Unit info: %07lx 28bit / %04lx:%08lx 48bit addressable blocks\n", unit->au_UnitNum, unit->au_Capacity, (ULONG)(unit->au_Capacity48 >> 32), (ULONG)(unit->au_Capacity48 & 0xfffffffful));

    /*
       For drive capacities > 8.3GB assume maximal possible layout.
       It really doesn't matter here, as BIOS will not handle them in
       CHS way anyway :)
       i guess this just solves that weirdo div-by-zero crash, if anything else...
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
            sec = ~0ul;
        
        if (sec < unit->au_Capacity)
            sec = unit->au_Capacity;

        unit->au_Sectors    = 63;
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
    }
    return 0;
}


/*
 * ata read32 commands
 */
static ULONG ata_ReadSector32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
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
        CT_LBA28
    };
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_ReadMultiple32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
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
        CT_LBA28
    };
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_ReadDMA32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    register ULONG err;
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
        CT_LBA28
    };

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}


/*
 * ata read64 commands
 */
static ULONG ata_ReadSector64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
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
    register ULONG err = 0;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_ReadMultiple64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
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
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_ReadDMA64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
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
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}


/*
 * ata write32 commands 
 */
static ULONG ata_WriteSector32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
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
        CT_LBA28
    };
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_WriteMultiple32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
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
        CT_LBA28
    };
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_WriteDMA32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
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
        CT_LBA28
    };
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}


/*
 * ata write64 commands
 */
static ULONG ata_WriteSector64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
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
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_WriteMultiple64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
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
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

static ULONG ata_WriteDMA64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
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
    register ULONG err;

    *act = 0;
    if (0 != (err = ata_exec_blk(unit, &acb)))
        return err;

    *act = count << unit->au_SectorShift;
    return 0;
}

/*
 * ata miscellaneous commands
 */
static ULONG ata_Eject(struct ata_Unit *unit)
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

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_SenseData = (void*)&sense;
    sc.scsi_SenseLength = sizeof(sense);
    sc.scsi_Flags = SCSIF_AUTOSENSE;

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

    DATAPI(bug("[ATA%02ld] Test Unit Ready sense: %02lx, Media %s\n", unit->au_UnitNum, sense[2], unit->au_Flags & AF_DiscPresent ? "PRESENT" : "ABSENT"));
    return sense[2];
}

static ULONG atapi_Read(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    UBYTE cmd[] = { 
       SCSI_READ10, 0, block>>24, block>>16, block>>8, block, 0, count>>8, count, 0
    };
    struct SCSICmd sc = {
       0
    };

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_Data = buffer;
    sc.scsi_Length = count << unit->au_SectorShift;
    sc.scsi_Flags = SCSIF_READ;

    return unit->au_DirectSCSI(unit, &sc);
}

static ULONG atapi_Write(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    UBYTE cmd[] = { 
       SCSI_WRITE10, 0, block>>24, block>>16, block>>8, block, 0, count>>8, count, 0
    };
    struct SCSICmd sc = {
       0
    };

    sc.scsi_Command = (void*) &cmd;
    sc.scsi_CmdLength = sizeof(cmd);
    sc.scsi_Data = buffer;
    sc.scsi_Length = count << unit->au_SectorShift;
    sc.scsi_Flags = SCSIF_WRITE;

    return unit->au_DirectSCSI(unit, &sc);
}

static ULONG atapi_Eject(struct ata_Unit *unit)
{
    struct atapi_StartStop cmd = {
        command: SCSI_STARTSTOP,
        immediate: 1,
        flags: ATAPI_SS_EJECT,
    };

    struct SCSICmd sc = {
       0
    };

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
    DATAPI(bug("[SENSE] sensed data: %lx %lx %lx\n", sense[2]&0xf, sense[12], sense[13]));
    return ((sense[2]&0xf)<<16) | (sense[12]<<8) | (sense[13]);
}

ULONG ata_ReadSignature(struct ata_Bus *bus, int unit)
{
    ULONG port = bus->ab_Port;
    UBYTE tmp1, tmp2;

    ata_out(0xa0 | (unit << 4), ata_DevHead, port);
    ata_400ns(bus->ab_Alt);

    /* Check basic signature. All live devices should provide it */
    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);
    DINIT(bug("[ATA  ] Checking Count / LBA against expected values (%d:%d)\n", tmp1, tmp2));

    DINIT(bug("[ATA  ] Status %08lx Device %08lx\n", ata_in(ata_Status, port), ata_in(ata_DevHead, port)));

    if ((tmp1 == 0x01) && (tmp2 == 0x01))
    {
        /* Ok, ATA/ATAPI device. Get detailed signature */
        DINIT(bug("[ATA  ] Found an ATA[PI] Device. Attempting to detect specific subtype\n"));

        tmp1 = ata_in(ata_LBAMid, port);
        tmp2 = ata_in(ata_LBAHigh, port);
        
        DINIT(bug("[ATA  ] Subtype check returned %02lx:%02lx (%04lx)\n", tmp1, tmp2, (tmp1 << 8) | tmp2));

    
        switch ((tmp1 << 8) | tmp2)
        {
            case 0x0000:
                if (0 == (ata_ReadStatus(bus) & 0xfe))
                    return DEV_NONE;
                ata_out(0xa0 | (unit << 4), ata_DevHead, port);
                ata_out(ATA_EXECUTE_DIAG, ata_Command, port);
                ata_out(0xa0 | (unit << 4), ata_DevHead, port);

                while (ata_ReadStatus(bus) & ATAF_BUSY)
                    ata_400ns(bus->ab_Alt);
                    
                DINIT(bug("[ATA  ] Further validating  ATA signature: %lx & 0x7f = 1, %lx & 0x10 = unit\n", ata_in(ata_Error, port), ata_in(ata_DevHead, port)));

                if ((1 == (0x7f & ata_in(ata_Error, port))) && 
                    (unit == ((ata_in(ata_DevHead, port) >> 4) & 1)))
                {
                    DINIT(bug("[ATA  ] Found *valid* signature for ATA device\n"));
                    return DEV_ATA;
                }
                return DEV_NONE;

            case 0x14eb:
                DINIT(bug("[ATA  ] Found signature for ATAPI device\n"));
                return DEV_ATAPI;

            case 0x3cc3:
                DINIT(bug("[ATA  ] Found signature for SATA device\n"));
                return DEV_SATA;

            case 0x6996:
                DINIT(bug("[ATA  ] Found signature for SATAPI device\n"));
                return DEV_SATAPI;

            default:
                if (((tmp1 | tmp2) == 0xff) &&
                    ((tmp1 & tmp2) == 0x00))
                {
                    bug("[ATA  ] Found valid subtype, but don't know how to handle this device: %02lx %02lx\n", tmp1, tmp2);
                }
                else
                {
                    bug("[ATA  ] Invalid signature: %02lx %02lx\n", tmp1, tmp2);
                }
                return DEV_NONE;
        }
    }
        
    return DEV_NONE;
}

void ata_ResetBus(struct timerequest *tr, struct ata_Bus *bus)
{

    ULONG alt = bus->ab_Alt;
    ULONG port = bus->ab_Port;
    UBYTE sc, sn;

// Set and then reset the soft reset bit in the Device Control
// register.  This causes device 0 be selected.

    D(bug("[ATALOW] Reset\n"));
    ata_out(0xa0 | (0 << 4), ata_DevHead, port);    //Select it never the less
    ata_400ns(bus->ab_Alt);

    ata_out(0x04, ata_AltControl, alt);
    ata_usleep(tr, 10);                 /* minimum required: 5us */
    ata_out(0x02, ata_AltControl, alt);
    ata_usleep(tr, 4*1000);             /* minimum required: 2ms */

// If there is a device 0, wait for device 0 to set BSY=0.
    if (DEV_NONE != bus->ab_Dev[0]) {
        D(bug("[ATALOW] Wait DEV0 to clear BSY\n"));
        while ( 1 ) {
            ata_out(0xa0 | (0 << 4), ata_DevHead, port);
            ata_400ns(bus->ab_Alt);
            if((ata_ReadStatus(bus) & ATAF_BUSY) == 0)
                break;
        }
    }

// If there is a device 1, wait until device 1 allows
// register access.
    if (DEV_NONE != bus->ab_Dev[1]) {
        D(bug("[ATALOW] Wait DEV1 to allow access\n"));
        while ( 1 ) {
            ata_out(0xa0 | (1 << 4), ata_DevHead, port);
            ata_400ns(bus->ab_Alt);

            sc = ata_in(2, port);
            sn = ata_in(3, port);
            D(bug("sc = %x sn = %x\n",sc, sn));
            if ( ( sc == 0x01 ) && ( sn == 0x01 ) )
                break;
        }
        D(bug("[ATALOW] Wait DEV1 to clear BSY\n"));
        while(ata_ReadStatus(bus) & ATAF_BUSY);
    }

    bus->ab_Dev[0] = ata_ReadSignature(bus, 0);
    bus->ab_Dev[1] = ata_ReadSignature(bus, 1);
}

/*
 *  --------------------------------------------------------------
 * -  here ends the code that makes any sense. rest to be removed -
 *  --------------------------------------------------------------
 */


void ata_usleep(struct timerequest *tr, ULONG usec)
{
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_micro = usec % 1000000;
    tr->tr_time.tv_secs = usec / 1000000;
    
    DoIO((struct IORequest *)tr);
}

/*
    Device scan routines - TO BE REPLACED
        Note: This code checks if a drive "shadows" non existent drive's register
*/
/*
 * same here
 */
void ata_InitBus(struct ata_Bus *bus)
{
    ULONG port = bus->ab_Port;
    UBYTE tmp1, tmp2;

    bug("[ATA  ] ata_InitBus()\n");

    struct MsgPort *p = CreateMsgPort();
    struct timerequest *tr = (struct timerequest *)CreateIORequest((struct MsgPort *)p,
        sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0);

    bus->ab_Dev[0] = DEV_NONE;
    bus->ab_Dev[1] = DEV_NONE;
    
    /* Disable IDE IRQ */
    ata_EnableIRQ(bus, FALSE);

    /* Select device 0 */
    ata_out(0xa0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Write some pattern to registers */
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);
    ata_out(0xaa, ata_Count, port);
    ata_out(0x55, ata_LBALow, port);
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);

    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);

    if ((tmp1 == 0x55) && (tmp2 == 0xaa))
        bus->ab_Dev[0] = DEV_UNKNOWN;
    D(bug("[ATALOW] DEV0 type = %x",bus->ab_Dev[0]));

    /* Select device 1 */
    ata_out(0xb0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Write some pattern to registers */
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);
    ata_out(0xaa, ata_Count, port);
    ata_out(0x55, ata_LBALow, port);
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);

    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);

    if ((tmp1 == 0x55) && (tmp2 == 0xaa))
        bus->ab_Dev[1] = DEV_UNKNOWN;
    D(bug("[ATALOW] DEV1 type = %x",bus->ab_Dev[1]));

    ata_ResetBus(tr, bus);

    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(p);
    bug("[ATA  ] ata_InitBus: Finished\n");
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

static ULONG atapi_EndCmd(struct ata_Unit *unit)
{
    UBYTE status;

    /*
     * read alternate status register (per specs)
     */
    status = ata_in(ata_AltStatus, unit->au_Bus->ab_Alt);
    DATAPI(bug("[ATAPI] Alternate status: %lx\n", status));
       
    status = ata_in(atapi_Status, unit->au_Bus->ab_Port);

    DATAPI(bug("[ATAPI] Command complete. Status: %lx\n", status));
    
    if (!(status & ATAPIF_CHECK))
    {
        return 0;
    }
    else
    {
       status = ata_in(atapi_Error, unit->au_Bus->ab_Port);
       DATAPI(bug("[ATAPI] Error code %lx\n", status >> 4));
       return ErrorMap[status >> 4];
    }
}

/* 
 * vim: ts=4 et sw=4 fdm=marker fmr={,}
 */
