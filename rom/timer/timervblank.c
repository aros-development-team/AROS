/*
    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: VBlank server for the timer.device/timer.hidd
    Lang: english
*/
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <aros/debug.h>

#include "timer_intern.h"
#undef SysBase

AROS_UFH4(ULONG, VBlankInt,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
    )
{
    /*
	Firstly increment the current time. No need to Disable() here as
	there are no other interrupts that are allowed to interrupt us
	that can do anything with this.
    */
    AddTime(&TimerBase->tb_CurrentTime, &TimerBase->tb_VBlankTime);

    /* XXX: We need to put the code to handle the wait lists here. */

    return 0;
}
	
