/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Unix filedescriptor/socket IO
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <exec/ports.h>
#include <exec/nodes.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <hidd/unixio.h>
#include <aros/asmcall.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>

#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <devices/timer.h>

/* Unix includes */
#define timeval sys_timeval /* We don't want the unix timeval to interfere with the AROS one */
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#undef timeval

#ifdef __AROS__
#include <aros/asmcall.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>
#endif /* __AROS__ */

static const UBYTE name[];
static const UBYTE version[];

static void * AROS_SLIB_ENTRY(init,UnixIO)();


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
    91, /* Has to be after OOP */
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
static const UBYTE version[] = "unixioclass 41.1 (27.10.1997)\r\n";

/************************************************************************/

/* instance data for the unixioclass */
struct UnixIOData
{
    struct MsgPort		* uio_ReplyPort;
};

/* static data for the unixioclass */
struct uio_data
{
    struct Library		* ud_UtilityBase;
    struct Library		* ud_OOPBase;
    struct ExecBase		* ud_SysBase;

    struct Task 		* ud_WaitForIO;
    struct MsgPort		* ud_Port;
};


AROS_UFH5 (void, SigIO_IntServer,
    AROS_UFHA (ULONG             ,dummy,  D0),
    AROS_UFHA (struct Custom    *,custom, A0),
    AROS_UFHA (struct List      *,intList,A1),
    AROS_UFHA (APTR              ,ivCode, A5),
    AROS_UFHA (struct ExecBase  *,SysBase,A6)
)
{
    AROS_USERFUNC_INIT

    struct uio_data * ud = (struct uio_data *) intList;
    Signal (ud -> ud_WaitForIO, SIGBREAKF_CTRL_C);

    AROS_USERFUNC_EXIT
}


#ifdef __linux__
static int unixio_start_timer(struct timerequest * timerio)
{
    int rc = FALSE;
    if (NULL != timerio) {
        timerio->tr_node.io_Command = TR_ADDREQUEST;
        timerio->tr_time.tv_secs    = 0;
        timerio->tr_time.tv_micro   = 250000;
        
        SendIO(&timerio->tr_node);
        rc = TRUE;
    }
    return rc;
}
#endif

