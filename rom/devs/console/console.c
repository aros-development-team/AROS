/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Console.device
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/console.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <devices/inputevent.h>
#include <devices/conunit.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <intuition/classusr.h>
#include <graphics/rastport.h>
#include <aros/libcall.h>

#include <graphics/rastport.h>

#ifdef __GNUC__
#    include "console_gcc.h"
#endif

#include "consoleif.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>


static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
static const UBYTE datatable;

struct ConsoleBase *AROS_SLIB_ENTRY(init,Console)();
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

static const char version[]="$VER: console 41.0 (17.11.1997)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct ConsoleBase),
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

/* init */

AROS_LH2(struct ConsoleBase *, init,
 AROS_LHA(struct ConsoleBase *, ConsoleDevice, D0),
 AROS_LHA(BPTR,              segList,   A0),
	   struct ExecBase *, sysBase, 0, Console)
{
    AROS_LIBFUNC_INIT
    

    /* Store arguments */
    ConsoleDevice->sysBase = sysBase;
    ConsoleDevice->seglist = segList;
    
    NEWLIST(&ConsoleDevice->unitList);
    InitSemaphore(&ConsoleDevice->unitListLock);
    
    ConsoleDevice->device.dd_Library.lib_OpenCnt=1;

    return ConsoleDevice;
    AROS_LIBFUNC_EXIT
}

/* open */

