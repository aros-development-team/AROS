/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel HW floppy stuff
    Lang: English
*/

#include <exec/types.h>
#include <devices/trackdisk.h>

#include "trackdisk_device.h"
#include "trackdisk_hw.h"

#define DEBUG 0
#include <aros/debug.h>

#undef kprintf

#define ioStd(x)  ((struct IOStdReq *)x)

/*-------------------- Hardware section --------------------*/

/* All procedures here are based on Intel documentation */

#define TD_DMA	2		/* Define DMA channel */

/* Wait for status flags to settle */
int td_waitUntilReady(void)
{
    int i, status;

    for (i=0; i < 10000; i++)
    {
	status = inb(FDC_MSR);
	if (status & MSRF_RQM)
	    return status;
    }
    return -1;
}


/* Start motor and select drive */
void td_motoron(UBYTE unitnum, struct TrackDiskBase *tdb, BOOL wait)
{
    UBYTE dor;

    dor = tdb->td_dor & 0x30;
    dor |= 0x0c  | (0x10 << unitnum) | unitnum;
    tdb->td_dor = dor;

    outb(dor,FDC_DOR);
    /* Did the user want to wait for spinup? */
    if (wait)
    {
    	td_waitUntilReady();
    }
}

/* Stop motor */
void td_motoroff(UBYTE unitnum, struct TrackDiskBase *tdb)
{
    tdb->td_dor = tdb->td_dor & ~(0x10 << unitnum);
    outb(tdb->td_dor,FDC_DOR);
}

// Send byte to drive. Returns DriveInUse error if busy, 0 otherwise
int td_sendbyte(unsigned char byte, struct TrackDiskBase *TDBase)
{
    int i;

    for(i=0;i<100000;i++)
    {
	if ((inb(FDC_MSR) & (MSRF_RQM | MSRF_DIO))==MSRF_RQM)
	{
	    outb(byte, FDC_FIFO);
	    return 0;
	}
    }
    D(bug("TD: sendbyte failed\n"));
    return TDERR_DriveInUse;

}

// Get byte from drive. Returns the same as td_sendbyte
int td_getbyte(unsigned char *byte, struct TrackDiskBase *TDBase)
{
    int i;

    for(i=0;i<100000;i++)
    {
	if ((inb(FDC_MSR) & (MSRF_RQM | MSRF_DIO))==(MSRF_RQM | MSRF_DIO))
	{
	    *byte = inb(FDC_FIFO);
	    return 0;
	}
    }
    return TDERR_DriveInUse;
}

// Send full command to drive
int td_sendcommand(struct TrackDiskBase *TDBase)
{
    int err = 0;

    if (TDBase->td_comsize)
    {
	int i;
	for (i=0; (i < TDBase->td_comsize) && !err; i++)
	{
	    err = td_sendbyte(TDBase->td_rawcom[i], TDBase);
	}
    }
    // If there was error just reset drive
    if (err)
    {
	int i;

	kprintf("[Floppy] Resending command\n"));
	err = 0;
	td_dinit(TDBase);
	// Resend command
	for (i=0; (i < TDBase->td_comsize) && !err; i++)
	    err = td_sendbyte(TDBase->td_rawcom[i], TDBase);
    }
    return err;
}

/* Wait for interrupt */
int td_waitint(struct TrackDiskBase *TDBase, UBYTE results,BOOL sensei)
{
    int i;
    ULONG sigs;

    TDBase->td_inttmo = 150;	// Each IO command has 1s to complete before error occurs
    sigs = Wait((1L << TDBase->td_IntBit)|(1L<<TDBase->td_TmoBit));
    if (sigs & (1L << TDBase->td_IntBit))
    {
	TDBase->td_inttmo = 0;
	if (results)
	{
	    i = 0;
	    while ( (i<results) && (inb(FDC_MSR) & MSRF_CMDBSY) )
	    {
		td_getbyte(&TDBase->td_result[i],TDBase);
	    }
	}
	/* Do we run a Sense Interrupt? */
	if (sensei)
	{
	    td_sendbyte(FD_SENSEI,TDBase);
	    td_getbyte(&TDBase->td_sr0,TDBase);
	    td_getbyte(&TDBase->td_pcn,TDBase);
	}
	return 0;
    }
    return TDERR_NotSpecified;
}

