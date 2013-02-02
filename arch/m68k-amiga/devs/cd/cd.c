/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include <libraries/expansion.h>

#include <dos/filehandler.h>

#include <devices/cd.h>

#include "cd_intern.h"

struct cdUnit {
    struct MinNode      cu_Node;
    LONG                cu_Unit;
    ULONG               cu_Usage;
    APTR                cu_Private;
    const struct cdUnitOps   *cu_UnitOps;
    struct Task        *cu_Task;
    struct MsgPort     *cu_MsgPort;
};

/* We have a synchonous task for dispatching IO
 * to each cd.device unit.
 */
static VOID cdTask(IPTR base, IPTR unit)
{
    struct cdUnit *cu = (APTR)unit;
    struct IOStdReq *io;

    D(bug("%s.%d Task, Port %p\n", cu->cu_UnitOps->uo_Name, cu->cu_Unit, cu->cu_MsgPort));
    do {
        WaitPort(cu->cu_MsgPort);
        io = (struct IOStdReq *)GetMsg(cu->cu_MsgPort);

        D(bug("%s: Processing %p\n", __func__, io));

        if (io->io_Flags & IOF_ABORT) {
            io->io_Error = CDERR_ABORTED;
        } else if (io->io_Unit == (struct Unit *)cu &&
                   cu->cu_UnitOps->uo_DoIO != NULL) {
            io->io_Error = cu->cu_UnitOps->uo_DoIO(io, cu->cu_Private);
        } else {
            io->io_Error = CDERR_NOCMD;
        }

        D(bug("%s: Reply %p\n", __func__, io));
        ReplyMsg(&io->io_Message);

    } while (io->io_Unit != NULL);

    /* Terminate by fallthough */
}

/* Add a bootnode using expansion.library */
static BOOL cdRegisterVolume(struct cdUnit *unit, const struct DosEnvec *de)
{
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    TEXT dosdevname[4] = "CD0";

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
                                                        40L);

    if (ExpansionBase)
    {
        IPTR pp[24];
        
        CopyMem((IPTR *)de, &pp[4], sizeof(IPTR)*de->de_TableSize);

        /* This should be dealt with using some sort of volume manager or such. */
        if (unit->cu_Unit < 10)
            dosdevname[2] += unit->cu_Unit % 10;
        else
            dosdevname[2] = 'A' - 10 + unit->cu_Unit;
   
        pp[0]                   = (IPTR)dosdevname;
        pp[1]                   = (IPTR)MOD_NAME_STRING;
        pp[2]                   = unit->cu_Unit;
        pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
        pp[DE_BOOTPRI      + 4] = -10;
        pp[DE_DOSTYPE      + 4] = AROS_MAKE_ID('C','D','F','S');
        pp[DE_CONTROL      + 4] = 0;
        pp[DE_BOOTBLOCKS   + 4] = 0;
    
        devnode = MakeDosNode(pp);

        if (devnode)
        {
            AddBootNode(pp[DE_BOOTPRI + 4], ADNF_STARTPROC, devnode, NULL);
            
            return TRUE;
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}

VOID cdDelayMS(struct cdBase *cb, ULONG ms)
{
    cb->cb_TimerPort.mp_SigTask = FindTask(NULL);
    cb->cb_TimerRequest.tr_node.io_Command = TR_ADDREQUEST;
    cb->cb_TimerRequest.tr_time.tv_secs = ms / 1000;
    cb->cb_TimerRequest.tr_time.tv_micro = (ms * 1000) % 1000000;

    DoIO((struct IORequest *)&cb->cb_TimerRequest);
}


LONG cdAddUnit(struct cdBase *cb, const struct cdUnitOps *ops, APTR priv, const struct DosEnvec *de)
{
    struct cdUnit *cu;

    cu = AllocVec(sizeof(*cu), MEMF_CLEAR | MEMF_ANY);
    if (cu) {
        cu->cu_Private = priv;
        cu->cu_UnitOps = ops;
        cu->cu_Task = NewCreateTask(TASKTAG_PC, cdTask,
                                    TASKTAG_NAME, ops->uo_Name,
                                    TASKTAG_ARG1, cb,
                                    TASKTAG_ARG2, cu,
                                    TASKTAG_TASKMSGPORT, &cu->cu_MsgPort,
                                    TAG_END);
        if (cu->cu_Task) {
            cu->cu_Unit = cb->cb_MaxUnit++;
            ObtainSemaphore(&cb->cb_UnitsLock);
            ADDTAIL(&cb->cb_Units, &cu->cu_Node);
            ReleaseSemaphore(&cb->cb_UnitsLock);
            cdRegisterVolume(cu, de);
            return cu->cu_Unit;
        }
        FreeVec(cu);
    }

    return -1;
}

static int cd_Init(LIBBASETYPE *cb)
{
    NEWLIST(&cb->cb_Units);
    InitSemaphore(&cb->cb_UnitsLock);

    /* Hand-hacked port for timer responses */
    cb->cb_TimerPort.mp_SigBit = SIGB_SINGLE;
    cb->cb_TimerPort.mp_Flags = PA_SIGNAL;
    cb->cb_TimerPort.mp_SigTask = FindTask(NULL);
    cb->cb_TimerPort.mp_Node.ln_Type = NT_MSGPORT;

    cb->cb_TimerRequest.tr_node.io_Message.mn_ReplyPort = &cb->cb_TimerPort;
    cb->cb_TimerRequest.tr_node.io_Message.mn_Length = sizeof(cb->cb_TimerRequest);

    return (0 == OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)&cb->cb_TimerRequest, 0));
}

