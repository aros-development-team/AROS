/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amigastyle device for trackdisk
    Lang: English
*/

#include <devices/trackdisk.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/alib_protos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/bootloader.h>
#include <oop/oop.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <proto/bootloader.h>
#include <proto/dos.h>
#include <hidd/irq.h>
#include <asm/io.h>

#include "trackdisk_device.h"
#include "trackdisk_hw.h"

#define DEBUG 0
#include <aros/debug.h>

#undef kprintf

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct TrackDiskBase *AROS_SLIB_ENTRY(init, TrackDisk)();
void AROS_SLIB_ENTRY(open, TrackDisk)();
BPTR AROS_SLIB_ENTRY(close, TrackDisk)();
BPTR AROS_SLIB_ENTRY(expunge, TrackDisk)();
int  AROS_SLIB_ENTRY(null, TrackDisk)();
void AROS_SLIB_ENTRY(beginio, TrackDisk)();
LONG AROS_SLIB_ENTRY(abortio, TrackDisk)();
void td_floppytimer(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
void td_floppyint(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
int td_getbyte(unsigned char *, struct TrackDiskBase *);
int td_sendbyte(unsigned char, struct TrackDiskBase *);
ULONG TD_InitTask(struct TrackDiskBase *);
void TD_DevTask(struct TrackDiskBase *);
BOOL TD_PerformIO( struct IOExtTD *, struct TrackDiskBase *);

static const char end;

int AROS_SLIB_ENTRY(entry,TrackDisk)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

static const struct Resident TrackDisk_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&TrackDisk_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[] = "trackdisk.device";

static const char version[] = "$VER: trackdisk.device 41.1 (2001-10-08)\r\n";

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

struct TDU *TD_InitUnit(ULONG num, struct TrackDiskBase *tdb)
{
    struct TDU     *unit;
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    IPTR *pp;

    /* Try to get memory for structure */
    unit = AllocMem(sizeof(struct TDU), MEMF_PUBLIC | MEMF_CLEAR);

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",40);
    if (unit)
    {
	unit->tdu_DiskIn = TDU_NODISK;	/* Assume there is no floppy in there */
	unit->pub.tdu_StepDelay=4;	/* Standard values here */
	unit->pub.tdu_SettleDelay=16;
	unit->pub.tdu_RetryCount=3;
	unit->tdu_UnitNum=num;
	unit->tdu_flags = 0;
	unit->tdu_lastcyl = -1;
	unit->tdu_lasthd = -1;
	NEWLIST(&unit->tdu_Listeners);

	/* Alloc memory for track buffering */
	unit->td_DMABuffer=AllocMem(DP_SECTORS*512,MEMF_CLEAR|MEMF_CHIP);

	if (!unit->td_DMABuffer)
	{
	    Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
	}

	/* If buffer doesn't fit into DMA page realloc it */
	if (( (((ULONG)unit->td_DMABuffer + DP_SECTORS*512) & 0xffff0000) -
		    ((ULONG)unit->td_DMABuffer&0xffff0000) ) != 0)
	{
	    APTR buffer;

	    buffer = AllocMem(DP_SECTORS*512, MEMF_CLEAR | MEMF_CHIP);
	    if (!buffer)
	    {
		Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
	    }

	    FreeMem(unit->td_DMABuffer, DP_SECTORS*512);
	    unit->td_DMABuffer = buffer;
	}
	/* Store the unit in TDBase */
	tdb->td_Units[num] = unit;

	D(bug("TD: Adding bootnode\n"));
	if (ExpansionBase)
	{
	    pp = (IPTR *)AllocMem(sizeof(struct DosEnvec)+sizeof(IPTR)*4,MEMF_PUBLIC|MEMF_CLEAR);

	    if (pp)
	    {
		pp[0] = (IPTR)"afs.handler";
		pp[1] = (IPTR)name;
		pp[2] = num;
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
			AROS_BSTR_putchar(devnode->dn_OldName, 2, '0' + num);
			AROS_BSTR_setstrlen(devnode->dn_OldName, 3);
			devnode->dn_NewName = AROS_BSTR_ADDR(devnode->dn_OldName);

			AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
		    }
		}
	    }
	}
    }
    if (ExpansionBase)
    {
	CloseLibrary((struct Library *)ExpansionBase);
    }
    return (unit);
}