// Read status bytes
int td_readstatus(struct TrackDiskBase *TDBase, int num)
{
    int i, err = 0;
    if (num > 7) num = 7;
    for (i=0; (i < num) && !err; i++)
    {
	err = td_getbyte(&TDBase->td_result[i], TDBase);
    }
    return err;
}

#define MORE_OUTPUT 1

int needMoreOutput(struct TrackDiskBase *TDBase)
{
    int status;

    if ((status=td_waitUntilReady())<0)
	return -1;
    if ((status & (MSRF_RQM|MSRF_DIO|MSRF_NONDMA)) == MSRF_RQM)
	return MORE_OUTPUT;
    td_readstatus(TDBase, 7);
    return 0;
}

int td_configure(struct TrackDiskBase *TDBase) 
{

    // Do Configure (enable FIFO and turn polling off)
    // We also enable implied seeks
    td_sendbyte(FD_CONFIGURE, TDBase);
    if (needMoreOutput(TDBase) == MORE_OUTPUT)
    {
	td_sendbyte(0, TDBase);
	td_sendbyte(0x1a, TDBase);
	td_sendbyte(0, TDBase);
	return 1;
    }
    return 0;
}

UBYTE td_getprotstatus(UBYTE unitnum, struct TrackDiskBase *tdb)
{
    /* Issue Sense Drive Status command to check whether write is possible */
    tdb->td_comsize = 2;
    tdb->td_rawcom[0] = FD_GETSTATUS;
    tdb->td_rawcom[1] = unitnum;
    td_sendcommand(tdb);
    td_readstatus(tdb,1);
    if (tdb->td_result[0] & 0x40)
	return(TDU_READONLY);
    else
	return(TDU_WRITABLE);
}


/*
 * Reset FDC, and initialize it again
 */

int td_dinit(struct TrackDiskBase *TDBase)
{
    int i;

    // Assert RESET in DOR
    outb(DORF_DMA, FDC_DOR);
    outb(DORF_DMA, FDC_DOR);
    // deassert RESET signal
    outb(DORF_RESET | DORF_DMA, FDC_DOR);
    td_waitint(TDBase,0,FALSE);
    /* Issue configure */
    td_configure(TDBase);
    // programm data rate
    outb(0,FDC_CCR);
    // issue Sense Interrupt Status (loop 4 times)
    for (i=0; i<4; i++)
    {
	TDBase->td_comsize = 1;
	TDBase->td_rawcom[0] = FD_SENSEI;
	td_sendcommand(TDBase);
	td_readstatus(TDBase, 2);
    }
    // issue Specify
    TDBase->td_comsize = 3;
    TDBase->td_rawcom[0] = FD_SPECIFY;
    TDBase->td_rawcom[1] = DP_SPEC1;
    TDBase->td_rawcom[2] = DP_SPEC2;
    td_sendcommand(TDBase);
    return 0;
}

// Recalibrate (type != 0) or seek (type = 0) command.
int td_recalibrate(unsigned char unitn, char type, int sector, struct TrackDiskBase *TDBase)
{
    int err;

    /* If we are not running already, this will spin up. */
    td_motoron(unitn,TDBase,FALSE);

    if (type)
    {
	/* Issue specify */
	TDBase->td_comsize = 3;
	TDBase->td_rawcom[0] = FD_SPECIFY;
	TDBase->td_rawcom[1] = DP_SPEC1;
	TDBase->td_rawcom[2] = DP_SPEC2;
	td_sendcommand(TDBase);
	/* Issue recalibrate */
	TDBase->td_comsize = 2;
	TDBase->td_rawcom[0] = FD_RECALIBRATE;
	TDBase->td_rawcom[1] = unitn;
	err = td_sendcommand(TDBase);
	sector = 0;
    }
    else
    {
	sector /= 2*DP_SECTORS; // cyl contains real cyl number
	/* Issue seek command */
	TDBase->td_comsize = 3;
	TDBase->td_rawcom[0] = FD_SEEK;
	TDBase->td_rawcom[1] = unitn;	/* Unit number */
	TDBase->td_rawcom[2] = sector;	/* New Track Number */
	err = td_sendcommand(TDBase);
    }
    if (!err)
    {
	/* Wait for interrupt */
	err = td_waitint(TDBase,0,TRUE);
	if (!err)
	{
	    /* if drive doesn't report any error return 0 */
	    if (((TDBase->td_sr0 & 0xf0) == 0x20) && (TDBase->td_pcn == sector))
	    {
		/* Store current track */
		TDBase->td_Units[unitn]->tdu_pcn = TDBase->td_pcn;
		return 0;
	    }
	    /* SeekError otherwise */
	    return TDERR_SeekError;
	}
    }
    return err;
}

