/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: h4bthci.device -- device entry points.
*/

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/errors.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/pl011bt.h>

#include "h4bthci_intern.h"

#include LC_LIBDEFS_FILE

#define NewList(list) NEWLIST(list)

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR H4BTHCIBase)
{
    InitSemaphore(&H4BTHCIBase->hu_Lock);
    return TRUE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR H4BTHCIBase)
{
    return H4BTHCIBase->hu_Unit ? FALSE : TRUE;
}

/*
 * One unit, because there is one controller soldered to the board. A second
 * unit number is not a second radio, so it is refused rather than silently
 * answered with the same hardware.
 */
static struct H4BTHCIUnit *h4bthci_OpenUnit(LIBBASETYPEPTR H4BTHCIBase,
                                                ULONG unitnum)
{
    struct H4BTHCIUnit *unit;
    struct Task *task;

    if (unitnum != 0)
        return NULL;

    ObtainSemaphore(&H4BTHCIBase->hu_Lock);
    if (H4BTHCIBase->hu_Unit)
    {
        ReleaseSemaphore(&H4BTHCIBase->hu_Lock);
        return NULL;
    }

    unit = AllocVec(sizeof(struct H4BTHCIUnit), MEMF_PUBLIC | MEMF_CLEAR);
    if (unit)
    {
        unit->hu_Base = H4BTHCIBase;
        NewList(&unit->hu_Unit.unit_MsgPort.mp_MsgList);
        NewList((struct List *)&unit->hu_EventQueue);
        NewList((struct List *)&unit->hu_ACLQueue);
        NewList((struct List *)&unit->hu_Listeners);
        InitSemaphore(&unit->hu_QueueLock);

        unit->hu_ReadySignal = SIGB_SINGLE;
        unit->hu_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);

        task = (struct Task *)CreateNewProcTags(
            NP_Entry,    (IPTR)h4bthci_UnitTask,
            NP_Name,     (IPTR)"h4bthci.device unit",
            NP_Priority, 10,
            NP_UserData, (IPTR)unit,
            TAG_END);

        if (task)
            Wait(SIGF_SINGLE);

        unit->hu_ReadySigTask = NULL;

        if (!unit->hu_Task)
        {
            FreeVec(unit);
            unit = NULL;
        }
        else
        {
            unit->hu_Open = TRUE;
            H4BTHCIBase->hu_Unit = unit;
        }
    }

    ReleaseSemaphore(&H4BTHCIBase->hu_Lock);
    return unit;
}

static void h4bthci_CloseUnit(LIBBASETYPEPTR H4BTHCIBase,
                                struct H4BTHCIUnit *unit)
{
    ObtainSemaphore(&H4BTHCIBase->hu_Lock);
    H4BTHCIBase->hu_Unit = NULL;
    ReleaseSemaphore(&H4BTHCIBase->hu_Lock);

    Forbid();
    unit->hu_ReadySignal = SIGB_SINGLE;
    unit->hu_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if (unit->hu_Task)
        Signal(unit->hu_Task, SIGBREAKF_CTRL_C);
    Permit();

    while (unit->hu_Task)
        Wait(SIGF_SINGLE);

    FreeVec(unit);
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR H4BTHCIBase,
                               struct IOBTHCIReq *ioreq, ULONG unitnum,
                               ULONG flags)
{
    struct H4BTHCIUnit *unit;

    ioreq->iobt_Req.io_Unit = NULL;

    if (ioreq->iobt_Req.io_Message.mn_Length < sizeof(struct IOBTHCIReq))
    {
        ioreq->iobt_Req.io_Error = IOERR_BADLENGTH;
        return FALSE;
    }

    unit = h4bthci_OpenUnit(H4BTHCIBase, unitnum);
    if (!unit)
    {
        ioreq->iobt_Req.io_Error = IOERR_OPENFAIL;
        return FALSE;
    }

    ioreq->iobt_Req.io_Unit = (struct Unit *)unit;
    ioreq->iobt_Req.io_Error = 0;
    return TRUE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR H4BTHCIBase,
                                struct IOBTHCIReq *ioreq)
{
    struct H4BTHCIUnit *unit = (struct H4BTHCIUnit *)ioreq->iobt_Req.io_Unit;

    if (unit)
        h4bthci_CloseUnit(H4BTHCIBase, unit);

    ioreq->iobt_Req.io_Unit = (APTR)-1;
    ioreq->iobt_Req.io_Device = (APTR)-1;
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, BeginIO,
         AROS_LHA(struct IOBTHCIReq *, ioreq, A1),
         struct H4BTHCIBase *, H4BTHCIBase, 5, H4BTHCI)
{
    AROS_LIBFUNC_INIT

    struct H4BTHCIUnit *unit = (struct H4BTHCIUnit *)ioreq->iobt_Req.io_Unit;
    LONG ret;

    ioreq->iobt_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iobt_Req.io_Error = 0;

    if (!unit || !unit->hu_Task)
    {
        ioreq->iobt_Req.io_Error = IOERR_OPENFAIL;
        ret = 0;
    }
    else
        ret = h4bthci_QueueRequest(unit, ioreq);

    if (ret >= 0)
    {
        if (!(ioreq->iobt_Req.io_Flags & IOF_QUICK))
            ReplyMsg(&ioreq->iobt_Req.io_Message);
    }
    else
        ioreq->iobt_Req.io_Flags &= ~IOF_QUICK;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
         AROS_LHA(struct IOBTHCIReq *, ioreq, A1),
         struct H4BTHCIBase *, H4BTHCIBase, 6, H4BTHCI)
{
    AROS_LIBFUNC_INIT

    struct H4BTHCIUnit *unit = (struct H4BTHCIUnit *)ioreq->iobt_Req.io_Unit;
    struct MinNode *mn;
    LONG found = -1;

    if (!unit)
        return -1;

    ObtainSemaphore(&unit->hu_QueueLock);
    for (mn = unit->hu_EventQueue.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
    {
        if ((APTR)mn == (APTR)ioreq)
        {
            Remove((struct Node *)mn);
            found = 0;
            break;
        }
    }
    if (found)
    {
        for (mn = unit->hu_ACLQueue.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ)
        {
            if ((APTR)mn == (APTR)ioreq)
            {
                Remove((struct Node *)mn);
                found = 0;
                break;
            }
        }
    }
    ReleaseSemaphore(&unit->hu_QueueLock);

    if (!found)
    {
        ioreq->iobt_Req.io_Error = IOERR_ABORTED;
        ReplyMsg(&ioreq->iobt_Req.io_Message);
    }

    return found;

    AROS_LIBFUNC_EXIT
}