/******************
**  UnixIO task  **
******************/
#ifdef SysBase
stop
#undef SysBase
#endif
static void WaitForIO (void)
{
    struct uio_data * ud = FindTask(NULL)->tc_UserData;
    struct ExecBase * SysBase = ud->ud_SysBase;
    int maxfd;
    int selecterr;
    int err;
    fd_set rfds, wfds, efds;
    fd_set * rp, * wp, * ep;
    struct sys_timeval tv;
    struct List waitList;
    struct uioMessage * msg, * nextmsg;
    ULONG rmask;
    int flags;
    pid_t my_pid = getpid();
#ifdef __linux__
    int terminals_write_counter = 0;
    struct MsgPort * timer_port = NULL;
    struct timerequest * timerio = NULL;
#endif
    /*
     * Since the signal was allocated by other task, but really is mine
     * I need to allocate it here 'manually'. Otherwise the CreateMsgPort()
     * would get the wrong signal.
     */
    AllocSignal(ud->ud_Port -> mp_SigBit);

#ifdef __linux__
    timer_port = CreateMsgPort();
    timerio = CreateIORequest(timer_port, sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_VBLANK, &timerio->tr_node, 0);
#endif

    NEWLIST (&waitList);

    D(bug("wfio: UnixIO.hidd ready ud=%08lx\n", ud));

    for (;;)
    {
        if (IsListEmpty (&waitList))
	{
	    D(bug("wfio: Waiting for message for task %s\n",ud->ud_WaitForIO->tc_Node.ln_Name));
	    WaitPort (ud->ud_Port);
	    D(bug("wfio: Got messages\n"));
	}
        else
	{
#if 0	
	kprintf("FLAGS FOR FDS: ");
	ForeachNode (&waitList, msg) {
	    int flags;
	   
	    flags = fcntl (msg->fd, F_GETFL);
	    kprintf("fd %d: %d", msg->fd, flags);
	    if (flags & FASYNC)
	    	kprintf(" ASYNC SET, ");
	    else
	    	kprintf(" ASYNC NOT SET, ");
	    }
	    
	    kprintf("\n");
#endif		    
		    
	    D(bug("wfio: Waiting for message or signal for task %s\n",ud->ud_WaitForIO->tc_Node.ln_Name));
	    rmask = Wait ((ULONG) (1 << ud->ud_Port -> mp_SigBit) | 
	                          (1 << timer_port  -> mp_SigBit) |
	                          SIGBREAKF_CTRL_C);
#ifdef __linux__
	    if (rmask & 1 << timer_port -> mp_SigBit)
	    {
	        /*
	         * Must take the message from the timer port.
	         * Will be sending it again.
	         */
	        GetMsg(timer_port);
	        if (terminals_write_counter > 0) {
	          //kprintf("RE STARTING TIMER! (%d)\n",terminals_write_counter);
	          unixio_start_timer(timerio);
	        } else {
	          //kprintf("NOT RE STARTING TIMER!\n");
	        }
	    }
	    else
#endif
	    if (rmask & 1 << ud->ud_Port -> mp_SigBit)
	    {
	        D(bug("wfio: Got message\n"));
	    }
	    else if (rmask & SIGBREAKF_CTRL_C)
	    {
	        D(bug("wfio: Got signal\n"));
	    }
	}

	while ((msg = (struct uioMessage *)GetMsg (ud->ud_Port)))
	{
	    if (msg->mode != vHidd_UnixIO_Abort)
	    {

	      D(bug("wfio: Got msg fd=%ld mode=%ld\n", msg->fd, msg->mode));
	      AddTail (&waitList, (struct Node *)msg);

	      fcntl (msg->fd, F_SETOWN, my_pid);
	      flags = fcntl (msg->fd, F_GETFL);
	      fcntl (msg->fd, F_SETFL, flags | FASYNC | O_NONBLOCK);
#ifdef __linux__
	      if (msg->mode & vHidd_UnixIO_Write &&
	          msg->fd_type & vHidd_UnixIO_Terminal) {
	          terminals_write_counter++;
	          if (1 == terminals_write_counter) {
	              unixio_start_timer(timerio);
	          }
	      }
#endif
	    }
	    else
	    {
	      /*
	      ** I must look for all messages that tell me to watch on this
	      ** filedescriptor.
	      */
	      struct uioMessage * umsg,  *_umsg;
	      
	      ForeachNodeSafe(&waitList, umsg, _umsg)
	      {
	        if (umsg->fd == msg->fd)
	        {
#ifdef __linux__
                  if (umsg->mode & vHidd_UnixIO_Write &&
                      umsg->fd_type & vHidd_UnixIO_Terminal) {
                      terminals_write_counter--;
#if 0
                      if (0 == terminals_write_counter) {
                        if (!CheckIO(&timerio->tr_node)) 
                          AbortIO(&timerio->tr_node);
                        WaitIO(&timerio->tr_node);
                        kprintf("KIIIIILLLED!\n");
                      }
#endif
                  }
#endif

	          Remove((struct Node *)umsg);
	          FreeMem(umsg, sizeof(struct uioMessage));
	        }
	      }
	      ReplyMsg((struct Message *)msg);
	    }
	} /* while (there are messages) */

	FD_ZERO (&rfds);
	FD_ZERO (&wfds);
	FD_ZERO (&efds);

	rp = wp = ep = NULL;

	maxfd = 0;

	D(bug("Waiting on fd "));

	ForeachNode (&waitList, msg)
	{
	    D(bug("%d, ", msg->fd));
	    if (msg->mode & vHidd_UnixIO_Read)
	    {
		FD_SET (msg->fd, &rfds);
		rp = &rfds;
	    }

	    if (msg->mode & vHidd_UnixIO_Write)
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

        tv.tv_sec  = 0;
	tv.tv_usec = 0;

	errno = 0; /* set errno to zero before select() call */
	selecterr = select (maxfd+1, rp, wp, ep, &tv);
	err = errno;

#if 1
	D(bug("wfio: got io sel=%ld err=%ld\n", selecterr, err));
#endif

	if (selecterr < 0 && err == EINTR)
	    continue;

	if (selecterr >= 0)
	{
	    ForeachNodeSafe (&waitList, msg, nextmsg)
	    {
		if (FD_ISSET (msg->fd, &efds))
		{
		    msg->result = err;
		    goto reply;
		}
		else if ((vHidd_UnixIO_Read & msg->mode) &&
		         FD_ISSET (msg->fd, &rfds))
		{
		    if (msg->callback)
		    {
		        if ( ((int (*)(int, void *))msg->callback)(msg->fd, msg->callbackdata) )
		        {
			    msg->result = 0;
			    goto reply;
			}
		    }
		    else
		    {
			msg->result = 0;
			goto reply;
		    }
		}
		else if ((vHidd_UnixIO_Write & msg->mode) &&
		         FD_ISSET (msg->fd, &wfds))
		{
		    msg->result = 0;
reply:
		    D(bug("wfio: Reply: fd=%ld res=%ld replying to task %s on port %x\n", msg->fd, msg->result, ((struct Task *)((struct Message *)msg)->mn_ReplyPort->mp_SigTask)->tc_Node.ln_Name,((struct Message *)msg)->mn_ReplyPort));
/*
kprintf("\tUnixIO task: Replying a message from task %s (%x) to port %x (flags : 0x%0x)\n",((struct Task *)((struct Message *)msg)->mn_ReplyPort->mp_SigTask)->tc_Node.ln_Name,((struct Message *)msg)->mn_ReplyPort->mp_SigTask,((struct Message *)msg)->mn_ReplyPort,((struct Message *)msg)->mn_ReplyPort->mp_Flags);
*/
		    if (0 == (msg->mode & vHidd_UnixIO_Keep)) {
			Remove ((struct Node *)msg);
			flags = fcntl (msg->fd, F_GETFL);
			fcntl (msg->fd, F_SETFL, flags & ~FASYNC);
		        ReplyMsg ((struct Message *)msg);
#ifdef __linux__
                        if (msg->mode & vHidd_UnixIO_Write)
                            terminals_write_counter--;
#endif
		    } else {
		        /*
		         * Since I am supposed to keep the message
		         * I cannot use ReplyMsg() on it, because that
		         * would put it on the reply port's message port.
		         * So I am doing things 'manually' here what
		         * ReplyMsg() does internally, except for putting
		         * the message onto the message port.
		         */
			struct MsgPort *port;
			Disable();
                        port=((struct Message *)msg)->mn_ReplyPort;
			if(port->mp_SigTask)
			{
			    /* And trigger the arrival action. */
			    switch(port->mp_Flags&PF_ACTION)
			    {
				case PA_SIGNAL:
				    /* Send a signal */
				    Signal((struct Task *)port->mp_SigTask,1<<port->mp_SigBit);
				    break;

				case PA_SOFTINT:
				    /* Raise a software interrupt */
				    Cause((struct Interrupt *)port->mp_SoftInt);
				    break;

				case PA_IGNORE:
				    /* Do nothing */
				    break;
			    }
			}
			Enable();
		    }
		}
	    }
	}
	else
	{
	    D(bug("wfio: Timeout sel=%ld err=%ld\n", selecterr, err));
	}
    } /* Forever */
}

#define OOPBase	(((struct uio_data *)cl->UserData)->ud_OOPBase)
#define UtilityBase	(((struct uio_data *)cl->UserData)->ud_UtilityBase)

/* This is the dispatcher for the UnixIO HIDD class. */


/********************
**  UnixIO::New()  **
********************/
static OOP_Object *unixio_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("UnixIO::New(cl=%s)\n", cl->ClassNode.ln_Name));
    D(bug("DoSuperMethod:%p\n", cl->DoSuperMethod));
    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
	struct UnixIOData *id;
	ULONG dispose_mid;
	
	id = OOP_INST_DATA(cl, o);
	D(bug("inst: %p, o: %p\n", id, o));

	id->uio_ReplyPort = CreatePort (NULL, 0);
	if (id->uio_ReplyPort)
	{
/*
kprintf("\tUnixIO::New(): Task %s (%x) Replyport: %x\n",FindTask(NULL)->tc_Node.ln_Name,FindTask(NULL),id->uio_ReplyPort);
*/
	    D(bug("Port created at %x\n",id->uio_ReplyPort));
	    ReturnPtr("UnixIO::New", Object *, o);
    	}


	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
    }
    ReturnPtr("UnixIO::New", OOP_Object *, NULL);
}