int td_rseek(UBYTE unitn, UBYTE dir, UBYTE cyls, struct TrackDiskBase *TDBase)
{
    int err;

    /* Select the unit, start motor */
    td_motoron(unitn,TDBase,FALSE);

    if (dir)
    {
	/* Issue seek command */
	TDBase->td_comsize = 3;
	TDBase->td_rawcom[0] = FD_RSEEK_OUT;
	TDBase->td_rawcom[1] = unitn;	/* Unit number */
	TDBase->td_rawcom[2] = cyls;	/* New Track Number */
	err = td_sendcommand(TDBase);
    }
    else
    {
	TDBase->td_comsize = 3;
	TDBase->td_rawcom[0] = FD_RSEEK_IN;
	TDBase->td_rawcom[1] = unitn;
	TDBase->td_rawcom[2] = cyls;
	err = td_sendcommand(TDBase);
    }
    if (!err)
    {
	/* Wait for interrupt */
	err = td_waitint(TDBase,0,TRUE);
	if (!err)
	{
	    /* if drive doesn't report any error return 0 */
	    if (((TDBase->td_sr0 & 0xf0) == 0x20))
	    {
		if (dir)
		{
		    TDBase->td_Units[unitn]->tdu_pcn--;
		}
		else
		{
		    TDBase->td_Units[unitn]->tdu_pcn++;
		}
		return 0;
	    }
	    /* SeekError otherwise */
	    return TDERR_SeekError;
	}
    }
    return err;
}

/************************************************
 Name  : td_checkDisk
 Descr.: checks for disk in drive
 Input : none
 Output: 0 - disk in drive
         1 - no disk in drive
 Note  : The correct drive must be selected
*************************************************/
UBYTE td_getDiskChange( void )
{
    return(inb(FDC_DIR)>>DIRB_DCHG);
}


int td_readwritetrack(UBYTE unitnum, char cyl, char hd, char mode, struct TrackDiskBase *TDBase)
{
    int rwcnt = 3;	// Read/Write retries
    int skcnt = 3;	// Seek retries
    char *buf;
    int err;		// Error

    /* Program data rate */
    outb(0, FDC_CCR);	// 500kbit/s only!
    do
    {
	rwcnt = 3;	// Max 3 retries of read/write
	err = 0;
	/* If we are on the correct track, dont seek */
	if (TDBase->td_Units[unitnum]->tdu_pcn != ((cyl*DP_SECTORS)<<1))
		err = td_recalibrate(unitnum,0,(cyl*DP_SECTORS)<<1,TDBase);
	if (!err)
	{
	    do
	    {
		/* Clear err flag */
		err = 0;
		/* Set DMA up */
		clear_dma_ff(TD_DMA);
		// Should place some cache flush in future (when cache implemented)
		set_dma_addr(TD_DMA, (ULONG)(TDBase->td_Units[unitnum]->td_DMABuffer));
		set_dma_count(TD_DMA, DP_SECTORS*512);
		set_dma_mode(TD_DMA, (mode == FD_READ) ? DMA_MODE_READ : DMA_MODE_WRITE);
		enable_dma(TD_DMA);
		/* Issue read/write command */
		TDBase->td_comsize = 9;
		buf = TDBase->td_rawcom;
		*buf++ = mode;							// Command
		*buf++ = unitnum | (hd << 2);	// Drive Select
		*buf++ = cyl;					// Cylinder
		*buf++ = hd;					// Head
		*buf++ = 1;						// Sector
		*buf++ = DP_SSIZE;			// Sector size
		*buf++ = DP_SECTORS;			// End sector - the same as sec field for a while
		*buf++ = DP_GAP1;				// Gap length
		*buf++ = -1;					// DTL
		/* Command prepared, now send it */
		td_sendcommand(TDBase);
		/* Wait for end phase */
		err = td_waitint(TDBase,7,FALSE);
		if (!err)
		{
		    /* Check if everything went OK */
		    if (!(TDBase->td_result[0] & 0xc0))
		    {
			return 0;
		    }
		}
		/* Something went wrong. Let's see what. */
		/* if err != then timeout err. */
		if (!err)
		{	
		    err = TDERR_NotSpecified;
		    if (TDBase->td_result[1] & 0x80)
			err = TDERR_TooFewSecs;
		    else if (TDBase->td_result[1] & 0x20)
		    {
			err = TDERR_BadHdrSum;
			if (TDBase->td_result[2] & 0x20)
			    err = TDERR_BadSecSum;
		    }
		    else if (TDBase->td_result[1] & 0x04)
			err = TDERR_TooFewSecs;
		}
	    } while (--rwcnt);
	}
    } while(--skcnt);	
    return err;
}

