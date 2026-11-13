/*
 * vbthci.device -- device entry points.
 */

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/exec.h>
#include <exec/errors.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "vbthci_intern.h"

#include LC_LIBDEFS_FILE

#define NewList(list) NEWLIST(list)

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR VBTHCIBase)
{
    InitSemaphore(&VBTHCIBase->vb_Lock);
    return TRUE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR VBTHCIBase)
{
    ULONG n;
    for(n = 0; n < VBTHCI_NUMUNITS; n++) {
        if(VBTHCIBase->vb_Units[n]) {
            return FALSE;
        }
    }
    return TRUE;
}

static struct VBTHCIUnit *vbthci_OpenUnit(LIBBASETYPEPTR VBTHCIBase, ULONG unitnum)
{
    struct VBTHCIUnit *unit;
    struct Task *task;

    if(unitnum >= VBTHCI_NUMUNITS) {
        return NULL;
    }
    ObtainSemaphore(&VBTHCIBase->vb_Lock);
    unit = VBTHCIBase->vb_Units[unitnum];
    if(unit) {
        /* one opener per unit, like the USB class devices */
        ReleaseSemaphore(&VBTHCIBase->vb_Lock);
        return NULL;
    }
    unit = AllocVec(sizeof(struct VBTHCIUnit), MEMF_PUBLIC|MEMF_CLEAR);
    if(unit) {
        unit->vu_Base = VBTHCIBase;
        unit->vu_UnitNo = unitnum;
        NewList(&unit->vu_Unit.unit_MsgPort.mp_MsgList);
        NewList((struct List *) &unit->vu_ReadQueue);
        NewList((struct List *) &unit->vu_Timed);
        unit->vu_ReadySignal = SIGB_SINGLE;
        unit->vu_ReadySigTask = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);
        task = (struct Task *) CreateNewProcTags(NP_Entry, (IPTR) vbthci_UnitTask,
                                                 NP_Name, (IPTR) "vbthci.device unit",
                                                 NP_Priority, 10,
                                                 NP_UserData, (IPTR) unit,
                                                 TAG_END);
        if(task) {
            Wait(SIGF_SINGLE);
        }
        unit->vu_ReadySigTask = NULL;
        if(!unit->vu_Task) {
            FreeVec(unit);
            unit = NULL;
        } else {
            unit->vu_Open = TRUE;
            VBTHCIBase->vb_Units[unitnum] = unit;
        }
    }
    ReleaseSemaphore(&VBTHCIBase->vb_Lock);
    return unit;
}

static void vbthci_CloseUnit(LIBBASETYPEPTR VBTHCIBase, struct VBTHCIUnit *unit)
{
    ObtainSemaphore(&VBTHCIBase->vb_Lock);
    VBTHCIBase->vb_Units[unit->vu_UnitNo] = NULL;
    ReleaseSemaphore(&VBTHCIBase->vb_Lock);
    Forbid();
    unit->vu_ReadySignal = SIGB_SINGLE;
    unit->vu_ReadySigTask = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    if(unit->vu_Task) {
        Signal(unit->vu_Task, SIGBREAKF_CTRL_C);
    }
    Permit();
    while(unit->vu_Task) {
        Wait(SIGF_SINGLE);
    }
    FreeVec(unit);
}

static int GM_UNIQUENAME(Open)(LIBBASETYPEPTR VBTHCIBase, struct IOBTHCIReq *ioreq, ULONG unitnum, ULONG flags)
{
    struct VBTHCIUnit *unit;

    ioreq->iobt_Req.io_Unit = NULL;
    if(ioreq->iobt_Req.io_Message.mn_Length < sizeof(struct IOBTHCIReq)) {
        ioreq->iobt_Req.io_Error = IOERR_BADLENGTH;
        return FALSE;
    }
    unit = vbthci_OpenUnit(VBTHCIBase, unitnum);
    if(!unit) {
        ioreq->iobt_Req.io_Error = IOERR_OPENFAIL;
        return FALSE;
    }
    ioreq->iobt_Req.io_Unit = (struct Unit *) unit;
    ioreq->iobt_Req.io_Error = 0;
    return TRUE;
}

static int GM_UNIQUENAME(Close)(LIBBASETYPEPTR VBTHCIBase, struct IOBTHCIReq *ioreq)
{
    struct VBTHCIUnit *unit = (struct VBTHCIUnit *) ioreq->iobt_Req.io_Unit;
    if(unit) {
        vbthci_CloseUnit(VBTHCIBase, unit);
    }
    ioreq->iobt_Req.io_Unit = (APTR) -1;
    ioreq->iobt_Req.io_Device = (APTR) -1;
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)

AROS_LH1(void, BeginIO,
         AROS_LHA(struct IOBTHCIReq *, ioreq, A1),
         struct VBTHCIBase *, VBTHCIBase, 5, VBTHCI)
{
    AROS_LIBFUNC_INIT
    struct VBTHCIUnit *unit = (struct VBTHCIUnit *) ioreq->iobt_Req.io_Unit;
    LONG ret;

    ioreq->iobt_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    ioreq->iobt_Req.io_Error = 0;

    if(!unit || !unit->vu_Task) {
        ioreq->iobt_Req.io_Error = IOERR_OPENFAIL;
        ret = 0;
    } else {
        ret = vbthci_QueueRequest(unit, ioreq);
    }
    if(ret >= 0) {
        /* completed synchronously */
        if(!(ioreq->iobt_Req.io_Flags & IOF_QUICK)) {
            ReplyMsg(&ioreq->iobt_Req.io_Message);
        }
    } else {
        /* queued: the unit task replies */
        ioreq->iobt_Req.io_Flags &= ~IOF_QUICK;
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
         AROS_LHA(struct IOBTHCIReq *, ioreq, A1),
         struct VBTHCIBase *, VBTHCIBase, 6, VBTHCI)
{
    AROS_LIBFUNC_INIT
    struct VBTHCIUnit *unit = (struct VBTHCIUnit *) ioreq->iobt_Req.io_Unit;
    struct MinNode *mn;

    if(!unit) {
        return -1;
    }
    Forbid();
    /* only queued reads can be aborted */
    for(mn = unit->vu_ReadQueue.mlh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        if((struct IOBTHCIReq *) mn == ioreq) {
            Remove((struct Node *) mn);
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&ioreq->iobt_Req.io_Message);
            Permit();
            return 0;
        }
    }
    /* still in the unit port? */
    for(mn = (struct MinNode *) unit->vu_Unit.unit_MsgPort.mp_MsgList.lh_Head; mn->mln_Succ; mn = mn->mln_Succ) {
        if((struct IOBTHCIReq *) mn == ioreq) {
            Remove((struct Node *) mn);
            ioreq->iobt_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&ioreq->iobt_Req.io_Message);
            Permit();
            return 0;
        }
    }
    Permit();
    return -1;
    AROS_LIBFUNC_EXIT
}
