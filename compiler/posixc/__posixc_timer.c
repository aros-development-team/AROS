

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <exec/types.h>
#include <devices/timer.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>

#include <time.h>
#include <errno.h>

#include "__posixc_intbase.h"

void __init_timerbase(struct PosixCIntBase *PosixCBase)
{
    memset( &PosixCBase->timerPort, 0, sizeof( PosixCBase->timerPort ) );
    PosixCBase->timerPort.mp_Node.ln_Type   = NT_MSGPORT;
    PosixCBase->timerPort.mp_Flags          = PA_IGNORE;
    PosixCBase->timerPort.mp_SigTask        = FindTask(NULL);
    NEWLIST(&PosixCBase->timerPort.mp_MsgList);

    PosixCBase->timerReq.tr_node.io_Message.mn_Node.ln_Type    = NT_MESSAGE;
    PosixCBase->timerReq.tr_node.io_Message.mn_Node.ln_Pri     = 0;
    PosixCBase->timerReq.tr_node.io_Message.mn_Node.ln_Name    = NULL;
    PosixCBase->timerReq.tr_node.io_Message.mn_ReplyPort       = &PosixCBase->timerPort;
    PosixCBase->timerReq.tr_node.io_Message.mn_Length          = sizeof (PosixCBase->timerReq);

    if
    (
        OpenDevice
        (
            "timer.device",
            UNIT_VBLANK,
            (struct IORequest *)&PosixCBase->timerReq,
            0
        )
        ==
        0
    )
    {
        PosixCBase->timerBase = (struct Device *)PosixCBase->timerReq.tr_node.io_Device;
    }
}

static void __exit_timerbase(struct PosixCIntBase *PosixCBase)
{
    if (PosixCBase->timerBase != NULL)
    {
        CloseDevice((struct IORequest *)&PosixCBase->timerReq);
        PosixCBase->timerBase = NULL;
    }
}

ADD2EXIT(__exit_timerbase, 0);
