/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IDE device
    Lang: English

    This version handles ATA and ATAPI devices
*/

#include <dos/filehandler.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/initializers.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>
#include <devices/timer.h>
#include <devices/trackdisk.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/timer.h>

#include "include/cd.h"
#include "include/scsicmds.h"
#include "include/scsidisk.h"
//#include "include/hardblocks.h"

#include "ide_intern.h"

#define DEBUG 1
#include <aros/debug.h>

#undef kprintf

#define ioStd(x)  ((struct IOStdReq *)x)

/* IMMEDIATES_CMD = %10000000001111111111000111100011 */
#define IMMEDIATES_CMD      0x803ff1e3

static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable=0;

#define BLOCK_SIZE          512
#define CONST_NUM        0x2222
#define ATA_TimeOut      500000
#define ATAPI_TimeOut   1000000

/* Library prototypes */
struct ideBase  *AROS_SLIB_ENTRY(init, ide)();
void            AROS_SLIB_ENTRY(Open, ide)();
BPTR            AROS_SLIB_ENTRY(Close, ide)();
BPTR            AROS_SLIB_ENTRY(Expunge, ide)();
int             AROS_SLIB_ENTRY(Null, ide)();
void            AROS_SLIB_ENTRY(BeginIO, ide)();
LONG            AROS_SLIB_ENTRY(AbortIO, ide)();
ULONG           AROS_SLIB_ENTRY(GetRdskLba, ide)();
ULONG           AROS_SLIB_ENTRY(GetBlkSize, ide)();

/* Internal function prototypes */
ULONG               InitTask(struct ideBase *);
ULONG               InitDaemon(struct ideBase *);
void                TaskCode(struct ideBase *);
void                DaemonCode(struct ideBase *);

static const char end;

int AROS_SLIB_ENTRY(Entry, ide)(void)
{
    /* If the device was executed by accident return null. */
    return 0;
}

const struct Resident ide_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&ide_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    4,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[] = "ide.device";
static const char version[] = "$VER: ide.device 41.0 (14.6.2000)\r\n";

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct ideBase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init, ide)
};

static void *const functable[] =
{
    &AROS_SLIB_ENTRY(Open, ide),
    &AROS_SLIB_ENTRY(Close, ide),
    &AROS_SLIB_ENTRY(Expunge, ide),
    &AROS_SLIB_ENTRY(Null, ide),
    &AROS_SLIB_ENTRY(BeginIO, ide),
    &AROS_SLIB_ENTRY(AbortIO, ide),
    &AROS_SLIB_ENTRY(GetRdskLba, ide),
    &AROS_SLIB_ENTRY(GetBlkSize, ide),
    (void *)-1
};

/*
** IO ports that are potential IDE ports
*/
#define MAX_PORTS   2
static const ULONG Ports[] = {
        0x01f0,     // First IDE, irq 14
        0x0170,     // Second IDE, irq 15
        0x01e8,     // Third IDE, irq 11
        0x0168,     // Fourth IDE, irq 10
};

AROS_LH2(struct ideBase *,  init,
 AROS_LHA(struct ideBase *, IBase, D0),
 AROS_LHA(BPTR,         segList, A0),
	  struct ExecBase *, sysBase, 0, ide)
{
    AROS_LIBFUNC_INIT
    int i;
    struct ide_Bus bus;
            
    D(bug("ide device: init\n"));
            
    /* Save seglist and execbase */
    IBase->ide_SysLib = sysBase;
    IBase->ide_SegList = segList;

