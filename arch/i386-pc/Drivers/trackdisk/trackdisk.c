/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: TrackDisk device
    Lang: English
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <devices/trackdisk.h>
#include <proto/exec.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/lists.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include "trackdisk_intern.h"
#include <proto/expansion.h>
#include <libraries/expansionbase.h>
#include <dos/filehandler.h>

#include <oop/oop.h>
#include <proto/oop.h>
#include <hidd/irq.h>

#define DEBUG 0 
#include <aros/debug.h>

#undef kprintf

#define ioStd(x)  ((struct IOStdReq *)x)

void td_floppyint(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
void td_floppytimer(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

int td_update(struct IOExtTD *, struct TrackDiskBase *);
int td_read(struct IOExtTD *, struct TrackDiskBase *);
int td_write(struct IOExtTD *, struct TrackDiskBase *);

int td_recalibrate(unsigned char , char , int , struct IOExtTD *,struct TrackDiskBase *);

int td_dinit(struct TrackDiskBase *TDBase);

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable=0;

struct TrackDiskBase *AROS_SLIB_ENTRY(init, TrackDisk)();
void AROS_SLIB_ENTRY(open, TrackDisk)();
BPTR AROS_SLIB_ENTRY(close, TrackDisk)();
BPTR AROS_SLIB_ENTRY(expunge, TrackDisk)();
int  AROS_SLIB_ENTRY(null, TrackDisk)();
void AROS_SLIB_ENTRY(beginio, TrackDisk)();
LONG AROS_SLIB_ENTRY(abortio, TrackDisk)();

static const char end;

int AROS_SLIB_ENTRY(entry, TrackDisk)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident TrackDisk_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&TrackDisk_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    4,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[] = "trackdisk.device";

static const char version[] = "$VER: trackdisk.device 41.0 (09.04.2000)\r\n";

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct TrackDiskBase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init, TrackDisk)
};

static void *const functable[] =
{
    &AROS_SLIB_ENTRY(open, TrackDisk),
    &AROS_SLIB_ENTRY(close, TrackDisk),
    &AROS_SLIB_ENTRY(expunge, TrackDisk),
    &AROS_SLIB_ENTRY(null, TrackDisk),
    &AROS_SLIB_ENTRY(beginio, TrackDisk),
    &AROS_SLIB_ENTRY(abortio, TrackDisk),
    (void *)-1
};

AROS_LH2(struct TrackDiskBase *,  init,
 AROS_LHA(struct TrackDiskBase *, TDBase, D0),
 AROS_LHA(BPTR,         segList, A0),
	  struct ExecBase *, sysBase, 0, TrackDisk)
{
    AROS_LIBFUNC_INIT

struct DeviceNode *devnode;
ULONG *pp;


    int i;
    ULONG drives;
	struct ExpansionBase *ExpansionBase;

    TDBase->sysbase=sysBase;
    InitSemaphore(&TDBase->io_lock);

