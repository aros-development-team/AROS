/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/23 17:32:23  digulla
    Implementation of the console.device


    Desc:
    Lang:
*/
#include <exec/resident.h>
#include <devices/inputevent.h>
#include <clib/exec_protos.h>
#include <clib/console_protos.h>
#include <aros/libcall.h>
#ifdef __GNUC__
#    include "console_gcc.h"
#endif

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;
struct consolebase *Console_init();
void Console_open();
BPTR Console_close();
BPTR Console_expunge();
int Console_null();
void Console_beginio();
LONG Console_abortio();
extern struct InputEvent * Console_CDInputHandler ();
extern LONG Console_RawKeyConvert ();
static const char end;

int Console_entry(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident Console_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Console_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="console.device";

static const char version[]="$VER: console 1.0 (23.8.96)\n\015";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct consolebase),
    (APTR)functable,
    (APTR)&datatable,
    &Console_init
};

static void *const functable[]=
{
    &Console_open,
    &Console_close,
    &Console_expunge,
    &Console_null,
    &Console_beginio,
    &Console_abortio,
    &Console_CDInputHandler,
    &Console_RawKeyConvert,
    (void *)-1
};

__AROS_LH2(struct consolebase *, init,
 __AROS_LHA(struct consolebase *, consoleDevice, D0),
 __AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, Console)
{
    __AROS_FUNC_INIT

    /* Store arguments */
    consoleDevice->sysBase = sysBase;
    consoleDevice->seglist = segList;

    consoleDevice->device.dd_Library.lib_OpenCnt=1;

    return consoleDevice;
    __AROS_FUNC_EXIT
}

__AROS_LH3(void, open,
 __AROS_LHA(struct IORequest *, ioreq, A1),
 __AROS_LHA(ULONG,              unitnum, D0),
 __AROS_LHA(ULONG,              flags, D0),
	   struct consolebase *, ConsoleDevice, 1, Console)
{
    __AROS_FUNC_INIT

    /* Keep compiler happy */
    unitnum=0;
    flags=0;

    /* I have one more opener. */
    ConsoleDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    __AROS_FUNC_EXIT
}

__AROS_LH1(BPTR, close,
 __AROS_LHA(struct IORequest *, ioreq, A1),
	   struct consolebase *, ConsoleDevice, 2, Console)
{
    __AROS_FUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device=(struct Device *)-1;
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge, struct consolebase *, ConsoleDevice, 3, Console)
{
    __AROS_FUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    ConsoleDevice->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null, struct consolebase *, ConsoleDevice, 4, Console)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH1(void, beginio,
 __AROS_LHA(struct IORequest *, ioreq, A1),
	   struct consolebase *, ConsoleDevice, 5, Console)
{
    __AROS_FUNC_INIT
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

    __AROS_FUNC_EXIT
}

__AROS_LH1(LONG, abortio,
 __AROS_LHA(struct IORequest *, ioreq, A1),
	   struct consolebase *, ConsoleDevice, 6, Console)
{
    __AROS_FUNC_INIT
    /* Everything already done. */
    return 0;
    __AROS_FUNC_EXIT
}

static const char end=0;
