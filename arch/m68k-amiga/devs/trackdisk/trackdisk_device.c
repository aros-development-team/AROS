/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Amigastyle device for trackdisk
    Lang: English
*/

#define DEBUG 0
#include <aros/debug.h>

#include <devices/trackdisk.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/interrupts.h>
#include <clib/alib_protos.h>
#include <aros/symbolsets.h>

#include <dos/filehandler.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>

#include <proto/exec.h>
#include <proto/disk.h>
#include <proto/expansion.h>

#include <hardware/custom.h>
#include <hardware/cia.h>

#include <resources/disk.h>

#include "trackdisk_device.h"
#include "trackdisk_hw.h"

#include LC_LIBDEFS_FILE

static BOOL ishd(ULONG id)
{
    return id == DRT_150RPM;
}

static UBYTE getsectors(struct TDU *tdu)
{
    BYTE hdmult = tdu->tdu_hddisk ? 2 : 1;
    if (tdu->tdu_disktype == DT_UNDETECTED)
    	return 11 * hdmult;
    return tdu->tdu_sectors;
}

static void getunit(struct TrackDiskBase *tdb)
{
    struct DiskBase *DiskBase = tdb->td_DiskBase;
    while (GetUnit(&tdb->td_dru) == NULL) {
	WaitPort (&tdb->td_druport);
    }
}
static void giveunit(struct TrackDiskBase *tdb)
{
    struct DiskBase *DiskBase = tdb->td_DiskBase;
    GiveUnit();
}

static AROS_INTH1(disk_block_interrupt, struct TrackDiskBase *, tdb)
{ 
    AROS_INTFUNC_INIT
 
    Signal (tdb->td_task, 1L << tdb->td_IntBit);
    return 0;
	
    AROS_INTFUNC_EXIT
}

static void TestInsert(struct TrackDiskBase *tdb, struct TDU *tdu, BOOL dostep)
{
    struct IOExtTD *iotd;

    if (dostep) {
    	if (tdu->pub.tdu_PubFlags & TDPF_NOCLICK) {
    	    td_seek(tdu, -1, 0, tdb);
    	} else {
    	    // step towards cyl 0 if > 0, if not, step to cyl 1
    	    td_seek(tdu, tdu->pub.tdu_CurrTrk >= 2 ? (tdu->pub.tdu_CurrTrk - 2) / 2 : 1, 0, tdb);
    	}
    }
    if (td_getDiskChange (tdu, tdb)) {
	struct DiskBase *DiskBase = tdb->td_DiskBase;
        D(bug("[Floppy] Insertion detected\n"));
        td_recalibrate(tdu, tdb);
        tdu->tdu_hddisk = ishd(ReadUnitID(tdu->tdu_UnitNum));
        td_detectformat(tdu, tdb);
        tdu->tdu_DiskIn = TDU_DISK;
        tdu->pub.tdu_Counter++;
        tdu->tdu_ProtStatus = td_getprotstatus(tdu,tdb);
        Forbid();
        ForeachNode(&tdu->tdu_Listeners,iotd) {
	    Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
	}
	Permit();
    }
}

