/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved
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
 */

#define DEBUG 1
#include <aros/debug.h>
#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <dos/bptr.h>

#include <proto/exec.h>
#include <devices/timer.h>

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

static ULONG atapi_ErrCmd();
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
 */

/*
 * the outsl and insl commands improperly assumed that every transfer is sized to multiple of four
 */
static VOID insw(APTR address, UWORD port, ULONG count)
{
    asm volatile ("cld; rep insw"::"Nd"(port),"c"(count >> 1),"D"(address):"memory");
}

static VOID insl(APTR address, UWORD port, ULONG count)
{
    asm volatile ("cld; testb $2,%%al; je _insl_; insw; _insl_: rep insl" ::"Nd"(port),"a"(count&2),"c"(count >> 2),"D"(address));
}

static VOID outsw(APTR address, UWORD port, ULONG count)
{
    asm volatile ("cld; rep outsw"::"Nd"(port),"c"(count >> 1),"S"(address));
}

static VOID outsl(APTR address, UWORD port, ULONG count)
{
    asm volatile ("cld; testb $2,%%al; je _outsl_; outsw; _outsl_: rep outsl" ::"Nd"(port),"a"(count&2),"c"(count >> 2),"S"(address));
}

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
    D(bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum));
    return CDERR_NOCMD;
}

static ULONG ata_STUB_IO32(struct ata_Unit *au, ULONG blk, ULONG len, APTR buf, ULONG* act)
{
    D(bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum));
    return CDERR_NOCMD;
}