    /* Alloc some memory */
    IBase->ide_BoardAddr = AllocMem(MAX_UNIT * 4, MEMF_PUBLIC|MEMF_CLEAR);
    if (IBase->ide_BoardAddr)
    {
        D(bug("ide_init: Got ide_BoardAddr=%p\n", IBase->ide_BoardAddr));
        IBase->ide_DevMaskArray = AllocMem(MAX_UNIT, MEMF_PUBLIC|MEMF_CLEAR);
        if (IBase->ide_DevMaskArray)
        {
            D(bug("ide_init: Got ide_DevMaskArray=%p\n", IBase->ide_DevMaskArray));
            
            IBase->ide_TimerMP = CreateMsgPort();
            if (IBase->ide_TimerMP)
            {
                /* Open timer.device */
                IBase->ide_TimerIO = (struct timerequest *)CreateIORequest(IBase->ide_TimerMP,sizeof(struct timerequest));
                if (IBase->ide_TimerIO)
                {
        	    D(bug("ide_init: Got ide_TimerIO=%p\n", IBase->ide_TimerIO));

                    if (!OpenDevice("timer.device", UNIT_VBLANK,
                                    (struct IORequest*)IBase->ide_TimerIO, 0))
                    {
                        int	cunit = 0;

                    	D(bug("ide_init: Got timer.device\n"));

                    	/* Find all drives */
                    	for (i=0; i < MAX_PORTS; i++)
                    	{
                            bus.ib_Port = Ports[i];
                            ScanBus(&bus);
                            /* Is there a master connected */
                            if ( bus.ib_Dev0 >= IDE_DEVTYPE_ATA )
                            {
                                IBase->ide_BoardAddr[cunit] = Ports[i];
                                IBase->ide_DevMaskArray[cunit++] = bus.ib_Dev0;
                                D(bug("ide_init: Master found at port %x\n",Ports[i]));
                            }
                            /* Slave, perhaps */
                            if ( bus.ib_Dev1 >= IDE_DEVTYPE_ATA )
                            {
                                IBase->ide_BoardAddr[cunit] = Ports[i];
                                IBase->ide_DevMaskArray[cunit++] = bus.ib_Dev1 | ATAF_SLAVE;
                                D(bug("ide_init:  Slave found at port %x\n",Ports[i]));
                            }                            
                        }
                        /* Store number of available units */
                        IBase->ide_NumUnit = cunit;
	  	
                        /* Close timer.device */
                        CloseDevice((struct IORequest *)IBase->ide_TimerIO);
                        DeleteIORequest((struct IORequest *)IBase->ide_TimerIO);
                        IBase->ide_TimerIO = NULL;
                        DeleteMsgPort(IBase->ide_TimerMP);
                        IBase->ide_TimerMP = NULL;

                        /* If there are valid units */
                        if (cunit)
                        {
                            /* Init device task */
                            if (InitTask(IBase))
                            {
                                /* Init device daemon */
                                if(InitDaemon(IBase))
                                {
                                    ReturnPtr("ide_init", struct ideBase *, IBase);
                                }
                            }
                        }
                    }
                    /* Free timerequest memory */
                    if (IBase->ide_TimerIO)
                        DeleteIORequest((struct IORequest *)IBase->ide_TimerIO);
                }
                if (IBase->ide_TimerMP)
                    DeleteMsgPort(IBase->ide_TimerMP);
            }
            FreeMem(IBase->ide_DevMaskArray, MAX_UNIT);
        }
        FreeMem(IBase->ide_BoardAddr, MAX_UNIT * 4);
    }
            
    ReturnPtr("ide_init", struct ideBase *, NULL);
    AROS_LIBFUNC_EXIT
}

/* Init ide.task - message processor */
ULONG InitTask(struct ideBase *ib)
{
    struct  TaskData *t;
    struct  MemList *ml;
        
    /* Allocate Task Data structure */
    t = AllocMem(sizeof(struct TaskData), MEMF_PUBLIC|MEMF_CLEAR);
    /* Allocate MemEntry for this task */
    ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);
    
    if (t && ml)
    {
        /* prepare stack */
        BYTE	*sp = t->td_Stack;

        D(bug("Creating ide.task..."));
        /* Save stack info into task structure */
        t->td_Task.tc_SPLower = sp;
        t->td_Task.tc_SPUpper = (BYTE *)sp + STACK_SIZE;
        t->td_Task.tc_SPReg = (BYTE *)t->td_Task.tc_SPUpper - SP_OFFSET - sizeof(APTR);
        /* Store ideBase on stack */
        ((APTR *)t->td_Task.tc_SPUpper)[-1] = ib;

        /* Init MsgPort */
        NEWLIST(&t->td_Port.mp_MsgList);
        t->td_Port.mp_Node.ln_Type  = NT_MSGPORT;
        t->td_Port.mp_Flags 	    = PA_SIGNAL;
	t->td_Port.mp_SigBit 	    = SIGBREAKB_CTRL_F;
        t->td_Port.mp_SigTask 	    = &t->td_Task;
	
	    t->td_Port.mp_Node.ln_Name = "ide.device";

        /* Init MemList */
        ml->ml_NumEntries = 1;
        ml->ml_ME[0].me_Addr = t;
        ml->ml_ME[0].me_Length = sizeof(struct TaskData);
        NEWLIST(&t->td_Task.tc_MemEntry);
        AddHead(&t->td_Task.tc_MemEntry, &ml->ml_Node);
        
        /* Init Task structure */
        t->td_Task.tc_Node.ln_Name = "ide.task";
        t->td_Task.tc_Node.ln_Type = NT_TASK;
        t->td_Task.tc_Node.ln_Pri  = 5;

        ib->ide_TaskData = t;

        /* Add task to system task list */
        AddTask(&t->td_Task, &TaskCode, NULL);

        D(bug("OK\n"));
	
        return 1;
    }
    
    return 0;
}

