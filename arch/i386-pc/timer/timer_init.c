/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: timer_init.c 22734 2005-01-23 11:49:48Z verhaegs $

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/alerts.h>
#include <exec/initializers.h>
#include <devices/timer.h>
#include <hidd/timer.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include <aros/symbolsets.h>

#include <asm/io.h>

#include <aros/debug.h>
#undef kprintf
#include <proto/arossupport.h>

//#include "timer_intern.h"
#include LC_LIBDEFS_FILE

AROS_UFP4(ULONG, VBlankInt,
    AROS_UFPA(ULONG, dummy, A0),
    AROS_UFPA(struct TimerBase *, TimerBase, A1),
    AROS_UFPA(ULONG, dummy2, A5),
    AROS_UFPA(struct ExecBase *, SysBase, A6)
);

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Init), LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    /* Setup the timer.device data */
    LIBBASE->tb_CurrentTime.tv_secs = 0;
    LIBBASE->tb_CurrentTime.tv_micro = 0;
    LIBBASE->tb_VBlankTime.tv_secs = 0;
    LIBBASE->tb_VBlankTime.tv_micro = 1000000 / (SysBase->VBlankFrequency * SysBase->PowerSupplyFrequency);
    LIBBASE->tb_Elapsed.tv_secs = 0;
    LIBBASE->tb_Elapsed.tv_micro = 0;

    D(kprintf("Timer period: %ld secs, %ld micros\n",
	LIBBASE->tb_VBlankTime.tv_secs, LIBBASE->tb_VBlankTime.tv_micro));

    LIBBASE->tb_MiscFlags = TF_GO;
    
    /* Initialise the lists */
    NEWLIST( &LIBBASE->tb_Lists[0] );
    NEWLIST( &LIBBASE->tb_Lists[1] );
    NEWLIST( &LIBBASE->tb_Lists[2] );
    NEWLIST( &LIBBASE->tb_Lists[3] );
    NEWLIST( &LIBBASE->tb_Lists[4] );
    
    /* Start the timer2 */
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    outb(0xb4, 0x43);   /* Binary mode on Timer2, count mode 2 */
    outb(0x00, 0x42);   /* We're counting whole range */
    outb(0x00, 0x42);
    
    LIBBASE->tb_prev_tick = 0xffff;

    /* Start up the interrupt server. This is shared between us and the 
	HIDD that deals with the vblank */
    LIBBASE->tb_VBlankInt.is_Node.ln_Pri = 0;
    LIBBASE->tb_VBlankInt.is_Node.ln_Type = NT_INTERRUPT;
    LIBBASE->tb_VBlankInt.is_Node.ln_Name = (STRPTR)MOD_NAME_STRING;
    LIBBASE->tb_VBlankInt.is_Code = (APTR)&VBlankInt;
    LIBBASE->tb_VBlankInt.is_Data = LIBBASE;

    AddIntServer(INTB_VERTB, &LIBBASE->tb_VBlankInt);

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_OPENDEVFUNC(GM_UNIQUENAME(Open),
		     LIBBASETYPE, LIBBASE,
		     struct timerequest,  tr,
		     unitNum, flags
)
{
    AROS_SET_DEVFUNC_INIT
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */

    /*
        Normally, we should check the length of the message and other
        such things, however the RKM documents an example where the
        length of the timerrequest isn't set, so we must not check
        this.

        This fixes bug SF# 741580
    */

    switch(unitNum)
    {
	case UNIT_VBLANK:
	case UNIT_WAITUNTIL:
    case UNIT_MICROHZ:
    case UNIT_ECLOCK:
    case UNIT_WAITECLOCK:   
	    tr->tr_node.io_Error = 0;
	    tr->tr_node.io_Unit = (struct Unit *)unitNum;
	    tr->tr_node.io_Device = (struct Device *)LIBBASE;
	    break;

	default:
	    tr->tr_node.io_Error = IOERR_OPENFAIL;
    }

    return TRUE;
    
    AROS_SET_DEVFUNC_EXIT
}

/****************************************************************************************/

AROS_SET_LIBFUNC(GM_UNIQUENAME(Expunge), LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    RemIntServer(INTB_VERTB, &LIBBASE->tb_VBlankInt);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
