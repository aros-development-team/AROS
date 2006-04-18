/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: switch.c 18579 2003-07-11 06:42:29Z hkiel $

    Desc: Native version of Switch().
    Lang: english
*/

#include <exec/execbase.h>
#include <proto/exec.h>

AROS_LH0(void, Switch,
    struct ExecBase *, SysBase, 9, Exec)
{
    AROS_LIBFUNC_INIT

    struct Task *this = SysBase->ThisTask;

    /*
        If the state is not TS_RUN then the task is already in a list
    */

    Disable();
    
    if( (this->tc_State != TS_RUN)
         && !(this->tc_Flags & TF_EXCEPT) )
    {
        /*      Its quite possible that they have interrupts Disabled(),
            we should fix that here, otherwise we can't switch. 

            We can't call the dispatcher because we need a signal,
            lets just create one.

            Have to set the dispatch-required flag.
            I use SIGUSR1 (maps to SoftInt) because it has less effect on
            the system clock, and is probably quicker.
        */

        SysBase->AttnResched |= 0x8000;
        __asm__ __volatile__ ("move.l %d7,-(%sp)\n\t"
                              "clr.l %d7\n\t"
                              "trap #1\n\t"
                              "move.l (%sp)+,%d7\n\t");
    }

    Enable();
    
    AROS_LIBFUNC_EXIT
} /* Switch() */
