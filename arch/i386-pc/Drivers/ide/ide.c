/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: IDE device
    Lang: English

    This version handles ATA and ATAPI devices
*/

#define AROS_ALMOST_COMPATIBLE 1
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
#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/timer.h>

#include "include/cd.h"
#include "include/scsicmds.h"
#include "include/scsidisk.h"
//#include "include/hardblocks.h"

#include "ide_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#undef kprintf

#define ioStd(x)  ((struct IOStdReq *)x)

/* IMMEDIATES_CMD = %10000000001111111111000111100011 */
#define IMMEDIATES_CMD      0x803ff1e3

static const char name[];
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
ULONG               TestDevice(struct timerequest *, ULONG, ULONG);
ULONG               TestMirror(ULONG);
struct IORequest    *MyCreateIORequest(struct MsgPort *reply, ULONG length, struct ExecBase *);
struct MsgPort      *MyCreateMsgPort(struct ExecBase *);
ULONG               InitTask(struct ideBase *);
ULONG               InitDaemon(struct ideBase *);
void                TaskCode(struct ideBase *);
void                DaemonCode(struct ideBase *);
struct ide_Unit     *InitUnit(ULONG, struct ideBase *);
void                UnitInfo(struct ide_Unit *);
void                PerformIO(struct IORequest *, struct ide_Unit *, struct TaskData *);

ULONG               SendPacket(struct ide_Unit *, ULONG, APTR);
ULONG               WaitBusySlow(ULONG, struct ide_Unit *);

/* HW function prototypes */
ULONG ata_Read(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act);
ULONG ata_Write(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act);
ULONG ata_Seek(ULONG block, struct ide_Unit *unit);
ULONG ata_Eject(struct ide_Unit *unit);
ULONG ata_Identify(APTR buffer, struct ide_Unit *unit);

