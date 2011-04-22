/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup, common part
*/

#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include "timer_intern.h"

static int common_Init(struct TimerBase *LIBBASE)
{
    ULONG i;

    /* kernel.resource is optional for some implementations, so no check */
    LIBBASE->tb_KernelBase = OpenResource("kernel.resource");

    /* Setup the timer.device data */
    LIBBASE->tb_CurrentTime.tv_secs  = 0;
    LIBBASE->tb_CurrentTime.tv_micro = 0;
    LIBBASE->tb_Elapsed.tv_secs  = 0;
    LIBBASE->tb_Elapsed.tv_micro = 0;

    /* Initialise the lists */
    for (i = 0; i < NUM_LISTS; i++)
	NEWLIST(&LIBBASE->tb_Lists[i]);

    return TRUE;
}

/* This is run before hardware-specific init which has zero priority */
ADD2INITLIB(common_Init, -10);