    /* Build list of all available units */
    NEWLIST((struct List*)&TDBase->units);
    {
	/* Install floppy interrupts */
	struct Library	*OOPBase;
	
	OOPBase = OpenLibrary(AROSOOP_NAME, 0);
	
	if (OOPBase)
	{
	    OOP_Object *o;
	    
	    o = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
	    
	    if (o)
	    {
		HIDDT_IRQ_Handler *irq;
		
		irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);

		if(!irq)
		{
			/* PANIC! No memory for trackdisk IntServer ! */
			Alert(AT_DeadEnd|AO_TrackDiskDev|AN_IntrMem);
		}
		irq->h_Node.ln_Pri=127;		/* Set the highest pri */
		irq->h_Node.ln_Name = (STRPTR)name;
		irq->h_Code = td_floppyint;
		irq->h_Data = (APTR)TDBase;
		
		HIDD_IRQ_AddHandler(o, irq, vHidd_IRQ_Floppy);
		
		irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);

		if(!irq)
		{
			/* PANIC! No memory for trackdisk IntServer ! */
			Alert(AT_DeadEnd|AO_TrackDiskDev|AN_IntrMem);
		}
		irq->h_Node.ln_Pri=10;		/* Set the highest pri */
		irq->h_Node.ln_Name = (STRPTR)name;
		irq->h_Code = td_floppytimer;
		irq->h_Data = (APTR)TDBase;
		
		HIDD_IRQ_AddHandler(o, irq, vHidd_IRQ_Timer);
		
		OOP_DisposeObject(o);
	    }
	    CloseLibrary(OOPBase);
	}
    }	
    /* Get installed drives info */

    asm volatile (
    /*
     * This does not work on all machines for some reason...
     */
/******************************
    	"inb $0x70,%%al		\n\t"
    	"andb $0xc0,%%al	\n\t"
    	"orb $0x10,%%al		\n\t"
******************************/
    /* 
     * ... therefore I added the mov command and then it works fine.
     * 0x10 + 0x80 : $0x10 = disk drive info (like the orb $0x10 above)
     *               $0x80 = mask the NMI bit. Otherwise keys reapeat themselves.
     */
    	"movb $0x90,%%al	\n\t"
    	"outb %%al,$0x70	\n\t"
   	"inb $0x71,%%al		\n\t"
    	"rorb $4,%%al		\n\t"
    	"andl $0xff,%%eax"
    	:"=a"(drives)
    	:
    	:"cc");

    /* Build units */
    	
	ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",40);
    for (i=0; i<2; i++)
    {
    	struct TDU *unit=NULL;

	/* If there is defined a drive */
    	if (((drives>>(4*i))&15)!=0)
    	{
	    /* Get mem for it */
	    unit=(struct TDU*)AllocMem(sizeof(struct TDU), MEMF_CLEAR|MEMF_PUBLIC);
	    if (!unit)	/* PANIC! No memory for units! */
    	   	Alert(AT_DeadEnd|AO_TrackDiskDev|AG_NoMemory);
	    unit->pub.tdu_StepDelay=4;	/* Standard values here */
	    unit->pub.tdu_SettleDelay=16;
	    unit->pub.tdu_RetryCount=3;
	    unit->unitnum=i;
	    unit->unittype=(drives>>(4*i))&15;
	    
	    /* Alloc memory for track buffering */
	    unit->dma_buffer=AllocMem(DP_SECTORS*512,MEMF_CLEAR|MEMF_CHIP);

	    if (!unit->dma_buffer)
	    {
    	    	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
	    }

	    /* If buffer doesn't fit into DMA page realloc it */
	    if (( (((ULONG)unit->dma_buffer + DP_SECTORS*512) & 0xffff0000) -
		  ((ULONG)unit->dma_buffer&0xffff0000) ) != 0)
	    {
    	    	APTR buffer;

    	    	buffer = AllocMem(DP_SECTORS*512, MEMF_CLEAR | MEMF_CHIP);

    	    	if (!buffer)
		{
    	    	    Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
		}

    	    	FreeMem(unit->dma_buffer, DP_SECTORS*512);
		unit->dma_buffer = buffer;
	    }

	    unit->flags = 0;
	    unit->lastcyl = -1;
	    unit->lasthd = -1;

	    if (ExpansionBase)
	    {
		pp = (ULONG *)AllocMem(24*4,MEMF_PUBLIC|MEMF_CLEAR);

		if (pp)
		{
		    pp[0] = (ULONG)"afs.handler";
		    pp[1] = (ULONG)name;
		    pp[2] = i;
		    pp[DE_TABLESIZE + 4] = DE_BOOTBLOCKS;
		    pp[DE_SIZEBLOCK + 4] = 128;
		    pp[DE_NUMHEADS + 4] = 2;
		    pp[DE_SECSPERBLOCK + 4] = 1;
		    pp[DE_BLKSPERTRACK + 4] = 18;
		    pp[DE_RESERVEDBLKS + 4] = 2;
		    pp[DE_LOWCYL + 4] = 0;
		    pp[DE_HIGHCYL + 4] = 79;
		    pp[DE_NUMBUFFERS + 4] = 10;
		    pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC | MEMF_CHIP;
		    pp[DE_MAXTRANSFER + 4] = 0x00200000;
		    pp[DE_MASK + 4] = 0x7FFFFFFE;
		    pp[DE_BOOTPRI + 4] = 5;
		    pp[DE_DOSTYPE + 4] = 0x444F5300;
		    pp[DE_BOOTBLOCKS + 4] = 2;
		    devnode = MakeDosNode(pp);

		    if (devnode)
		    {
			devnode->dn_OldName = MKBADDR(AllocMem(5, MEMF_PUBLIC | MEMF_CLEAR));
			
			if (devnode->dn_OldName != NULL)
			{
			    AROS_BSTR_putchar(devnode->dn_OldName, 0, 'D');
			    AROS_BSTR_putchar(devnode->dn_OldName, 1, 'F');
			    AROS_BSTR_putchar(devnode->dn_OldName, 2, '0' + i);
			    AROS_BSTR_setstrlen(devnode->dn_OldName, 3);
			    devnode->dn_NewName = AROS_BSTR_ADDR(devnode->dn_OldName);

			    AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
			}
		    }
		}
	    }
    	}
	
    	TDBase->units[i] = (struct TDU*)unit;
    	unit = NULL;
    }

    if (ExpansionBase)
    {
	CloseLibrary((struct Library *)ExpansionBase);
    }

    TDBase->units[2] = NULL;	/* Third and fourth drives are disabled now */
    TDBase->units[3] = NULL;
    
    fd_outb(0, FD_DOR);	/* Reset controller */
    fd_outb(0, FD_DOR);
    fd_outb(4, FD_DOR);	/* Turn it on again */
    
    td_dinit(TDBase);	/* Init drives */
    
    return TDBase;

    AROS_LIBFUNC_EXIT
}