ULONG atapi_TestUnit(struct ide_Unit *unit);
ULONG atapi_Read(ULONG, ULONG, APTR, struct ide_Unit *, ULONG *);
ULONG atapi_Write(ULONG, ULONG, APTR, struct ide_Unit *, ULONG *);
ULONG atapi_Seek(ULONG, struct ide_Unit *);
ULONG atapi_Eject(struct ide_Unit *);

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
            
            /* Open timer.device */
            IBase->ide_TimerIO = AllocMem(sizeof(struct timerequest),
                                            MEMF_PUBLIC|MEMF_CLEAR);
            if (IBase->ide_TimerIO)
            {
        	    D(bug("ide_init: Got ide_TimerIO=%p\n", IBase->ide_TimerIO));

                if (!OpenDevice("timer.device", UNIT_VBLANK,
                                (struct IORequest*)IBase->ide_TimerIO, 0))
                {
                    ULONG	devID;
                    int	    cunit = 0;
			
                    D(bug("ide_init: Got timer.device\n"));

                    /* Find all drives */
                    for (i=0; i < MAX_PORTS; i++)
                    {
                        /* Test if MASTER is connected */
                        devID = TestDevice(IBase->ide_TimerIO, 0x00, Ports[i]);
                        if (devID)
                        {
                            IBase->ide_BoardAddr[cunit] = Ports[i];
                            IBase->ide_DevMaskArray[cunit++] = (UBYTE)devID;
                            D(bug("ide_init: device (%x) found at port 0%x\n",
                                                    (UBYTE)devID, Ports[i]));
                        }
                        /* Check for SLAVE drive */
                        if (!devID)
                            devID = TestDevice(IBase->ide_TimerIO, 0x10, Ports[i]);
                        else if(!TestMirror(Ports[i]))
                            devID = 0;
                        else devID = TestDevice(IBase->ide_TimerIO, 0x10, Ports[i]);
                        /* Something found? */
                        if (devID)
                        {
                            /* Great! We have another ide device. Add it to
                             * list of available devices */
                            IBase->ide_BoardAddr[cunit] = Ports[i];
                            IBase->ide_DevMaskArray[cunit++] = (UBYTE)devID;
                            D(bug("ide_init: device (%x) found at port 0%x\n",
                                                    (UBYTE)devID, Ports[i]));
                        }
                    }
                    /* Store number of available units */
                    IBase->ide_NumUnit = cunit;
		
                    /* Close timer.device */
                    CloseDevice((struct IORequest*)IBase->ide_TimerIO);
                    FreeMem(IBase->ide_TimerIO, sizeof(struct timerequest));
                    IBase->ide_TimerIO = NULL;

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
                    FreeMem(IBase->ide_TimerIO, sizeof(struct timerequest));
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
        t->td_Port.mp_Node.ln_Type = NT_MSGPORT;
        t->td_Port.mp_Flags = PA_IGNORE;
        t->td_Port.mp_SigTask = &t->td_Task;
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
    /* Not implemented yet */
}

void cmd_Write64(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    /* Not implemented yet */
}

void cmd_Seek64(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    ULONG ptr;

    ptr = ioStd(iorq)->io_Offset >> unit->au_SecShift;
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
#if 0
    if (unit->au_Flags & AF_AtapiDev)
        atapi_ScsiCmd((struct SCSICmd *)ioStd(iorq)->io_Data, unit);
    else
        ata_ScsiCmd((struct SCSICmd *)ioStd(iorq)->io_Data, unit);
#endif
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

struct nsdqr
{
    ULONG   DevQueryFormat;
    ULONG   SizeAvailable;
    UWORD   DeviceType;
    UWORD   DeviceSubType;
    APTR    Data;
};

void cmd_DevQuery(struct IORequest *iorq, struct ide_Unit *unit, struct TaskData *td)
{
    struct nsdqr    *d;

    d = (struct nsdqr *)ioStd(iorq)->io_Data;
    d->DevQueryFormat = 0;
    d->SizeAvailable = sizeof(struct nsdqr);
    d->DeviceType = 1;  // NSDEVTYPE_TRACKDISK
    d->DeviceSubType = 0;
    d->Data = &Support;

    ioStd(iorq)->io_Actual = sizeof(struct nsdqr);
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

    if (comm < NSCMD_TD_READ64)
        (*map32[comm])(iorq, unit, td);
    else
        (*map64[comm - NSCMD_TD_READ64])(iorq, unit, td);
}

struct ide_Unit *InitUnit(ULONG num, struct ideBase *ib)
{
    struct ide_Unit     *unit = NULL;

    /* Try to get memory for structure */
    unit = AllocMem(sizeof(struct ide_Unit), MEMF_PUBLIC | MEMF_CLEAR);

    if (unit)
    {
        UBYTE   dev;
        
        /* Init structure */
        unit->au_UnitNumber = num;
        unit->au_Device     = ib;
        unit->au_ReadSub    = &ata_Read;
        unit->au_WriteSub   = &ata_Write;

        NEWLIST(&unit->au_SoftList);
        
        dev = ib->ide_DevMaskArray[num];

        if (dev & ATAF_LBA)
            unit->au_Flags |= AF_LBAMode;

        unit->au_NumLoop = ATA_TimeOut * 8;

        if (dev & ATAF_ATAPI)
        {
            unit->au_Flags |= AF_LBAMode | AF_AtapiDev;
            unit->au_NumLoop <<= 1;
        }

        unit->au_DevMask    = dev & 0xf0;
        unit->au_OldDevMask = dev & 0x70;
        unit->au_CtrlNumber = dev & 0x0f;
           
        unit->au_PortAddr   = ib->ide_BoardAddr[num];

        /* Fill structure with drive parameters */
        UnitInfo(unit);
        
        ib->ide_Units[num] = unit;
    }

    return unit;
}


void RDBInfo(struct ide_Unit *unit)
{
    /* Not impemented! */
    return;
}

ULONG ata_ReadSector(ULONG hd, ULONG sec, ULONG cyl, APTR buffer, struct ide_Unit *unit);

/* Try to get CHS info from this drive */
void SearchCHS(struct ide_Unit *unit)
{
    /* Buffer */
    APTR    empty;
    ULONG   hd, sec, cyl, incr;
    
    /* Belive, we can AllocSome memory */
    empty = AllocMem(512, MEMF_PUBLIC);
    
    /* Search for first invalid sector number */
    for(hd = 0, sec = 1, cyl = 0;
        (sec < 65) & ata_ReadSector(hd, sec, cyl, empty, unit); sec++);
    
    if (sec == 65) sec = 36;

    sec--;

    /* store it! */
    unit->au_SectorsT = sec;
    
    /* Do the same for number of heads */
    for(hd = 0, sec = 1, cyl = 0;
        (hd < 16) & ata_ReadSector(hd, sec, cyl, empty, unit); hd++)

    unit->au_Heads = hd;

    /* Do binary search - guess total number of cylinders */
    hd = 0;
    sec = 1;
    cyl = 32768;
    incr = 32768;

    while (incr > 1)
    {
        incr >>= 1;     /* Make add/sub step smaller */
        /* Can you read such sector? */
        if (ata_ReadSector(hd, sec, cyl, empty, unit))
        {
            /* Yes, increment cyl number */
            cyl += incr;

        }
        else
        {
            /* No, decrement (go back) */
            cyl -= incr;
        }
    }

    if (ata_ReadSector(hd, sec, cyl, empty, unit))
        cyl++;

    /* Calculate drive geometry */
    unit->au_Cylinders = cyl;

    unit->au_SectorsC = unit->au_SectorsT * unit->au_Heads;
    unit->au_Blocks = unit->au_SectorsC * cyl;

    unit->au_DevType = DG_DIRECT_ACCESS;
    unit->au_SecShift = 9;
    unit->au_SectSize = 512;
    
    FreeMem(empty, 512);
}

/* Copy string and fix it */
void strcp(char *dest, char *src, int num)
{
    while (num)
    {
        *(char*)dest++ = *(char*)(src+1);
	    *(char*)dest++ = *(char*)src++;
	    src++;
	    num -= 2;
    }
    dest--;
    /* Now, go back till got something other than \0 or space */
    while ((*dest == 0) || (*dest == ' '))
    {
        /* Replace it with '\0' */
        *(char*)dest-- = 0;
    }
}

void UnitInfo(struct ide_Unit *unit)
{
    struct iDev     id;
    UBYTE           tmp;
    ULONG           cnt;

    unit->au_RDBSector = 0xff;  /* Invalidate RDB sector */

    /* Try to identify drive */
    if (ata_Identify(&id, unit))
    {
        /* Copy name/version/serial */
        strcp(&unit->au_ModelID[0], &id.idev_ModelNumber[0], 32);
        strcp(&unit->au_RevNumber[0], &id.idev_RevisionNumber[0], 4);
        strcp(&unit->au_SerNumber[0], &id.idev_SerialNumber[0], 8);

        D(bug("0x0%x: %s,%s,%s\n", unit->au_PortAddr, &unit->au_ModelID[0],
                                &unit->au_RevNumber[0], &unit->au_SerNumber[0]));
        
        /* Get Disk geometry */
        unit->au_Heads      = id.idev_Heads;
        unit->au_SectorsT   = id.idev_Sectors;
        unit->au_Cylinders  = id.idev_Cylinders;
        unit->au_SectorsC   = id.idev_Sectors * id.idev_Heads;
        unit->au_Blocks     = unit->au_SectorsC * id.idev_Cylinders;

        /* is drive removable? */
        if (id.idev_DInfo & 0x80)
        {
            unit->au_Flags |= AF_Removable;
            unit->au_SenseKey = 6;          /* Unit attention */
        }

    	unit->au_Flags |= AF_DiskPresent;

        /* if Atapi then get drive type */
        tmp = (id.idev_DInfo >> 8) & 0x0f;
        if (!(unit->au_Flags & AF_AtapiDev))
            tmp = DG_DIRECT_ACCESS;     /* Set type HDD otherwise */

        unit->au_DevType = tmp;

        /* If DG_DIRECT_ACCESS then sector is 512 bytes long */
        if(!tmp)
        {
            unit->au_SecShift = 9;
        }
        else    /* Non DG_DIRECT_ACCESS drive - sector is 2048 bytes long */
        {
            unit->au_SecShift = 11;
        }

        unit->au_SectSize = 1 << unit->au_SecShift;

        /* If drive is not removable */
        if (!(unit->au_Flags & AF_Removable))
        {
            /* And you can hopefully read last sector */
            if (ReadBlocks(unit->au_Blocks - 1, 1, &id, unit, &cnt))
            {
		goto id_again;
            }
        }
        /* And the drive is HDD... */
        if (!unit->au_DevType)
    	    /* Then get RigidDiskBlock info */
            RDBInfo(unit);

	return;
    }

id_again:
    /* Well, ata_Identify failed. Try to get somehow needed information */
    if (!(unit->au_Flags & AF_AtapiDev))
        SearchCHS(unit);

    /* If it is a hdd then get RDB info */
    if(!unit->au_DevType)
        RDBInfo(unit);

    return;
}

ULONG ActualAddr(ULONG port, struct ide_Unit *unit);
void IncBlockAddr(ULONG port, struct ide_Unit *unit);
int BlockAddr(ULONG block, struct ide_Unit *unit);
int WaitBusy(ULONG port, struct ide_Unit *unit);
int WaitBusyLong(ULONG port, struct ide_Unit *unit);
void ResumeError(ULONG port, struct ide_Unit *unit);

/**** ATAPI commands section *************************************************/

ULONG atapi_ErrCmd()
{
    return CDERR_ABORTED;
}

ULONG ErrorMap[] = {
        CDERR_NotSpecified,         // NO SENSE
        CDERR_NoSecHdr,             // RECOVERED ERROR
        CDERR_NoDisk,               // NOT READY
        CDERR_NoSecHdr,             // MEDIUM ERROR
        CDERR_NoSecHdr,             // HARDWARE ERROR
        CDERR_NOCMD,                // ILLEGAL REQUEST
        CDERR_NoDisk,               // UNIT ATTENTION
        CDERR_WriteProt,            // DATA PROTECT
        CDERR_NotSpecified,         // Reserved
        CDERR_NotSpecified,         // Reserved
        CDERR_NotSpecified,         // Reserved
        CDERR_ABORTED,              // ABORTED COMMAND
        CDERR_NotSpecified,         // Reserved
        CDERR_NotSpecified,         // Reserved
        CDERR_NoSecHdr,             // MISCOMPARE
        CDERR_NotSpecified          // Reserved
};

ULONG atapi_EndCmd(struct ide_Unit *unit, ULONG port)
{
    unit->au_Flags |= AF_Used;
    if (!(ide_in(atapi_Status, port) & ATAPIF_CHECK))
        return 0;

    return ErrorMap[ide_in(atapi_Error, port) >> 4];
}

ULONG atapi_TestUnit(struct ide_Unit *unit)
{
    ULONG port;
    ULONG cmd[3] = {0,0,0};
    UBYTE sense;
    
    port = unit->au_PortAddr;

    if (SendPacket(unit, port, &cmd))
    {
        if (WaitBusySlow(port, unit))
        {
            sense = ide_in(ata_Error, port) >> 4;
            if (sense)
                unit->au_SenseKey = sense;
            return sense;
        }
    }
    return atapi_ErrCmd();
}

ULONG atapi_Read(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *cnt)
{
    ULONG                   port;
    struct atapi_Read10     cmd = {};
    
    *cnt = 0;
    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_READ10;
    cmd.LBA[0]  = block >> 24;
    cmd.LBA[1]  = block >> 16;
    cmd.LBA[2]  = block >> 8;
    cmd.LBA[3]  = block;
    cmd.Len[0]  = count >> 8;
    cmd.Len[1]  = count;

    if (SendPacket(unit, port, &cmd))
    {
        while (1)
        {
            if (WaitBusySlow(port, unit))
            {
                if (ide_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    ULONG size;
                
                    if ((ide_in(atapi_Reason, port) & ATAPIF_MASK) != ATAPIF_READ)
                        return atapi_ErrCmd();

                    size = ide_in(atapi_ByteCntH, port) << 8 |
                           ide_in(atapi_ByteCntL, port);

                    insl(port, buffer, size / 4);
                    
                    buffer += size;
                    *cnt += size;
                } else return atapi_EndCmd(unit, port);
            } else return atapi_ErrCmd();
        }
    }
    return atapi_ErrCmd();
}

ULONG atapi_Write(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *cnt)
{
    ULONG                   port;
    struct atapi_Write10    cmd = {};
    
    *cnt = 0;
    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_WRITE10;
    cmd.LBA[0]  = block >> 24;
    cmd.LBA[1]  = block >> 16;
    cmd.LBA[2]  = block >> 8;
    cmd.LBA[3]  = block;
    cmd.Len[0]  = count >> 8;
    cmd.Len[1]  = count;
    
    if (SendPacket(unit, port, &cmd))
    {
        while (1)
        {
            if (WaitBusySlow(port, unit))
            {
                if (ide_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    ULONG size;

                    if ((ide_in(atapi_Reason, port) & ATAPIF_MASK) != ATAPIF_WRITE)
                        return atapi_ErrCmd();

                    size = ide_in(atapi_ByteCntH, port) << 8 |
                           ide_in(atapi_ByteCntL, port);

                    outsl(port, buffer, size / 4);

                    buffer += size;
                    *cnt += size;
                } else return atapi_EndCmd(unit, port);
            } else return atapi_ErrCmd();
        }
    }
    
    return atapi_ErrCmd();
}

ULONG atapi_Seek(ULONG block, struct ide_Unit *unit)
{
    ULONG                   port;
    struct atapi_Seek10     cmd = {};
    
    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_SEEK10;
    cmd.LBA[0]  = block >> 24;
    cmd.LBA[1]  = block >> 16;
    cmd.LBA[2]  = block >> 8;
    cmd.LBA[3]  = block;

    if (SendPacket(unit, port, &cmd))
    {
        if (WaitBusySlow(port, unit))
        {
            return atapi_EndCmd(unit, port);
        }
    }
    
    return atapi_ErrCmd();
}

ULONG atapi_Eject(struct ide_Unit *unit)
{
    ULONG                   port;
    struct atapi_StartStop  cmd = {};

    port = unit->au_PortAddr;
    cmd.opcode  = SCSI_STARTSTOP;
    cmd.immed   = 1;    /* Immediate command */
    cmd.flgs    = ATAPI_SS_EJECT;
    
    if (SendPacket(unit, port, &cmd))
    {
        if (WaitBusySlow(port, unit))
        {
            return atapi_EndCmd(unit, port);
        }
    }
    
    return atapi_ErrCmd();
}



ULONG SendPacket(struct ide_Unit *unit, ULONG port, APTR cmd)
{
    ide_out(unit->au_DevMask, atapi_DriveSel, port);
    if (WaitBusy(port, unit))
    {
        ide_out(0, atapi_Features, port);
        ide_out(0, atapi_ByteCntL, port);
        ide_out(0, atapi_ByteCntH, port);
        ide_out(ATAPI_PACKET, atapi_Command, port);
        if (WaitBusySlow(port, unit))
        {
            if (ide_in(atapi_Status, port) & ATAF_DATAREQ)
            {
                UBYTE stat;
                stat = ide_in(atapi_Reason, port);
                stat &= ATAPIF_MASK;
                if (stat == ATAPIF_COMMAND)
                {
                    outsw(port, cmd, 6);
                    return 1;
                }
            }
        }
    }
    return 0;       
}

struct wait { ULONG time; ULONG cnt; };

struct wait WaitTable[] = {
        {   1000, 20 },     //   1 ms x 20
        {   5000, 16 },     //   5 ms x 16
        {  10000, 20 },     //  10 ms x 20
        {  20000, 10 },     //  20 ms x 10
        {  50000, 10 },     //  50 ms x 10
        { 100000, 90 },     // 100 ms x 90
        {      0,  0 }      //------------ = 10000 ms
};

ULONG WaitBusySlow(ULONG port, struct ide_Unit *unit)
{
    int i=1000;
    
    if (unit->au_Flags & AF_SlowDevice)
    {
        int t=0;
        
        do
        {
            if (!(ide_in(ata_Status, port) & ATAF_BUSY))
                return i;
        } while (--i);
        
        while (WaitTable[t].time)
        {
            int                 loop;
            struct timerequest  *tr;

            loop = WaitTable[t].cnt;
            
            tr = unit->au_Device->ide_TimerIO;
            
            while(loop--)
            {
                tr->tr_node.io_Command = TR_ADDREQUEST;
                tr->tr_time.tv_secs = 0;
                tr->tr_time.tv_micro = WaitTable[t].time;
                DoIO((struct IORequest *)tr);
                if (!(ide_in(ata_Status, port) & ATAF_BUSY))
                    return 1;
            }
            t++;
        }
        return 0;
    }
    return WaitBusy(port, unit);
}

/**** ATA commands section ***************************************************/

ULONG ata_ReadSector(ULONG hd, ULONG sec, ULONG cyl, APTR buffer, struct ide_Unit *unit)
{
    ULONG port;
    UBYTE err;

    port = unit->au_PortAddr;

    ide_out(hd | unit->au_OldDevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(sec, ata_SectorNum, port);
        ide_out(cyl & 0xff, ata_CylinderL, port);
        ide_out(cyl >> 8, ata_CylinderH, port);
        ide_out(1, ata_SectorCnt, port);
        ide_out(ATA_READ, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            err = ide_in(ata_Status, port);
            
            insl(port, buffer, 512 / 4);

            if (!(err & ATAF_ERROR))
            {
                if (!(ide_in(ata_Status, port) & ATAF_DATAREQ))
                {
                    return 1;
                }
            }
        }
    }

    ResumeError(port, unit);
    return 0;
}

ULONG ata_Read(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act)
{
    struct ideBase  *ib;
    ULONG           port;

    ib = unit->au_Device;
    port = unit->au_PortAddr;

    *act = count;

    /* if block addr is valid (less than total blocks) */
    if (block < unit->au_Blocks)
    {
        /* If count is valid (there is something to read) */
        if (count)
        {
            /* Try to set up block addr */
            if (!BlockAddr(block, unit))
                return TDERR_NotSpecified;

            /* Do whole count. Repeat until count >0. At end of each loop
             * increment block address */
            for(;*act;IncBlockAddr(port, unit))
            {
                /* Sector count (0 == 256 sectors) */
                ide_out(count, ata_SectorCnt, port);
                /* Do read! */
                ide_out(ATA_READ, ata_Command, port);

                do
                {
                    /* Wait for completion */
                    if (!WaitBusy(port, unit))
                        return TDERR_NotSpecified;

                    /* Data buffer ready? */
                    if (!(ide_in(ata_Status, port) & ATAF_DATAREQ))
                        return TDERR_NotSpecified;

                    /* Yes. Copy ide buffer */
                    insl(port, buffer, 512 / 4);

                    /* Handle swapped bit here!!!!!!!! */

                    /* Wait for completion */
                    if (!WaitBusy(port, unit))
                        return TDERR_NotSpecified;

                    /* Exit if there was any error */
                    if (ide_in(ata_Status, port) & ATAF_ERROR)
                        return TDERR_NotSpecified;
                /* Loop 'till lobyte of count !=0 */
                } while((--(*act)) & 0xff);
            }
            
            *act = count;
            /* No errors. Return */
            return 0;       
        }
    }
    
    ResumeError(port, unit);
    
    /* Fail. */
    return TDERR_NotSpecified;
}

ULONG ata_Write(ULONG block, ULONG count, APTR buffer, struct ide_Unit *unit, ULONG *act)
{
    ULONG port;

    port = unit->au_PortAddr;

    *act = count;
    
    if (block < unit->au_Blocks)
    {
        if (count)
        {
            if (!BlockAddr(block, unit))
                return TDERR_NotSpecified;
            
            for (; *act; IncBlockAddr(port, unit))
            {
                ide_out(count, ata_SectorCnt, port);
                ide_out(ATA_WRITE, ata_Command, port);

                /* Handle swapped right now!!!! */

                do
                {
                    if (!WaitBusy(port, unit))
                        return TDERR_NotSpecified;
                    
                    if ((ide_in(ata_Status, port) & (ATAF_ERROR | ATAF_DATAREQ)) !=
                                    ATAF_DATAREQ)
                        return TDERR_NotSpecified;

                    outsl(port, buffer, 512 / 4);
                } while((--(*act)) & 0xff);
                
                if (!WaitBusy(port, unit))
                    return TDERR_NotSpecified;
            }

            if (ide_in(ata_Status, port) & ATAF_ERROR)
                return TDERR_NotSpecified;
            
            *act = count;
            return 0;
        }
    }

    ResumeError(port, unit);
    
    return TDERR_NotSpecified;
}

ULONG ata_Seek(ULONG block, struct ide_Unit *unit)
{
    ULONG port;
    ULONG err;

    port = unit->au_PortAddr;

    if (block < unit->au_Blocks)
    {
        if (BlockAddr(port, unit))
        {
            ide_out(ATA_SEEK, ata_Command, port);

            if (WaitBusy(port, unit))
                if (!ide_in(ata_Status, port) & ATAF_ERROR)
                    return 0;
            
            err = 0x0b02;       /* Aborted command + No seek complete */
            block = ActualAddr(port, unit);
        } else err = 0x0205;    /* Not ready + logical unit not respond */
    } else err = 0x0521;        /* Illegal request + LBA out of range */

    unit->au_LBASense = block;
    unit->au_SenseKey = err;

    return TDERR_NotSpecified;
}

ULONG ata_Eject(struct ide_Unit *unit)
{
    ULONG port;

    port = unit->au_PortAddr;

    ide_out(unit->au_DevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(ATA_MEDIAEJECT, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            return 0;
        }
    }

    unit->au_LBASense = 0;
    unit->au_SenseKey = 0x0205; /* Not ready + logical unit not respond */
    
    return TDERR_NotSpecified;
}

ULONG ata_Identify(APTR buffer, struct ide_Unit *unit)
{
    ULONG port;
    UBYTE comm;
    
    port = unit->au_PortAddr;

    comm = (unit->au_Flags & AF_AtapiDev) ? ATAPI_IDENTDEV : ATA_IDENTDEV;

    ide_out(unit->au_DevMask, ata_DevHead, port);
    if (WaitBusy(port, unit))
    {
        ide_out(comm, ata_Command, port);
        if (WaitBusy(port, unit))
        {
            if (ide_in(ata_Status, port) & ATAF_DATAREQ)
            {
                struct iDev *id = buffer;

                insl(port, buffer, 512 / 4);

                if (!(ide_in(ata_Status, port) & ATAF_ERROR))
                {
                    if (unit->au_Flags & AF_AtapiDev)
                    {
                        UBYTE type;
                        type = (id->idev_DInfo >> 8) & 0x1f;
                        
                        switch (type)
                        {
                            case DG_CDROM:
                            case DG_WORM:
                            case DG_OPTICAL_DISK:
                                id->idev_Heads = 1;
                                id->idev_Sectors = 75;
                                id->idev_Cylinders = 4440;
                                unit->au_Flags |= AF_SlowDevice;
                                break;

                            case DG_DIRECT_ACCESS:
                                if (!strcmp("LS-120", &id->idev_ModelNumber[0]))
                                {
                                    id->idev_Heads = 2;
                                    id->idev_Sectors = 18;
                                    id->idev_Cylinders = 6848;
                                    unit->au_Flags |= AF_SlowDevice;
                                }
                                else if (!strcmp("ZIP 100 ", &id->idev_ModelNumber[8]))
                                {
                                    id->idev_Heads = 1;
                                    id->idev_Sectors = 64;
                                    id->idev_Cylinders = 3072;
                                    unit->au_Flags &= ~AF_SlowDevice;
                                }
                                break;
                        }
                    }
                    return 1;
                }
            }
        }               
    }

    ResumeError(port, unit);
    return 0;
}

/**** Helper functions ********************************************************/

void ResumeError(ULONG port, struct ide_Unit *unit)
{
    if (unit->au_Flags & AF_AtapiDev)
    {
        ide_out(ATAPI_RESET, ata_Command, port);
    }
    else
    {
        ide_out(ATA_RECALIBRATE, ata_Command, port);
    }
    WaitBusyLong(port, unit);
}

void IncBlockAddr(ULONG port, struct ide_Unit *unit)
{
    if (unit->au_Flags & AF_LBAMode)
    {
        UBYTE tmp;
        tmp = ide_in(ata_SectorNum, port);
        ide_out(++tmp, ata_SectorNum, port);
        if (tmp)
            return;
        tmp = ide_in(ata_CylinderL, port);
        ide_out(++tmp, ata_CylinderL, port);
        if (tmp)
            return;
        tmp = ide_in(ata_CylinderH, port);
        ide_out(++tmp, ata_CylinderH, port);
        if (tmp)
            return;
        tmp = ide_in(ata_DevHead, port);
        ide_out(++tmp, ata_DevHead, port);
    }
    else
    {
        UBYTE tmp;

        tmp = ide_in(ata_SectorNum, port);
        if (tmp == unit->au_SectorsT)
        {
            ide_out(1, ata_SectorNum, port);
            tmp = (ide_in(ata_DevHead, port) && 0x0f) + 1;
            if (tmp == unit->au_Heads)
            {
                ide_out(ide_in(ata_DevHead, port) && 0xf0,
                                ata_DevHead, port);
                tmp = ide_in(ata_CylinderL, port);
                ide_out(++tmp, ata_CylinderL, port);
                if (!tmp)
                    ide_out(ide_in(ata_CylinderH, port) + 1,
                                    ata_CylinderH, port);
            } else ide_out(ide_in(ata_DevHead, port) + 1,
                            ata_DevHead, port);
        }
        else ide_out(++tmp, ata_SectorNum, port);
    }
}

int BlockAddr(ULONG block, struct ide_Unit *unit)
{
    ULONG port = unit->au_PortAddr;
    
    /* LBA mode active? */
    if (unit->au_Flags & AF_LBAMode)
    {
        /* LBA[27..24] + dev select */
        ide_out((block >> 24) | unit->au_DevMask, ata_DevHead, port);
        if (!WaitBusy(port, unit))
            return 0;
        ide_out(block >> 16, ata_CylinderH, port);  /* LBA[23..16] */
        ide_out(block >> 8, ata_CylinderL, port);   /* LBA[15..8] */
        ide_out(block, ata_SectorNum, port);        /* LBA[7..0] */
    }
    /* Use CHS mode instead */
    else
    {
        ULONG sec, cyl, hd;

        sec     =   block;                      /* sec = block */
        cyl     =   sec / unit->au_SectorsC;    /* cyl = real cyn number */
        sec     %=  unit->au_SectorsC;          /* sec = real sec number */
        hd      =   sec / unit->au_SectorsT;    /* hd = real hd number */
        sec     %=  unit->au_SectorsT;          /* sec = sec number -1 */
        sec++;                                  /* End of translation */

        /* We have calculated all needed valuse.
         * Send them right now */
        ide_out(hd | unit->au_DevMask, ata_DevHead, port);
        if (!WaitBusy(port, unit))
            return 0;
        ide_out(sec, ata_SectorNum, port);
        ide_out(cyl, ata_CylinderL, port);
        ide_out(cyl >> 8, ata_CylinderH, port);
    }

    return 1;
}

ULONG ActualAddr(ULONG port, struct ide_Unit *unit)
{
    ULONG block;
    
    if (unit->au_Flags & AF_LBAMode)
    {
        block = ide_in(ata_DevHead, port) & 0x0f;
        block <<= 8;
        block |= ide_in(ata_CylinderH, port);
        block <<= 8;
        block |= ide_in(ata_CylinderL, port);
        block <<= 8;
        block |= ide_in(ata_SectorNum, port);
    }
    else
    {
        block = ide_in(ata_CylinderH, port) << 8 |
                ide_in(ata_CylinderL, port);
        block *= unit->au_SectorsC;
        block += (ide_in(ata_DevHead, port) & 0x0f) * unit->au_SectorsT;
        block += ide_in(ata_SectorNum, port) - 1;
    }
    
    return block;
}

int WaitBusy(ULONG port, struct ide_Unit *unit)
{
    ULONG cnt = unit->au_NumLoop;
    UBYTE status;
    
    do
    {
	    status = ide_in(ata_Status, port);
    } while((status & ATAF_BUSY) && --cnt);
    
    return cnt;
}

int WaitBusyLong(ULONG port, struct ide_Unit *unit)
{
    ULONG cnt = unit->au_NumLoop << 2;
    UBYTE status;

    do
    {
        status = ide_in(ata_Status, port);
    } while ((status & ATAF_BUSY) && --cnt);
    
    return cnt;
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
    
    ib->ide_TimerMP = MyCreateMsgPort(sysBase);
    ib->ide_TimerIO = (struct timerequest *)
                MyCreateIORequest(ib->ide_TimerMP, sizeof(struct timerequest), sysBase);
    // UNIT_MICROHZ!!!!!!!!!
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)ib->ide_TimerIO, 0);

    /*
     * Allocate signal for port. Since this is freshly created task we can
     * do it without any check
     */
    td->td_Port.mp_SigBit = AllocSignal(-1);
    td->td_Port.mp_Flags = 0;
    
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
    
    dd->dd_TimerMP = MyCreateMsgPort(sysBase);
    dd->dd_TimerIO = (struct timerequest *)
            MyCreateIORequest(dd->dd_TimerMP, sizeof(struct timerequest), sysBase);
    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)dd->dd_TimerIO, 0);

    /* Allocate MsgPort for querying for diskchange */
    dd->dd_DevMP = MyCreateMsgPort(sysBase);
    /* Allocate IOStdReq for each unit */
    for(i = 0; i < ib->ide_NumUnit; i++)
    {
        dd->dd_DevIO[i] = (struct IOStdReq *)
                MyCreateIORequest(dd->dd_DevMP, sizeof(struct IOStdReq), sysBase);
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

struct IORequest *MyCreateIORequest(struct MsgPort *reply, ULONG length, 
				    struct ExecBase *sysBase)
{
    struct IORequest *io;

    io = AllocMem(length, MEMF_PUBLIC|MEMF_CLEAR);

    if (io)
    {
        io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        io->io_Message.mn_ReplyPort = reply;
        io->io_Message.mn_Length = length;
        
        return io;
    }
    return 0;
}

struct MsgPort *MyCreateMsgPort(struct ExecBase *sysBase)
{
    struct MsgPort *mp;

    mp = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC|MEMF_CLEAR);

    if (mp)
    {
        mp->mp_SigBit = AllocSignal(-1);
        mp->mp_Node.ln_Type = NT_MSGPORT;
        mp->mp_Flags = 0;
        if (mp->mp_SigBit != 0xff)
        {
            mp->mp_SigTask = FindTask(NULL);
            NEWLIST(&mp->mp_MsgList);
            return mp;
        }
        FreeMem(mp, sizeof(struct MsgPort));
    }
    return 0;
}

/**** Do not place any code here unless you redefine SysBase again! ***********/

static const char end = 0;
