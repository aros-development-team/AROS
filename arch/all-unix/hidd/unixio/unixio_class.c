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
#include <proto/alib.h>

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

#define DEBUG 0
#include <aros/debug.h>
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
    91, /* Has to be after BOOPSI */
    (UBYTE *)name,
    (UBYTE *)version,
    (APTR)&AROS_SLIB_ENTRY(init,UnixIO)
};

struct newMemList
{
  struct Node	  nml_Node;
  UWORD 	  nml_NumEntries;
  struct MemEntry nml_ME[2];
};

static const struct newMemList MemTemplate =
{
    { 0, },
    2,
    {
	{ { MEMF_CLEAR|MEMF_PUBLIC }, sizeof(struct Task) },
	{ { MEMF_CLEAR		   }, 0 		  }
    }
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
    struct Library		* ud_UtilityBase;
    struct Library		* ud_BOOPSIBase;
    struct ExecBase		* ud_SysBase;

    struct Task 		* ud_WaitForIO;
    struct MsgPort		* ud_Port;
};

struct uioMessage
{
    struct Message Message;
    int 	   fd;
    int 	   mode;
    int 	   result;
};

static void WaitForIO (void)
{
    struct uio_data * ud = FindTask(NULL)->tc_UserData;
    struct ExecBase *SysBase = ud->ud_SysBase;
    int maxfd;
    int selecterr;
    int err;
    fd_set rfds, wfds, efds;
    fd_set * rp, * wp, * ep;
    struct sys_timeval tv;
    struct List waitList;
    struct uioMessage * msg, * nextmsg;

    NEWLIST (&waitList);

    D(bug("wfio: UnixIO.hidd ready ud=%08lx\n", ud));

    for (;;)
    {
	if (IsListEmpty (&waitList))
	{
	    D(bug("wfio: Waiting for messages\n"));
	    WaitPort (ud->ud_Port);
	    D(bug("wfio: Got messages\n"));
	}

fetchmsg:
	while ((msg = (struct uioMessage *)GetMsg (ud->ud_Port)))
	{
	    D(bug("wfio: Got msg fd=%ld mode=%ld\n", msg->fd, msg->mode));
	    AddTail (&waitList, (struct Node *)msg);
	}

	FD_ZERO (&rfds);
	FD_ZERO (&wfds);
	FD_ZERO (&efds);

	rp = wp = ep = NULL;

	maxfd = 0;

D(bug("Waiting on fd "));

	ForeachNode (&waitList, msg)
	{
D(bug("%d, ", msg->fd));
	    if (msg->mode == HIDDV_UnixIO_Read)
	    {
		FD_SET (msg->fd, &rfds);
		rp = &rfds;
	    }
	    else
	    {
		FD_SET (msg->fd, &wfds);
		wp = &wfds;
	    }

	    FD_SET (msg->fd, &efds);
	    ep = &efds;

	    if (maxfd < msg->fd)
		maxfd = msg->fd;
	}
D(bug("\n"));

	for (;;)
	{
	    tv.tv_sec  = 0;
	    tv.tv_usec = 100000;

#if 0
	D(bug("wfio: waiting for io (maxfd=%ld)\n", maxfd));
#endif

	    selecterr = select (maxfd+1, rp, wp, ep, &tv);
	    err = errno;

#if 0
	D(bug("wfio: got io sel=%ld err=%ld\n", selecterr, err));
#endif
	    if (selecterr == 0 || (selecterr < 0 && err == EINTR))
	    {
		if (!IsMsgPortEmpty (ud->ud_Port))
		    goto fetchmsg;

		Disable ();
		SysBase->ThisTask->tc_State = TS_READY;
		AddTail (&SysBase->TaskReady, &SysBase->ThisTask->tc_Node);
		Enable ();
		Switch ();
	    }
	    else
		break;
	}


	D(bug("wfio: got io sel=%ld err=%ld\n", selecterr, err));

	if (selecterr != 0)
	{
	    ForeachNodeSafe (&waitList, msg, nextmsg)
	    {
		if (FD_ISSET (msg->fd, &efds))
		{
		    msg->result = err;
		    goto reply;
		}
		else if (FD_ISSET (msg->fd, &rfds))
		{
		    msg->result = 0;
		    goto reply;
		}
		else if (FD_ISSET (msg->fd, &wfds))
		{
		    msg->result = 0;
reply:
		    D(bug("wfio: Reply: fd=%ld res=%ld\n", msg->fd, msg->result));

		    Remove ((struct Node *)msg);
		    ReplyMsg ((struct Message *)msg);
		}
	    }
	}
	else
	{
	    D(bug("wfio: Timeout sel=%ld err=%ld\n", selecterr, err));
	}
    } /* Forever */
}

