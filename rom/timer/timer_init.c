/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Timer startup and device commands
*/

/****************************************************************************************/


#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/initializers.h>
#include <devices/timer.h>
#include <hidd/timer.h>
#include <exec/resident.h>
#include <hardware/intbits.h>

#include <proto/exec.h>
#include <proto/timer.h>

#include <aros/debug.h>
#undef kprintf
#include <proto/arossupport.h>

#include "timer_intern.h"
#include LC_LIBDEFS_FILE

/****************************************************************************************/

static const char name[];
static const char version[];
static const APTR inittabl[4];
static const void * const functable[];
extern const char LIBEND;

struct TimerBase *AROS_SLIB_ENTRY(init,Timer)();
void AROS_SLIB_ENTRY(open,Timer)();
BPTR AROS_SLIB_ENTRY(close,Timer)();
BPTR AROS_SLIB_ENTRY(expunge,Timer)();
int AROS_SLIB_ENTRY(null,Timer)();

extern ULONG VBlankInt();

extern void AROS_SLIB_ENTRY(BeginIO,Timer)();
extern void AROS_SLIB_ENTRY(AbortIO,Timer)();
extern void AROS_SLIB_ENTRY(AddTime,Timer)();
extern void AROS_SLIB_ENTRY(SubTime,Timer)();
extern void AROS_SLIB_ENTRY(CmpTime,Timer)();
extern void AROS_SLIB_ENTRY(ReadEClock,Timer)();
extern void AROS_SLIB_ENTRY(GetSysTime,Timer)();

/****************************************************************************************/

int timer_entry(void)
{
    return -1;
}

/****************************************************************************************/

const struct Resident Timer_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Timer_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_DEVICE,
    50,
    (UBYTE *)name,
    (UBYTE *)&version[6],
    (ULONG *)inittabl
};

static const char name[] = NAME_STRING;
static const char version[] = VERSION_STRING;

static const APTR inittabl[4] =
{
    (APTR)sizeof(struct TimerBase),
    (APTR)functable,
    NULL,
    &AROS_SLIB_ENTRY(init,Timer)
};

static const void * const functable[] =
{
    &AROS_SLIB_ENTRY(open,Timer),
    &AROS_SLIB_ENTRY(close,Timer),
    &AROS_SLIB_ENTRY(null,Timer),
    &AROS_SLIB_ENTRY(null,Timer),
    &AROS_SLIB_ENTRY(BeginIO,Timer),
    &AROS_SLIB_ENTRY(AbortIO,Timer),
    &AROS_SLIB_ENTRY(AddTime,Timer),
    &AROS_SLIB_ENTRY(SubTime,Timer),
    &AROS_SLIB_ENTRY(CmpTime,Timer),

    &AROS_SLIB_ENTRY(null /* ReadEClock */,Timer),
    &AROS_SLIB_ENTRY(GetSysTime,Timer),
    (void *)-1
};

/****************************************************************************************/

