/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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

static const char version[]="$VER: input 41.0 (3.4.97)\r\n";

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

/* This must be global so that the input.device task can now it */
struct inputbase *IBase;

AROS_LH2(struct inputbase *, init,
 AROS_LHA(struct inputbase *, InputDevice, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, Input)
{
    AROS_LIBFUNC_INIT


    /* Store arguments */
    InputDevice->sysBase = sysBase;
    InputDevice->seglist = segList;
    
    InitSemaphore( &(InputDevice->HandlerSema) );
    NEWLIST( &(InputDevice->HandlerList) );
    
    /* Put Input Device library base into global variable, 
    ** so that the input.device task can see it
    */
    IBase = InputDevice;

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
    
    if (!InputDevice->InputTask)
    {
    	D(bug("id_open: Creating input task\n"));
    	InputDevice->InputTask = CreateInputTask(IDTASK_STACKSIZE, InputDevice);
    	D(bug("id_open: input task created: %p\n", InputDevice->InputTask));
    	
    	if (!InputDevice->InputTask)
    	{
    	    ioreq->io_Error = IOERR_OPENFAIL;
    	}
    
	/* !!! May be obsolete !!!
	** It was there in the old code that I mode to the input.device,
	** so I include it for now.
	*/    	
	idleT = FindTask("Idle Task");
    	if( idleT )
	   Signal(idleT, SIGBREAKF_CTRL_F);
    }
    
    D(bug("id: open(): everything went OK\n"));


    /* I have one more opener. */
    InputDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;

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
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct inputbase *, InputDevice, 5, Input)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    
    D(bug("id: beginio(ioreq=%p)\n", ioreq));


    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;

    /*
	Do everything quick no matter what, still if I *DO*
	may have to Wait() in IND_ADDHANDLER an IND_REMHANDLER,
	since the inputhandler list is semaphore protected.

	In the case that these commands were called asynchrounously,
	we will still force them to be performed synchronous.
	
	This of course has a reason: The intui_WaitEvent()
	function (config/x11/intuition_driver.c),
	can not wait for x11 events and events at the
	input device's port at the same time.
	
	But hopefully a fix for this can to poll the
	input process' msg port each time before entering
	or when returned from intui_WaitEvent()
	
	
    */
    switch (ioreq->io_Command)
    {
    case IND_ADDHANDLER: {
    
    	struct Interrupt *ir;
    
    	D(bug("id: IND_ADDHANDLER\n"));

    	D(bug("id: Obtaining semaphore\n"));
    	ObtainSemaphore( &(InputDevice->HandlerSema) );

    	D(bug("id: Enqueue()ing handler %p of pri %d\n",
    		ioStd(ioreq)->io_Data,
    		((struct Node *)ioStd(ioreq)->io_Data)->ln_Pri));
    		
    	Enqueue( (struct List *)&(InputDevice->HandlerList),
    			(struct Node *)ioStd(ioreq)->io_Data);

    	D(bug("id: Releasing semaphore\n"));

    	ReleaseSemaphore(&InputDevice->HandlerSema);
    	
    			
	ForeachNode( &(InputDevice->HandlerList), ir)
	{
	    D(bug("id: List conatins inputhandler %s\n",
	    	ir->is_Node.ln_Name));
	}
    	
    	D(bug("id: All done !\n"));
    	
    	} break;
    
    case IND_REMHANDLER:
    	D(bug("id: IND_REMHANDLER\n"));

    	ObtainSemaphore( &(InputDevice->HandlerSema) );
    	Remove( (struct Node*)ioStd(ioreq)->io_Data);
    	ReleaseSemaphore( &(InputDevice->HandlerSema) );

    	D(bug("id: All done !\n"));

    	break;
    	
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