AROS_LH3(void, open,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct TrackDiskBase *, TDBase, 1, TrackDisk)
{
    AROS_LIBFUNC_INIT

    struct TDU *unit;

    ObtainSemaphore(&TDBase->io_lock);
    flags=0;	// Should add flags support soon
    
    /* If there is something to alloc */
    if (!(TDBase->units[unitnum]))
    {
    	iotd->iotd_Req.io_Error=TDERR_BadUnitNum;
    	return;
    }
    unit=TDBase->units[unitnum];
    ReleaseSemaphore(&TDBase->io_lock);
    iotd->iotd_Req.io_Unit=(struct Unit*)unit;
    iotd->iotd_Req.io_Error=0;
    iotd->iotd_Req.io_Message.mn_Node.ln_Type=NT_REPLYMSG;
    return;

    AROS_LIBFUNC_EXIT
}


AROS_LH1(BPTR, close,
 AROS_LHA(struct IOExtTD *,    iotd,  A1),
	  struct TrackDiskBase *, TDBase, 2, TrackDisk)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    iotd->iotd_Req.io_Device = (struct Device *)-1;

    TDBase->td_device.dd_Library.lib_OpenCnt--;
    if(TDBase->td_device.dd_Library.lib_OpenCnt == 0)
		expunge();

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, expunge, struct TrackDiskBase *, TDBase, 3, TrackDisk)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    TDBase->td_device.dd_Library.lib_Flags |= LIBF_DELEXP;
    return 0;

    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, null, struct TrackDiskBase *, TDBase, 4, TrackDisk)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH1(void, beginio,
 AROS_LHA(struct IOExtTD *, iotd, A1),
	   struct TrackDiskBase *, TDBase, 5, TrackDisk)
{
    AROS_LIBFUNC_INIT
    struct TDU *unit;
    int err;
    int trk;
	
    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;