static ULONG ata_STUB_IO64(struct ata_Unit *au, UQUAD blk, ULONG len, APTR buf, ULONG* act)
{
    bug("[ATA%02ld] CALLED STUB FUNCTION -- IO ACCESS TO BLOCK %08lx:%08lx, LENGTH %08lx. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum, (blk >> 32), (blk & 0xffffffff), len);
    return CDERR_NOCMD;
}

static ULONG ata_STUB_SCSI(struct ata_Unit *au, struct SCSICmd* cmd)
{
    D(bug("[ATA%02ld] CALLED STUB FUNCTION. THIS OPERATION IS NOT SUPPORTED BY DEVICE\n", au->au_UnitNum));
    return CDERR_NOCMD;
}

/*
 * Very short delay (TM) by someone who assumes slow data ports.
 * well, glad it works anyways.
 */
void ata_400ns()
{
    ata_in(ata_AltControl, 0x3f6);
    ata_in(ata_AltControl, 0x3f6);
    ata_in(ata_AltControl, 0x3f6);
    ata_in(ata_AltControl, 0x3f6);
}

inline void ata_SelectUnit(struct ata_Unit* unit)
{
    ata_out(unit->au_DevMask, ata_DevHead, unit->au_Bus->ab_Port);
}

inline void ata_ClearOldIRQ(struct ata_Unit *unit)
{
    SetSignal(0, (1 << unit->au_Bus->ab_SleepySignal) | SIGBREAKF_CTRL_C);
}

/*
 * wait for timeout or drive ready
 */
BOOL ata_WaitBusyTO(struct ata_Unit *unit, UWORD tout)
{
    UBYTE status;
    ULONG step = 0;
            
    SetSignal(0, SIGBREAKF_CTRL_C);

    Disable();
    unit->au_Bus->ab_Timeout = tout;
    Enable();

    do
    {
        status = ata_in(ata_Status, unit->au_Bus->ab_Port);
        ++step;
        if ((step & 16) == 0)
        {
            if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
            {
                D(bug("[ATA%02ld] Device still busy after timeout. Aborting\n", unit->au_UnitNum));
                return FALSE;   
            }
        }
        D(bug("[ATA%02ld] Still waiting (%02lx)...\n", unit->au_UnitNum, status));
    } while(status & ATAF_BUSY);

    D(bug("[ATA%02ld] Ready.\n", unit->au_UnitNum));
    Disable();
    unit->au_Bus->ab_Timeout = 0;
    Enable();
            
    SetSignal(0, SIGBREAKF_CTRL_C);

    return TRUE;
}

/*
 * wait for timeout or IRQ
 */
BOOL ata_WaitIRQ(struct ata_Unit *unit, UWORD tout)
{
    SetSignal(0, SIGBREAKF_CTRL_C);
            
    Disable();
    unit->au_Bus->ab_Timeout = tout;
    Enable();

    D(bug("[ATA%02ld] Awaiting IRQ.\n", unit->au_UnitNum));

    if (Wait((1<<unit->au_Bus->ab_SleepySignal) | SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
    {
        D(bug("[ATA%02ld] Timed out.\n", unit->au_UnitNum));
        return FALSE;
    }

    D(bug("[ATA%02ld] Ready.\n", unit->au_UnitNum));
    Disable();
    unit->au_Bus->ab_Timeout = 0;
    Enable();
            
    /*
     * clear signal just in case
     */
    SetSignal(0, SIGBREAKF_CTRL_C);

    return TRUE;
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
    UBYTE stat;
    ULONG port = au->au_Bus->ab_Port;
    ULONG err = 0;
    APTR mem = block->buffer;
    BOOL dma = FALSE;


    /*
     * initial checks and stuff
     */
    switch (block->method)
    {
        case CM_DMARead:
        case CM_DMAWrite:
            dma = TRUE;
            break;
        case CM_PIORead:
        case CM_PIOWrite:
        case CM_NoData:
            break;
        default:
            return IOERR_NOCMD;
    }

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



    /*
     * PRECONDITIONS: ENABLED INTRQ (0x08 written to ata_Command port)
     * 
     * - select device
     */
    ata_out(0x08, ata_AltControl, au->au_Bus->ab_Alt);
    D(bug("[ATA%02ld] Executing command %02lx\n", au->au_UnitNum, block->command));
    ata_out(au->au_DevMask, ata_DevHead, port);

    /*
     * generally we could consider marking unit as 'retarded' upon three attempts or stuff like that
     */
    if (ata_WaitBusyTO(au, 100) == FALSE)
    {
        bug("[ATA%02ld] UNIT BUSY AT SELECTION\n", au->au_UnitNum);
        return IOERR_UNITBUSY;
    }

    if (block->feature != 0)
    {
        ata_out(block->feature, ata_Feature, port);
    }


    /*
     * - clear all old signals
     */
    //D(bug("[ATA%02ld] Clearing old signals\n", au->au_UnitNum));
    SetSignal(0, (1 << au->au_Bus->ab_SleepySignal) | SIGBREAKF_CTRL_C);
    ata_in(ata_Status, port);



    /*
     * - set LBA and sector count
     */
    switch (block->type)
    {
        case CT_LBA28:
            //D(bug("[ATA%02ld] Command uses 28bit LBA addressing (OLD)\n", au->au_UnitNum));
            ata_out(((block->blk >> 24) & 0x0f) | 0x40 | au->au_DevMask, ata_DevHead, port);
            ata_out(block->blk >> 16, ata_LBAHigh, port);
            ata_out(block->blk >> 8, ata_LBAMid, port);
            ata_out(block->blk, ata_LBALow, port);
            ata_out(block->sectors, ata_Count, port);
            break;

        case CT_LBA48:
            //D(bug("[ATA%02ld] Command uses 48bit LBA addressing (NEW)\n", au->au_UnitNum));
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
            //D(bug("[ATA%02ld] Command does not address any block\n", au->au_UnitNum));
            break;
    }



    /*
     * setup DMA & push command
     */
    switch (block->method)
    {
        case CM_DMARead:
            //D(bug("[ATA%02ld] Initializing DMA for Read\n", au->au_UnitNum));
            dma_SetupPRDSize(au, mem, block->length, TRUE);
            CachePreDMA(mem, &block->length, DMA_ReadFromRAM);
            //D(bug("[ATA%02ld] Sending command\n", au->au_UnitNum));
            ata_out(block->command, ata_Command, port);
            dma_StartDMA(au);
            break;

        case CM_DMAWrite:
            //D(bug("[ATA%02ld] Initializing DMA for Write\n", au->au_UnitNum));
            dma_SetupPRDSize(au, mem, block->length, FALSE);
            CachePreDMA(mem, &block->length, 0);
            //D(bug("[ATA%02ld] Sending command\n", au->au_UnitNum));
            ata_out(block->command, ata_Command, port);
            dma_StartDMA(au);
            break;

        case CM_PIOWrite:
            //D(bug("[ATA%02ld] Sending command\n", au->au_UnitNum));
            ata_out(block->command, ata_Command, port);
            ata_400ns();
            break;

        case CM_PIORead:
        case CM_NoData:
            D(bug("[ATA%02ld] Sending command\n", au->au_UnitNum));
            ata_ClearOldIRQ(au);
            ata_out(block->command, ata_Command, port);
            if (FALSE == ata_WaitIRQ(au, 1000))
            {
                D(bug("[ATA%02ld] Device is late - no response\n", au->au_UnitNum));
                err = IOERR_UNITBUSY;
                break;
            }

            if (block->method == CM_PIORead)
                break;
            
            return 0;
    }


    /*
     * entering main command loop here.
     */   
    while (err == 0) 
    {
        /*
         * THEORY: PIO is entering stage H1. Waiting for drive to clear 'busy'
         * once busy clears and no data is requested, we're free to go.
         * DMA is waiting for the DMARQ to be raised
         */
        if (dma)
        {
            /*
             * we're not really expected to do much here. 
             * just wait for the dma transfer to complete
             * check if dma transfer completed, otherwise give it another wait.
             */
            stat = ata_in(dma_Status, au->au_DMAPort);
            D(bug("[ATA%02ld] DMA status %02lx\n", au->au_UnitNum, stat));

            if (stat & DMAF_Interrupt)
            {
                //D(bug("[ATA%02ld] DMA transfer is now complete\n", au->au_UnitNum));
                block->actual = block->length;
            }
            else if (stat & DMAF_Error)
            {
                //D(bug("[ATA%02ld] DMA ended with an error\n", au->au_UnitNum));
                err = IOERR_ABORTED;
            }
            else
            {
                //D(bug("[ATA%02ld] DMA transfer is still in progress\n", au->au_UnitNum));
            }
        }
        else
        {
            UWORD len = (block->secmul << au->au_SectorShift);

            /*
             * wait for drive to clear busy
             */
            if (FALSE == ata_WaitBusyTO(au, 1000))
            {
                bug("[ATA%02ld] Device busy after timeout\n", au->au_UnitNum);
                err = IOERR_UNITBUSY;
                break;
            }



            /*
             * check if we have the state: BSY=1 && DRQ=0 && DMARQ=1
             * this means dma is still progressing
             */
            stat = ata_in(ata_Status, port);
            if (0 == (stat & ATAF_DATAREQ))
            {
                bug("[ATA%02ld] No more data\n", au->au_UnitNum);
                break;
            }


            /*
             * THEORY:   we loop here: wait for interrupt, then wait for busy cleared, then
             *           while datareq transfer one long (or byte, or whatever).
             * PRACTICE: transfer all data as it goes, as do most other systems.
             */
            while (stat & ATAF_DATAREQ)
            {
                if (len > (block->length - block->actual))
                    len = block->length - block->actual;

                //D(bug("[ATA%02ld] Transferring %ld bytes to %08lx\n", au->au_UnitNum, len, mem));

                switch (block->method)
                {
                        /*
                         * first case
                         * data is ready to transfer over PIO
                         * actually we are expected to check status prior to each insw/insl,
                         * doubt anyone does this. could cause problems with SLOW ata devices
                         * right here.
                         * transferring one block at a time, no more.
                         */
                    case CM_PIORead:
                        D(bug("[ATA%02ld] Reading bytes (PIO)\n", au->au_UnitNum));
                        au->au_ins(mem, au->au_Bus->ab_Port, len);
                        block->actual += len;
                        mem = &((char*)mem)[block->actual];
                        break;

                        /*
                         * second case
                         * drive is ready to accept data over PIO
                         * we're supposed to check status prior to each outsw/outsl
                         * to make sure drive is actually ready to accept more data. 
                         * otherwise slow devices will have problems accepting flood
                         * right here
                         * transferring one block at a time, no more.
                         */
                    case CM_PIOWrite:
                        D(bug("[ATA%02ld] Writing bytes (PIO)\n", au->au_UnitNum));
                        au->au_outs(mem, au->au_Bus->ab_Port, len);
                        block->actual += len;
                        mem = &((char*)mem)[block->actual];
                        break;

                        /*
                         * we will never get here.
                         */
                    default:
                        break;

                }

                stat = ata_in(ata_Status, port);
                D(bug("[ATA%02ld] Current command status %02lx\n", au->au_UnitNum, stat));
                if (stat & ATAF_ERROR)
                {
                    err = IOERR_ABORTED;
                    bug("[ATA%02ld] Command %lx finished with an error. Aborting.\n", au->au_UnitNum, block->command);
                    break;
                }
            }

        }

        stat = ata_in(ata_Status, port);

        /*
         * check if previous transfer failed OR if we got all data we need
         */
        if ((err != 0) || (block->actual >= block->length))
            break;

        /*
         * THEORY: entering stage H0, waititng for drive to ping us with an interrupt
         */
        //D(bug("[ATA%02ld] Waiting for device to raise interrupt (data ready)\n", au->au_UnitNum));
        /*
         * wait for interrupt
         */
        if (FALSE == ata_WaitIRQ(au, 1000))
        {
            bug("[ATA%02ld] Device is late - no response\n", au->au_UnitNum);
            err = IOERR_UNITBUSY;
            break;
        }
    } 



    /*
     * clean up DMA
     * don't use 'mem' pointer here as it's already invalid.
     */
    if (block->method == CM_DMARead)
    {
        //D(bug("[ATA%02ld] Finalizing DMA Read\n", au->au_UnitNum));
        dma_StopDMA(au);
        CachePostDMA(block->buffer, &block->length, DMA_ReadFromRAM);
    }
    else if (block->method == CM_DMAWrite)
    {
        //D(bug("[ATA%02ld] Finalizing DMA Write\n", au->au_UnitNum));
        dma_StopDMA(au);
        CachePostDMA(block->buffer, &block->length, 0);
    }
    //D(bug("[ATA%02ld] All done\n", au->au_UnitNum));



    /*
     * signal error back to caller
     */
    if (0 != err)
        return err;

    stat = ata_in(ata_Status, port);
    if (stat & ATAF_ERROR)
        return IOERR_ABORTED;

    return 0;
}

/*
 * atapi packet iface
 */
int atapi_SendPacket(struct ata_Unit *unit, APTR packet, LONG datalen, BOOL *dma, BOOL write)
{
    *dma &= (unit->au_XferModes & AF_XFER_DMA) ? TRUE : FALSE;

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
        bug("[ATAPI] Sending %s ATA packet: ", (*dma) ? "DMA" : "PIO");
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

    ata_out(0x08, ata_AltControl, unit->au_Bus->ab_Alt);
    ata_out(unit->au_DevMask, atapi_DevSel, port);
    if (ata_WaitBusyTO(unit, 10))
    {
        /*
         * since the device is now ready (~BSY) && (~DRQ), we can set up features and transfer size
         */
        /*
         * we should consider using the DIRECTION flag here, too
         */
        ata_out(((*dma) ? 1 : 0) | ((write) ? 4 : 0), atapi_Features, port);
        ata_out((datalen & 0xff), atapi_ByteCntL, port);
        ata_out((datalen >> 8) & 0xff, atapi_ByteCntH, port);

        /*
         * once we're done with that, we can go ahead and inform device that we're about to send atapi packet
         * after command is dispatched, we are obliged to give 400ns for the unit to parse command and set status
         */
        ata_ClearOldIRQ(unit);
        ata_out(ATA_PACKET, atapi_Command, port);
        if (ata_WaitBusyTO(unit, 50) == FALSE)
        {
            D(bug("[ATAPI] Unit not ready to accept command.\n"));
            return IOERR_UNITBUSY;
        }

        /*
         * now we're waiting for the device to process following:
         * - set BSY to prepare for packet reception
         * - set COD bit and clear IO,
         * - set DRQ bit and finally
         * - clear BSY
         */
        /* 
         * at this point we can expect DATAREQ (that would indicate device wants to retrieve packet
         * OR some fault meaning device does not support packets
         */
        UBYTE status = ata_in(atapi_Status, port);
            
        if (status & ATAF_DATAREQ)
        {
            /*
             * if we got here, it means that device most likely expects us to send exactly 12 bytes
             * of packet data. no more, and no less. 12 bytes.
             */
            unit->au_outs(cmd, port, 12);
            return 1;
        }
        else
        {
            D(bug("[ATAPI] interface not ready to retrieve data. Status: %lx\n", status));
        }
    }
    else
    {
       D(bug("[ATAPI] WaitBusy failed at first check\n"));
    }

    return 0;
}

ULONG atapi_DirectSCSI(struct ata_Unit *unit, struct SCSICmd *cmd)
{
    ULONG port = unit->au_Bus->ab_Port;
    APTR buffer = cmd->scsi_Data;
    ULONG length = cmd->scsi_Length;
    ULONG size = 0;
    UBYTE status;
    UBYTE err = 0;
    BOOL dma = FALSE;

    cmd->scsi_Actual = 0;

    D(bug("[DSCSI] Sending packet!\n"));


    /*
     * setup DMA & push command
     * it does not really mean we will use dma here btw
     */
    if ((unit->au_XferModes & AF_XFER_DMA) && (cmd->scsi_Length !=0) && (cmd->scsi_Data != 0))
    {
        if ((cmd->scsi_Flags & SCSIF_READ) != 0)
        {
            dma_SetupPRDSize(unit, cmd->scsi_Data, cmd->scsi_Length, TRUE);
            CachePreDMA(cmd->scsi_Data, &cmd->scsi_Length, DMA_ReadFromRAM);
        }
        else
        {
            dma_SetupPRDSize(unit, cmd->scsi_Data, cmd->scsi_Length, FALSE);
            CachePreDMA(cmd->scsi_Data, &cmd->scsi_Length, 0);
        }
        dma = TRUE;
    }

    if (atapi_SendPacket(unit, cmd->scsi_Command, cmd->scsi_Length, &dma, (cmd->scsi_Flags & SCSIF_READ) == 0)) 
    {
        /*
         * PIO vs DMA
         */
        if (FALSE == dma)
        {
            if (ata_WaitIRQ(unit, 1000) == FALSE)
            {
                D(bug("[DSCSI] Command timed out.\n"));
                err = IOERR_UNITBUSY;
            }

            while (err == 0)
            {

                while ((status = ata_in(atapi_Status, port)) & ATAF_BUSY);
                D(bug("[DSCSI] Command status: %lx\n", status));

                if (status & ATAF_DATAREQ)
                {
                    UBYTE reason = ata_in(atapi_Reason, port);
                    D(bug("[DSCSI] Current status: %ld, SCSI flags: %ld\n", reason, cmd->scsi_Flags));

                    if (((cmd->scsi_Flags & SCSIF_READ) == SCSIF_READ) &&
                        ((reason & ATAPIF_MASK) == ATAPIF_READ))
                    {
                        size = ata_in(atapi_ByteCntH, port) << 8 |
                               ata_in(atapi_ByteCntL, port);
                        D(bug("[DSCSI] data available for read (%ld bytes, max: %ld bytes)\n", size, length));
                        if (size > length)
                        {
                            D(bug("[DSCSI] CRITICAL! MORE DATA THAN STORAGE ALLOWS: %ld bytes vs %ld bytes left!\n", size, length));
                            /* damnit!! need to report some critical error here! */
                            //size = length;
                        }
                        unit->au_ins(buffer, port, size);
                        D(bug("[DSCSI] %lu bytes read.\n", size));
                    }   
                    else if (((cmd->scsi_Flags & SCSIF_READ) == 0) &&
                             ((reason & ATAPIF_MASK) == ATAPIF_WRITE))
                    {
                        size = ata_in(atapi_ByteCntH, port) << 8 |
                               ata_in(atapi_ByteCntL, port);
                        D(bug("[DSCSI] device available for write\n"));
                        if (size > length)
                        {
                            D(bug("[DSCSI] CRITICAL! MORE DATA THAN STORAGE ALLOWS: %ld bytes vs %ld bytes left!\n", size, length));
                            /* damnit!! need to report some critical error here! */
                            //size = length;
                        }
                        unit->au_outs(buffer, port, size);
                        D(bug("[DSCSI] %lu bytes written.\n", size));
                    }
                    else
                    {
                        D(bug("[DSCSI] Drive data ready, but there's no storage to transfer. R != W\n"));
                        size = 0;
                        err = CDERR_InvalidState;
                        break;
                    }
               
                    cmd->scsi_Actual += size;
                    buffer += size;
                    length -= size;
                } 
                else
                {
                    D(bug("[DSCSI] User transfer complete, %ld bytes transferred.\n", cmd->scsi_Actual));
                    err = atapi_EndCmd(unit);
                    break;
                }
            } 
        }
        else
        {
            dma_StartDMA(unit);

            while (err == 0)
            {
                if (FALSE == ata_WaitIRQ(unit, 300))
                {
                    err = IOERR_UNITBUSY;
                    break;
                }

                status = ata_in(ata_Status, port);
                if (0 == (status & (ATAF_BUSY | ATAF_DATAREQ)))
                    break;

                status = ata_in(dma_Status, unit->au_DMAPort);
                D(bug("[ATAPI] DMA status: %lx\n", status));
                if (0 == (status & DMAF_Interrupt))
                {
                    break;
                }
                if (0 != (status & DMAF_Error))
                {
                    err = IOERR_ABORTED;
                    break;
                }
            } 
            dma_StopDMA(unit);
        }
    }
    else
    {
       err = atapi_ErrCmd();
    }

    /*
     * clean up DMA
     * don't use 'mem' pointer here as it's already invalid.
     */
    if (unit->au_XferModes & AF_XFER_DMA)
    {
        if ((cmd->scsi_Flags & SCSIF_READ) == SCSIF_READ)
        {
            CachePostDMA(cmd->scsi_Data, &cmd->scsi_Length, DMA_ReadFromRAM);
        }
        else if ((cmd->scsi_Flags & SCSIF_READ) == 0)
        {
            CachePostDMA(cmd->scsi_Data, &cmd->scsi_Length, 0);
        }
    }


    /*
     * on check condition - grab sense data
     */
    status = ata_in(atapi_Status, unit->au_Bus->ab_Port);
    if (((err != 0) || (status & ATAPIF_CHECK)) && (cmd->scsi_Flags & SCSIF_AUTOSENSE))
    {
       atapi_RequestSense(unit, cmd->scsi_SenseData, cmd->scsi_SenseLength);
       D(dump(cmd->scsi_SenseData, cmd->scsi_SenseLength));
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

    D(bug("[ATA%02ld] Accessing %ld sectors starting from %lx\n", unit->au_UnitNum, count, blk->blk));
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
BOOL ata_setup_unit(struct ata_Bus *bus, UBYTE u)
{
    struct ata_Unit *unit=NULL;

    /*
     * this stuff always goes along the same way
     */
    D(bug("[ATA  ] setting up unit %ld\n", u));

    unit = bus->ab_Units[u];
    if (NULL == unit)
        return FALSE;

    unit->au_Bus        = bus;
    unit->au_Drive      = AllocPooled(bus->ab_Base->ata_MemPool, sizeof(struct DriveIdent));
    unit->au_UnitNum    = bus->ab_BusNum << 1 | u;      // b << 8 | u
    unit->au_DevMask    = 0xa0 | (u << 4);
    if (bus->ab_Base->ata_32bit)
    {
        unit->au_ins        = insl;
        unit->au_outs       = outsl;
    }
    else
    {
        unit->au_ins        = insw;
        unit->au_outs       = outsw;
    }
    unit->au_SectorShift= 9;    /* this really has to be set here. */
    unit->au_Flags      = 0;
    
    NEWLIST(&unit->au_SoftList);

    /*
     * since the stack is always handled by caller
     * it's safe to stub all calls with one function
     */
    unit->au_Read32     = ata_STUB_IO32;
    unit->au_Read64     = ata_STUB_IO64;
    unit->au_Write32    = ata_STUB_IO32;
    unit->au_Write64    = ata_STUB_IO64;
    unit->au_Eject      = ata_STUB;
    unit->au_DirectSCSI = ata_STUB_SCSI;
    unit->au_Identify   = ata_STUB;

    ata_SelectUnit(unit);

    if (FALSE == ata_WaitBusyTO(unit, 10))
    {
        D(bug("[ATA%02ld] ERROR: Drive not ready for use. Keeping functions stubbed\n", unit->au_UnitNum));
        FreePooled(bus->ab_Base->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
        bus->ab_Base->ata_MemPool = 0;
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
            D(bug("[ATA%02ld] Unsupported device %lx. All functions will remain stubbed.\n", unit->au_UnitNum, bus->ab_Dev[u]));
            FreePooled(bus->ab_Base->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
            bus->ab_Base->ata_MemPool = 0;
            return FALSE;
    }

    /*
     * now make unit self diagnose
     */
    if (unit->au_Identify(unit) != 0)
    {
        FreePooled(bus->ab_Base->ata_MemPool, unit->au_Drive, sizeof(struct DriveIdent));
        bus->ab_Base->ata_MemPool = 0;
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
        D(bug("[ATA%02ld] This controller does not own DMA port! Will set best PIO\n", unit->au_UnitNum));
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
            if (unit->au_XferModes & AF_XFER_RWMULTI)
            {
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
        D(bug("[ATA%02ld] ERROR: Failed to apply new xfer mode.\n", unit->au_UnitNum));
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

        D(bug("[DSCSI] Trying to apply new DMA (%lx) status: %02lx (unit %ld)\n", unit->au_DMAPort, type, unit->au_UnitNum & 1));

        ata_SelectUnit(unit);
        ata_out(type, dma_Status, unit->au_DMAPort);
        if (type == (ata_in(dma_Status, unit->au_DMAPort) & 0x60))
        {
            D(bug("[DSCSI] New DMA Status: %02lx\n", type));
        }
        else
        {
            D(bug("[DSCSI] Failed to modify DMA state for this device\n"));
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
        D(bug("[ATA%02ld] This controller does not own DMA port\n", unit->au_UnitNum));
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
    
    D(bug("[ATA%02ld] Supports\n", unit->au_UnitNum));

    if (unit->au_Drive->id_Commands4 & (1 << 4))
    {
        D(bug("[ATA%02ld] - Packet interface\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_PACKET;
        unit->au_DirectSCSI     = atapi_DirectSCSI;
    }
    else if (unit->au_Drive->id_Commands5 & (1 << 10))
    {
        /* ATAPI devices do not use this bit. */
        D(bug("[ATA%02ld] - 48bit I/O\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_48BIT;
    }
    
    if ((unit->au_XferModes & AF_XFER_PACKET) || (unit->au_Drive->id_Capabilities & (1<< 9)))
    {
        D(bug("[ATA%02ld] - LBA Addressing\n", unit->au_UnitNum));
        unit->au_XferModes     |= AF_XFER_LBA;
    }
    else
    {
        D(bug("[ATA%02ld] - DEVICE DOES NOT SUPPORT LBA ADDRESSING >> THIS IS A POTENTIAL PROBLEM <<\n", unit->au_UnitNum));
    }

    if (unit->au_Drive->id_RWMultipleSize & 0xff)
    {
        D(bug("[ATA%02ld] - R/W Multiple (%ld sectors per xfer)\n", unit->au_UnitNum, unit->au_Drive->id_RWMultipleSize & 0xff));
        unit->au_XferModes     |= AF_XFER_RWMULTI;
    }

    if (unit->au_Drive->id_PIOSupport & 0xff)
    {
        D(bug("[ATA%02ld] - ", unit->au_UnitNum));
        for (iter=0; iter<8; iter++)
        {
            if (unit->au_Drive->id_MWDMASupport & (1 << iter))
            {
                D(bug("PIO%ld ", iter));
                unit->au_XferModes     |= AF_XFER_PIO(iter);;
            }
        }
        D(bug("\n"));
    }

    if (unit->au_Drive->id_Capabilities & (1<<8))
    {
        D(bug("[ATA%02ld] DMA:\n", unit->au_UnitNum));
        if (unit->au_Drive->id_MWDMASupport & 0xff)
        {
            D(bug("[ATA%02ld] - ", unit->au_UnitNum));
            for (iter=0; iter<8; iter++)
            {
                if (unit->au_Drive->id_MWDMASupport & (1 << iter))
                {
                    unit->au_XferModes     |= AF_XFER_MDMA(iter);;
                    if (unit->au_Drive->id_MWDMASupport & (256 << iter))
                    {
                        D(bug("[MDMA%ld] ", iter));
                    }
                    else
                    {
                        D(bug("MDMA%ld ", iter));
                    }
                }
            }
            D(bug("\n"));
        }

        if (unit->au_Drive->id_UDMASupport & 0xff)
        {
            D(bug("[ATA%02ld] - ", unit->au_UnitNum));
            for (iter=0; iter<8; iter++)
            {
                if (unit->au_Drive->id_UDMASupport & (1 << iter))
                {
                    unit->au_XferModes     |= AF_XFER_MDMA(iter);;
                    if (unit->au_Drive->id_UDMASupport & (256 << iter))
                    {
                        D(bug("[UDMA%ld] ", iter));
                    }
                    else
                    {
                        D(bug("UDMA%ld ", iter));
                    }
                }
            }
            D(bug("\n"));
        }
    }
}

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
    if (ata_WaitBusyTO(unit, 100) == FALSE)
    {
        bug("[ATA%02ld] Unit not ready after timeout. Aborting.\n", unit->au_UnitNum);
        return IOERR_UNITBUSY;
    }

    if (ata_exec_cmd(unit, &acb))
    {
        return IOERR_OPENFAIL;
    }

    D(dump(unit->au_Drive, sizeof(struct DriveIdent)));

    unit->au_SectorShift    = 11;
    unit->au_Read32         = atapi_Read;
    unit->au_Write32        = atapi_Write;
    unit->au_DirectSCSI     = atapi_DirectSCSI;
    unit->au_Eject          = atapi_Eject;
    unit->au_Flags         |= AF_DiscPresenceUnknown;
    unit->au_DevType        = (unit->au_Drive->id_General >>8) & 0x1f;
    unit->au_XferModes      = AF_XFER_PACKET;

    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

    D(bug("[ATA%02ld] Unit info: %s / %s / %s\n", unit->au_UnitNum, unit->au_Model, unit->au_SerialNumber, unit->au_FirmwareRev));
    common_DetectXferModes(unit);
    common_SetBestXferMode(unit);

    if (unit->au_Drive->id_General & 0x80)
    {
        D(bug("[ATA%02ld] Device is removable.\n", unit->au_UnitNum));
        unit->au_Flags |= AF_Removable;
    }

    unit->au_Capacity   = unit->au_Drive->id_LBASectors;
    unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;

    /*
     * ok, this is not very original, but quite compatible :P
     */
    switch (unit->au_DevType)
    {
        case DG_CDROM:
        case DG_WORM:
        case DG_OPTICAL_DISK:
            unit->au_SectorShift    = 11;
            unit->au_Flags         |= AF_SlowDevice;
            unit->au_Heads          = 1;
            unit->au_Sectors        = 75;
            unit->au_Cylinders      = 4440;
            break;

        case DG_DIRECT_ACCESS:
            unit->au_SectorShift = 9;
            if (!strcmp("LS-120", &unit->au_Model[0]))
            {
                unit->au_Flags     |= AF_SlowDevice;
                unit->au_Heads      = 2;
                unit->au_Sectors    = 18;
                unit->au_Cylinders  = 6848;
            }
            else if (!strcmp("ZIP 100 ", &unit->au_Model[8]))
            {
                unit->au_Flags     &= ~AF_SlowDevice;
                unit->au_Heads      = 1;
                unit->au_Sectors    = 64;
                unit->au_Cylinders  = 3072;
            }
            break;
    }
                        
    unit->au_NumLoop = 10000000;
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

    D(dump(unit->au_Drive, sizeof(struct DriveIdent)));

    unit->au_SectorShift    = 9;
    unit->au_DevType        = DG_DIRECT_ACCESS;
    unit->au_Read32         = ata_ReadSector32;
    unit->au_Write32        = ata_WriteSector32;
    unit->au_DirectSCSI     = atapi_DirectSCSI;
    unit->au_Eject          = ata_Eject;
    unit->au_XferModes      = 0;
    unit->au_Flags         |= AF_DiscPresent;
    unit->au_DevType        = DG_DIRECT_ACCESS;

    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

    D(bug("[ATA%02ld] Unit info: %s / %s / %s\n", unit->au_UnitNum, unit->au_Model, unit->au_SerialNumber, unit->au_FirmwareRev));
    common_DetectXferModes(unit);
    common_SetBestXferMode(unit);

    if (unit->au_Drive->id_General & 0x80)
    {
        D(bug("[ATA%02ld] Device is removable.\n", unit->au_UnitNum));
        unit->au_Flags |= AF_Removable;
    }

    unit->au_NumLoop = 4000000;

    unit->au_Capacity   = unit->au_Drive->id_LBASectors;
    unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;

    /*
       For drive capacities > 8.3GB assume maximal possible layout.
       It really doesn't matter here, as BIOS will not handle them in
       CHS way anyway :)
       i guess this just solves that weirdo div-by-zero crash, if anything else...
       */
    if (unit->au_Drive->id_LBASectors > (63*255*1024))
    {
        unit->au_Cylinders  = unit->au_Drive->id_LBASectors / (255*63);
        unit->au_Heads      = 255;
        unit->au_Sectors    = 63;
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
    UBYTE sense[4] = {
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
    D(bug("[ATA%02ld] Test Unit Ready sense: %02lx\n", unit->au_UnitNum, sense[2]));
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
    D({
        int t,p,q;
        for (q=0; q<((senselen+15)>>4); q++)
        {
            p = 16;
            if ((senselen - (q<<4)) < p)
                p = (senselen - (q<<4));

            bug("[SENSE] %02lx:", q<<4);
            for (t=0; t<p; t++)
                bug("%02lx ", sense[(q<<4) + t]);
            bug("\n");
        }
    });

    D(bug("[SENSE] sensed data: %lx %lx %lx\n", sense[2]&0xf, sense[12], sense[13]));
    return ((sense[2]&0xf)<<16) | (sense[12]<<8) | (sense[13]);
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
*/
void ata_ResetBus(struct timerequest *tr, struct ata_Bus *bus)
{
    ULONG port = bus->ab_Port;
    ULONG alt = bus->ab_Alt;
    int cnt;

    /* Exclusive use of ATA registers */
    ObtainSemaphore(&bus->ab_Lock);

    /* Disable IRQ */
    ata_out(0x0a, ata_AltControl, alt);
    /* Issue software reset */
    ata_out(0x0e, ata_AltControl, alt);
    /* wait a while */
    ata_usleep(tr, 400);
    /* Clear reset signal */
    ata_out(0x0a, ata_AltControl, alt);
    /* And wait again */
    ata_usleep(tr, 400);
    /* wait for dev0 to come online. Limited delay up to 30µs */
    if (bus->ab_Dev[0] != DEV_NONE)
    {
        cnt=1000;    // 1000ms delay for slowest devices to reply.
        while (cnt--)
        {
            if ((ata_in(ata_Status, port) & ATAF_BUSY) == 0)
                break;
            ata_usleep(tr, 1000);
        }
    }

    /* wait for dev1 to come online. Limited delay up to 30µs */
    if (bus->ab_Dev[1] != DEV_NONE)
    {
        ata_out(0xb0, ata_DevHead, port);
        ata_usleep(tr, 100);

        cnt=1000;
        while (cnt--)
        {
            if ((ata_in(ata_Status, port) & ATAF_BUSY) == 0)
                break;

            ata_usleep(tr, 1000);
        }
    }

    ata_out(0x08, ata_AltControl, alt);
    ReleaseSemaphore(&bus->ab_Lock);
}

/*
 * same here
 */
void ata_ScanBus(struct ata_Bus *bus)
{
    ULONG port = bus->ab_Port;
    UBYTE tmp1, tmp2;

    struct MsgPort *p = CreateMsgPort();
    struct timerequest *tr = (struct timerequest *)CreateIORequest((struct MsgPort *)p,
        sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0);

    /* Exclusive use of ATA registers */
    ObtainSemaphore(&bus->ab_Lock);

    bus->ab_Dev[0] = DEV_NONE;
    bus->ab_Dev[1] = DEV_NONE;

    /* Disable IDE IRQ */
    ata_out(0x0a, ata_AltControl, bus->ab_Alt);

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

    /* 
     * According to ATA specs it is quite possible, that the dev0 will respond
     * for both self and dev1. Similar, when only dev1 is available, it may as
     * well respond as dev0. Do more precise test now.
     */
    ata_out(0xa0, ata_DevHead, port);
    ata_usleep(tr, 100);
    ata_ResetBus(tr, bus);
    
    /* check device 0 */
    ata_out(0xa0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Check basic signature. All live devices should provide it */
    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);
    D(bug("[ATA  ] Checking Count / LBA against expected values (%d:%d)\n", tmp1, tmp2));

    if ((tmp1 == 0x01) && (tmp2 == 0x01))
    {
        /* Ok, ATA/ATAPI device. Get detailed signature */
        D(bug("[ATA  ] Found an ATA[PI] Device[0]. Attempting to detect specific subtype\n"));

        bus->ab_Dev[0] = DEV_UNKNOWN;
        tmp1 = ata_in(ata_LBAMid, port);
        tmp2 = ata_in(ata_LBAHigh, port);
        
        D(bug("[ATA  ] Subtype check returned %02lx:%02lx\n", tmp1, tmp2));
    
        if ((tmp1 == 0x14) && (tmp2 == 0xeb))
        {
            bus->ab_Dev[0] = DEV_ATAPI;
        }
        else if ((tmp1 == 0) && (tmp2 == 0) && ((ata_in(ata_Status, port) & 0xfe) != 0))
        {
            bus->ab_Dev[0] = DEV_ATA;
        }
        else if ((tmp1 == 0x3c) && (tmp2 == 0xc3))
        {
            bus->ab_Dev[0] = DEV_SATA;
        }
        else if ((tmp1 == 0x69) && (tmp2 == 0x96))
        {
            bus->ab_Dev[0] = DEV_SATAPI;
        }
    }

    /* check device 1 */
    ata_out(0xb0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Check basic signature. All live devices should provide it */
    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);
    D(bug("[ATA  ] Checking Count / LBA against expected values (%d:%d)\n", tmp1, tmp2));
 
    if ((tmp1 == 0x01) && (tmp2 == 0x01))
    {
        D(bug("[ATA  ] Found an ATA[PI] Device[1]. Attempting to detect specific subtype\n"));
        /* Ok, ATA/ATAPI device. Get detailed signature */
        bus->ab_Dev[1] = DEV_UNKNOWN;
        tmp1 = ata_in(ata_LBAMid, port);
        tmp2 = ata_in(ata_LBAHigh, port);
        
        D(bug("[ATA  ] Subtype check returned %02lx:%02lx\n", tmp1, tmp2));

        if ((tmp1 == 0x14) && (tmp2 == 0xeb))
        {
            bus->ab_Dev[1] = DEV_ATAPI;
        }
        else if ((tmp1 == 0) && (tmp2 == 0) && ((ata_in(ata_Status, port) & 0xfe) != 0))
        {
            bus->ab_Dev[1] = DEV_ATA;
        }
        else if ((tmp1 == 0x3c) && (tmp2 == 0xc3))
        {
            bus->ab_Dev[0] = DEV_SATA;
        }
        else if ((tmp1 == 0x69) && (tmp2 == 0x96))
        {
            bus->ab_Dev[0] = DEV_SATAPI;
        }
    }

    /*
     * restore IRQ
     */
    ata_out(0x08, ata_AltControl, bus->ab_Alt);
    ReleaseSemaphore(&bus->ab_Lock);

    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(p);
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

static ULONG atapi_ErrCmd()
{
    return CDERR_ABORTED;
}

static ULONG atapi_EndCmd(struct ata_Unit *unit)
{
    unit->au_Flags |= AF_Used;
    UBYTE status;

    /*
     * read alternate status register (per specs)
     */
    status = ata_in(ata_AltStatus, unit->au_Bus->ab_Alt);
    D(bug("[ATAPI] Alternate status: %lx\n", status));
       
    status = ata_in(atapi_Status, unit->au_Bus->ab_Port);

    D(bug("[ATAPI] Command complete. Status: %lx\n", status));
    
    if (!(status & ATAPIF_CHECK))
    {
        return 0;
    }
    else
    {
       status = ata_in(atapi_Error, unit->au_Bus->ab_Port);
       D(bug("[ATAPI] Error code %lx\n", status >> 4));
       return ErrorMap[status >> 4];
    }
}

/* 
 * vim: ts=4 et sw=4 fdm=marker fmr={,}
 */