AROS_LH2(struct TrackDiskBase *, init,
 AROS_LHA(struct TrackDiskBase *, TDBase, D0),
 AROS_LHA(BPTR, segList, A0),
 struct ExecBase *, sysBase, 0, TrackDisk)
{
    AROS_LIBFUNC_INIT
    struct Library *OOPBase;
    struct BootLoaderBase *BootLoaderBase;
    ULONG i;
    UBYTE drives;

    D(bug("TD: Init\n"));
    
    TDBase->td_click = TRUE;
    
    /* First thing, are we disabled from the bootloader? */
    if ((BootLoaderBase = OpenResource("bootloader.resource")))
    {
	struct List *list;
	struct Node *node;
	
	list = (struct List *)GetBootInfo(BL_Args);
	if (list)
	{
	    ForeachNode(list,node)
	    {
		if (0 == strncmp(node->ln_Name,"nofdc",5))
		{
		    bug("[Floppy] Disabled with bootloader argument\n");
		    ReturnPtr("Trackdisk",struct TrackDiskBase *,NULL);
		}
		if (0 == strncmp(node->ln_Name,"noclick",7))
		{
		    bug("[Floppy] Diskchange detection disabled\n");
		    TDBase->td_click = FALSE;
		}
	    }
	}
    }

    /* First we check if there are any floppy drives configured in BIOS */
    /* We do this by reading CMOS byte 0x10 */
    /* It should really reside in battclock.resource */
    outb(0x10,0x70);
    drives = inb(0x71);

    if (drives == 0)
    {
    	/* No drives here. abort */
    	D(bug("TD: No drives defined in BIOS\n"));
    	ReturnPtr("Trackdisk",struct TrackDiskBase *,NULL);
    }
    
#if 0
    /* Now lets verify that there really is a controller present */
    outb(0,FDC_DOR);
    outb(0,FDC_DOR);
    outb(DORF_RESET,FDC_DOR);

    /* Wait for the controller to report OK */
    for (i=0;i<10000;i++)
    {
	drives = inb(FDC_MSR);
	if (drives & MSRF_RQM)
	    goto foundctrlr;
    }

    bug("TD: No floppy controller present, disabling\n");
    ReturnPtr("Trackdisk",struct TrackDiskBase *,NULL);

foundctrlr:
    D(bug("TD: Floppy controller found\n"));
#endif

    /* Set up the IRQ system */
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

    /* Swap drivebits around */
    drives = ( (drives&0xf0)>>4 | (drives&0x0f)<<4 );

    for (i=0;i<TD_NUMUNITS;i++)
    {
	/* We only want 3.5" 1.44Mb drives */
	if (((drives >> (4*i))&0x0f) == 4)
	{
	    kprintf("[Floppy] Unit %d is a 1.44Mb drive\n",i);
	    TD_InitUnit(i,TDBase);
	}
    }

    /* Create the message processor task */
    TD_InitTask(TDBase);

    ReturnPtr("Trackdisk",struct TrackDiskBase *,TDBase);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
 struct TrackDiskBase *, TDBase, 1, TrackDisk)
{
    AROS_LIBFUNC_INIT

    D(bug("TD: Open\n"));
    iotd->iotd_Req.io_Error = IOERR_OPENFAIL;