    switch (iotd->iotd_Req.io_Command)
    {
		/* Do nothing for these two */
      case CMD_UPDATE:
			/* Lock TDBase */
			ObtainSemaphore(&TDBase->io_lock);
			iotd->iotd_Req.io_Error = td_update(iotd, TDBase);
			ReleaseSemaphore(&TDBase->io_lock);
			break;

      case CMD_CLEAR:
			/* Lock TDBase */
			ObtainSemaphore(&TDBase->io_lock);
			unit=(struct TDU*)iotd->iotd_Req.io_Unit;
			unit->lastcyl = -1;
			unit->lasthd = -1;
			unit->flags &= ~TDUF_WRITE;
			iotd->iotd_Req.io_Error=0;
			ReleaseSemaphore(&TDBase->io_lock);
         break;

		case TD_CHANGESTATE:
         ObtainSemaphore(&TDBase->io_lock);
			iotd->iotd_Req.io_Actual=td_getDiskChange(iotd, TDBase);
			iotd->iotd_Req.io_Error=0;
			ReleaseSemaphore(&TDBase->io_lock);
			break;

		/* Turn drive motor on and off */
      case TD_MOTOR:
         ObtainSemaphore(&TDBase->io_lock);
         iotd->iotd_Req.io_Length&=1;
			unit=(struct TDU*)iotd->iotd_Req.io_Unit;
			iotd->iotd_Req.io_Actual=(TDBase->DOR>>(unit->unitnum+4))&1;
			TDBase->DOR&=~(1<<(unit->unitnum+4));
			TDBase->DOR|=iotd->iotd_Req.io_Length<<(unit->unitnum+4);
			TDBase->DOR|=0xc;
			fd_outb(TDBase->DOR,FD_DOR);
			iotd->iotd_Req.io_Error=0;
			ReleaseSemaphore(&TDBase->io_lock);
			break;

		/* Seek the drive */
		case TD_SEEK:
			/* Lock TDBase */
			ObtainSemaphore(&TDBase->io_lock);
	    	unit=(struct TDU*)iotd->iotd_Req.io_Unit;
			trk = iotd->iotd_Req.io_Offset >> 9;
			err = td_recalibrate(unit->unitnum, 0, trk, iotd, TDBase);
			if (!err)
			{
				trk /= 2 * DP_SECTORS;
				unit->pub.tdu_CurrTrk = trk;	/* Got track */
			}
			iotd->iotd_Req.io_Error = err;
			ReleaseSemaphore(&TDBase->io_lock);
			break;
			
		case CMD_READ:
			/* Lock TDBase */
			ObtainSemaphore(&TDBase->io_lock);
			iotd->iotd_Req.io_Error = td_read(iotd, TDBase);
			ReleaseSemaphore(&TDBase->io_lock);
			break;
			
		case CMD_WRITE:
			/* Lock TDBase */
			ObtainSemaphore(&TDBase->io_lock);
			iotd->iotd_Req.io_Error = td_write(iotd, TDBase);
			ReleaseSemaphore(&TDBase->io_lock);
			break;
		
        default:
	    	iotd->iotd_Req.io_Error = IOERR_NOCMD;
	    	break;
    }
    
    /* If the quick bit is not set, send the message to the port */
    if(!(iotd->iotd_Req.io_Flags & IOF_QUICK))
	ReplyMsg(&iotd->iotd_Req.io_Message);
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOExtTD *,    iotd,  A1),
	  struct TrackDiskBase *, TDBase, 6,  TrackDisk)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}



/*-------------------- Hardware section --------------------*/

/* All procedures here are based on Intel documentation */

#define	TOFF	50		/* after TOFF ticks drive motor is turned off */
#define TD_DMA	2		/* Define DMA channel */

/* Wait for interrupt */
int td_waitint(struct IOExtTD *iotd, struct TrackDiskBase *TDBase)
{
struct Task *sigtask;
int err = TDERR_NotSpecified;

	TDBase->io_msg = &iotd->iotd_Req.io_Message;
	sigtask = TDBase->io_msg->mn_ReplyPort->mp_SigTask;
	TDBase->io_msg->mn_ReplyPort->mp_SigTask = FindTask(NULL);
	TDBase->iotime = 150;	// Each IO command has 1s to complete before error occurs
									// atdisk.c says 3 seconds (sheutlin)
	Wait(1 << iotd->iotd_Req.io_Message.mn_ReplyPort->mp_SigBit);
	if (TDBase->iotime)
	{
		TDBase->iotime = 0;
		err = 0;
	}
	TDBase->io_msg->mn_ReplyPort->mp_SigTask = sigtask;
	return err;
}