static int cd_Expunge(LIBBASETYPE *cb)
{
    struct cdUnit *cu;
    struct IORequest io;
    struct MsgPort *mp = CreateMsgPort();

    CloseDevice((struct IORequest *)&cb->cb_TimerPort);

    while ((cu = (APTR)REMOVE(&cb->cb_Units)) != NULL) {
        D(bug("%s: Remove Unit %d\n", __func__, cu->cu_Unit));

        /* Shut down the unit's task */
        io.io_Device = (struct Device *)cb;
        io.io_Unit   = NULL;
        io.io_Command = CMD_INVALID;
        io.io_Message.mn_ReplyPort = mp;
        io.io_Message.mn_Length = sizeof(io);
        PutMsg(cu->cu_MsgPort, &io.io_Message);
        WaitPort(mp);
        GetMsg(mp);

        if (cu->cu_UnitOps->uo_Expunge)
            cu->cu_UnitOps->uo_Expunge(cu->cu_Private);
        FreeVec(cu);
    }

    DeleteMsgPort(mp);

    return 1;
}

ADD2INITLIB(cd_Init, 0);
ADD2EXPUNGELIB(cd_Expunge, 0);

AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 5, cd)
{
    AROS_LIBFUNC_INIT

    struct IOStdReq *iostd = (struct IOStdReq *)io;
    struct cdUnit *cu = (struct cdUnit *)(io->io_Unit);

    D(bug("%s.%d: %p\n"
          "io_Command: %d\n"
          "io_Length:  %d\n"
          "io_Data:    %p\n"
          "io_Offset:  %d\n",
          __func__, cu->cu_Unit, iostd,
          iostd->io_Command,
          iostd->io_Length, iostd->io_Data, iostd->io_Offset));

    io->io_Error = CDERR_NOCMD;

    io->io_Flags &= ~IOF_QUICK;
    PutMsg(cu->cu_MsgPort, &iostd->io_Message);

    return;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 6, cd)
{
    AROS_LIBFUNC_INIT

    D(bug("%s.%d: %p\n", __func__, ((struct cdUnit *)(io->io_Unit))->cu_Unit, io));
    Forbid();
    io->io_Flags |= IOF_ABORT;
    Permit();

    return TRUE;

    AROS_LIBFUNC_EXIT
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR cdBase, struct IOStdReq *ioreq, ULONG unitnum, ULONG flags)
{
    struct cdUnit *cu = NULL;

    ObtainSemaphore(&cdBase->cb_UnitsLock);
    ForeachNode(&cdBase->cb_Units, cu) {
        if (cu->cu_Unit == unitnum) {
            cu->cu_Usage++;
            ioreq->io_Unit = (struct Unit *)cu;
            ReleaseSemaphore(&cdBase->cb_UnitsLock);
            return TRUE;
        }
    }
    ReleaseSemaphore(&cdBase->cb_UnitsLock);

    return FALSE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR cdBase, struct IORequest *ioreq)
{
    struct cdUnit *cu = (APTR)ioreq->io_Unit;

    ObtainSemaphore(&cdBase->cb_UnitsLock);
    cu->cu_Usage--;
    ReleaseSemaphore(&cdBase->cb_UnitsLock);

    return TRUE;
}

ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)
