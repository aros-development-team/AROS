/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Unix filedescriptor/socket IO
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <intuition/classusr.h>
#include <intuition/classes.h>
#include <hidd/unixio.h>

#include <proto/exec.h>
#include <proto/intuition.h>	/* for DoSuperMethodA() */
#include <proto/boopsi.h>
#include <proto/utility.h>

/* Unix includes */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define timeval sys_timeval
#include <sys/stat.h>
#include <sys/time.h>
#undef timeval

#ifdef _AROS
#include <aros/asmcall.h>
#endif /* _AROS */

static const UBYTE name[];
static const UBYTE version[];
static ULONG AROS_SLIB_ENTRY(init,UnixIO)();
extern const char UnixIO_End;

int unixio_entry(void)
{
    return -1;
}

const struct Resident UnixIO_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&UnixIO_resident,
    (APTR)&UnixIO_End,
    RTF_COLDSTART,
    41,
    NT_UNKNOWN,
    91,	/* Has to be after BOOPSI */
    (UBYTE *)name,
    (UBYTE *)version,
    (APTR)&AROS_SLIB_ENTRY(init,UnixIO)
};

static const UBYTE name[] = "unixioclass";
static const UBYTE version[] = "unixioclass 41.1 (27.10.1997)";

static const char unknown[] = "--unknown device--";

/************************************************************************/

struct UnixIOData
{
    void *uio_Dummy;
};

/* Static Data for the unixioclass. */
struct uio_data
{
    struct Library 		*ud_UtilityBase;
    struct Library 		*ud_BOOPSIBase;
    struct ExecBase 		*ud_SysBase;
    struct SignalSemaphore	 ud_Lock;
};

static int wait_for_io (struct uioMsg *um, struct uio_data *ud)
{
    struct ExecBase *SysBase = ud->ud_SysBase;
    int fd = um->um_Filedesc,
        mo = um->um_Mode;
    int selecterr;

    Disable();

    do
    {
        fd_set fds;
	struct sys_timeval tv;

	FD_ZERO(&fds);
	FD_SET(fd,&fds);
	tv.tv_sec=0;
	tv.tv_usec=100000;

        /* Should be waiting on all filedescriptors at once */

        if (mo == HIDDV_UnixIO_Read) {
	    if((selecterr = select(fd+1,&fds,NULL,NULL,&tv)))
	        break;
	}
        else if (mo == HIDDV_UnixIO_Write) {
	    if((selecterr = select(fd+1,NULL,&fds,NULL,&tv)))
	        break;
	}
	else
	  break;

	/* Since we are a friendly task, we give control back to
	   exec from time to time. */
        SysBase->ThisTask->tc_State=TS_READY;
	AddTail(&SysBase->TaskReady, &SysBase->ThisTask->tc_Node);
	Switch();
    }
    while (selecterr == 0 || errno == 4); /* EINTR */

    Enable();

    if (selecterr == -1)
      return errno;
    return 0;
}

#define BOOPSIBase	(((struct uio_data *)cl->cl_UserData)->ud_BOOPSIBase)
#define UtilityBase	(((struct uio_data *)cl->cl_UserData)->ud_UtilityBase)

/* This is the dispatcher for the UnixIO HIDD class. */

AROS_UFH3(static IPTR, dispatch_unixioclass,
    AROS_UFHA(Class *, 	cl, 	A0),
    AROS_UFHA(Object *, o,  	A2),
    AROS_UFHA(Msg,	msg,	A1)
)
{
    IPTR retval = 0UL;
    struct UnixIOData *id = NULL;
    struct uio_data *ud = (struct uio_data *)cl->cl_UserData;

    /* Don't try and get instance data if we don't have an object. */
    if(	   msg->MethodID != OM_NEW
	&& msg->MethodID != HIDDM_Class_Get 
	&& msg->MethodID != HIDDM_Class_MGet
      )
	id = INST_DATA(cl, o);

    /* We now dispatch the actual methods */
    switch(msg->MethodID)
    {
    case OM_NEW:
	retval = DoSuperMethodA(cl, o, msg);
	if(!retval)
	    break;

	id = INST_DATA(cl, retval);	
	if(id != NULL)
	{
	}
	break;

    case HIDDM_WaitForIO:
        retval = wait_for_io ((struct uioMsg *) msg, ud);
	break;

    case OM_DISPOSE:
	/* We don't actually have anything to free - fall through. */
    
    default:
	/* No idea, send it to the superclass */
	retval = DoSuperMethodA(cl, o, msg);
    } /* switch(msg->MethodID) */

    return retval;
}

/* This is the initialisation code for the HIDD class itself. */
#undef BOOPSIBase
#undef UtilityBase

AROS_UFH3(static ULONG, AROS_SLIB_ENTRY(init, UnixIO),
    AROS_UFHA(ULONG, dummy1, D0),
    AROS_UFHA(ULONG, dummy2, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    struct Library *BOOPSIBase;
    struct IClass *cl;
    struct uio_data *ud;

    /*
	We map the memory into the shared memory space, because it is
	to be accessed by many processes, eg searching for a HIDD etc.

	Well, maybe once we've got MP this might help...:-)
    */
    ud = AllocMem(sizeof(struct uio_data), MEMF_CLEAR|MEMF_PUBLIC);
    if(ud == NULL)
    {
	/* If you are not running from ROM, don't use Alert() */
        Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return NULL;
    }

    ud->ud_SysBase = SysBase;

    BOOPSIBase = ud->ud_BOOPSIBase = OpenLibrary("boopsi.library", 0);
    if(ud->ud_BOOPSIBase == NULL)
    {
	FreeMem(ud, sizeof(struct uio_data));
	Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_Unknown);
	return NULL;
    }

    ud->ud_UtilityBase = OpenLibrary("utility.library",0);
    if(ud->ud_UtilityBase == NULL)
    {
	CloseLibrary(ud->ud_UtilityBase);
    	FreeMem(ud, sizeof(struct uio_data));
	Alert(AT_DeadEnd | AG_OpenLib | AN_Unknown | AO_UtilityLib);
	return NULL;
    }

    /* Create the class structure for the "unixioclass" */
    if((cl = MakeClass(UNIXIOCLASS, HIDDCLASS, NULL, sizeof(struct UnixIOData), 0)))
    {
	cl->cl_Dispatcher.h_Entry = (APTR)dispatch_unixioclass;
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_Dispatcher.h_Data = cl;
	cl->cl_UserData = (IPTR)ud;

	AddClass(cl);
    }
    return NULL;
}