int td_update(struct TDU *unit, struct TrackDiskBase *TDBase)
{
    int err;		// Error

    if (unit->tdu_flags & TDUF_WRITE)
    {
	if (td_getDiskChange())
	    return TDERR_DiskChanged;	/* No disk in drive */
	td_motoron(unit->tdu_UnitNum,TDBase,TRUE);
	err=td_readwritetrack(unit->tdu_UnitNum, unit->tdu_lastcyl, unit->tdu_lasthd, FD_WRITE, TDBase);
	if (err)
	    return err;
	unit->tdu_flags &= ~TDUF_WRITE;
    }
    return 0;
}

int td_readtrack(struct IOExtTD *iotd, UBYTE cyl, UBYTE hd, struct TrackDiskBase *TDBase)
{
    struct TDU *unit = (struct TDU *)iotd->iotd_Req.io_Unit;
    int err;		// Error

    if (td_getDiskChange())
	return TDERR_DiskChanged;	/* No disk in drive */
    if ((unit->tdu_lastcyl!=cyl) || (unit->tdu_lasthd!=hd))
    {
	err=td_update(unit, TDBase);
	if (err)
	{
	    return err;
	}
	err=td_readwritetrack(unit->tdu_UnitNum, cyl, hd, FD_READ, TDBase);
	if (err)
	    return err;
	unit->tdu_lastcyl=cyl;
	unit->tdu_lasthd=hd;
    }
    return 0;
}

int td_read(struct IOExtTD *iotd, struct TrackDiskBase *TDBase)
{
    struct TDU *unit=(struct TDU *)iotd->iotd_Req.io_Unit;
    int cyl, hd, sec;	// C/H/S for operation
    ULONG length=0;
    ULONG size;
    int err;		// Error

    if (unit->tdu_DiskIn == TDU_NODISK)
    {
	return TDERR_DiskChanged;
    }

    /* Gentlemen, start your engines */
    td_motoron(unit->tdu_UnitNum,TDBase,FALSE);

    sec = iotd->iotd_Req.io_Offset >> 9; // sector is wrong right now (LBA)
    cyl = (sec >> 1) / DP_SECTORS; // cyl contains real cyl number
    sec %= 2*DP_SECTORS;	// sector on track (on both sides)
    hd = sec / DP_SECTORS;	// head number
    sec %= DP_SECTORS;		// real sector number
    while (length<iotd->iotd_Req.io_Length)
    {
	size = (DP_SECTORS*512) - (sec*512);
	if (size>iotd->iotd_Req.io_Length)
	    size=iotd->iotd_Req.io_Length;
	err=td_readtrack(iotd, (UBYTE)cyl, (UBYTE)hd, TDBase);
	if (err)
	{
	    return err;
	}
	CopyMem(unit->td_DMABuffer+(sec*512), iotd->iotd_Req.io_Data+length, size);
	length += size;
	sec = 0;
	if (hd)
	{
	    hd=0;
	    cyl++;
	}
	else
	    hd=1;
    }
    return 0;
}

