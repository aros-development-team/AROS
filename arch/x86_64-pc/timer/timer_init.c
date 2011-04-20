/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

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
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/kernel.h>

#include <aros/symbolsets.h>

#include <asm/io.h>

#include <aros/debug.h>

#include LC_LIBDEFS_FILE

void VBlankInt(struct TimerBase *TimerBase, struct ExecBase *SysBase);

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    APTR KernelBase = OpenResource("kernel.resource");

    /* We must have kernel.resource */
    if (!KernelBase)
    	return FALSE;

    /* Setup the timer.device data */
    LIBBASE->tb_CurrentTime.tv_secs = 0;
    LIBBASE->tb_CurrentTime.tv_micro = 0;
    LIBBASE->tb_VBlankTime.tv_secs = 0;
    LIBBASE->tb_VBlankTime.tv_micro = 1000000 / (SysBase->VBlankFrequency * SysBase->PowerSupplyFrequency);
    LIBBASE->tb_Elapsed.tv_secs = 0;
    LIBBASE->tb_Elapsed.tv_micro = 0;

    D(kprintf("Timer period: %ld secs, %ld micros\n",
	LIBBASE->tb_VBlankTime.tv_secs, LIBBASE->tb_VBlankTime.tv_micro));
    
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

    /* Start up the interrupt server */
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(0, VBlankInt, LIBBASE, SysBase);

    /* VBlank EMU */
    
    LIBBASE->tb_vblank_timerequest.tr_node.io_Command = TR_ADDREQUEST;
    LIBBASE->tb_vblank_timerequest.tr_node.io_Device = (struct Device *)TimerBase;        
    LIBBASE->tb_vblank_timerequest.tr_node.io_Unit = (struct Unit *)UNIT_MICROHZ;    
    LIBBASE->tb_vblank_timerequest.tr_time.tv_secs = 0;
    LIBBASE->tb_vblank_timerequest.tr_time.tv_micro = 1000000 / SysBase->VBlankFrequency;
    
    SendIO(&LIBBASE->tb_vblank_timerequest.tr_node);
    
    return TRUE;
}

/****************************************************************************************/

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct timerequest *tr,
    ULONG unitNum,
    ULONG flags
)
{
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
}

/****************************************************************************************/

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR LIBBASE)
{
    APTR KernelBase = LIBBASE->tb_KernelBase;

    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
