/*
    Copyright © 2001 AROS - The Amiga Research OS
    $Id$

    Desc: Intel HW floppy stuff
    Lang: English
*/

#include <exec/types.h>
#include <devices/trackdisk.h>

#include "trackdisk_device.h"
#include "trackdisk_hw.h"

#define DEBUG 1
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
	status = fd_inb(FD_STATUS);
	if (status & STATUS_READY)
	    return status;
    }
    return -1;
}


/* Start motor and select drive */
void td_motoron(UBYTE unitnum, struct TrackDiskBase *tdb)
{
    UBYTE dor;

    dor = tdb->td_dor & 0x30;
    dor |= 0x0c;
    dor |= (0x10 << unitnum);
    dor |= unitnum;
    tdb->td_dor = dor;

    outb(dor,FD_DOR);
    td_waitUntilReady();
}

/* Stop motor */
void td_motoroff(UBYTE unitnum, struct TrackDiskBase *tdb)
{
    tdb->td_dor = tdb->td_dor & ~(0x10 << unitnum);
    outb(tdb->td_dor,FD_DOR);
    td_waitUntilReady();
}

/* Just select a drive, leaving motorbits as is */
void td_select(UBYTE unitnum, struct TrackDiskBase *tdb)
{
    UBYTE dor;

    dor = tdb->td_dor;
    dor &= 0xfc;
    dor |= unitnum;
    tdb->td_dor = dor;
    outb(dor,FD_DOR);
}

/* Wait for interrupt */
int td_waitint(struct TrackDiskBase *TDBase)
{
    TDBase->td_iotime = 150;	// Each IO command has 3s to complete before error occurs
    Wait((1L << TDBase->td_IntBit));
    if (TDBase->td_iotime)
    {
	TDBase->td_iotime = 0;
	return 0;
    }
    return TDERR_NotSpecified;
}

// Send byte to drive. Returns DriveInUse error if busy, 0 otherwise
int td_sendbyte(unsigned char byte, struct TrackDiskBase *TDBase)
{
    TDBase->td_iotime=50;	//1s to send a command
    do
    {
	if ((fd_inb(FD_STATUS) & 0xc0)==0x80)
	{
	    fd_outb(byte, FD_DATA);
	    return 0;
	}
    } while (TDBase->td_iotime);
    D(bug("TD: sendbyte failed\n"));
    return TDERR_DriveInUse;

}

// Get byte from drive. Returns the same as td_sendbyte
int td_getbyte(unsigned char *byte, struct TrackDiskBase *TDBase)
{
    TDBase->td_iotime=50;
    do
    {
	if ((fd_inb(FD_STATUS) & 0xc0)==0xc0)
	{
	    *byte = fd_inb(FD_DATA);
	    return 0;
	}
    } while (TDBase->td_iotime);
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

	D(bug("TD: Resending command\n"));
	err = 0;
	fd_outb(0, FD_DOR);
	fd_outb(0, FD_DOR);
	fd_outb(4, FD_DOR);
	td_dinit(TDBase);
	// Resend command
	for (i=0; (i < TDBase->td_comsize) && !err; i++)
	    err = td_sendbyte(TDBase->td_rawcom[i], TDBase);
    }
    return err;
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
    if ((status & (STATUS_READY|STATUS_DIR|STATUS_DMA)) == STATUS_READY)
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
	td_sendbyte(0x5a, TDBase);
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


// Initialize drive
int td_dinit(struct TrackDiskBase *TDBase)
{
    int i;

    // deassert RESET signal
    fd_outb(0x0c, FD_DOR);
    /* Issue configure */
    td_configure(TDBase);
    // programm data rate
    fd_outb(0,FD_DCR);
    td_waitint(TDBase);
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
    td_motoron(unitn,TDBase);

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
	err = td_waitint(TDBase);
	if (!err)
	{
	    /* Issue Sense Interrupt Status command */
	    TDBase->td_comsize = 1;
	    TDBase->td_rawcom[0] = FD_SENSEI;
	    td_sendcommand(TDBase);
	    /* Get 2 bytes of status */
	    td_readstatus(TDBase,2);
	    /* if drive doesn't report any error return 0 */
	    if (((TDBase->td_result[0] & 0xf0) == 0x20) && (TDBase->td_result[1] == sector))
	    {
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
    td_motoron(unitn,TDBase);

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
	err = td_waitint(TDBase);
	if (!err)
	{
	    /* Issue Sense Interrupt Status command */
	    TDBase->td_comsize = 1;
	    TDBase->td_rawcom[0] = FD_SENSEI;
	    td_sendcommand(TDBase);
	    /* Get 2 bytes of status */
	    td_readstatus(TDBase,2);
	    /* if drive doesn't report any error return 0 */
	    if (((TDBase->td_result[0] & 0xf0) == 0x20))
	    {
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
    return(inb(FD_DIR)>>7);
}


int td_readwritetrack(UBYTE unitnum, char cyl, char hd, char mode, struct TrackDiskBase *TDBase)
{
    int rwcnt = 3;	// Read/Write retries
    int skcnt = 3;	// Seek retries
    char *buf;
    int err;		// Error

    /* Enable drive & motor */
    td_motoron(unitnum,TDBase);

    /* Program data rate */
    fd_outb(0, FD_DCR);	// 500kbit/s only!
    do
    {
	rwcnt = 3;	// Max 3 retries of read/write

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
	    err = td_waitint(TDBase);
	    if (!err)
	    {
		/* Read result bytes */
		td_readstatus(TDBase, 7);
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
    td_motoron(unit->tdu_UnitNum,TDBase);

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
    td_motoron(unit->tdu_UnitNum,TDBase);

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