/***********************
**  UnixIO::Dispose()  **
***********************/
static IPTR unixio_dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct UnixIOData *id = OOP_INST_DATA(cl, o);

    if (id -> uio_ReplyPort)
	DeletePort (id->uio_ReplyPort);
	
    return OOP_DoSuperMethod(cl, o, msg);
}

/*********************
**  UnixIO::Wait()  **
*********************/
static IPTR unixio_wait(OOP_Class *cl, OOP_Object *o, struct uioMsg *msg)
{
    IPTR retval = 0UL;
//    struct UnixIOData *id = OOP_INST_DATA(cl, o);
    struct uioMessage * umsg = AllocMem (sizeof (struct uioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct MsgPort  * port = CreatePort(NULL, 0);
    struct uio_data *ud = (struct uio_data *)cl->UserData;

    if (umsg  && port)
    {
	port->mp_Flags = PA_SIGNAL;
	port->mp_SigTask = FindTask (NULL);

	umsg->Message.mn_ReplyPort = port;
	umsg->fd   = ((struct uioMsg *)msg)->um_Filedesc;
	umsg->mode = ((struct uioMsg *)msg)->um_Mode;
	umsg->callback = ((struct uioMsg *)msg)->um_CallBack;
	umsg->callbackdata = ((struct uioMsg *)msg)->um_CallBackData;

	D(bug("UnixIO::Wait() Sending msg fd=%ld mode=%ld to port %x\n", umsg->fd, umsg->mode, ud->ud_Port));

/*
kprintf("\tUnixIO::Wait() Task %s (%x) waiting on port %x\n",FindTask(NULL)->tc_Node.ln_Name,FindTask(NULL),port);
*/
	PutMsg (ud->ud_Port, (struct Message *)umsg);
	WaitPort (port);
	GetMsg (port);

        DeletePort(port);

	D(bug("Get msg fd=%ld mode=%ld res=%ld\n", umsg->fd, umsg->mode, umsg->result));
	retval = umsg->result;
    }
    else
	retval = ENOMEM;

    if (umsg)
	FreeMem (umsg, sizeof (struct uioMessage));

    return retval;
}

/************************
**  UnixIO::AsyncIO()  **
************************/
static IPTR unixio_asyncio(OOP_Class *cl, OOP_Object *o, struct uioMsgAsyncIO *msg)
{
    IPTR retval = 0UL;
    struct uioMessage * umsg = AllocMem (sizeof (struct uioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct MsgPort  * port = msg->um_ReplyPort;
    struct uio_data *ud = (struct uio_data *)cl->UserData;

    if (umsg)
    {
    	/* nlorentz: What action should be taken on reply of this message
	   should be the choice of the caller. (The caller might want
	   a signal instead of a softint)
	   
	port->mp_Flags   = PA_SOFTINT;
	
	*/

	umsg->Message.mn_ReplyPort = port;
	umsg->fd      = ((struct uioMsg *)msg)->um_Filedesc;
	umsg->fd_type = ((struct uioMsg *)msg)->um_Filedesc_Type;
	umsg->mode    = ((struct uioMsg *)msg)->um_Mode;
	umsg->callback = NULL;
	umsg->callbackdata = NULL;

	D(bug("Sending msg fd=%ld mode=%ld to port %x\n", umsg->fd, umsg->mode, ud->ud_Port));

        /*
        ** Just send the message and leave
        ** When the message arrives on the port the user must free
        ** the message!
        */
	PutMsg (ud->ud_Port, (struct Message *)umsg);

    }
    else
	retval = ENOMEM;

    return retval;
}


/*****************************
**  UnixIO::AbortAsyncIO()  **
*****************************/
static VOID unixio_abortasyncio(OOP_Class *cl, OOP_Object *o, struct uioMsgAbortAsyncIO *msg)
{
    struct uioMessage * umsg = AllocMem (sizeof (struct uioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct uio_data *ud = (struct uio_data *)cl->UserData;
    struct MsgPort  * port = CreatePort(NULL, 0);

    if (umsg  && port)
    {
	umsg->Message.mn_ReplyPort = port;
	umsg->fd   = ((struct uioMsg *)msg)->um_Filedesc;
	umsg->mode = vHidd_UnixIO_Abort;

	PutMsg (ud->ud_Port, (struct Message *)umsg);

	WaitPort (port);
	GetMsg (port);

    }
    
    if (umsg)
	FreeMem (umsg, sizeof (struct uioMessage));

    if (port)
        DeletePort(port);
}



/* This is the initialisation code for the HIDD class itself. */
#undef OOPBase
#undef UtilityBase


#define NUM_ROOT_METHODS 2
#define NUM_UNIXIO_METHODS 3

AROS_UFH3(static void *, AROS_SLIB_ENTRY(init,UnixIO),
    AROS_UFHA(ULONG, dummy1, D0),
    AROS_UFHA(ULONG, dummy2, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct Library  * OOPBase;
    struct OOP_IClass * cl;
    struct uio_data * ud;
    struct Task     * newtask,
		    * task2 = NULL; /* keep compiler happy */
    struct newMemList nml;
    struct MemList  * ml;
    struct Interrupt * is;

    struct OOP_MethodDescr root_mdescr[NUM_ROOT_METHODS + 1] =
    {
    	{ (IPTR (*)())unixio_new,	moRoot_New	},
    	{ (IPTR (*)())unixio_dispose,	moRoot_Dispose	},
    	{ NULL, 0UL }
    };

    struct OOP_MethodDescr unixio_mdescr[NUM_UNIXIO_METHODS + 1] =
    {
    	{ (IPTR (*)())unixio_wait,	moHidd_UnixIO_Wait		},
    	{ (IPTR (*)())unixio_asyncio,	moHidd_UnixIO_AsyncIO		},
    	{ (IPTR (*)())unixio_abortasyncio,moHidd_UnixIO_AbortAsyncIO	},
    	{ NULL, 0UL }
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
    	{root_mdescr, IID_Root, NUM_ROOT_METHODS},
	{unixio_mdescr, IID_Hidd_UnixIO, NUM_UNIXIO_METHODS},
	{NULL, NULL, 0UL}
    };

    
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

    OOPBase = ud->ud_OOPBase = OpenLibrary("oop.library", 0);
    if(ud->ud_OOPBase == NULL)
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

    /*
	The original stack size was 8192, however some emulated systems
	require a large stack during signal handlers. FreeBSD in fact
	says that it requires 8192 just FOR the signal handler. I have
	changed this to AROS_STACKSIZE for that reason.
    */
    nml.nml_ME[1].me_Length = AROS_STACKSIZE;

    ml = AllocEntry ((struct MemList *)&nml);

    if (AROS_CHECK_ALLOCENTRY(ml))
    {
	newtask = ml->ml_ME[0].me_Addr;

	newtask->tc_Node.ln_Type = NT_TASK;
	newtask->tc_Node.ln_Pri  = 30;
	newtask->tc_Node.ln_Name = "UnixIO.task";

	newtask->tc_SPReg   = (APTR)((ULONG)ml->ml_ME[1].me_Addr + AROS_STACKSIZE);
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

    is=(struct Interrupt *)AllocMem(sizeof(struct Interrupt),MEMF_PUBLIC);
    if (!is)
    {
	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return NULL;
    }
    is->is_Code=(void (*)())&SigIO_IntServer;
    is->is_Data=(APTR)ud;
    SetIntVector(INTB_DSKBLK,is);

    /* Create the class structure for the "unixioclass" */
    
    {
        OOP_AttrBase MetaAttrBase = OOP_GetAttrBase(IID_Meta);
	
        struct TagItem tags[] =
    	{
            {aMeta_SuperID,		(IPTR)CLID_Hidd},
	    {aMeta_InterfaceDescr,	(IPTR)ifdescr},
	    {aMeta_ID,			(IPTR)CLID_Hidd_UnixIO},
	    {aMeta_InstSize,		(IPTR)sizeof (struct UnixIOData) },
	    {TAG_DONE, 0UL}
    	};

    	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
    
    	if(cl)
    	{
	    cl->UserData = (APTR)ud;

	    OOP_AddClass(cl);
        }
    }
    return NULL;

    AROS_USERFUNC_EXIT
}



/************
**  Stubs  **
************/

#define OOPBase ( ((struct uio_data *)OOP_OCLASS(o)->UserData)->ud_OOPBase )

IPTR Hidd_UnixIO_Wait(HIDD *o, ULONG fd, ULONG mode, APTR callback, APTR callbackdata, struct ExecBase * SysBase)
{
     static OOP_MethodID mid = 0UL;
     struct uioMsg p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_Wait);
     p.um_MethodID = mid;
     p.um_Filedesc = fd;
     p.um_Mode	   = mode;
     p.um_CallBack = callback;
     p.um_CallBackData = callbackdata;
     
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}

IPTR Hidd_UnixIO_AsyncIO(HIDD *o, ULONG fd, ULONG fd_type, struct MsgPort * port, ULONG mode, struct ExecBase * SysBase)
{
     static OOP_MethodID mid = 0UL;
     struct uioMsgAsyncIO p;
     
     if (!mid)
     	mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_AsyncIO);
     p.um_MethodID      = mid;
     p.um_Filedesc      = fd;
     p.um_Filedesc_Type = fd_type;
     p.um_ReplyPort     = port;
     p.um_Mode	        = mode;
     
     return OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}


VOID Hidd_UnixIO_AbortAsyncIO(HIDD *o, ULONG fd, struct ExecBase * SysBase)
{
     static OOP_MethodID mid = 0UL;
     struct uioMsgAbortAsyncIO p;
     
     if (!mid) mid = OOP_GetMethodID(IID_Hidd_UnixIO, moHidd_UnixIO_AbortAsyncIO);
     p.um_MethodID = mid;
     p.um_Filedesc = fd;
     
     OOP_DoMethod((OOP_Object *)o, (OOP_Msg)&p);
}





/* The below function is just a hack to avoid
   name conflicts inside intuition_driver.c
*/

#undef OOPBase
HIDD *New_UnixIO(struct Library *OOPBase, struct ExecBase * SysBase)
{
   struct TagItem tags[] = {{ TAG_END, 0 }};
   return (HIDD)OOP_NewObject (NULL, CLID_Hidd_UnixIO, (struct TagItem *)tags);
}