#define BOOPSIBase	(((struct uio_data *)cl->cl_UserData)->ud_BOOPSIBase)
#define UtilityBase	(((struct uio_data *)cl->cl_UserData)->ud_UtilityBase)

/* This is the dispatcher for the UnixIO HIDD class. */

AROS_UFH3(static IPTR, dispatch_unixioclass,
    AROS_UFHA(Class *,  cl,     A0),
    AROS_UFHA(Object *, o,      A2),
    AROS_UFHA(Msg,      msg,    A1)
)
{
    IPTR retval = 0UL;
    struct UnixIOData *id = NULL;
    struct uio_data *ud = (struct uio_data *)cl->cl_UserData;

    /* Don't try and get instance data if we don't have an object. */
    if(    msg->MethodID != OM_NEW
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
	{
	    struct uioMessage * umsg = AllocMem (sizeof (struct uioMessage), MEMF_CLEAR|MEMF_PUBLIC);
	    struct MsgPort * port = CreatePort (NULL, 0);

	    if (msg && port)
	    {
		port->mp_Flags = PA_SIGNAL;
		port->mp_SigTask = FindTask (NULL);

		umsg->Message.mn_ReplyPort = port;
		umsg->fd   = ((struct uioMsg *)msg)->um_Filedesc;
		umsg->mode = ((struct uioMsg *)msg)->um_Mode;

		D(bug("Sending msg fd=%ld mode=%ld\n", umsg->fd, umsg->mode));
		PutMsg (ud->ud_Port, (struct Message *)umsg);
		WaitPort (port);

		D(bug("Get msg fd=%ld mode=%ld res=%ld\n", umsg->fd, umsg->mode, umsg->result));
		retval = umsg->result;
	    }
	    else
		retval = ENOMEM;

	    if (umsg)
		FreeMem (umsg, sizeof (struct uioMessage));
	    if (port)
		DeletePort (port);
	}
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
    struct Task     * newtask,
		    * task2;
    struct newMemList nml;
    struct MemList  * ml;

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

    ud->ud_Port = CreatePort (NULL, 0);
    if(ud->ud_Port == NULL)
    {
	/* If you are not running from ROM, don't use Alert() */
	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return NULL;
    }

    nml = MemTemplate;

#define STACKSIZE 8192
    nml.nml_ME[1].me_Length = STACKSIZE;

    ml = AllocEntry ((struct MemList *)&nml);

    if (!((IPTR)ml & (0x80ul<<(sizeof(APTR)-1)*8)) )
    {
	newtask = ml->ml_ME[0].me_Addr;

	newtask->tc_Node.ln_Type = NT_TASK;
	newtask->tc_Node.ln_Pri  = 55;
	newtask->tc_Node.ln_Name = "UnixIO.task";

	newtask->tc_SPReg   = (APTR)((ULONG)ml->ml_ME[1].me_Addr + STACKSIZE);
	newtask->tc_SPLower = ml->ml_ME[1].me_Addr;
	newtask->tc_SPUpper = newtask->tc_SPReg;

	newtask->tc_UserData = ud;

	NEWLIST (&newtask->tc_MemEntry);
	AddHead (&newtask->tc_MemEntry, (struct Node *)ml);

	task2 = (struct Task *)AddTask (newtask, WaitForIO, 0);

	if (SysBase->LibNode.lib_Version>36 && !task2)
	{
	    FreeEntry (ml);
	    newtask = NULL;
	}
    }
    else
	newtask = NULL;

    if (!newtask)
    {
	/* If you are not running from ROM, don't use Alert() */
	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return NULL;
    }

    ud->ud_WaitForIO = task2;

    ud->ud_Port->mp_Flags   = PA_SIGNAL;
    ud->ud_Port->mp_SigTask = task2;

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