ULONG InitDaemon(struct ideBase *ib)
{
    struct  DaemonData *t;
    struct  MemList *ml;
        
    t = AllocMem(sizeof(struct DaemonData), MEMF_PUBLIC|MEMF_CLEAR);
    /* Allocate MemEntry for this task */
    ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);

    if (t)
    {
        BYTE	*sp = t->dd_Stack;
        
        D(bug("Creating ide.daemon..."));
        /* Save stack info into task structure */
        t->dd_Task.tc_SPLower = sp;
        t->dd_Task.tc_SPUpper = (BYTE *)sp + STACK_SIZE;
        t->dd_Task.tc_SPReg = (BYTE *)t->dd_Task.tc_SPUpper - SP_OFFSET - sizeof(APTR);
        /* Store ideBase on stack */
        ((APTR *)t->dd_Task.tc_SPUpper)[-1] = ib;

        /* Init MemList */
        ml->ml_NumEntries = 1;
        ml->ml_ME[0].me_Addr = t;
        ml->ml_ME[0].me_Length = sizeof(struct DaemonData);
        NEWLIST(&t->dd_Task.tc_MemEntry);
        AddHead(&t->dd_Task.tc_MemEntry, &ml->ml_Node);
        
        /* Init Task structure */
        t->dd_Task.tc_Node.ln_Name = "ide.daemon";
        t->dd_Task.tc_Node.ln_Type = NT_TASK;
        t->dd_Task.tc_Node.ln_Pri  = 5;

        ib->ide_DaemonData = t;
        
        AddTask(&t->dd_Task, &DaemonCode, (APTR)-1);

        D(bug("OK\n"));
        
        return 1;
    }
    
    return 0;
}