int td_write(struct IOExtTD *iotd, struct TrackDiskBase *TDBase)
{
    struct TDU *unit=(struct TDU *)iotd->iotd_Req.io_Unit;
    int cyl, hd, sec;	// C/H/S for operation
    ULONG length=0;
    ULONG size;
    int err;		// Error

    if (unit->tdu_DiskIn == TDU_NODISK)
    {
	return TDERR_DiskChanged;
    }

    /* Gentlemen, start your engines */
    td_motoron(unit->tdu_UnitNum,TDBase,TRUE);

    sec = iotd->iotd_Req.io_Offset >> 9; // sector is wrong right now (LBA)
    cyl = (sec >> 1) / DP_SECTORS; // cyl contains real cyl number
    sec %= 2*DP_SECTORS;	// sector on track (on both sides)
    hd = sec / DP_SECTORS;	// head number
    sec %= DP_SECTORS;		// real sector number

    while (length<iotd->iotd_Req.io_Length)
    {
	size = (DP_SECTORS*512) - (sec*512);
	if (size>iotd->iotd_Req.io_Length)
	    size=iotd->iotd_Req.io_Length;
	if (size<(DP_SECTORS*512))
	{
	    /* read new track only if we don't
	       want to write a full track
	       */
	    err=td_readtrack(iotd, (UBYTE)cyl, (UBYTE)hd, TDBase);
	    if (err)
		return err;
	}
	else
	{
	    err=td_update(unit, TDBase);
	    if (err)
		return err;
	    unit->tdu_lastcyl=(UBYTE)cyl;
	    unit->tdu_lasthd=(UBYTE)hd;
	}
	CopyMem(iotd->iotd_Req.io_Data+length, unit->td_DMABuffer+(sec*size), size);
	unit->tdu_flags |= TDUF_WRITE;
	length += size;
	sec = 0;
	if (hd)
	{
	    hd=0;
	    cyl++;
	}
	else
	    hd=1;
    }
    return 0;
}

/*
 * Format a track
 * Could probable need a lot more errorchecking/handling,
 * but atleast it works.
 */
int td_format(struct IOExtTD *iotd, struct TrackDiskBase *tdb)
{
    struct TDU *unit=(struct TDU *)iotd->iotd_Req.io_Unit;
    UBYTE *dmabuf;
    int cyl, hd, sec;
    int i,off;
    int err;		// Error

    /* Start motor */
    td_motoron(unit->tdu_UnitNum,tdb,TRUE);
    /* Set datarate */
    outb(0,FDC_CCR);

    /* Calculate CHS style address */
    sec = iotd->iotd_Req.io_Offset >> 9; // sector is wrong right now (LBA)
    cyl = (sec >> 1) / DP_SECTORS; // cyl contains real cyl number
    sec %= 2*DP_SECTORS;	// sector on track (on both sides)
    hd = sec / DP_SECTORS;	// head number

    /* Then go to the correct track */
    err = td_recalibrate(unit->tdu_UnitNum,0,(cyl*DP_SECTORS)<<1,tdb);
    if (err)
	return err;

    /* We start by filling the DMA buffer with values needed */
    dmabuf = (UBYTE *)unit->td_DMABuffer;
    off=0;
    for (i=1;i<=DP_SECTORS;i++)
    {
	dmabuf[off++] = (UBYTE)cyl;
	dmabuf[off++] = hd;
	dmabuf[off++] = i;
	dmabuf[off++] = 2;
    }

    /* Set DMA up */
    clear_dma_ff(TD_DMA);
    // Should place some cache flush in future (when cache implemented)
    set_dma_addr(TD_DMA, (ULONG)(unit->td_DMABuffer));
    set_dma_count(TD_DMA, 4*DP_SECTORS);
    set_dma_mode(TD_DMA, DMA_MODE_WRITE);
    enable_dma(TD_DMA);
    /* Issue format command */
    tdb->td_comsize = 6;
    tdb->td_rawcom[0] = FD_FORMAT;
    tdb->td_rawcom[1] = unit->tdu_UnitNum | (hd << 2);
    tdb->td_rawcom[2] = DP_SSIZE;
    tdb->td_rawcom[3] = DP_SECTORS;
    tdb->td_rawcom[4] = DP_GAP2;
    tdb->td_rawcom[5] = 0;
    /* Command prepared, now send it */
    td_sendcommand(tdb);
    /* Wait for end phase */
    err = td_waitint(tdb,7,FALSE);
    if (!err)
    {
	/* Check if everything went OK */
	if (!(tdb->td_result[0] & 0xc0))
	{
	    /* We are fine, now write that data! */
	    return td_write(iotd,tdb);
	}
    }
    return TDERR_NotSpecified;
}
