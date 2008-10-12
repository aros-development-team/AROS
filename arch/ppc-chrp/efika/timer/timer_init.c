/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/

#define DEBUG 1

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
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/openfirmware.h>

#include <utility/tagitem.h>

#include <aros/symbolsets.h>

#include <asm/io.h>
#include <asm/mpc5200b.h>

#include <aros/debug.h>
#include <proto/arossupport.h>

//#include "timer_intern.h"
#include LC_LIBDEFS_FILE

#include "lowlevel.h"

void SliceHandler(struct TimerBase *TimerBase, struct ExecBase *SysBase);
extern volatile slt_t *slice_timer;

/****************************************************************************************/

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct ExecBase *SysBase = getSysBase();
    void *KernelBase = getKernelBase();

    TimerBase->tb_prev_tick = mftbl();

    /* Setup the timer.device data */
    LIBBASE->tb_CurrentTime.tv_secs = 0;
    LIBBASE->tb_CurrentTime.tv_micro = 0;
    LIBBASE->tb_VBlankTime.tv_secs = 0;
    LIBBASE->tb_VBlankTime.tv_micro = 1000000 / SysBase->VBlankFrequency;
    LIBBASE->tb_Elapsed.tv_secs = 0;
    LIBBASE->tb_Elapsed.tv_micro = 0;

    D(bug("Timer period: %ld secs, %ld micros\n",
        LIBBASE->tb_VBlankTime.tv_secs, LIBBASE->tb_VBlankTime.tv_micro));

    LIBBASE->tb_MiscFlags = TF_GO;

    /* Initialise the lists */
    NEWLIST( &LIBBASE->tb_Lists[0] );
    NEWLIST( &LIBBASE->tb_Lists[1] );
    NEWLIST( &LIBBASE->tb_Lists[2] );
    NEWLIST( &LIBBASE->tb_Lists[3] );
    NEWLIST( &LIBBASE->tb_Lists[4] );

    /* Start up the interrupt server. This is shared between us and the
        HIDD that deals with the vblank */
    LIBBASE->tb_VBlankInt.is_Node.ln_Pri = 0;
    LIBBASE->tb_VBlankInt.is_Node.ln_Type = NT_INTERRUPT;
    LIBBASE->tb_VBlankInt.is_Node.ln_Name = (STRPTR)MOD_NAME_STRING;
    LIBBASE->tb_VBlankInt.is_Data = LIBBASE;

    LIBBASE->tb_VBlankInt.is_Code = KrnAddIRQHandler(MPC5200B_ST1, SliceHandler, LIBBASE, SysBase); //KrnAddExceptionHandler(10, DecrementerHandler, LIBBASE, SysBase);

    void *OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key = OF_OpenKey("/builtin");
    if (key)
    {
    	void *prop = OF_FindProperty(key, "reg");
    	if (prop)
    	{
    		intptr_t *mbar = OF_GetPropValue(prop);
    		slice_timer = (slt_t *)(*mbar + 0x710);

    		D(bug("MBAR located at %08x\n", *mbar));
    		D(bug("slice timer at %08x\n", slice_timer));
    	}
    }

    /* Start the slice timer 1 */

    outl(SLT_TS_ST, &slice_timer->slt_ts);
    TimerSetup(TimerBase, mftbl());

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

    KrnRemIRQHandler(LIBBASE->tb_VBlankInt.is_Code);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)