// Send byte to drive. Returns DriveInUse error if busy, 0 otherwise
int td_sendbyte(unsigned char byte, struct TrackDiskBase *TDBase)
{

	TDBase->iotime=50;	//1s to send a command
	do
	{
		if ((fd_inb(FD_STATUS) & 0xc0)==0x80)
		{
			fd_outb(byte, FD_DATA);
			return 0;
		}
	} while (TDBase->iotime);
	return TDERR_DriveInUse;

}

// Get byte from drive. Returns the same as td_sendbyte
int td_getbyte(unsigned char *byte, struct TrackDiskBase *TDBase)
{

	TDBase->iotime=50;
	do
	{
		if ((fd_inb(FD_STATUS) & 0xc0)==0xc0)
		{
			*byte = fd_inb(FD_DATA);
			return 0;
		}
	} while (TDBase->iotime);
	return TDERR_DriveInUse;
}

// Send full command to drive
int td_sendcommand(struct TrackDiskBase *TDBase)
{
	int err = 0;
	if (TDBase->comsize)
	{
		int i;
		for (i=0; (i < TDBase->comsize) && !err; i++)
		{
			err = td_sendbyte(TDBase->rawcom[i], TDBase);
		}
	}
	// If there was error just reset drive
	if (err)
	{
		int i;
		
		err = 0;
		fd_outb(0, FD_DOR);
		fd_outb(0, FD_DOR);
		fd_outb(4, FD_DOR);
		td_dinit(TDBase);
		// Resend command
		for (i=0; (i < TDBase->comsize) && !err; i++)
			err = td_sendbyte(TDBase->rawcom[i], TDBase);
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
		err = td_getbyte(&TDBase->result[i], TDBase);
	}
	return err;
}

int waitUntilReady(void) {
int i, status;

	for (i=0; i < 10000; i++)
	{
		status = fd_inb(FD_STATUS);
		if (status & STATUS_READY)
			return status;
	}
	return -1;
}

#define MORE_OUTPUT 1

int needMoreOutput(struct TrackDiskBase *TDBase) {
int status;

	if ((status=waitUntilReady())<0)
		return -1;
	if ((status & (STATUS_READY|STATUS_DIR|STATUS_DMA)) == STATUS_READY)
		return MORE_OUTPUT;
	td_readstatus(TDBase, 7);
	return 0;
}

