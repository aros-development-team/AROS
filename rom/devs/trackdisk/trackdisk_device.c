/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amiga-style device for trackdisk
    Lang: English
*/

#include <devices/trackdisk.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>
#include <dos/filehandler.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/alib_protos.h>
#include <aros/symbolsets.h>
#include <aros/bootloader.h>
#include <oop/oop.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/utility.h>
#include <proto/bootloader.h>
#include <proto/dos.h>
#include <asm/io.h>

#include <SDI/SDI_interrupt.h>

#include "trackdisk_device.h"
#include "trackdisk_hw.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

#undef kprintf

int td_getbyte(unsigned char *, struct TrackDiskBase *);
int td_sendbyte(unsigned char, struct TrackDiskBase *);
ULONG TD_InitTask(struct TrackDiskBase *);
static void TD_DevTask();
BOOL TD_PerformIO( struct IOExtTD *, struct TrackDiskBase *);

struct TDU *TD_InitUnit(ULONG num, struct TrackDiskBase *tdb)
{
    struct TDU     *unit;
    struct ExpansionBase *ExpansionBase = NULL;
    struct DeviceNode *devnode;
    IPTR *pp;
    TEXT dosdevname[4] = "DF0", *handler = "afs-handler";
    UWORD len;

    /* Try to get memory for structure */
    unit = AllocMem(sizeof(struct TDU), MEMF_PUBLIC | MEMF_CLEAR);