static BOOL TD_PerformIO(struct IOExtTD *iotd, struct TrackDiskBase *tdb)
{
    struct TDU *tdu = (struct TDU *)iotd->iotd_Req.io_Unit;
    struct DriveGeometry *geo;
    UBYTE temp;
    BOOL reply;

    D(bug("TD%d PerformIO cmd=%d\n", tdu->tdu_UnitNum, iotd->iotd_Req.io_Command));

    getunit(tdb);
    reply = TRUE;
    switch(iotd->iotd_Req.io_Command)
    {
	case ETD_CLEAR:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_CLEAR:
	    td_flush(tdu, tdb);
	    td_clear(tdb);
	    tdu->tdu_flags = 0;
	    iotd->iotd_Req.io_Error = 0;
	    break;
	case ETD_READ:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_READ:
	    iotd->iotd_Req.io_Error = td_read(iotd, tdu, tdb);
	    break;
	case ETD_UPDATE:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_UPDATE:
	    iotd->iotd_Req.io_Error = td_flush(tdu, tdb);
	    break;
	case ETD_WRITE:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case CMD_WRITE:
	    iotd->iotd_Req.io_Error = td_write(iotd, tdu, tdb);
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
    	    td_select(tdu, tdb);
 	    if (tdu->tdu_DiskIn == TDU_NODISK)
		TestInsert(tdb, tdu, FALSE);
 	    if (tdu->tdu_DiskIn == TDU_DISK) {
		/* test if disk is still in there */
		temp = td_getDiskChange(tdu, tdb);
		iotd->iotd_Req.io_Actual = temp ? 0 : 1;
		tdu->tdu_DiskIn = temp ? TDU_DISK : TDU_NODISK;
	    } else {
		/* No disk in drive */
		iotd->iotd_Req.io_Actual = 1;
	    }
	    D(bug("TD%d_CHANGESTATE=%d\n", tdu->tdu_UnitNum, iotd->iotd_Req.io_Actual));
	    iotd->iotd_Req.io_Error=0;
	    break;
	case ETD_FORMAT:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case TD_FORMAT:
	    iotd->iotd_Req.io_Error = td_format(iotd, tdu, tdb);
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
    	    	    td_select(tdu, tdb);
  	    	    td_flush(tdu, tdb);
		    td_motoroff(tdu, tdb);
		    break;
		case 1:
    	    	    td_select(tdu, tdb);
		    td_motoron(tdu, tdb, TRUE);
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
	    {
		BYTE sectors = getsectors(tdu);
		geo = (struct DriveGeometry *)iotd->iotd_Req.io_Data;
		geo->dg_SectorSize = 512;
		geo->dg_Cylinders = 80;
		geo->dg_CylSectors = sectors * 2;
		geo->dg_Heads = 2;
		geo->dg_TrackSectors = sectors;
		geo->dg_CylSectors = geo->dg_TrackSectors * geo->dg_Heads;
		geo->dg_TotalSectors = geo->dg_CylSectors * geo->dg_Cylinders;
		geo->dg_BufMemType = MEMF_PUBLIC;
		geo->dg_DeviceType = DG_DIRECT_ACCESS;
		geo->dg_Flags = DGF_REMOVABLE;
                iotd->iotd_Req.io_Error=0;
            }
	    break;
	case TD_GETDRIVETYPE:
	    iotd->iotd_Req.io_Actual = tdu->tdu_hddisk ? DRIVE3_5_150RPM : DRIVE3_5;
            iotd->iotd_Req.io_Error = 0;
	    break;
	case TD_GETNUMTRACKS:
	    iotd->iotd_Req.io_Actual = 80 * 2;
            iotd->iotd_Req.io_Error = 0;
	    break;
	case ETD_SEEK:
	    if (iotd->iotd_Count > tdu->pub.tdu_Counter) {
		iotd->iotd_Req.io_Error = TDERR_DiskChanged;
		break;
	    }
	case TD_SEEK:
	    {
		BYTE sectors = getsectors(tdu);
    	        td_select(tdu, tdb);
	        temp = (iotd->iotd_Req.io_Offset >> 10) / sectors;
	        iotd->iotd_Req.io_Error = td_seek(tdu, temp >> 1, temp & 1, tdb);
	    }
	    break;
	default:
	    /* Not supported */
	    D(bug("TD: Unknown command received\n"));
	    iotd->iotd_Req.io_Error = IOERR_NOCMD;
	    break;
    } /* switch(iotd->iotd_Req.io_Command) */
    td_deselect(tdu, tdb);
    giveunit(tdb);
    return reply;
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 struct TrackDiskBase *, TDBase, 5, TrackDisk)
{
    AROS_LIBFUNC_INIT

    iotd->iotd_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    if (iotd->iotd_Req.io_Flags & IOF_QUICK) {
	switch(iotd->iotd_Req.io_Command)
	{
	    case TD_GETNUMTRACKS:
	    case TD_GETDRIVETYPE:
	    case TD_GETGEOMETRY:
	    case TD_REMCHANGEINT:
	    case TD_ADDCHANGEINT:
	    case TD_PROTSTATUS:
	    case TD_CHANGENUM:
	    TD_PerformIO(iotd,TDBase);
	    return;
	}
    }
    /* Not done quick */
    iotd->iotd_Req.io_Flags &= ~IOF_QUICK;
    /* Forward to devicetask */
    PutMsg(&TDBase->td_Port, &iotd->iotd_Req.io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOExtTD *, iotd, A1),
 struct TrackDiskBase *, TDBase, 6, TrackDisk)
{
    AROS_LIBFUNC_INIT

    return IOERR_NOCMD;

    AROS_LIBFUNC_EXIT
}

