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

#define DEBUG 0 
#include <aros/debug.h>

#define ioStd(x)  ((struct IOStdReq *)x)

AROS_UFP2(int, td_floppyint,
    AROS_UFHA(struct TrackDiskBase *, TDBase, A1),
    AROS_UFHA(struct ExecBase *, SysBase, A6));

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

static const char version[] = "$VER: trackdisk.device 41.0 (30.12.1999)\r\n";

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

#ifdef kprintf
#undef kprintf
#endif

//void kprintf(const char *string,...);

AROS_LH2(struct TrackDiskBase *,  init,
 AROS_LHA(struct TrackDiskBase *, TDBase, D0),
 AROS_LHA(BPTR,         segList, A0),
	  struct ExecBase *, sysBase, 0, TrackDisk)
{
    AROS_LIBFUNC_INIT

    int i;
    ULONG drives;
	
    TDBase->sysbase=sysBase;
    InitSemaphore(&TDBase->io_lock);
    NEWLIST((struct List*)&TDBase->units);
    {
	/* Install floppy controller interrupt */
	struct Interrupt *is;
	is = (struct Interrupt *)AllocMem(sizeof(struct Interrupt), MEMF_CLEAR|MEMF_PUBLIC);
	if(!is)
	{
	    /* PANIC! No memory for trackdisk IntServer ! */
	    Alert(AT_DeadEnd|AO_TrackDiskDev|AN_IntrMem);
	}
	is->is_Node.ln_Pri=127;		/* Set the highest pri */
	is->is_Code = (void (*)())&td_floppyint;
	is->is_Data = (APTR)TDBase;
	AddIntServer(0x80000006,is);	//<-- int_floppy
    }

    /* Get installed drives info */

    asm volatile (
    	"inb $0x70,%%al		\n\t"
    	"andb $0xc0,%%al	\n\t"
    	"orb $0x10,%%al		\n\t"
    	"outb %%al,$0x70	\n\t"
    	"inb $0x71,%%al		\n\t"
    	"rorb $4,%%al		\n\t"
    	"andl $0xff,%%eax"
    	:"=a"(drives)
    	:
    	:"cc");

    /* Build units */
    	
    for (i=0; i<2; i++)
    {
    	struct TDU *unit=NULL;
    	if (((drives>>(4*i))&15)!=0)
    	{
	    unit=(struct TDU*)AllocMem(sizeof(struct TDU), MEMF_CLEAR|MEMF_PUBLIC);
    	    if (!unit)	/* PANIC! No memory for units! */
    	        Alert(AT_DeadEnd|AO_TrackDiskDev|AG_NoMemory);
    	    // Do not initialize MsgPort now
    	    TDU_P(unit)->tdu_StepDelay=4;
    	    TDU_P(unit)->tdu_SettleDelay=16;
    	    TDU_P(unit)->tdu_RetryCount=3;
    	    unit->unitnum=i;
    	    unit->unittype=(drives>>(4*i))&15;
    	
    	    /* Alloc memory for track */
    	    unit->dma_buffer=AllocMem(DP_SECTORS*512,MEMF_CLEAR|MEMF_CHIP);
    	    if (!unit->dma_buffer)
    	    	Alert(AT_DeadEnd|AO_TrackDiskDev|AG_NoMemory);
    	    /* If buffer doesn't fit into DMA page realloc it */
    	    if (( (((ULONG)unit->dma_buffer+DP_SECTORS*512)&0xffff0000) -
    	    	   ((ULONG)unit->dma_buffer&0xffff0000) )!=0)
    	    {
    	    	APTR buffer;
    	    	buffer=AllocMem(DP_SECTORS*512,MEMF_CLEAR|MEMF_CHIP);
    	    	if (!buffer)
    	    	    Alert(AT_DeadEnd|AO_TrackDiskDev|AG_NoMemory);
    	    	FreeMem(unit->dma_buffer,DP_SECTORS*512);
    	    	unit->dma_buffer=buffer;
    	    }
    	}
    	TDBase->units[i]=(struct TDU*)unit;
    	unit=NULL;
    }
    TDBase->units[2]=NULL;
    TDBase->units[3]=NULL;

    fd_outb(0,FD_DOR);	/* Reset controller */
    fd_outb(0,FD_DOR);
    fd_outb(4,FD_DOR);	/* Turn it on again */

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

    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    
    switch (iotd->iotd_Req.io_Command)
    {
        case CMD_UPDATE:
        case CMD_CLEAR:
            iotd->iotd_Req.io_Error=0;
            break;
        case TD_MOTOR:
            ObtainSemaphore(&TDBase->io_lock);
            iotd->iotd_Req.io_Length&=1;
	    unit=(struct TDU*)iotd->iotd_Req.io_Unit;
	    iotd->iotd_Req.io_Actual=(TDBase->DOR>>(unit->unitnum+4))&1;
	    TDBase->DOR&=~(1<<(unit->unitnum+4));
	    TDBase->DOR|=iotd->iotd_Req.io_Length<<(unit->unitnum+4);
	    TDBase->DOR|=0xc;
	    fd_outb(TDBase->DOR,FD_DOR);
	    ReleaseSemaphore(&TDBase->io_lock);
	    iotd->iotd_Req.io_Error=0;
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


#ifdef SysBase
#undef SysBase
#endif

AROS_UFH2(int, td_floppyint,
    AROS_UFHA(struct TrackDiskBase *, TDBase, A1),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    return 0; 	/* Allow other ints processing */
}

static const char end = 0;
