AROS_UFH3(struct TimerBase *, AROS_SLIB_ENTRY(init,Timer),
    AROS_UFHA(struct TimerBase *,   TimerBase,	D0),
    AROS_UFHA(BPTR,		    segList,	A0),
    AROS_UFHA(struct ExecBase *,    sysBase,	A6)
)
{
    AROS_USERFUNC_INIT

    /* Store arguments and setup devbase */
    TimerBase->tb_SysBase = sysBase;
    TimerBase->tb_SegList = segList;
    TimerBase->tb_Device.dd_Library.lib_OpenCnt = 0;
    TimerBase->tb_Device.dd_Library.lib_Node.ln_Pri = 0;
    TimerBase->tb_Device.dd_Library.lib_Node.ln_Type = NT_DEVICE;
    TimerBase->tb_Device.dd_Library.lib_Node.ln_Name = (STRPTR)name;
    TimerBase->tb_Device.dd_Library.lib_Version = VERSION_NUMBER;
    TimerBase->tb_Device.dd_Library.lib_Revision = REVISION_NUMBER;
    TimerBase->tb_Device.dd_Library.lib_IdString = (STRPTR)&version[6];

    /* Setup the timer.device data */
    TimerBase->tb_CurrentTime.tv_secs = 0;
    TimerBase->tb_CurrentTime.tv_micro = 0;
    TimerBase->tb_VBlankTime.tv_secs = 0;
    TimerBase->tb_VBlankTime.tv_micro = 1000000 / SysBase->VBlankFrequency;
    TimerBase->tb_Elapsed.tv_secs = 0;
    TimerBase->tb_Elapsed.tv_micro = 0;

    D(kprintf("Timer period: %ld secs, %ld micros\n",
	TimerBase->tb_VBlankTime.tv_secs, TimerBase->tb_VBlankTime.tv_micro));

    TimerBase->tb_MiscFlags = TF_GO;
    
    /* Initialise the lists */
    NEWLIST( &TimerBase->tb_Lists[0] );
    NEWLIST( &TimerBase->tb_Lists[1] );
    NEWLIST( &TimerBase->tb_Lists[2] );
    NEWLIST( &TimerBase->tb_Lists[3] );
    NEWLIST( &TimerBase->tb_Lists[4] );
    
#if 0
    /* Open the boopsi.library */
    TimerBase->tb_BOOPSIBase = OpenLibrary("boopsi.library", 0);
    if( BOOPSIBase == NULL )
    {
	/* timer.device couldn't open intuition.library (think about it!) */
	Alert( AT_DeadEnd | AG_OpenLib | AO_Intuition | AN_TimerDev );
    }
#endif

    /* Start up the interrupt server. This is shared between us and the 
	HIDD that deals with the vblank */
    TimerBase->tb_VBlankInt.is_Node.ln_Pri = 0;
    TimerBase->tb_VBlankInt.is_Node.ln_Type = NT_INTERRUPT;
    TimerBase->tb_VBlankInt.is_Node.ln_Name = (STRPTR)name;
    TimerBase->tb_VBlankInt.is_Code = (APTR)&VBlankInt;
    TimerBase->tb_VBlankInt.is_Data = TimerBase;

    AddIntServer(INTB_VERTB, &TimerBase->tb_VBlankInt);

    return TimerBase;

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/

AROS_LH3(void, open,
    AROS_LHA(struct timerequest *,  tr, A1),
    AROS_LHA(ULONG,		    unitNum, D0),
    AROS_LHA(ULONG,		    flags, D1),
    struct TimerBase *, TimerBase, 1, Timer)
{
    AROS_LIBFUNC_INIT

    /*
        Normally, we should check the length of the message and other
        such things, however the RKM documents an example where the
        length of the timerrequest isn't set, so we must not check
        this.

        This fixes bug SF# 741580
    */

    TimerBase->tb_Device.dd_Library.lib_OpenCnt++;
    TimerBase->tb_Device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
    tr->tr_node.io_Message.mn_Node.ln_Type = NT_REPLYMSG;

    switch(unitNum)
    {
	case UNIT_VBLANK:
	case UNIT_WAITUNTIL:
	    tr->tr_node.io_Error = 0;
	    tr->tr_node.io_Unit = (struct Unit *)unitNum;
	    tr->tr_node.io_Device = (struct Device *)TimerBase;
	    break;

	case UNIT_MICROHZ:
	case UNIT_ECLOCK:
	case UNIT_WAITECLOCK:	
	default:
	    tr->tr_node.io_Error = IOERR_OPENFAIL;
    }

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(BPTR, close,
    AROS_LHA(struct timerequest *, tr, A1),
    struct TimerBase *, TimerBase, 2, Timer)
{
    AROS_LIBFUNC_INIT

    tr->tr_node.io_Device = (struct Device *)-1;
    tr->tr_node.io_Unit = 0;

    if (!--TimerBase->tb_Device.dd_Library.lib_OpenCnt)
    {
	if (TimerBase->tb_Device.dd_Library.lib_Flags & LIBF_DELEXP)
	    return AROS_LC0(BPTR, expunge, struct TimerBase *, TimerBase, 3, Timer);
    }
    return 0;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct TimerBase *, TimerBase, 3, Timer)
{
    AROS_LIBFUNC_INIT

    BPTR ret;

    if (TimerBase->tb_Device.dd_Library.lib_OpenCnt)
    {
	TimerBase->tb_Device.dd_Library.lib_Flags |= LIBF_DELEXP;
        return 0;
    }

    RemIntServer(INTB_VERTB, &TimerBase->tb_VBlankInt);
    Remove(&TimerBase->tb_Device.dd_Library.lib_Node);

    ret = TimerBase->tb_SegList;

    FreeMem((char *)TimerBase-TimerBase->tb_Device.dd_Library.lib_NegSize,
	    TimerBase->tb_Device.dd_Library.lib_NegSize
	    + TimerBase->tb_Device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(int, null, struct TimerBase *, TimerBase, 4, Timer)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