static void TD_DevTask(struct Task *parent, struct TrackDiskBase *tdb)
{
    struct Task                 *me;
    struct IOExtTD		*iotd;
    struct TDU			*tdu;
    ULONG			tasig,tisig,sigs,i;
    struct Interrupt *inter;
    struct DiskBase *DiskBase = tdb->td_DiskBase;

    D(bug("[TDTask] TD_DevTask(tdb=%p)\n", tdb));

    me = FindTask(0);

    /* Init MsgPort */
    NEWLIST(&tdb->td_Port.mp_MsgList);
    tdb->td_Port.mp_Node.ln_Type  = NT_MSGPORT;
    tdb->td_Port.mp_Flags = PA_SIGNAL;
    tdb->td_Port.mp_SigBit = SIGBREAKB_CTRL_F;
    tdb->td_Port.mp_SigTask = me;
    tdb->td_Port.mp_Node.ln_Name = "trackdisk.device";

    D(bug("[TDTask] TD_DevTask: struct TaskData @ %p\n", td));

    tdb->td_IntBit = AllocSignal(-1);
    tdb->td_TimerMP = CreateMsgPort();
    tdb->td_TimerMP2 = CreateMsgPort();
    tdb->td_TimerIO = (struct timerequest *) CreateIORequest(tdb->td_TimerMP, sizeof(struct timerequest));
    tdb->td_TimerIO2 = (struct timerequest *) CreateIORequest(tdb->td_TimerMP2, sizeof(struct timerequest));
    if (tdb->td_IntBit == -1)
     	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoSignal);
    if (!tdb->td_TimerMP || !tdb->td_TimerMP2 || !tdb->td_TimerIO || !tdb->td_TimerIO2)
    	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);
    if (OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tdb->td_TimerIO, 0))
    	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_OpenDev);
    if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tdb->td_TimerIO2, 0))
    	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_OpenDev);
    	

    NEWLIST(&tdb->td_druport.mp_MsgList);
    tdb->td_dru.dru_Message.mn_ReplyPort = &tdb->td_druport;
    tdb->td_druport.mp_SigBit = AllocSignal(-1);
    tdb->td_task = me;
    tdb->td_druport.mp_SigTask = tdb->td_task;

    inter = &tdb->td_dru.dru_DiscBlock;
    inter->is_Node.ln_Pri = 0;
    inter->is_Node.ln_Type = NT_INTERRUPT;
    inter->is_Node.ln_Name = "trackdisk.device disk dma done";
    inter->is_Code = (APTR)disk_block_interrupt;
    inter->is_Data = tdb;

    tdb->td_buffer_unit = -1;
    tdb->td_buffer_track = -1;

    /* Initial check for floppies */
    for (i = 0; i < TD_NUMUNITS; i++) {
    	struct TDU *tdu = tdb->td_Units[i];
	if (tdu) {
	    getunit(tdb);
	    td_select(tdu, tdb);
	    td_motoroff(tdu, tdb);
	    tdu->tdu_broken = td_recalibrate(tdu, tdb) == 0;
	    if (tdu->tdu_broken)
	 	bug("DF%d failed to recalibrate!?\n", i);
	    else
		D(bug("DF%d initialized\n", i));
	    tdu->tdu_DiskIn = td_getDiskChange(tdu, tdb) ? TDU_DISK : TDU_NODISK;
	    tdu->tdu_ProtStatus = td_getprotstatus(tdu,tdb);
	    tdu->tdu_hddisk = ishd(GetUnitID(i));
	    tdu->tdu_sectors = tdu->tdu_hddisk ? 22 : 11;
	    tdu->tdu_disktype = DT_UNDETECTED;
	    td_deselect(tdu, tdb);
	    giveunit(tdb);
	}
    }

    tasig = 1L << tdb->td_Port.mp_SigBit;
    tisig = 1L << tdb->td_TimerMP->mp_SigBit;

    /* Reply to startup message */
    Signal(parent, SIGBREAKF_CTRL_F);

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
	if (sigs & tasig) {
	    /* We received a message. Deal with it */
	    while((iotd = (struct IOExtTD *)GetMsg(&tdb->td_Port)) != NULL) {
		/* Execute command */
		if (TD_PerformIO(iotd, tdb)) {
		    /* Finish message */
		    ReplyMsg((struct Message *)iotd);
		}
	    }
	}
	if (sigs & tisig)
	{
	    /* We were woken up by the timer. */
	    WaitIO((struct IORequest *)tdb->td_TimerIO);
	    for(i = 0; i < TD_NUMUNITS; i++) {
	        tdu = tdb->td_Units[i];
		/* If there is no floppy in drive, scan for changes */
		if (tdu && !tdu->tdu_broken) {
		    getunit(tdb);
   	    	    td_select(tdu, tdb);
		    switch (tdu->tdu_DiskIn)
		    {
			case TDU_NODISK:
			TestInsert(tdb, tdu, TRUE);
			break;
			case TDU_DISK:
			if (td_getDiskChange(tdu, tdb) == 0) {
			    D(bug("[Floppy] Removal detected\n"));
			    /* Go to cylinder 0 */
			    td_motoroff(tdu, tdb);
			    td_recalibrate(tdu, tdb);
			    tdu->tdu_DiskIn = TDU_NODISK;
			    tdu->tdu_sectors = 11;
			    tdu->tdu_hddisk = 0;
			    tdu->tdu_disktype = DT_UNDETECTED;
			    tdu->pub.tdu_Counter++;
			    if (tdu->tdu_UnitNum == tdb->td_buffer_unit)
			    	td_clear(tdb);
			    Forbid();
			    ForeachNode(&tdu->tdu_Listeners,iotd) {
				Cause((struct Interrupt *)((struct IOExtTD *)iotd->iotd_Req.io_Data));
			    }
			    Permit();
			} else {
			    td_flush(tdu, tdb);
			}
			break;
		    }
		    td_deselect(tdu, tdb);
		    giveunit(tdb);
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

ULONG TD_InitTask(struct TrackDiskBase *tdb)
{
    struct Task *t;

    D(bug("TD: Creating devicetask...\n"));

    t = NewCreateTask(
	TASKTAG_PC, TD_DevTask,
	TASKTAG_NAME, "trackdisk.device",
	TASKTAG_PRI, 5,
	TASKTAG_ARG1, FindTask(0),
	TASKTAG_ARG2, tdb,
	TAG_DONE);
    if (t) {
	/* Wait until started */
	Wait(SIGBREAKF_CTRL_F);

	D(bug("done\n"));

	return 1;
    }

    D(bug("failed\n"));

    return 0;
}


struct TDU *TD_InitUnit(ULONG num, struct TrackDiskBase *tdb)
{
    struct TDU     *unit;
    struct DiskBase *DiskBase = tdb->td_DiskBase;

    if (AllocUnit(num) == 0)
	return NULL;

    /* Try to get memory for structure */
    unit = AllocMem(sizeof(struct TDU), MEMF_PUBLIC | MEMF_CLEAR);
 
    if (unit) {
	unit->tdu_DiskIn = TDU_NODISK;	/* Assume there is no floppy in there */
	unit->pub.tdu_StepDelay = 3;	/* Standard values here */
	unit->pub.tdu_SettleDelay = 15;
	unit->pub.tdu_RetryCnt = 3;
	unit->pub.tdu_CalibrateDelay = 3;
	unit->tdu_UnitNum = num;
	NEWLIST(&unit->tdu_Listeners);

	/* Store the unit in TDBase */
	tdb->td_Units[num] = unit;
    }
    return unit;
}

/*
 * Create BootNodes for all 4 possible devices
 */
static void TD_BootNode(
        struct ExpansionBase *ExpansionBase,
        ULONG unit, ULONG id)
{
    TEXT dosdevname[4] = "DF0";
    IPTR pp[4 + DE_BOOTBLOCKS + 1] = {};
    struct DeviceNode *devnode;
    BOOL hddisk = ishd(id);

    dosdevname[2] += unit;
    D(bug("trackdisk.device: Adding bootnode %s: DDHD=%d\n", dosdevname, hddisk ? 1 : 0));

    pp[0] = (IPTR)dosdevname;
    pp[1] = (IPTR)"trackdisk.device";
    pp[2] = unit;
    pp[DE_TABLESIZE + 4] = DE_BOOTBLOCKS;
    pp[DE_SIZEBLOCK + 4] = 128;
    pp[DE_NUMHEADS + 4] = 2;
    pp[DE_SECSPERBLOCK + 4] = 1;
    pp[DE_BLKSPERTRACK + 4] = hddisk ? 22 : 11;
    pp[DE_RESERVEDBLKS + 4] = 2;
    pp[DE_LOWCYL + 4] = 0;
    pp[DE_HIGHCYL + 4] = 79;
    pp[DE_NUMBUFFERS + 4] = 10;
    pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC;
    pp[DE_MAXTRANSFER + 4] = 0x00200000;
    pp[DE_MASK + 4] = 0x7FFFFFFE;
    pp[DE_BOOTPRI + 4] = 5 - (unit * 10);
    pp[DE_DOSTYPE + 4] = 0;
    pp[DE_BOOTBLOCKS + 4] = 2;
    devnode = MakeDosNode(pp);

    if (devnode)
   	AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, NULL);
}
static int GM_UNIQUENAME(init)(LIBBASETYPEPTR TDBase)
{
    ULONG i;
    UBYTE drives;
    struct DiskBase *DiskBase;
    struct ExpansionBase *ExpansionBase;
    ULONG ids[TD_NUMUNITS];
 
    D(bug("TD: Init\n"));
    
    TDBase->td_supportHD = 0;
    DiskBase = OpenResource("disk.resource");
    if (!DiskBase)
  	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_OpenRes);

    TDBase->td_DiskBase = DiskBase;
    TDBase->ciaa = (struct CIA*)0xbfe001;
    TDBase->ciab = (struct CIA*)0xbfd000;
    TDBase->custom = (struct Custom*)0xdff000;

    drives = 0;
    for (i = 0; i < TD_NUMUNITS; i++) {
  	ids[i] = GetUnitID(i);
  	if (ids[i] != DRT_EMPTY)
  	    drives++;
    }
  	
    if (drives == 0) {
    	/* No drives here. abort */
    	D(bug("TD: No drives\n"));
    	return FALSE;
    }

    ExpansionBase = (struct ExpansionBase *)TaggedOpenLibrary(TAGGEDOPEN_EXPANSION);
    if (!ExpansionBase)
  	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_OpenLib);

    /* Alloc memory for track buffering, DD buffer only, reallocated
     * later if HD disk detected to save RAM on unexpanded machines */
    TDBase->td_DMABuffer = AllocMem(DISK_BUFFERSIZE, MEMF_CHIP);
    TDBase->td_DataBuffer = AllocMem(11 * 512, MEMF_ANY);
    if (!TDBase->td_DMABuffer || !TDBase->td_DataBuffer)
	Alert(AT_DeadEnd | AO_TrackDiskDev | AG_NoMemory);

    for (i = 0; i < TD_NUMUNITS; i++) {
	TDBase->td_Units[i] = NULL;
	if (ids[i] != DRT_EMPTY)
	    TD_InitUnit(i, TDBase);
	D(bug("TD%d id=%08x status=%d\n", i, id, TDBase->td_Units[i] ? 1 : 0));
    }

    /* Create the message processor task */
    TD_InitTask(TDBase);

    /* Only add bootnode if recalibration succeeded */
    for (i = 0; i < TD_NUMUNITS; i++) {
	if (TDBase->td_Units[i] && !TDBase->td_Units[i]->tdu_broken)
	    TD_BootNode(ExpansionBase, i, ids[i]);
    }

    CloseLibrary((struct Library *)ExpansionBase);

    D(bug("TD: done %d\n", drives));

    return TRUE;
}