int td_configure(struct TrackDiskBase *TDBase) {

	// Do Configure (enable FIFO and turn polling off)
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

// Initialize drive
int td_dinit(struct TrackDiskBase *TDBase)
{
	int i;
	// deassert RESET signal
	fd_outb(0x0c, FD_DOR);
	// programm data rate
	fd_outb(0,FD_DCR);
	// issue Sense Interrupt Status (loop 4 times)
	for (i=0; i<4; i++)
	{
		TDBase->comsize = 1;
		TDBase->rawcom[0] = FD_SENSEI;
		td_sendcommand(TDBase);
		td_readstatus(TDBase, 2);
	}
	// issue Specify
	TDBase->comsize = 3;
	TDBase->rawcom[0] = FD_SPECIFY;
	TDBase->rawcom[1] = DP_SPEC1;
	TDBase->rawcom[2] = DP_SPEC2;
	td_sendcommand(TDBase);
	td_configure(TDBase);
	return 0;
}

// Recalibrate (type != 0) or seek (type = 0) command.
int td_recalibrate(unsigned char unitn, char type, int sector, struct IOExtTD *iotd,
				struct TrackDiskBase *TDBase)
{
	int err;
	fd_outb(0x0c | unitn | (0x10 << unitn),FD_DOR); /* Select drive and turn motor on */
	/* If freshly turned on wait for spinup */
	if (TDBase->timeout[unitn] == 0)
	{
		TDBase->timeout[unitn] = 250;
		do {} while(TDBase->timeout[unitn] > 240);
	}
	TDBase->timeout[unitn] = 255;	/* Lock drive motor */
	if (type)
	{
		/* Issue specify */
		TDBase->comsize = 3;
		TDBase->rawcom[0] = FD_SPECIFY;
		TDBase->rawcom[1] = DP_SPEC1;
		TDBase->rawcom[2] = DP_SPEC2;
		td_sendcommand(TDBase);
		/* Issue recalibrate */
		TDBase->comsize = 2;
		TDBase->rawcom[0] = FD_RECALIBRATE;
		TDBase->rawcom[1] = unitn;
		err = td_sendcommand(TDBase);
		sector = 0;
	}
	else
	{
		sector /= 2*DP_SECTORS; // cyl contains real cyl number

		/* Issue seek command */
		TDBase->comsize = 3;
		TDBase->rawcom[0] = FD_SEEK;
		TDBase->rawcom[1] = unitn;	/* Unit number */
		TDBase->rawcom[2] = sector;	/* New Track Number */
		err = td_sendcommand(TDBase);
	}
	if (!err)
	{
		/* Wait for interrupt */
		err = td_waitint(iotd, TDBase);
		if (!err)
		{
			/* Issue Sense Interrupt Status command */
			TDBase->comsize = 1;
			TDBase->rawcom[0] = FD_SENSEI;
			td_sendcommand(TDBase);
			/* Get 2 bytes of status */
			td_readstatus(TDBase,2);
			/* Set timeout to TOFF so that interrupt turn motor off later */
			TDBase->timeout[unitn] = TOFF;
			/* if drive doesn't report any error return 0 */
			if (((TDBase->result[0] & 0xf0) == 0x20) && (TDBase->result[1] == sector))
				return 0;
			/* SeekError otherwise */
			return TDERR_SeekError;
		}
	}
	return err;
}

/************************************************
 Name  : td_checkDisk
 Descr.: checks for disk in drive
 Input : unit - unit of drive
 Output: 0 - disk in drive
         1 - no disk in drive
 Note  : motor must be turned on before calling
         this function; motor remains on when a
         disk is in drive - it is turned off
         otherwise
 SideEffect: trackbuffer will be marked as invalid
             if a media change was recognized
*************************************************/
int td_checkDisk(struct TDU *unit, struct IOExtTD *iotd, struct TrackDiskBase *TDBase) {
int actual;
unsigned char DOR;

	actual=fd_inb(FD_DIR) & (1<<7) ? 1 : 0;
	if (actual)
	{
		/* a disk has been removed */
		td_recalibrate(unit->unitnum, 0, 0, iotd, TDBase);
		td_recalibrate(unit->unitnum, 0, DP_SECTORS*2, iotd, TDBase);
		/* is a disk in drive ? */
		fd_outb(0x0c | unit->unitnum | (0x10 << unit->unitnum), FD_DOR);	/* Motor ON, use DMA */
		actual=fd_inb(FD_DIR) & (1<<7) ? 1 : 0;
		if (actual)
		{
			/* no disk, turn motor off */
			DOR = fd_inb(FD_DOR);
			DOR &= ~(0x10 << unit->unitnum);
			fd_outb(DOR, FD_DOR);
		}	
#warning This doesn't look good if the track wasn't written
		unit->lastcyl=-1;
		unit->lasthd=-1;
		unit->flags &= ~TDUF_WRITE;
	}
	return actual;
}

int td_getDiskChange(struct IOExtTD *iotd, struct TrackDiskBase *TDBase) {
struct TDU *unit=(struct TDU *)iotd->iotd_Req.io_Unit;
unsigned char DOR;
int actual;
	
	TDBase->timeout[unit->unitnum] = 255;	/* Lock the motor */
	fd_outb(0x0c | unit->unitnum | (0x10 << unit->unitnum), FD_DOR);	/* Motor ON, use DMA */
	actual=td_checkDisk(unit, iotd, TDBase);
	if (!actual)
	{
		/* turn motor off */
		DOR = fd_inb(FD_DOR);
		DOR &= ~(0x10 << unit->unitnum);
		fd_outb(DOR, FD_DOR);
	}
	TDBase->timeout[unit->unitnum] = TOFF;
	return actual;
}

int td_readwritetrack(struct TDU *unit, char cyl, char hd, char mode,
							struct IOExtTD *iotd, struct TrackDiskBase *TDBase) {
int wait_first;
int rwcnt = 3;	// Read/Write retries
int skcnt = 3;	// Seek retries
char *buf;
int err;		// Error

	/* Check whether we need to wait 500ms after seek */
	wait_first = TDBase->timeout[unit->unitnum];
	
	/* Enable drive & motor */
	TDBase->timeout[unit->unitnum] = 255;	/* Lock the motor */
	fd_outb(0x0c | unit->unitnum | (0x10 << unit->unitnum), FD_DOR);	/* Motor ON, use DMA */

	/* Program data rate */
	fd_outb(0, FD_DCR);	// 500kbit/s only!
	do
	{
		/* Seek drive */
		err = td_recalibrate(unit->unitnum, 0, (cyl*DP_SECTORS) << 1, iotd, TDBase);
		if (!err)
		{
			if (!wait_first)
			{
				/* Set timeout to 250... int handler will decrease it every tick */
				TDBase->timeout[unit->unitnum] = 250;
				do {} while (TDBase->timeout[unit->unitnum] > 225);
				/* Set wait_first to non zero value */
				wait_first = 1;
			}
			TDBase->timeout[unit->unitnum] = 255;
			rwcnt = 3;	// Max 3 retries of read/write

			do
			{
				/* Clear err flag */
				err = 0;
				/* Set DMA up */
				clear_dma_ff(TD_DMA);
				// Should place some cache flush in future (when cache implemented)
				set_dma_addr(TD_DMA, (ULONG)(unit->dma_buffer));
				set_dma_count(TD_DMA, DP_SECTORS*512);
				set_dma_mode(TD_DMA, (mode == FD_READ) ? DMA_MODE_READ : DMA_MODE_WRITE);
				enable_dma(TD_DMA);
				/* Issue read/write command */
				TDBase->comsize = 9;
				buf = TDBase->rawcom;
				*buf++ = mode;							// Command
				*buf++ = unit->unitnum | (hd << 2);	// Drive Select
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
				err = td_waitint(iotd, TDBase);
				if (!err)
				{
					/* Read result bytes */
					td_readstatus(TDBase, 7);
					/* Check if everything went OK */
					if (!(TDBase->result[0] & 0xc0))
					{
						TDBase->timeout[unit->unitnum] = TOFF;
						return 0;
					}
				}
				/* Something went wrong. Let's see what. */
				/* if err != then timeout err. */
				if (!err)
				{	
					err = TDERR_NotSpecified;
					if (TDBase->result[1] & 0x80)
						err = TDERR_TooFewSecs;
					else if (TDBase->result[1] & 0x20)
					{
						err = TDERR_BadHdrSum;
						if (TDBase->result[2] & 0x20)
							err = TDERR_BadSecSum;
					}
					else if (TDBase->result[1] & 0x04)
						err = TDERR_TooFewSecs;
				}
			} while (--rwcnt);
		}
		td_recalibrate(unit->unitnum, 1, 0, iotd, TDBase);
	} while(--skcnt);	
		
	TDBase->timeout[unit->unitnum] = TOFF;
	return err;
}

int td_update(struct IOExtTD *iotd, struct TrackDiskBase *TDBase) {
struct TDU *unit = (struct TDU *)iotd->iotd_Req.io_Unit;
int err;		// Error

	if (unit->flags & TDUF_WRITE)
	{
		if (td_getDiskChange(iotd, TDBase))
			return TDERR_DiskChanged;	/* No disk in drive */
		err=td_readwritetrack(unit, unit->lastcyl, unit->lasthd, FD_WRITE, iotd, TDBase);
		if (err)
			return err;
		unit->flags &= ~TDUF_WRITE;
	}
	return 0;
}

int td_readtrack(struct IOExtTD *iotd, UBYTE cyl, UBYTE hd, struct TrackDiskBase *TDBase) {
struct TDU *unit = (struct TDU *)iotd->iotd_Req.io_Unit;
int err;		// Error

	if (td_getDiskChange(iotd, TDBase))
		return TDERR_DiskChanged;	/* No disk in drive */
	if ((unit->lastcyl!=cyl) || (unit->lasthd!=hd))
	{
		err=td_update(iotd, TDBase);
		if (err)
			return err;
		err=td_readwritetrack(unit, cyl, hd, FD_READ, iotd, TDBase);
		if (err)
			return err;
		unit->lastcyl=cyl;
		unit->lasthd=hd;
	}
	return 0;
}

int td_read(struct IOExtTD *iotd, struct TrackDiskBase *TDBase) {
struct TDU *unit=(struct TDU *)iotd->iotd_Req.io_Unit;
int cyl, hd, sec;	// C/H/S for operation
ULONG length=0;
ULONG size;
int err;		// Error

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
			return err;
		CopyMem(unit->dma_buffer+(sec*512), iotd->iotd_Req.io_Data+length, size);
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

int td_write(struct IOExtTD *iotd, struct TrackDiskBase *TDBase) {
struct TDU *unit=(struct TDU *)iotd->iotd_Req.io_Unit;
int cyl, hd, sec;	// C/H/S for operation
ULONG length=0;
ULONG size;
int err;		// Error

	/* Issue Sense Drive Status command to check whether write is possible */
	TDBase->comsize = 2;
	TDBase->rawcom[0] = FD_GETSTATUS;
	TDBase->rawcom[1] = unit->unitnum;
	td_sendcommand(TDBase);
	td_readstatus(TDBase,1);
	if (TDBase->result[0] & 0x40)
	{
		/* If disk is write protected return error */
		return TDERR_WriteProt;
	}
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
			err=td_update(iotd, TDBase);
			if (err)
				return err;
			unit->lastcyl=(UBYTE)cyl;
			unit->lasthd=(UBYTE)hd;
		}
		CopyMem(iotd->iotd_Req.io_Data+length, unit->dma_buffer+(sec*size), size);
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

#ifdef SysBase
#undef SysBase
#endif
#define SysBase (hw->sysBase)
#define TDBase ((struct TrackDiskBase *)irq->h_Data)

// Timer interrupt (IRQ0). This pice of code decreases timeout register for
// every drive. It it gets 0 it turns floppy motor off. If timeout is set to 0
// (inactive) or 255 (currently used drive) it does nothing.
void td_floppytimer(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
	int i;
	unsigned char DOR;
	for (i=0; i<4; i++)
	{
		// Decrease only if timeout !=0 and !=255
		if ((TDBase->timeout[i] > 0) && (TDBase->timeout[i] < 255))
		{
			TDBase->timeout[i]--;
			// Zero? If yes, turn the motor off
			if (!TDBase->timeout[i])
			{
				DOR = fd_inb(FD_DOR);
				DOR &= ~(0x10 << i);
				fd_outb(DOR, FD_DOR);
			}
		}
	}

	// Does anyone wait for io?
	if (TDBase->iotime)
	{
		// Decrease timeout
		TDBase->iotime--;
		// timeout?
		if (!TDBase->iotime)
		{
			if (TDBase->io_msg)
			{
				struct MsgPort *port;
				port = TDBase->io_msg->mn_ReplyPort;
				// Remove waiting task
				TDBase->io_msg = (struct Message *)NULL;
				// And Signal() it
				Signal(port->mp_SigTask, 1 << port->mp_SigBit);
			}
		}
	}
	
	/* Allow other servers to process this int */
	return;
}


// Interrupt for IRQ6 (floppy int).

void td_floppyint(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
	struct MsgPort *port;
	if (TDBase->io_msg)
	{
		port = TDBase->io_msg->mn_ReplyPort;
		TDBase->io_msg = (struct Message *)NULL;	// Remove message
		/* Signal task waiting for competion (if there is such) */
		Signal(port->mp_SigTask, 1 << port->mp_SigBit);
	}
	return 0; 	/* Allow other ints processing */
}

static const char end = 0;
