/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

#define DEBUG 0

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
#include <asm/amcc440.h>

#include <aros/debug.h>
#undef kprintf
#include <proto/arossupport.h>

//#include "timer_intern.h"
#include LC_LIBDEFS_FILE

#include "kernel_syscall.h"
#include "kernel_globals.h"
#include "kernel_intern.h"

#include "lowlevel.h"

void DecrementerHandler(struct TimerBase *TimerBase, struct ExecBase *SysBase);
void GPTHandler(struct TimerBase *TimerBase, struct ExecBase *SysBase);

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    void *KernelBase = getKernelBase();
    struct ExecBase *SysBase = getSysBase();
    
    TimerBase->tb_prev_tick = inl(GPT0_TBC);
    
    /* Setup the timer.device data */
    LIBBASE->tb_CurrentTime.tv_secs = 0;
    LIBBASE->tb_CurrentTime.tv_micro = 0;
    LIBBASE->tb_Elapsed.tv_secs = 0;
    LIBBASE->tb_Elapsed.tv_micro = 0;

    /* Initialise the lists */
    NEWLIST( &LIBBASE->tb_Lists[0] );
    NEWLIST( &LIBBASE->tb_Lists[1] );
    NEWLIST( &LIBBASE->tb_Lists[2] );
    NEWLIST( &LIBBASE->tb_Lists[3] );
    NEWLIST( &LIBBASE->tb_Lists[4] );

    /* Start up the interrupt server */
    LIBBASE->tb_TimerIRQHandle = KrnAddIRQHandler(INTR_GDP, GPTHandler, LIBBASE, SysBase); //KrnAddExceptionHandler(10, DecrementerHandler, LIBBASE, SysBase);
    
    outl(GPT0_DCIS_DCIS, GPT0_DCIS);
    TimerSetup(TimerBase, inl(GPT0_TBC));
    //outl(1333, GPT0_DCT0);
    
    
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
    /*
        Normally, we should check the length of the message and other
        such things, however the RKM documents an example where the
        length of the timerrequest isn't set, so we must not check
        this.

        This fixes bug SF# 741580
    */

    D(bug("[timer] OpenDevice(%d)\n", unitNum));
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
    void *KernelBase = getKernelBase();
    
    KrnRemIRQHandler(LIBBASE->tb_TimerIRQHandle);
    
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