static int GM_UNIQUENAME(open)
(
    LIBBASETYPEPTR TDBase,
    struct IOExtTD *iotd,
    ULONG unitnum,
    ULONG flags
)
{
    iotd->iotd_Req.io_Error = IOERR_OPENFAIL;

    /* Is the requested unitNumber valid? */
    if (unitnum < TD_NUMUNITS) {
        struct TDU *unit;

        iotd->iotd_Req.io_Device = (struct Device *)TDBase;

        /* Get TDU structure */
        unit = TDBase->td_Units[unitnum];
	if (unit && !unit->tdu_broken) {
    	    iotd->iotd_Req.io_Unit = (struct Unit *)unit;
    	    ((struct Unit *)unit)->unit_OpenCnt++;
    	    iotd->iotd_Req.io_Error = 0;
	}
    }
    D(bug("TD%d: Open=%d\n", unitnum, iotd->iotd_Req.io_Error));
  
    return iotd->iotd_Req.io_Error == 0;

}

static int GM_UNIQUENAME(close)
(
    LIBBASETYPEPTR TDBase,
    struct IOExtTD *iotd
)
{
    iotd->iotd_Req.io_Unit->unit_OpenCnt --;

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(init), 0)
ADD2OPENDEV(GM_UNIQUENAME(open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(close), 0)