AROS_LH3(void, Open,
 AROS_LHA(struct IORequest *, iorq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct ideBase *, IBase, 1, ide)
{
    AROS_LIBFUNC_INIT
     
    iorq->io_Error = IOERR_OPENFAIL;
   
    /* Is the requested unitNumber valid? */
    if (unitnum < IBase->ide_NumUnit)
    {
        struct ide_Unit *unit;
        
        iorq->io_Device = (struct Device *)IBase;

        /* Get ide_Unit structure */
        unit = IBase->ide_Units[unitnum];

        /* No structure? Create it right now! */
        if (!unit)
        {
            unit = InitUnit(unitnum, IBase);

            /* If InitUnit fails return with nothing (io_Error is already set
             * to IOERR_OPENFAIL) */
            if (!unit)
                return;
        }

        iorq->io_Unit = (struct Unit *)unit;

        ((struct Library *) IBase)->lib_OpenCnt++;
        ((struct Unit *)    unit)->unit_OpenCnt++;

        ((struct Library *) IBase)->lib_Flags &= ~LIBF_DELEXP;

        iorq->io_Error = 0;
    }
    AROS_LIBFUNC_EXIT
}


AROS_LH1(BPTR, Close,
 AROS_LHA(struct IORequest *,    iorq,  A1),
	  struct ideBase *, IBase, 2, ide)
{
    AROS_LIBFUNC_INIT

    struct ide_Unit *unit;

    unit = (struct ide_Unit *)iorq->io_Unit;

    /* invalidate IORequest fields */
    iorq->io_Unit = (struct Unit *)~0;
    iorq->io_Device = (struct Device *)~0;
    
    ((struct Unit *)unit)->unit_OpenCnt--;
    ((struct Library *)IBase)->lib_OpenCnt--;
    
    if (!((struct Library *)IBase)->lib_OpenCnt)
        if (((struct Library *)IBase)->lib_Flags & LIBF_DELEXP)
            ide_Expunge(IBase);
                
    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0(BPTR, Expunge, struct ideBase *, IBase, 3, ide)
{
    AROS_LIBFUNC_INIT

    /* No expunge. Set delayed flag only */
    ((struct Library *)IBase)->lib_Flags |= LIBF_DELEXP;

    return 0;
    AROS_LIBFUNC_EXIT
}


AROS_LH0I(int, Null, struct ideBase *, IBase, 4, ide)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetRdskLba,
 AROS_LHA(struct IORequest *,    iorq,  A1),
	  struct ideBase *, IBase, 7, ide)
{
    AROS_LIBFUNC_INIT

    return ((struct ide_Unit *)iorq->io_Unit)->au_RDBSector;
            
    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetBlkSize,
 AROS_LHA(struct IORequest *,    iorq,  A1),
	  struct ideBase *, IBase, 8, ide)
{
    AROS_LIBFUNC_INIT

    return ((struct ide_Unit *)iorq->io_Unit)->au_SecShift;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, BeginIO,
 AROS_LHA(struct IORequest *, iorq, A1),
	   struct ideBase *, IBase, 5, ide)
{
    AROS_LIBFUNC_INIT

    struct ide_Unit *unit;
    ULONG           comm;

    /* Get ide_Unit structure */
    unit = (struct ide_Unit *)iorq->io_Unit;

    ((struct Node *)iorq)->ln_Type = NT_MESSAGE;
    comm = iorq->io_Command;

    /* Is this new command (NSCMD_)? */
    if (comm > APCMD_UNITPARAMS)
    {
        if (comm != NSCMD_DEVICEQUERY)
        {
            /* 64-bit commands? */
            if ((comm >= NSCMD_TD_READ64) && (comm <=NSCMD_TD_FORMAT64))
                comm -= 0x8007;     /* Prepare command number */
            else
            {
                iorq->io_Error = IOERR_NOCMD;
                iorq->io_Flags |= IOF_QUICK;
                return;
            }
        }
        /* Substract command number */
        comm -= 0x3fe1;
    }

    Disable();

    /* Should this command be immediate? */
    if (!((1 << comm) & IMMEDIATES_CMD))
    {
        /* No, if it's ATAPI device or active device send command through ide.task */
        if ((unit->au_Flags & AF_AtapiDev) || (unit->au_Unit.unit_flags & UNITF_ACTIVE))
        {
            /* Make unit active */
            unit->au_Unit.unit_flags |= UNITF_ACTIVE | UNITF_INTASK;
            iorq->io_Flags &= ~IOF_QUICK;   /* Not done quick */

            Enable();

            /* Send message to ide.task */
            PutMsg(&IBase->ide_TaskData->td_Port, (struct Message *)iorq);
            
            return;
        }
    } 

    /* Immediate command */
    
    /* Make unit active */
    unit->au_Unit.unit_flags |= UNITF_ACTIVE;
    
    Enable();

    /* And perform immediate command */
    PerformIO(iorq, unit, IBase->ide_TaskData);

    /* Unit inactive */
    unit->au_Unit.unit_flags &= ~UNITF_ACTIVE;
    /* If IO_QUICK cleared, reply message */
    if (!(iorq->io_Flags & IOF_QUICK))
        ReplyMsg((struct Message *)iorq);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
 AROS_LHA(struct IORequest *, iorq, A1),
	   struct ideBase *, IBase, 6, ide)
{
    AROS_LIBFUNC_INIT

    /* ide devices cannot abort commands */
    return 0;
    
    AROS_LIBFUNC_EXIT
}

/******************************************************************************/
/*    cmd commands                                                            */
/******************************************************************************/

/* Calls atapi_Read if drive is ATAPI, ata_Read otherwise */
inline ULONG ReadBlocks(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act)
{
    if (unit->au_Flags & AF_AtapiDev)
        return atapi_Read(block, count, buffer, unit, act);
    else
        return ata_Read(block, count, buffer, unit, act);
}

/* Calls atapi_Write if drive is ATAPI, ata_Write otherwise */
inline ULONG WriteBlocks(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act)
{
    if (unit->au_Flags & AF_AtapiDev)
        return atapi_Write(block, count, buffer, unit, act);
    else
        return ata_Write(block, count, buffer, unit, act);
}

/* 64 bit commands */

void cmd_Read64(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG   block;
    ULONG   size;
    APTR    buffer;
    ULONG   cnt;

    block  = ioStd(iorq)->io_Offset >> unit->au_SecShift;
    block |= ioStd(iorq)->io_Actual << (32 - unit->au_SecShift);

    size = ioStd(iorq)->io_Length >> unit->au_SecShift;
    buffer = ioStd(iorq)->io_Data;

    if (unit->au_Flags & AF_IntDisable)
        Disable();
    /* Add cache support */

    iorq->io_Error = ReadBlocks(block, size, buffer, unit, &cnt);

    if (unit->au_Flags & AF_IntDisable)
        Enable();

    ioStd(iorq)->io_Actual = cnt;
}

void cmd_Write64(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG block;
    ULONG size;
    APTR  buffer;
    ULONG cnt;

    if (unit->au_DevType)
    {
        iorq->io_Error = TDERR_WriteProt;
        ioStd(iorq)->io_Actual = 0;
        return;
    }

    block  = ioStd(iorq)->io_Offset >> unit->au_SecShift;
    block |= ioStd(iorq)->io_Actual << (32 - unit->au_SecShift);

    size = ioStd(iorq)->io_Length >> unit->au_SecShift;
    buffer = ioStd(iorq)->io_Data;

    if (unit->au_Flags & AF_IntDisable)
        Disable();

    iorq->io_Error = WriteBlocks(block, size, buffer, unit, &cnt);

    if (unit->au_Flags & AF_IntDisable)
        Enable();

    ioStd(iorq)->io_Actual = cnt;
}

void cmd_Seek64(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG ptr;

    ptr  = ioStd(iorq)->io_Offset >> unit->au_SecShift;
    ptr |= ioStd(iorq)->io_Actual << (32 - unit->au_SecShift);

    if (unit->au_Flags & AF_AtapiDev)
        iorq->io_Error = atapi_Seek(ptr, unit);
    else
        iorq->io_Error = ata_Seek(ptr, unit);
}

/* Standard commands */

void cmd_Invalid(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    iorq->io_Error = IOERR_NOCMD;
}

void cmd_Reset(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ioStd(iorq)->io_Actual = 0;
}

void cmd_Read32(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG   block;
    ULONG   size;
    APTR    buffer;
    ULONG   cnt;

    block = ioStd(iorq)->io_Offset >> unit->au_SecShift;
    size = ioStd(iorq)->io_Length >> unit->au_SecShift;
    buffer = ioStd(iorq)->io_Data;

    if (unit->au_Flags & AF_IntDisable)
        Disable();
    /* Add cache support */

    iorq->io_Error = ReadBlocks(block, size, buffer, unit, &cnt);

    if (unit->au_Flags & AF_IntDisable)
        Enable();

    ioStd(iorq)->io_Actual = cnt;
}

void cmd_Write32(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG block;
    ULONG size;
    APTR  buffer;
    ULONG cnt;

    if (unit->au_DevType)
    {
        iorq->io_Error = TDERR_WriteProt;
        ioStd(iorq)->io_Actual = 0;
        return;
    }

    block = ioStd(iorq)->io_Offset >> unit->au_SecShift;
    size = ioStd(iorq)->io_Length >> unit->au_SecShift;
    buffer = ioStd(iorq)->io_Data;

    if (unit->au_Flags & AF_IntDisable)
        Disable();

    iorq->io_Error = WriteBlocks(block, size, buffer, unit, &cnt);
    ioStd(iorq)->io_Actual = cnt;

    if (unit->au_Flags & AF_IntDisable)
        Enable();
}

void cmd_Update(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    /* No update since there is no cache right now */
}

void cmd_Flush(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    struct Message *msg;

    /* Flush waiting messages */

    Forbid();
    
    while ((msg = GetMsg((struct MsgPort *)&td->td_Port)))
    {
        ((struct IORequest *)msg)->io_Error = IOERR_ABORTED;
        ReplyMsg(msg);
    }

    Permit();
}

void cmd_Seek32(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG ptr;

    ptr = ioStd(iorq)->io_Offset >> unit->au_SecShift;

    if (unit->au_Flags & AF_AtapiDev)
        iorq->io_Error = atapi_Seek(ptr, unit);
    else
        iorq->io_Error = ata_Seek(ptr, unit);
}

void cmd_Remove(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    if (unit->au_RemoveInt)
        iorq->io_Error = TDERR_DriveInUse;
    else
        unit->au_RemoveInt = ioStd(iorq)->io_Data;
}

void cmd_ChangeNum(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ioStd(iorq)->io_Actual = unit->au_ChangeNum;
}

void cmd_ChangeState(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    if (unit->au_Flags & AF_DiskPresent)
        ioStd(iorq)->io_Actual = 0;
    else
        ioStd(iorq)->io_Actual = 1;
}

void cmd_ProtStatus(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    if (unit->au_DevType)
        ioStd(iorq)->io_Actual = -1;
    else
        ioStd(iorq)->io_Actual = 0;
}

void cmd_GetDriveType(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ioStd(iorq)->io_Actual = 1;     /* Type: drive 3"5 */
}

void cmd_GetNumTracks(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ioStd(iorq)->io_Actual = unit->au_Cylinders;
}

void cmd_AddChangeInt(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    Forbid();
    ADDHEAD(&unit->au_SoftList, iorq);
    Permit();

    iorq->io_Flags &= ~IOF_QUICK;
    unit->au_Unit.unit_flags &= ~UNITF_ACTIVE;
}

void cmd_RemChangeInt(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    Forbid();
    REMOVE(iorq);
    Permit();
}

struct cmt1_Apollo {
        char    ap_model[32];
        ULONG   size;
        UBYTE   heads;
        UBYTE   sectors;
        UWORD   cylinders;
        UWORD   sectsize;
};

void cmd_GetGeometry(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    if (ioStd(iorq)->io_Length == sizeof(struct DriveGeometry))
    {
        struct DriveGeometry *dg;

        dg = (struct DriveGeometry *)ioStd(iorq)->io_Data;

        dg->dg_SectorSize   = unit->au_SectSize;
        dg->dg_TotalSectors = unit->au_Blocks;
        dg->dg_Cylinders    = unit->au_Cylinders;
        dg->dg_CylSectors   = unit->au_SectorsC;
        dg->dg_Heads        = unit->au_Heads;
        dg->dg_TrackSectors = unit->au_SectorsT;
        dg->dg_BufMemType   = MEMF_PUBLIC;
        dg->dg_DeviceType   = unit->au_DevType;
        dg->dg_Flags        = (unit->au_Flags & AF_Removable) ? DGF_REMOVABLE : 0;
        dg->dg_Reserved     = 0;
        
        ioStd(iorq)->io_Actual = sizeof(struct DriveGeometry);
        return;
    }
    else if (ioStd(iorq)->io_Length == 512)
    {
        /* Apollo-install info */
        struct cmt1_Apollo *buf;
        buf = (struct cmt1_Apollo *)ioStd(iorq)->io_Data;

        strncpy(&buf->ap_model[0], &unit->au_ModelID[0], 32);   /* Copy model name */
        buf->size       = unit->au_Blocks << unit->au_SecShift;
        buf->heads      = unit->au_Heads;
        buf->sectors    = unit->au_SectorsT;
        buf->cylinders  = unit->au_Cylinders;
        buf->sectsize   = unit->au_SectSize;

        ioStd(iorq)->io_Actual = 512;
        return;
    }
    else if (ioStd(iorq)->io_Length == 514)
    {
        /* ATA-3 auto detection */
        if (ata_Identify(ioStd(iorq)->io_Data, unit))
        {
            ioStd(iorq)->io_Actual = 514;
            return;
        }
    }
    
    iorq->io_Error = TDERR_NotSpecified;
    return;
}

void cmd_Eject(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    if (unit->au_Flags & AF_Removable)
    {
        if (unit->au_Flags & AF_AtapiDev)
            iorq->io_Error = atapi_Eject(unit);
        else
            iorq->io_Error = ata_Eject(unit);
    }
}

void cmd_ScsiDirect(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ioStd(iorq)->io_Actual = sizeof (struct SCSICmd);
    if (unit->au_Flags & AF_AtapiDev)
#if 0
        atapi_ScsiCmd((struct SCSICmd *)ioStd(iorq)->io_Data, unit);
#else
        ioStd(iorq)->io_Error = IOERR_BADADDRESS;
#endif
    else
        ioStd(iorq)->io_Error = ata_ScsiCmd((struct SCSICmd *)ioStd(iorq)->io_Data, unit);
}

void cmd_TestChanged(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    if (unit->au_Flags & AF_AtapiDev)
    {
        if (unit->au_Flags & AF_Removable)
        {
            if (!(unit->au_Flags & AF_Used))
            {
                struct IORequest *msg;

                if (atapi_TestUnit(unit))
                {
                    /* Not present */
                    if (!(unit->au_Flags & AF_DiskPresent))
                    {
                        /* Nothing has changed from last check */
                        unit->au_Flags &= ~AF_Used;
                        return;
                    }
                    unit->au_Flags &= ~AF_DiskPresent;
                }
                else
                {
                    /* Present */
                    if (unit->au_Flags & AF_DiskPresent)
                    {
                        unit->au_Flags &= ~AF_Used;
                        return;
                    }
                    unit->au_Flags |= AF_DiskPresent;
                }
                unit->au_ChangeNum++;

                D(bug("(0%x:%x):Media changed\n", unit->au_PortAddr, unit->au_DevMask));

                Forbid();

                if (unit->au_RemoveInt)
                    Cause(unit->au_RemoveInt);

                ForeachNode(&unit->au_SoftList, msg)
                {
                    Cause((struct Interrupt *)ioStd(msg)->io_Data);
                }
                Permit();
            }
        }
    }
    unit->au_Flags &= ~AF_Used;
}

void cmd_UnitParams(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    /* Not implemented since there is no cache */
}

UWORD Support[] = 
{
    CMD_RESET,
    CMD_READ,
    CMD_WRITE,
    CMD_UPDATE,
    CMD_CLEAR,
    CMD_STOP,
    CMD_START,
    CMD_FLUSH,
    TD_MOTOR,
    TD_SEEK,
    TD_FORMAT,
    TD_REMOVE,
    TD_CHANGENUM,
    TD_CHANGESTATE,
    TD_PROTSTATUS,
    TD_GETDRIVETYPE,
    TD_GETNUMTRACKS,
    TD_ADDCHANGEINT,
    TD_REMCHANGEINT,
    TD_GETGEOMETRY,
    TD_EJECT,
    HD_SCSICMD,
    NSCMD_DEVICEQUERY,
    NSCMD_TD_READ64,
    NSCMD_TD_WRITE64,
    NSCMD_TD_SEEK64,
    NSCMD_TD_FORMAT64,
    0
};

void cmd_DevQuery(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    struct NSDeviceQueryResult *d;

    d = (struct NSDeviceQueryResult *)ioStd(iorq)->io_Data;
    
    d->DevQueryFormat 	 = 0;
    d->SizeAvailable 	 = sizeof(struct NSDeviceQueryResult);
    d->DeviceType 	 = NSDEVTYPE_TRACKDISK;
    d->DeviceSubType 	 = 0;
    d->SupportedCommands = Support;

    ioStd(iorq)->io_Actual = sizeof(struct NSDeviceQueryResult);
}

void (*map64[])() =
{
    cmd_Read64,         // NSCMD_TD_READ64
    cmd_Write64,        // NSCMD_TD_WRITE64
    cmd_Seek64,         // NSCMD_TD_SEEK64
    cmd_Write64,        // NSCMD_TD_FORMAT64
};

void (*map32[])() =
{
    cmd_Invalid,        // CMD_INVALID
    cmd_Reset,          // CMD_RESET
    cmd_Read32,         // CMD_READ
    cmd_Write32,        // CMD_WRITE
    cmd_Update,         // CMD_UPDATE
    cmd_Reset,          // CMD_CLEAR
    cmd_Reset,          // CMD_STOP
    cmd_Reset,          // CMD_START
    cmd_Flush,          // CMD_FLUSH
    cmd_Reset,          // TD_MOTOR
    cmd_Seek32,         // TD_SEEK
    cmd_Write32,        // TD_FORMAT
    cmd_Remove,         // TD_REMOVE
    cmd_ChangeNum,      // TD_CHANGENUM
    cmd_ChangeState,    // TD_CHANGESTATE
    cmd_ProtStatus,     // TD_PROTSTATUS
    cmd_Invalid,        // TD_RAWREAD
    cmd_Invalid,        // TD_RAWWRITE
    cmd_GetDriveType,   // TD_GETDRIVETYPE
    cmd_GetNumTracks,   // TD_GETNUMTRACKS
    cmd_AddChangeInt,   // TD_ADDCHANGEINT
    cmd_RemChangeInt,   // TD_REMCHANGEINT
    cmd_GetGeometry,    // TD_GETGEOMETRY
    cmd_Eject,          // TD_EJECT
    cmd_Invalid,        // -
    cmd_Invalid,        // -
    cmd_Invalid,        // -
    cmd_Invalid,        // -
    cmd_ScsiDirect,     // HD_SCSICMD
    cmd_TestChanged,    // APCMD_TESTCHANGED
    cmd_UnitParams      // APCMD_UNITPARAMS
};

void PerformIO(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG   comm = iorq->io_Command;
    iorq->io_Error = 0;

    if (comm == NSCMD_DEVICEQUERY)
        cmd_DevQuery(iorq, unit, td);
    else
    {
        if (comm < NSCMD_TD_READ64)
            (*map32[comm])(iorq, unit, td);
        else
            (*map64[comm - NSCMD_TD_READ64])(iorq, unit, td);
    }
}

/**** ide.task and ide.daemon placed here *************************************/

#ifdef	SysBase
#undef	SysBase
#endif	/* SysBase */
#define	SysBase sysBase

void TaskCode(struct ideBase *ib)
{
    struct TaskData     *td;
    struct ExecBase     *sysBase;
    struct IORequest    *msg;
    ULONG               sig;
    
    td = ib->ide_TaskData;
    sysBase = ib->ide_SysLib;
    
    ib->ide_TimerMP = CreateMsgPort();
    ib->ide_TimerIO = (struct timerequest *)
                CreateIORequest(ib->ide_TimerMP, sizeof(struct timerequest));
    // UNIT_MICROHZ!!!!!!!!!
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)ib->ide_TimerIO, 0);

