/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <dos/dos.h>

#include "detach.h"

LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;
STRPTR          __detached_name                 = "Wanderer";
LONG            __detached_return_value;

VOID DoDetach(LONG rvalue)
{
    if (__detacher_process)
    {
        __detached_return_value = rvalue;
	
	Signal
        (
            (struct Task *) __detacher_process, __detacher_must_wait_for_signal
        );
    }
}