    /* Is the requested unitNumber valid? */
    if (unitnum < TD_NUMUNITS)
    {
        struct TDU *unit;

        iotd->iotd_Req.io_Device = (struct Device *)TDBase;

        /* Get TDU structure */
        unit = TDBase->td_Units[unitnum];
        iotd->iotd_Req.io_Unit = (struct Unit *)unit;

        ((struct Library *) TDBase)->lib_OpenCnt++;
        ((struct Unit *)    unit)->unit_OpenCnt++;

        ((struct Library *) TDBase)->lib_Flags &= ~LIBF_DELEXP;

        iotd->iotd_Req.io_Error = 0;
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 struct TrackDiskBase *, TDBase, 2, TrackDisk)
{
    AROS_LIBFUNC_INIT

    D(bug("TD: Close\n"));

    /* Let any following attemps to use the device crash hard. */
    iotd->iotd_Req.io_Device = (struct Device *)-1;

    /* We do not wish to be unloaded */
    return (NULL);

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct TrackDiskBase *, TDBase, 3, TrackDisk)
{
    AROS_LIBFUNC_INIT

    D(bug("TD: Expunge\n"));
    return ((BPTR)IOERR_NOCMD);

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

    if (iotd->iotd_Req.io_Flags & IOF_QUICK)
    {
	switch(iotd->iotd_Req.io_Command)
	{
	    case TD_ADDCHANGEINT:
	    case TD_CHANGENUM:
	    case TD_CHANGESTATE:
	    case TD_PROTSTATUS:
	    case TD_REMCHANGEINT:
	    case TD_GETGEOMETRY:
		TD_PerformIO(iotd,TDBase);
		return;
	    case CMD_CLEAR:
	    case CMD_READ:
	    case CMD_UPDATE:
	    case CMD_WRITE:
	    case TD_FORMAT:
	    case TD_MOTOR:
		PutMsg(&TDBase->td_TaskData->td_Port, &iotd->iotd_Req.io_Message);
		iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
		return;
	    default:
		iotd->iotd_Req.io_Error = IOERR_NOCMD;
		return;
	}
    }
    else
    {
	/* Forward to devicetask */
	PutMsg(&TDBase->td_TaskData->td_Port, &iotd->iotd_Req.io_Message);
	/* Not done quick */
	iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
	return;
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 struct TrackDiskBase *, TDBase, 6, TrackDisk)
{
    AROS_LIBFUNC_INIT
    D(bug("TD: AbortIO\n"));
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

BOOL TD_PerformIO( struct IOExtTD *iotd, struct TrackDiskBase *tdb)
{
    struct TDU *tdu;
    struct DriveGeometry *geo;
    UBYTE temp;
    BOOL reply;

    reply = TRUE;
    tdu = (struct TDU *)iotd->iotd_Req.io_Unit;
    switch(iotd->iotd_Req.io_Command)
    {
	case CMD_CLEAR:
	    D(bug("CMD_CLEAR not done yet!\n"));
	    iotd->iotd_Req.io_Error = IOERR_NOCMD;
	    break;
	case CMD_READ:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_read(iotd, tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case CMD_UPDATE:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_update(tdu, tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case CMD_WRITE:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_write(iotd, tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case TD_ADDCHANGEINT:
	    Forbid();
	    AddTail(&tdu->tdu_Listeners,(struct Node *)iotd);
	    Permit();
	    reply = FALSE;
	    break;
	case TD_CHANGENUM:
	    iotd->iotd_Req.io_Actual = tdu->tdu_ChangeNum;
	    break;
	case TD_CHANGESTATE:
	    if (tdu->tdu_DiskIn == TDU_DISK)
	    {
		/* test if disk is still in there */
		temp = td_getDiskChange();
		iotd->iotd_Req.io_Actual = temp;
		tdu->tdu_DiskIn = temp ^ 1;
	    }
	    else
	    {
		/* No disk in drive */
		iotd->iotd_Req.io_Actual = 1;
	    }
	    iotd->iotd_Req.io_Error=0;
	    break;
	case TD_FORMAT:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_format(iotd,tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case TD_MOTOR:
	    iotd->iotd_Req.io_Error=0;
	    switch (iotd->iotd_Req.io_Length)
	    {
		case 0:
		    td_motoroff(tdu->tdu_UnitNum,tdb);
		    break;
		case 1:
		    td_motoron(tdu->tdu_UnitNum,tdb,TRUE);
		    break;
		default:
		    iotd->iotd_Req.io_Error = TDERR_NotSpecified;
		    break;
	    }
	    break;
	case TD_PROTSTATUS:
	    iotd->iotd_Req.io_Actual = tdu->tdu_ProtStatus;
	    break;
	case TD_REMCHANGEINT:
	    Forbid();
	    Remove((struct Node *)iotd);
	    Permit();
	    break;
	case TD_GETGEOMETRY:
	    geo = (struct DriveGeometry *)iotd->iotd_Req.io_Data;
	    geo->dg_SectorSize = 512;
	    geo->dg_TotalSectors = DP_STOTAL;
	    geo->dg_Cylinders = DP_TRACKS;
	    geo->dg_CylSectors = DP_SECTORS*2;
	    geo->dg_Heads = 2;
	    geo->dg_TrackSectors = DP_SECTORS;
	    geo->dg_BufMemType = MEMF_PUBLIC;
	    geo->dg_DeviceType = DG_DIRECT_ACCESS;
	    geo->dg_Flags = DGF_REMOVABLE;
	    break;
	default:
	    /* Not supported */
	    D(bug("TD: Unknown command received\n"));
	    iotd->iotd_Req.io_Error = IOERR_NOCMD;
	    break;
    } /* switch(iotd->iotd_Req.io_Command) */
    return (reply);
}

ULONG TD_InitTask(struct TrackDiskBase *tdb)
{
    struct  TaskData *t;
    struct  MemList *ml;
    struct  Task *me;

    /* Allocate Task Data structure */
    t = AllocMem(sizeof(struct TaskData), MEMF_PUBLIC|MEMF_CLEAR);
    /* Allocate MemEntry for this task */
    ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);

    /* Find the current task */
    me = FindTask(NULL);
    
    if (t && ml)
    {
	/* prepare stack */
	BYTE	*sp = t->td_Stack;

	D(bug("TD: Creating devicetask..."));
	/* Save stack info into task structure */
	t->td_Task.tc_SPLower = sp;
	t->td_Task.tc_SPUpper = (BYTE *)sp + STACK_SIZE;
	t->td_Task.tc_SPReg = (BYTE *)t->td_Task.tc_SPUpper - SP_OFFSET - sizeof(APTR);
	/* Store TDBase on stack */
	((APTR *)t->td_Task.tc_SPUpper)[-1] = tdb;

	/* Init MsgPort */
	NEWLIST(&t->td_Port.mp_MsgList);
	t->td_Port.mp_Node.ln_Type  = NT_MSGPORT;
	t->td_Port.mp_Flags 	    = PA_SIGNAL;
	t->td_Port.mp_SigBit 	    = SIGBREAKB_CTRL_F;
	t->td_Port.mp_SigTask 	    = &t->td_Task;
	t->td_Port.mp_Node.ln_Name = "trackdisk.device";

	/* Init MemList */
	ml->ml_NumEntries = 1;
	ml->ml_ME[0].me_Addr = t;
	ml->ml_ME[0].me_Length = sizeof(struct TaskData);
	NEWLIST(&t->td_Task.tc_MemEntry);
	AddHead(&t->td_Task.tc_MemEntry, &ml->ml_Node);

	/* Init Task structure */
	t->td_Task.tc_Node.ln_Name = "trackdisk.task";
	t->td_Task.tc_Node.ln_Type = NT_TASK;
	t->td_Task.tc_Node.ln_Pri  = 5;
	t->td_Task.tc_UserData = me;

	tdb->td_TaskData = t;

	/* Add task to system task list */
	AddTask(&t->td_Task, &TD_DevTask, NULL);

	/* Wait until started */
	Wait(SIGBREAKF_CTRL_F);

	D(bug(" OK\n"));

	return 1;
    }

    return 0;
}

/* Device task */

void TD_DevTask(struct TrackDiskBase *tdb)
{
    struct TaskData		*td;
    struct ExecBase		*sysBase;
    struct IOExtTD		*temp,*iotd;
    struct TDU			*tdu;
    ULONG			tasig,tisig,sigs,i;
    UBYTE			dir;

    td = tdb->td_TaskData;
    sysBase = tdb->sysbase;

    tdb->td_IntBit = AllocSignal(-1);
    tdb->td_TmoBit = AllocSignal(-1);
    tdb->td_TimerMP = CreateMsgPort();
    tdb->td_TimerIO = (struct timerequest *) CreateIORequest(tdb->td_TimerMP, sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tdb->td_TimerIO, 0);
    temp = (struct IOExtTD *)CreateExtIO(CreateMsgPort(),sizeof(struct IOExtTD));

    /* Initialize FDC */
    td_dinit(tdb);
    
    /* Initial check for floppies */
    for (i=0;i<TD_NUMUNITS;i++)
    {
	if(tdb->td_Units[i])
	{
	    td_recalibrate(tdb->td_Units[i]->tdu_UnitNum,1,0,tdb);
	    tdb->td_Units[i]->tdu_DiskIn = (td_getDiskChange() ^ 1);
	    tdb->td_Units[i]->tdu_ProtStatus = td_getprotstatus(i,tdb);
	    tdb->td_Units[i]->tdu_Busy = FALSE;
	}
    }

    tasig = 1L << td->td_Port.mp_SigBit;
    tisig = 1L << tdb->td_TimerMP->mp_SigBit;

    /* Reply to startup message */
    Signal(td->td_Task.tc_UserData,SIGBREAKF_CTRL_F);

    tdb->td_TimerIO->tr_node.io_Command = TR_ADDREQUEST;
    tdb->td_TimerIO->tr_time.tv_secs = 2;
    tdb->td_TimerIO->tr_time.tv_micro = 500000;
    SendIO((struct IORequest *)tdb->td_TimerIO);

    /* Endless task loop */
    for(;;)
    {
	sigs = 0L;
	sigs = Wait(tasig | tisig);  /* Wait for a message */
	/* If unit was not active process message */
	if (sigs & tasig)
	{
	    /* We received a message. Deal with it */
	    while((iotd = (struct IOExtTD *)GetMsg(&td->td_Port)) != NULL)
	    {
		/* Execute command */
		if (TD_PerformIO( iotd, tdb))
		{
		    /* Finish message */
		    ReplyMsg((struct Message *)iotd);
		}
	    }
	}
	if (sigs & tisig)
	{
	    /* We were woken up by the timer. */
	    for(i=0;i<TD_NUMUNITS;i++)
	    {
		/* Shall we scan for diskchanges? */
		if (tdb->td_click)
		{
		    /* If there is no floppy in drive, scan for changes */
		    if (tdb->td_Units[i])
		    {
			tdu = tdb->td_Units[i];
			switch (tdu->tdu_DiskIn)
			{
			    case TDU_NODISK:
				/* We need to double step, to avoid seek errors. */
				/* Drives should ignore seeks to track -1, but it */
				/* does not seem to work always */
				td_rseek(tdu->tdu_UnitNum,0,1,tdb);
				td_rseek(tdu->tdu_UnitNum,1,1,tdb);
				dir = (inb(FDC_DIR)>>7);
				if (dir == 0)
				{
				    D(bug("[Floppy] Insertion detected\n"));
				    td_recalibrate(tdu->tdu_UnitNum,1,0,tdb);
				    tdu->tdu_DiskIn = TDU_DISK;
				    tdu->tdu_ChangeNum++;
				    tdu->tdu_ProtStatus = td_getprotstatus(tdu->tdu_UnitNum,tdb);
				    Forbid();
				    ForeachNode(&tdu->tdu_Listeners,iotd)
				    {
					Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
				    }
				    Permit();
				}
				break;
			    case TDU_DISK:
				if (!tdu->tdu_Busy)
				{
				    /* We really should not do this here. */
				    td_motoron(tdu->tdu_UnitNum,tdb,FALSE);
				    dir = (inb(FDC_DIR)>>7);
				    td_motoroff(tdu->tdu_UnitNum,tdb);
				    if (dir == 1)
				    {
					D(bug("[Floppy] Removal detected\n"));
					/* Go to cylinder 0 */
					td_recalibrate(tdu->tdu_UnitNum,1,0,tdb);
					tdu->tdu_DiskIn = TDU_NODISK;
					tdu->tdu_ChangeNum++;
					Forbid();
					ForeachNode(&tdu->tdu_Listeners,iotd)
					{
					    Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
					}
					Permit();
				    }
				}
				break;
			}
		    }
		}
	    }

	    /* Reload the timer again */
	    GetMsg(tdb->td_TimerMP);
	    tdb->td_TimerIO->tr_node.io_Command = TR_ADDREQUEST;
	    tdb->td_TimerIO->tr_time.tv_secs = 2;
	    tdb->td_TimerIO->tr_time.tv_micro = 500000;
	    SendIO((struct IORequest *)tdb->td_TimerIO);
	}
    }
}

#ifdef SysBase
#undef SysBase
#endif
#define SysBase (hw->sysBase)
#define TDBase ((struct TrackDiskBase *)irq->h_Data)
void td_floppytimer(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    // Does anyone wait for io?
    if (TDBase->td_inttmo)
    {
	// Decrease timeout
	TDBase->td_inttmo--;
	// timeout?
	if (!TDBase->td_inttmo)
	{
	    Signal(&TDBase->td_TaskData->td_Task,(1L << TDBase->td_TmoBit));
	}
    }
}

void td_floppyint(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
    Signal(&TDBase->td_TaskData->td_Task,(1L << TDBase->td_IntBit));
}

static const char end = 0;