#if 0
    /* stegerg: commented out, because we now don't allocate a signal, but
     *          use always SIGBREAKB_CTRL_F. 
     */
     
    /*
     * Allocate signal for port. Since this is freshly created task we can
     * do it without any check
     */

    td->td_Port.mp_SigBit = AllocSignal(-1);
    td->td_Port.mp_Flags  = PA_SIGNAL;
      
#endif

   
    sig = 1L << td->td_Port.mp_SigBit;

    /* Endles task loop */
    for(;;)
    {
        Wait(sig);  /* Wait for a message */
        /* If unit was not active process message */
        if (!(td->td_Flags & UNITF_ACTIVE))
        {
            td->td_Flags |= UNITF_ACTIVE;
    
            while((msg = (struct IORequest *)GetMsg(&td->td_Port)))
            {
                PerformIO(msg, (struct ide_Unit *)msg->io_Unit, td);
                ReplyMsg((struct Message *)msg);
            }
            td->td_Flags &= ~(UNITF_INTASK | UNITF_ACTIVE);
        }
    }
}

void DaemonCode(struct ideBase *ib)
{
    struct DaemonData   *dd;
    struct ExecBase     *sysBase;
    int                 i;  // Temp. counter
    
    dd = ib->ide_DaemonData;
    sysBase = ib->ide_SysLib;
    
    dd->dd_TimerMP = CreateMsgPort();
    dd->dd_TimerIO = (struct timerequest *)
            CreateIORequest(dd->dd_TimerMP, sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)dd->dd_TimerIO, 0);

    /* Allocate MsgPort for querying for diskchange */
    dd->dd_DevMP = CreateMsgPort();
    /* Allocate IOStdReq for each unit */
    for(i = 0; i < ib->ide_NumUnit; i++)
    {
        dd->dd_DevIO[i] = (struct IOStdReq *)
                CreateIORequest(dd->dd_DevMP, sizeof(struct IOStdReq));
        dd->dd_DevIO[i]->io_Command = APCMD_TESTCHANGED;
        ide_Open((struct IORequest *)dd->dd_DevIO[i], i, 0, ib);
    }
   
    /* Loop forever... */
    for(;;)
    {
        dd->dd_TimerIO->tr_node.io_Command = TR_ADDREQUEST;
        dd->dd_TimerIO->tr_time.tv_secs = 3;
        dd->dd_TimerIO->tr_time.tv_micro = 0;
        DoIO((struct IORequest *)dd->dd_TimerIO);

        for (i = 0; i < ib->ide_NumUnit; i++)
        {
	    if (dd->dd_DevIO[i]->io_Unit)
        	DoIO((struct IORequest *)dd->dd_DevIO[i]);
        }
    }
}

/**** Do not place any code here unless you redefine SysBase again! ***********/

static const char end = 0;
