/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Console.device
    Lang: english
*/
#include <exec/resident.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include <proto/console.h>
#include <aros/libcall.h>
#ifdef __GNUC__
#    include "console_gcc.h"
#endif

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct consolebase *AROS_SLIB_ENTRY(init,Console)();
void AROS_SLIB_ENTRY(open,Console)();
BPTR AROS_SLIB_ENTRY(close,Console)();
BPTR AROS_SLIB_ENTRY(expunge,Console)();
int AROS_SLIB_ENTRY(null,Console)();
void AROS_SLIB_ENTRY(beginio,Console)();
LONG AROS_SLIB_ENTRY(abortio,Console)();

extern struct InputEvent * AROS_SLIB_ENTRY(CDInputHandler,Console) ();
extern LONG AROS_SLIB_ENTRY(RawKeyConvert,Console) ();
static const char end;

int AROS_SLIB_ENTRY(entry,Console)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident Console_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Console_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    5,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="console.device";

static const char version[]="$VER: console 41.0 (17.11.97)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct consolebase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Console)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,Console),
    &AROS_SLIB_ENTRY(close,Console),
    &AROS_SLIB_ENTRY(expunge,Console),
    &AROS_SLIB_ENTRY(null,Console),
    &AROS_SLIB_ENTRY(beginio,Console),
    &AROS_SLIB_ENTRY(abortio,Console),
    &AROS_SLIB_ENTRY(CDInputHandler,Console),
    &AROS_SLIB_ENTRY(RawKeyConvert,Console),
    (void *)-1
};

AROS_LH2(struct consolebase *, init,
 AROS_LHA(struct consolebase *, consoleDevice, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, Console)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    consoleDevice->sysBase = sysBase;
    consoleDevice->seglist = segList;

    consoleDevice->device.dd_Library.lib_OpenCnt=1;

    return consoleDevice;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct consolebase *, ConsoleDevice, 1, Console)
{
    AROS_LIBFUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    /* I have one more opener. */
    ConsoleDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct consolebase *, ConsoleDevice, 2, Console)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device=(struct Device *)-1;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct consolebase *, ConsoleDevice, 3, Console)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    ConsoleDevice->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct consolebase *, ConsoleDevice, 4, Console)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct consolebase *, ConsoleDevice, 5, Console)
{
    AROS_LIBFUNC_INIT
    LONG error=0;

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch (ioreq->io_Command)
    {
    default:
	error=ERROR_NOT_IMPLEMENTED;
	break;
    }

    /* If the quick bit is not set send the message to the port */
    if(!(ioreq->io_Flags&IOF_QUICK))
	ReplyMsg (&ioreq->io_Message);

    /* Trigger a rescedule every now and then */
    if(SysBase->TaskReady.lh_Head->ln_Pri==SysBase->ThisTask->tc_Node.ln_Pri&&
       SysBase->TDNestCnt<0&&SysBase->IDNestCnt<0)
    {
	SysBase->ThisTask->tc_State=TS_READY;
	Enqueue(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);
	Switch();
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct consolebase *, ConsoleDevice, 6, Console)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;