    if (!tdb->td_nomount)
	ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",40);
    if (unit)
    {
	unit->tdu_DiskIn = TDU_NODISK;	/* Assume there is no floppy in there */
	unit->pub.tdu_StepDelay = 4;	/* Standard values here */
	unit->pub.tdu_SettleDelay = 16;
	unit->pub.tdu_RetryCnt = 3;
	unit->pub.tdu_CalibrateDelay = 4;
	unit->tdu_UnitNum=num;
	unit->tdu_lastcyl = -1;
	unit->tdu_lasthd = -1;
	NEWLIST(&unit->tdu_Listeners);

	/* Alloc memory for track buffering */
	unit->td_DMABuffer=AllocMem(DP_SECTORS*512,MEMF_CLEAR | MEMF_CHIP | MEMF_24BITDMA);

	if (!unit->td_DMABuffer)
	{
	    Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
	}

	/* If buffer doesn't fit into DMA page realloc it */
	if (( (((ULONG)(IPTR)unit->td_DMABuffer + DP_SECTORS*512) & 0xffff0000) -
		    ((ULONG)(IPTR)unit->td_DMABuffer&0xffff0000) ) != 0)
	{
	    APTR buffer;

	    buffer = AllocMem(DP_SECTORS*512, MEMF_CLEAR | MEMF_CHIP | MEMF_24BITDMA);
	    if (!buffer)
	    {
		Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
	    }

	    FreeMem(unit->td_DMABuffer, DP_SECTORS*512);
	    unit->td_DMABuffer = buffer;
	}
	/* Store the unit in TDBase */
	tdb->td_Units[num] = unit;

	if (ExpansionBase)
	{
	    D(bug("TD: Adding bootnode\n"));
	    pp = (IPTR *)AllocMem(sizeof(struct DosEnvec)+sizeof(IPTR)*4,MEMF_PUBLIC|MEMF_CLEAR);

	    if (pp)
	    {
                dosdevname[2] += num;
		pp[0] = (IPTR)dosdevname;
		pp[1] = (IPTR)MOD_NAME_STRING;
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
		pp[DE_DOSTYPE + 4] = 0;
		pp[DE_BOOTBLOCKS + 4] = 2;
		devnode = MakeDosNode(pp);

		if (devnode)
		{
                    len = strlen(handler);
		    devnode->dn_Handler = MKBADDR(AllocMem(
                        AROS_BSTR_MEMSIZE4LEN(len), MEMF_PUBLIC | MEMF_CLEAR)
                    );

		    if (devnode->dn_Handler != BNULL)
		    {
                        CopyMem(handler, AROS_BSTR_ADDR(devnode->dn_Handler),
                            len);
                        AROS_BSTR_setstrlen(devnode->dn_Handler, len);
			AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, NULL);
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

static AROS_INTH1(td_floppytimer, struct TrackDiskBase *, TDBase)
{
    AROS_INTFUNC_INIT

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

    return FALSE;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(td_floppyint, struct TrackDiskBase *, TDBase)
{
    AROS_INTFUNC_INIT

    Signal(&TDBase->td_TaskData->td_Task,(1L << TDBase->td_IntBit));

    return FALSE;

    AROS_INTFUNC_EXIT
}

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR TDBase)
{
    struct BootLoaderBase *BootLoaderBase;
    ULONG i;
    UBYTE drives;
    struct Interrupt *irq;

    D(bug("TD: Init\n"));
    
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
		if (0 == strncmp(node->ln_Name,"floppy=",5))
		{
		    if (strstr(&node->ln_Name[7], "disabled"))
		    {
		        D(bug("[Floppy] Disabled with bootloader argument\n"));
			return FALSE;
		    }
		    TDBase->td_nomount = (strstr(&node->ln_Name[7], "nomount") != NULL);
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
    	return FALSE;
    }

    for (i=0; i<TD_NUMUNITS; i++)
	TDBase->td_Units[i] = NULL;
    
    irq = &TDBase->td_FloppyInt;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Pri=127;		/* Set the highest pri */
    irq->is_Node.ln_Name = (STRPTR)MOD_NAME_STRING;
    irq->is_Code = (VOID_FUNC)td_floppyint;
    irq->is_Data = (APTR)TDBase;

    AddIntServer(INTB_KERNEL + 6, irq);

    irq = &TDBase->td_TimerInt;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Pri=10;		/* Set the highest pri */
    irq->is_Node.ln_Name = (STRPTR)MOD_NAME_STRING;
    irq->is_Code = (VOID_FUNC)td_floppytimer;
    irq->is_Data = (APTR)TDBase;

    AddIntServer(INTB_KERNEL + 0, irq);

    /* Swap drivebits around */
    drives = ( (drives&0xf0)>>4 | (drives&0x0f)<<4 );

    for (i=0;i<TD_NUMUNITS;i++)
    {
	/* We only want 3.5" 1.44MB drives */
	if (((drives >> (4*i))&0x0f) == 4)
	{
	    kprintf("[Floppy] Unit %d is a 1.44MB drive\n",i);
	    TD_InitUnit(i,TDBase);
	}
    }

    /* Create the message processor task */
    TD_InitTask(TDBase);

    return TRUE;
}

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR TDBase,
    struct IOExtTD *iotd,
    ULONG unitnum,
    ULONG flags
)
{
    D(bug("TD: Open\n"));
    iotd->iotd_Req.io_Error = IOERR_OPENFAIL;

    /* Is the requested unitNumber valid? */
    if (unitnum < TD_NUMUNITS)
    {
        struct TDU *unit;

        iotd->iotd_Req.io_Device = (struct Device *)TDBase;

        /* Get TDU structure */
        unit = TDBase->td_Units[unitnum];
	if (unit && (unit->tdu_Present)) {
    	    iotd->iotd_Req.io_Unit = (struct Unit *)unit;
    	    ((struct Unit *)unit)->unit_OpenCnt++;
    	    iotd->iotd_Req.io_Error = 0;
	}
    }
    
    return iotd->iotd_Req.io_Error == 0;

}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR TDBase,
    struct IOExtTD *iotd
)
{
    iotd->iotd_Req.io_Unit->unit_OpenCnt --;

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, beginio,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 struct TrackDiskBase *, TDBase, 5, TrackDisk)
{
    AROS_LIBFUNC_INIT
    struct TDU *tdu;

    if (iotd->iotd_Req.io_Flags & IOF_QUICK)
    {
	switch(iotd->iotd_Req.io_Command)
	{
	    case TD_CHANGESTATE:
		tdu = (struct TDU *)iotd->iotd_Req.io_Unit;
		if ((!(tdu->pub.tdu_PubFlags & TDPF_NOCLICK)) || (tdu->tdu_DiskIn == TDU_DISK))
		    break;
	    case CMD_READ:
	    case CMD_UPDATE:
	    case CMD_WRITE:
	    case TD_FORMAT:
	    case TD_MOTOR:
	    case TD_SEEK:
	    case ETD_READ:
	    case ETD_UPDATE:
	    case ETD_WRITE:
	    case ETD_FORMAT:
	    case ETD_MOTOR:
	    case ETD_SEEK:
		PutMsg(&TDBase->td_TaskData->td_Port, &iotd->iotd_Req.io_Message);
		iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
		return;
	}
	TD_PerformIO(iotd,TDBase);
	return;
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

void TestInsert(struct TrackDiskBase *tdb, struct TDU *tdu)
{
    UBYTE dir;
    struct IOExtTD *iotd;

    td_rseek(tdu->tdu_UnitNum,tdu->tdu_stepdir,1,tdb);
    tdu->tdu_stepdir = !tdu->tdu_stepdir;
    dir = (inb(FDC_DIR)>>7);
    if (dir == 0)
    {
        D(bug("[Floppy] Insertion detected\n"));
        td_recalibrate(tdu->tdu_UnitNum,1,0,tdb);
        tdu->tdu_DiskIn = TDU_DISK;
        tdu->pub.tdu_Counter++;
        tdu->tdu_ProtStatus = td_getprotstatus(tdu->tdu_UnitNum,tdb);
        Forbid();
        ForeachNode(&tdu->tdu_Listeners,iotd)
        {
	    Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
	}
	Permit();
    }
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
	case ETD_CLEAR:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_CLEAR:
	    tdu->tdu_flags = 0;
	    tdu->tdu_lastcyl = -1;
	    tdu->tdu_lasthd = -1;
	    iotd->iotd_Req.io_Error = 0;
	    break;
	case ETD_READ:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_READ:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_read(iotd, tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case ETD_UPDATE:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_UPDATE:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_update(tdu, tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case ETD_WRITE:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
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
	    iotd->iotd_Req.io_Actual = tdu->pub.tdu_Counter;
            iotd->iotd_Req.io_Error=0;
	    break;
	case TD_CHANGESTATE:
	    if ((tdu->pub.tdu_PubFlags & TDPF_NOCLICK) && (tdu->tdu_DiskIn == TDU_NODISK)) {
		TestInsert(tdb, tdu);
		if (!tdu->tdu_MotorOn)
		    td_motoroff(tdu->tdu_UnitNum, tdb);
	    }
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
	case ETD_FORMAT:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case TD_FORMAT:
	    tdu->tdu_Busy = TRUE;
	    iotd->iotd_Req.io_Error = td_format(iotd,tdb);
	    tdu->tdu_Busy = FALSE;
	    break;
	case ETD_MOTOR:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case TD_MOTOR:
	    iotd->iotd_Req.io_Error=0;
	    switch (iotd->iotd_Req.io_Length)
	    {
		case 0:
		    tdu->tdu_MotorOn = 0;
		    td_motoroff(tdu->tdu_UnitNum,tdb);
		    break;
		case 1:
		    tdu->tdu_MotorOn = 1;
		    td_motoron(tdu->tdu_UnitNum,tdb,TRUE);
		    break;
		default:
		    iotd->iotd_Req.io_Error = TDERR_NotSpecified;
		    break;
	    }
	    break;
	case TD_PROTSTATUS:
	    iotd->iotd_Req.io_Actual = tdu->tdu_ProtStatus;
            iotd->iotd_Req.io_Error=0;
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
            iotd->iotd_Req.io_Error=0;
	    break;
	case TD_GETDRIVETYPE:
	    iotd->iotd_Req.io_Actual = DRIVE3_5;
            iotd->iotd_Req.io_Error=0;
	    break;
	case TD_GETNUMTRACKS:
	    iotd->iotd_Req.io_Actual = DP_TRACKS*2;
            iotd->iotd_Req.io_Error=0;
	    break;
	case ETD_SEEK:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case TD_SEEK:
	    temp = (iotd->iotd_Req.io_Offset >> 10) / DP_SECTORS;
	    tdu->tdu_MotorOn = 1;
	    iotd->iotd_Req.io_Error = td_recalibrate(tdu->tdu_UnitNum, 0, temp, tdb);
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
    /* Allocate Stack space */
    if ((t) && ((t->td_Stack = AllocMem(STACK_SIZE, MEMF_PUBLIC|MEMF_CLEAR)) == NULL))
    {
        FreeMem(t, sizeof(struct TaskData));
        t = NULL;
    }
    /* Allocate MemEntry for this task */
    ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);

    /* Find the current task */
    me = FindTask(NULL);
    
    if (t && ml)
    {
	D(bug("TD: Creating devicetask..."));
	/* Save stack info into task structure */
	t->td_Task.tc_SPLower = t->td_Stack;
	t->td_Task.tc_SPUpper = (BYTE *)t->td_Stack + STACK_SIZE;
	t->td_Task.tc_SPReg = (BYTE *)t->td_Task.tc_SPUpper - SP_OFFSET - sizeof(APTR);

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

    struct TagItem task_Tags[] = {
        { TASKTAG_ARG1,             (IPTR)tdb },
        { TAG_DONE,                 0   },
    };
	/* Add task to system task list */
    NewAddTask(&t->td_Task, &TD_DevTask, NULL, task_Tags );

	/* Wait until started */
	Wait(SIGBREAKF_CTRL_F);

	D(bug(" OK\n"));

	return 1;
    }
    else
    {
        if (t)
        {
            if (t->td_Stack) FreeMem(t->td_Stack, STACK_SIZE);
            FreeMem(t, sizeof(struct TaskData));
        }
        if (ml) FreeMem(ml, sizeof(struct MemList));
    }

    return 0;
}

static void TD_DevTask(struct TrackDiskBase *tdb)
{
    struct TaskData		*td;
    struct IOExtTD		*iotd;
    struct TDU			*tdu;
    ULONG			tasig,tisig,sigs,i;
    UBYTE			dir;

    D(bug("[TDTask] TD_DevTask(tdb=%p)\n", tdb));

    td = tdb->td_TaskData;

    D(bug("[TDTask] TD_DevTask: struct TaskData @ %p\n", td));

    tdb->td_IntBit = AllocSignal(-1);
    tdb->td_TmoBit = AllocSignal(-1);
    tdb->td_TimerMP = CreateMsgPort();
    tdb->td_TimerIO = (struct timerequest *) CreateIORequest(tdb->td_TimerMP, sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tdb->td_TimerIO, 0);

    /* Initialize FDC */
    td_dinit(tdb);
    
    /* Initial check for floppies */
    for (i=0;i<TD_NUMUNITS;i++)
    {
	if(tdb->td_Units[i])
	{
	    td_rseek(i, 0, 1, tdb);
	    tdb->td_Units[i]->tdu_Present = !td_recalibrate(tdb->td_Units[i]->tdu_UnitNum,1,0,tdb);
	    tdb->td_Units[i]->pub.tdu_CurrTrk = 0;
	    tdb->td_Units[i]->tdu_DiskIn = (td_getDiskChange() ^ 1);
	    tdb->td_Units[i]->tdu_ProtStatus = td_getprotstatus(i,tdb);
	    tdb->td_Units[i]->tdu_Busy = FALSE;
	    tdb->td_Units[i]->tdu_stepdir = 0;
	    if (((tdb->td_Units[i]->pub.tdu_PubFlags & TDPF_NOCLICK) && (tdb->td_Units[i]->tdu_DiskIn == TDU_NODISK))
	        || (!tdb->td_Units[i]->tdu_Present))
		td_motoroff(i, tdb);
	    D(bug("Drive %d presence: %ld\n", i, tdb->td_Units[i]->tdu_Present));
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
	    WaitIO((struct IORequest *)tdb->td_TimerIO);
	    for(i=0;i<TD_NUMUNITS;i++)
	    {
		/* If there is no floppy in drive, scan for changes */
		if (tdb->td_Units[i])
		{
		    tdu = tdb->td_Units[i];
		    switch (tdu->tdu_DiskIn)
		    {
			case TDU_NODISK:
			    /*
			       Unfortunately "NoClick" technology which works on Amiga will not
			       work on PC because i82077 does not send step pulse when told to
			       seek to "-1" track and the drive can't recognize disk insertion.
			       Many thanks to Intel! :-(((
			       
			       Here we use another technique: in NoClick mode we just do nothing
			       if the disk is not in drive. We can perform this test only once
			       inside TD_CHANGESTATE command which is invoked by DISKCHANGE
			       CLI command. This means that we'll have to issue DISKCHANGE command
			       manually after wi insert the disk, but this is probably better
			       than those clicks.
			    */
			    if (tdu->pub.tdu_PubFlags & TDPF_NOCLICK) {
				if (!tdu->tdu_MotorOn)
				    td_motoroff(tdu->tdu_UnitNum, tdb);
			    } else
				TestInsert(tdb, tdu);
			    break;
			case TDU_DISK:
			    /*
			       Fortunately this part is completely silent so we don't have to
			       do any extra mess here
			    */
			    if (!tdu->tdu_Busy)
			    {
				if (!tdu->tdu_MotorOn)
				    td_motoron(tdu->tdu_UnitNum,tdb,FALSE);
				dir = (inb(FDC_DIR)>>7);
				if (!tdu->tdu_MotorOn)
				    td_motoroff(tdu->tdu_UnitNum,tdb);
				if (dir == 1)
				{
				    D(bug("[Floppy] Removal detected\n"));
				    /* Go to cylinder 0 */
				    td_recalibrate(tdu->tdu_UnitNum,1,0,tdb);
				    tdu->tdu_DiskIn = TDU_NODISK;
				    tdu->pub.tdu_Counter++;
				    Forbid();
				    ForeachNode(&tdu->tdu_Listeners,iotd)
				    {
					Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
				    }
				    Permit();
				    tdu->tdu_stepdir = 0;
				}
			    }
			    break;
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


