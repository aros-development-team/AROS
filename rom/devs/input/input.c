/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Input device
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1
#include <exec/resident.h>
#include <exec/interrupts.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/input.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <aros/libcall.h>
#ifdef __GNUC__
#    include "input_intern.h"
#endif

#define DEBUG 0
#include <aros/debug.h>

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct inputbase *AROS_SLIB_ENTRY(init,Input)();
void AROS_SLIB_ENTRY(open,Input)();
BPTR AROS_SLIB_ENTRY(close,Input)();
BPTR AROS_SLIB_ENTRY(expunge,Input)();
int AROS_SLIB_ENTRY(null,Input)();
void AROS_SLIB_ENTRY(beginio,Input)();
LONG AROS_SLIB_ENTRY(abortio,Input)();

extern UWORD AROS_SLIB_ENTRY(PeekQualifier,Input) ();
static const char end;

int AROS_SLIB_ENTRY(entry,Input)(void)
{
    /* If the device was executed by accident return error code. */
    return -1;
}

const struct Resident Input_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Input_resident,
    (APTR)&end,
    RTF_AUTOINIT|RTF_COLDSTART,
    41,
    NT_DEVICE,
    30,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="input.device";

static const char version[]="$VER: input 41.0 (3.4.1997)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct inputbase),
    (APTR)functable,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Input)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,Input),
    &AROS_SLIB_ENTRY(close,Input),
    &AROS_SLIB_ENTRY(expunge,Input),
    &AROS_SLIB_ENTRY(null,Input),
    &AROS_SLIB_ENTRY(beginio,Input),
    &AROS_SLIB_ENTRY(abortio,Input),
    &AROS_SLIB_ENTRY(PeekQualifier,Input),
    (void *)-1
};


AROS_LH2(struct inputbase *, init,
 AROS_LHA(struct inputbase *, InputDevice, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, Input)
{
    AROS_LIBFUNC_INIT


    /* Store arguments */
    InputDevice->sysBase = sysBase;
    InputDevice->seglist = segList;
    
    NEWLIST( &(InputDevice->HandlerList) );
    
    InputDevice->KeyRepeatThreshold.tv_secs  = DEFAULT_KEY_REPEAT_THRESHOLD / 50;
    InputDevice->KeyRepeatThreshold.tv_micro = (DEFAULT_KEY_REPEAT_THRESHOLD % 50) * 1000000L / 50;
    InputDevice->KeyRepeatInterval.tv_secs   = DEFAULT_KEY_REPEAT_INTERVAL / 50;
    InputDevice->KeyRepeatInterval.tv_micro  = (DEFAULT_KEY_REPEAT_INTERVAL % 50) * 1000000L / 50;
    
    InputDevice->device.dd_Library.lib_OpenCnt=1;

    return (InputDevice);
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IORequest *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct inputbase *, InputDevice, 1, Input)
{
    AROS_LIBFUNC_INIT

    struct Task *idleT;

    D(bug("id: open()\n"));


    /* Keep compiler happy */
    unitnum=0;
    flags=0;
    
    ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    
    if (!InputDevice->CommandPort)
    {
    	D(bug("id_open: Fist time opened\n"));
    	InputDevice->CommandPort = AllocMem(sizeof (struct MsgPort), MEMF_PUBLIC);
    	if (InputDevice->CommandPort)
    	{
    	    /* Since the input device task will use this before this function
    	    ** has exited, we can put it on the stack
    	    */
    	    struct IDTaskParams idtask_params;
    	    idtask_params.InputDevice = InputDevice;
    	    idtask_params.Caller = FindTask(NULL);
    	    
    	    /* We don't use a OS signal (like SIGBREAKF_CTRL_D), because
    	    ** we might receive it from elsewere.
    	    */
    	    idtask_params.Signal = 1 << ioreq->io_Message.mn_ReplyPort->mp_SigBit;
    	
    	    D(bug("id_open: Creating input task\n"));
    	    
    	    InputDevice->InputTask = CreateInputTask(&idtask_params, InputDevice);
    	    
    	    D(bug("id_open: input task created: %p\n", InputDevice->InputTask));
    	
    	    if (InputDevice->InputTask)
    	    {
    	    	/* Here we wait for the input.device to initialize it's
    	    	** command msgport etc. (see processevents.c). This
    	    	** is to prevent race conditions.
    	    	** Say that we exited succesfully now and did
    	    	** an asynchronous IO request to the device, while
    	    	** the input.device yet not had created it's command port.
    	    	** It would most certainly crash the machine
    	    	*/
    	    	D(bug("id_open(): Waiting for idtask to initialize itself\n"));
    	    	Wait (idtask_params.Signal);
    	    	D(bug("id_open(): Got signal from idtask\n"));
    	    	
    	    	ioreq->io_Error = NULL;

   	 	/* I have one more opener. */
    		InputDevice->device.dd_Library.lib_Flags &= ~LIBF_DELEXP;
    		InputDevice->device.dd_Library.lib_OpenCnt ++;

    
		/* !!! May be obsolete !!!
		** It was there in the old code that I mode to the input.device,
		** so I include it for now.
		*/    	
		idleT = FindTask("Idle Task");
    		if( idleT )
	       	    Signal(idleT, SIGBREAKF_CTRL_F);
    	    	
    	    	return;
    	    	
    	    } /* if (input task created) */
    	    FreeMem(InputDevice->CommandPort, sizeof (struct MsgPort));
    	} /* if (command msgport created) */

    	ioreq->io_Error = IOERR_OPENFAIL;
    	    
    } /* if (first time opened) */

    if (!ioreq->io_Error != IOERR_OPENFAIL)
    {
    	InputDevice->device.dd_Library.lib_OpenCnt ++;
    	InputDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

    }
    
    return;    

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct inputbase *, InputDevice, 2, Input)
{
    AROS_LIBFUNC_INIT

    /* Let any following attemps to use the device crash hard. */
    ioreq->io_Device=(struct Device *)-1;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct inputbase *, InputDevice, 3, Input)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    InputDevice->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct inputbase *, InputDevice, 4, Input)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

#define ioStd(x)  ((struct IOStdReq *)x)
AROS_LH1(void, beginio,
 AROS_LHA(struct IOStdReq *, ioreq, A1),
	   struct inputbase *, InputDevice, 5, Input)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    
    BOOL done_quick = TRUE;
    
    D(bug("id: beginio(ioreq=%p)\n", ioreq));

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;

    switch (ioreq->io_Command)
    {
    case IND_ADDHANDLER:
    case IND_REMHANDLER:
    case IND_WRITEEVENT:
    case IND_SETTHRESH:
    case IND_SETPERIOD:
        done_quick = FALSE;
    	break;
    
    default:
	error = IOERR_NOCMD;
	break;
    }
    
    if (!done_quick)
    {
        /* Mark IO request to be done non-quick */
    	ioreq->io_Flags &= ~IOF_QUICK;
    	/* Send to input device task */
    	PutMsg(InputDevice->CommandPort, (struct Message *)ioreq);
    }
    else
    {

    	/* If the quick bit is not set but the IO request was done quick,
    	** reply the message to tell we're throgh
    	*/
    	ioreq->io_Error = error;
   	if (!(ioreq->io_Flags & IOF_QUICK))
	    ReplyMsg (&ioreq->io_Message);
    }

    D(bug("id: Return from BeginIO()\n"));

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct inputbase *, InputDevice, 6, Input)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;
