/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    Auto open the timer.device for function call use.
*/

#include <exec/types.h>
#include <devices/timer.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/dos.h>

static struct timerequest   __auto_tr;
static struct MsgPort       __auto_mp;
struct TimerBase            *TimerBase = NULL;

int
__init_timerbase(void)
{
    __auto_mp.mp_Node.ln_Type   = NT_MSGPORT;
    __auto_mp.mp_Node.ln_Pri    = 0;
    __auto_mp.mp_Node.ln_Name   = NULL;
    __auto_mp.mp_Flags          = PA_IGNORE;
    __auto_mp.mp_SigTask        = FindTask(NULL);
    __auto_mp.mp_SigBit         = 0;
    NEWLIST(&__auto_mp.mp_MsgList);

    __auto_tr.tr_node.io_Message.mn_Node.ln_Type    = NT_MESSAGE;
    __auto_tr.tr_node.io_Message.mn_Node.ln_Pri     = 0;
    __auto_tr.tr_node.io_Message.mn_Node.ln_Name    = NULL;
    __auto_tr.tr_node.io_Message.mn_ReplyPort       = &__auto_mp;
    __auto_tr.tr_node.io_Message.mn_Length          = sizeof (__auto_tr);

    if
    (
        OpenDevice
        (
            "timer.device",
            UNIT_VBLANK,
            (struct IORequest *)&__auto_tr,
            0
        )
        ==
        0
    )
    {
        TimerBase = (struct TimerBase *)__auto_tr.tr_node.io_Device;
        return 0;
    }
    else
    {
        return -1;
    }
}


int
__exit_timerbase(void)
{
    if (TimerBase != NULL)
        CloseDevice((struct IORequest *)&__auto_tr);
}

ADD2INIT(__init_timerbase, 1000);
ADD2EXIT(__exit_timerbase, 1000);
