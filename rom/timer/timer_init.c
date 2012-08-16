/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands, reference code
    
    This code uses VBlank interrupt as a source for both units. This can be
    considered a reference code for building basic timer.device implementations,
    using a periodic timer. However, in real life you'll want better timing source
    than 50 Hz, so you'll likely replace this with your own code.
*/

/****************************************************************************************/

#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/alerts.h>
#include <exec/initializers.h>
#include <devices/timer.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include <aros/symbolsets.h>
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "timer_macros.h"

/* exec.library VBlank interrupt handler  */
static AROS_INTH1(VBlankInt, struct TimerBase *, TimerBase)
{
    AROS_INTFUNC_INIT

    /*
     * First increment the current time. No need to Disable() here as
     * there are no other interrupts that are allowed to interrupt us
     * that can do anything with this.
     */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_Platform.tb_VBlankTime);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_Platform.tb_VBlankTime);
    TimerBase->tb_ticks_total++;

    /*
     * Now go to handle requests.
     * We are called at rather low rate, so don't bother and process both units.
     */
    handleMicroHZ(TimerBase, SysBase);
    handleVBlank(TimerBase, SysBase);

    return 0;

    AROS_INTFUNC_EXIT
}

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct Interrupt *is;

    /* If no frequency is set, assume 50Hz */
    if (SysBase->VBlankFrequency == 0)
        SysBase->VBlankFrequency = 50;

    /*
     * Here we do no checks, we simply assume we have working VBlank interrupt,
     * from whatever source it is.
     */

    LIBBASE->tb_eclock_rate = SysBase->VBlankFrequency;
    D(bug("[timer] Timer IRQ is %d, frequency is %u Hz\n", INTB_VERTB, LIBBASE->tb_eclock_rate));

    /* Calculate timer period in us */
    LIBBASE->tb_Platform.tb_VBlankTime.tv_secs  = 0;
    LIBBASE->tb_Platform.tb_VBlankTime.tv_micro = 1000000 / LIBBASE->tb_eclock_rate;

    D(bug("Timer period: %ld secs, %ld micros\n",
	LIBBASE->tb_Platform.tb_VBlankTime.tv_secs, LIBBASE->tb_Platform.tb_VBlankTime.tv_micro));

    /* Start up the interrupt server */
    is = AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC);
    if (is)
    {
	is->is_Node.ln_Pri = 0;
	is->is_Node.ln_Type = NT_INTERRUPT;
	is->is_Node.ln_Name = (STRPTR)MOD_NAME_STRING;
	is->is_Code = (VOID_FUNC)VBlankInt;
	is->is_Data = LIBBASE;

	AddIntServer(INTB_VERTB, is);
	LIBBASE->tb_TimerIRQHandle = is;
	
	return TRUE;
    }

    return FALSE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