AROS_LH3(void, open,
 AROS_LHA(struct IOStdReq *, ioreq, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct ConsoleBase *, ConsoleDevice, 1, Console)
{
    AROS_LIBFUNC_INIT
    
    BOOL success = FALSE;
    
    
    /* Keep compiler happy */
    flags=0;
    
    EnterFunc(bug("OpenConsole()\n"));
    
    if (!ConsoleDevice->gfxBase)
    {
    	ConsoleDevice->gfxBase = (GraphicsBase *)OpenLibrary("graphics.library", 37);
    	if (!ConsoleDevice->gfxBase)
	    goto open_fail;
    }
    
    if (!ConsoleDevice->intuitionBase)
    {
    	ConsoleDevice->intuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    	if (!ConsoleDevice->intuitionBase)
	    goto open_fail;
    }
    
    if (!ConsoleDevice->boopsiBase)
    {
    	ConsoleDevice->boopsiBase = OpenLibrary(BOOPSINAME, 37);
    	if (!ConsoleDevice->boopsiBase)
	    goto open_fail;
    }

    if (!ConsoleDevice->utilityBase)
    {
    	ConsoleDevice->utilityBase = OpenLibrary("utility.library", 37);
    	if (!ConsoleDevice->utilityBase)
	    goto open_fail;
    }
    
    /* Create the console classes */
    if (!CONSOLECLASSPTR)
    {
    	CONSOLECLASSPTR = makeConsoleClass(ConsoleDevice);
    	if (!CONSOLECLASSPTR)
	    goto open_fail;
    }	    
    
    if (!STDCONCLASSPTR)
    {
    	STDCONCLASSPTR = makeStdConClass(ConsoleDevice);
    	if (!STDCONCLASSPTR)
	    goto open_fail;
    }
    
    /* Create the console device task */
    if (!ConsoleDevice->consoleTask)
    {
    	struct coTaskParams ctp;
	BYTE sigbit;
	
	/* Initialize task parameters */
	ctp.consoleDevice = ConsoleDevice;
	ctp.parentTask = FindTask(NULL);
	
	sigbit = AllocSignal(-1L);
	if (sigbit != -1)
	{
	    ctp.initSignal = 1 << sigbit;
	    
	    
    	    ConsoleDevice->consoleTask = createConsoleTask(&ctp, ConsoleDevice);
	    if (ConsoleDevice->consoleTask)
	    {
	    	/* Wait for the console.device task to initialize itself
		*/
		
		Wait (1 << sigbit);
		
		/* OK. we can now go on */
		
	    }
	    /* Free the allocated signal, we don't need it anymore */
	    FreeSignal(sigbit);
	    
	} /* if (signal allocated) */

    } /* if (console.device task hasn't been initialized earlier) */
    
    
    if (((LONG)unitnum) == CONU_LIBRARY) /* unitnum is ULONG while CONU_LIBRARY is -1 :-(   */
    {
    	D(bug("Opening CONU_LIBRARY unit\n"));
    	ioreq->io_Device = (struct Device *)ConsoleDevice;
	
	/* Set io_Unit to NULL, so that CloseDevice knows this is a CONU_LIBRARY unit */
	ioreq->io_Unit = NULL;
	success = TRUE;
    }
    else
    {
	Class *classptr = NULL; /* Keep compiler happy */
	
	struct TagItem conunit_tags[] =
	{
	    {A_Console_Window,	NULL},
	    {TAG_DONE, 0}
	};
	
	/* Init tags */
	
	conunit_tags[0].ti_Data = (IPTR)ioreq->io_Data; /* Window */
	
    	
	/* Select class of which to create console object */
    	switch (unitnum)
    	{
    	    case CONU_STANDARD:
	    	D(bug("Opening CONU_STANDARD console\n"));
    	    	classptr = STDCONCLASSPTR;
    	    	break;
    		
    	    case CONU_CHARMAP:
    		classptr = CHARMAPCLASSPTR;
    		break;
    		
    	    case CONU_SNIPMAP:
    	    	classptr = SNIPMAPCLASSPTR;
    	    	break;
		
	    default:
	    	goto open_fail;


	}
    	/* Create console object */
    	ioreq->io_Unit = (struct Unit *)NewObjectA(classptr, NULL, conunit_tags);
    	if (ioreq->io_Unit)
    	{
	    struct opAddTail add_msg;
    	    success = TRUE;
	    
	    /* Add the newly created unit to console's list of units */
	    ObtainSemaphore(&ConsoleDevice->unitListLock);

	    add_msg.MethodID = OM_ADDTAIL;
	    add_msg.opat_List = (struct List *)&ConsoleDevice->unitList;
	    DoMethodA((Object *)ioreq->io_Unit, (Msg)&add_msg);
	    
	    ReleaseSemaphore(&ConsoleDevice->unitListLock);
    	} /* if (console unit created) */
    	
    } /* if (not CONU_LIBRARY) */

    if (!success)
    	goto open_fail;

    /* I have one more opener. */
    ConsoleDevice->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
    
    
    ReturnVoid("OpenConsole");
    
open_fail:

    ioreq->io_Error = IOERR_OPENFAIL;    
    ReturnVoid("OpenConsole failed");

    AROS_LIBFUNC_EXIT
}

/* close */

AROS_LH1(BPTR, close,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct ConsoleBase *, ConsoleDevice, 2, Console)
{
    AROS_LIBFUNC_INIT
    
    if (ioreq->io_Unit)
    {
    	DisposeObject((Object *)ioreq->io_Unit);
    }
    
    return 0;
    AROS_LIBFUNC_EXIT
}

/* expunge */

AROS_LH0(BPTR, expunge, struct ConsoleBase *, ConsoleDevice, 3, Console)
{
    AROS_LIBFUNC_INIT

    /* Do not expunge the device. Set the delayed expunge flag and return. */
    ConsoleDevice->device.dd_Library.lib_Flags|=LIBF_DELEXP;
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct ConsoleBase *, ConsoleDevice, 4, Console)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOStdReq *, ioreq, A1),
	   struct ConsoleBase *, ConsoleDevice, 5, Console)
{
    AROS_LIBFUNC_INIT
    LONG error=0;

    
    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;
    
    EnterFunc(bug("BeginIO(ioreq=%p)\n", ioreq));

    /*
	Do everything quick no matter what. This is possible
	because I never need to Wait().
    */
    switch (ioreq->io_Command)
    {
    	case CMD_WRITE:
    	    D(bug("CMD_WRITE\n"));

    	    ioreq->io_Actual = writeToConsole(ioreq, ConsoleDevice);
    	    break;
    	    
    default:
	error = ERROR_NOT_IMPLEMENTED;
	break;
    }

    /* If the quick bit is not set send the message to the port */
    if(!(ioreq->io_Flags & IOF_QUICK))
	ReplyMsg (&ioreq->io_Message);

    /* Trigger a rescedule every now and then */
/*    if(SysBase->TaskReady.lh_Head->ln_Pri==SysBase->ThisTask->tc_Node.ln_Pri&&
       SysBase->TDNestCnt<0&&SysBase->IDNestCnt<0)
    {
	SysBase->ThisTask->tc_State=TS_READY;
	Enqueue(&SysBase->TaskReady,&SysBase->ThisTask->tc_Node);
	Switch();
    }
*/
    ReturnVoid("BeginIO");
    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IORequest *, ioreq, A1),
	   struct ConsoleBase *, ConsoleDevice, 6, Console)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

static const char end=0;
