/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: hostio_class.c 32324 2010-01-14 08:44:13Z sonic $

    Desc: Host OS filedescriptor/socket IO
    Lang: english
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/asmcall.h>
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
#include <hidd/hostio.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <hardware/intbits.h>
#include <hardware/custom.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <devices/timer.h>
#include <fcntl.h>
#include <errno.h>

#include LC_LIBDEFS_FILE

#include "hostio.h"

#define DERROR(x) x

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


/************************************************************************/

static ULONG errtab[][2]=
{
  { ERROR_PATH_NOT_FOUND	, ENOENT    },
  { ERROR_ACCESS_DENIED		, EACCES    },
  { ERROR_NO_MORE_FILES		, ENOENT    },
  { ERROR_NOT_ENOUGH_MEMORY	, ENOMEM    },
  { ERROR_FILE_NOT_FOUND	, ENOENT    },
  { ERROR_FILE_EXISTS		, EEXIST    },
  { ERROR_WRITE_PROTECT		, EACCES    },
  { WIN32_ERROR_DISK_FULL	, ENOSPC    },
  { ERROR_DIR_NOT_EMPTY		, ENOTEMPTY },
  { ERROR_SHARING_VIOLATION	, EBUSY     },
  { ERROR_LOCK_VIOLATION	, EBUSY     },
  { ERROR_BUFFER_OVERFLOW	, ENOBUFS   },
  { ERROR_INVALID_NAME		, EINVAL    },
  { ERROR_INVALID_PARAMETER	, EINVAL    },
  { ERROR_IO_PENDING		, EAGAIN    },
  { ERROR_HANDLE_EOF		, 0	    },
  { 0				, 0	    }
};

ULONG Errno_win2std(ULONG e)
{
    ULONG i;
  
    DERROR(bug("[HostIO] Windows error code: %lu\n", e));
    for (i=0; i < sizeof(errtab)/sizeof(errtab[0]); i++) {
	if(errtab[i][0] == e) {
	    DERROR(bug("[HostIO] Translated to standard error code: %lu\n", errtab[i][1]));
	    return errtab[i][1];
	}
    }
    DERROR(bug("[HostIO] Unknown error code\n"));
    return 123;
}

#define SetError(err) if (msg->hm_ErrNoPtr) *msg->hm_ErrNoPtr = Errno_win2std(err);
#define SetStdError(err) if (msg->hm_ErrNoPtr) *msg->hm_ErrNoPtr = err;
#define SetRawError(err) if (msg->hm_RawErrNoPtr) *msg->hm_RawErrNoPtr = err;

AROS_UFH5 (void, SigIO_IntServer,
    AROS_UFHA (ULONG             ,dummy,  D0),
    AROS_UFHA (struct Custom    *,custom, A0),
    AROS_UFHA (struct List      *,intList,A1),
    AROS_UFHA (APTR              ,ivCode, A5),
    AROS_UFHA (struct ExecBase  *,SysBase,A6)
)
{
    AROS_USERFUNC_INIT

    struct hio_data * ud = (struct hio_data *) intList;

    Signal (ud -> hd_WaitForIO, SIGBREAKF_CTRL_C);

    AROS_USERFUNC_EXIT
}


/******************
**  HostIO task  **
******************/
static void WaitForIO (void)
{
    struct hio_data * ud = FindTask(NULL)->tc_UserData;
    int maxfd;
    int selecterr;
    int err;
    fd_set rfds, wfds, efds;
    fd_set * rp, * wp, * ep;
    struct timeval tv;
    struct List waitList;
    struct hioMessage * msg, * nextmsg;
    ULONG rmask;
    int flags;
    /*
     * Since the signal was allocated by other task, but really is mine
     * I need to allocate it here 'manually'. Otherwise the CreateMsgPort()
     * would get the wrong signal.
     */
    AllocSignal(ud->hd_Port -> mp_SigBit);

    NEWLIST (&waitList);

    D(bug("wfio: HostIO.hidd ready ud=%08lx\n", ud));

    for (;;)
    {
        if (IsListEmpty (&waitList))
	{
	    D(bug("wfio: Waiting for message for task %s\n",ud->hd_WaitForIO->tc_Node.ln_Name));
	    WaitPort (ud->hd_Port);
	    D(bug("wfio: Got messages\n"));
	}
        else
	{
		    
	D(bug("wfio: Waiting for message or signal for task %s\n",ud->hd_WaitForIO->tc_Node.ln_Name));
            rmask = Wait((1UL << ud->hd_Port->mp_SigBit) | SIGBREAKF_CTRL_C);

	    if (rmask & 1 << ud->hd_Port -> mp_SigBit)
	    {
	        D(bug("wfio: Got message\n"));
	    }
	    else if (rmask & SIGBREAKF_CTRL_C)
	    {
	        D(bug("wfio: Got signal\n"));
	    }
	}

	while ((msg = (struct hioMessage *)GetMsg (ud->hd_Port)))
	{
	    if (msg->mode != vHidd_HostIO_Abort)
	    {

	      D(bug("wfio: Got msg fd=%ld mode=%ld\n", msg->fd, msg->mode));
	      AddTail (&waitList, (struct Node *)msg);

/*	      flags = fcntl (msg->fd, F_GETFL);
	      fcntl (msg->fd, F_SETFL, flags | FASYNC | O_NONBLOCK);*/
	    }
	    else
	    {
	      /*
	      ** I must look for all messages that tell me to watch on this
	      ** filedescriptor.
	      */
	      struct hioMessage * umsg,  *_umsg;
	      
	      ForeachNodeSafe(&waitList, umsg, _umsg)
	      {
	        if (umsg->fd == msg->fd)
	        {
	          Remove((struct Node *)umsg);
	          FreeMem(umsg, sizeof(struct hioMessage));
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
	    if (msg->mode & vHidd_HostIO_Read)
	    {
		FD_SET (msg->fd, &rfds);
		rp = &rfds;
	    }

	    if (msg->mode & vHidd_HostIO_Write)
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
//	selecterr = select (maxfd+1, rp, wp, ep, &tv);
	err = errno;

	D(bug("wfio: got io sel=%ld err=%ld\n", selecterr, err));

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
		else if ((vHidd_HostIO_Read & msg->mode) && FD_ISSET (msg->fd, &rfds))
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
		else if ((vHidd_HostIO_Write & msg->mode) && FD_ISSET (msg->fd, &wfds))
		{
		    msg->result = 0;
reply:
		    D(bug("wfio: Reply: fd=%ld res=%ld replying to task %s on port %x\n", msg->fd, msg->result, ((struct Task *)((struct Message *)msg)->mn_ReplyPort->mp_SigTask)->tc_Node.ln_Name,((struct Message *)msg)->mn_ReplyPort));
/*
kprintf("\tHostIO task: Replying a message from task %s (%x) to port %x (flags : 0x%0x)\n",((struct Task *)((struct Message *)msg)->mn_ReplyPort->mp_SigTask)->tc_Node.ln_Name,((struct Message *)msg)->mn_ReplyPort->mp_SigTask,((struct Message *)msg)->mn_ReplyPort,((struct Message *)msg)->mn_ReplyPort->mp_Flags);
*/
		    if (0 == (msg->mode & vHidd_HostIO_Keep)) {
			Remove ((struct Node *)msg);
/*			flags = fcntl (msg->fd, F_GETFL);
			fcntl (msg->fd, F_SETFL, flags & ~FASYNC);*/
		        ReplyMsg ((struct Message *)msg);
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

/* This is the dispatcher for the HostIO HIDD class. */


/********************
**  HostIO::New()  **
********************/
OOP_Object *HIO__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("HostIO::New(cl=%s)\n", cl->ClassNode.ln_Name));

    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (o)
    {
	struct HostIOData *id;
	ULONG dispose_mid;
	
	id = OOP_INST_DATA(cl, o);
	D(bug("inst: %p, o: %p\n", id, o));

	id->hio_ReplyPort = CreateMsgPort();
	if (id->hio_ReplyPort)
	{
/*
kprintf("\tHostIO::New(): Task %s (%x) Replyport: %x\n",FindTask(NULL)->tc_Node.ln_Name,FindTask(NULL),id->hio_ReplyPort);
*/
	    D(bug("Port created at %x\n",id->hio_ReplyPort));
	    ReturnPtr("HostIO::New", OOP_Object *, o);
    	}


	dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
	OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
    }
    ReturnPtr("HostIO::New", OOP_Object *, NULL);
}

/***********************
**  HostIO::Dispose()  **
***********************/
IPTR HIO__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HostIOData *id = OOP_INST_DATA(cl, o);

    if (id -> hio_ReplyPort)
	DeleteMsgPort (id->hio_ReplyPort);
	
    return OOP_DoSuperMethod(cl, o, msg);
}

/*********************
**  HostIO::Wait()  **
*********************/
IPTR HIO__Hidd_HostIO__Wait(OOP_Class *cl, OOP_Object *o, struct hioMsg *msg)
{
    IPTR retval = 0UL;
//    struct HostIOData *id = OOP_INST_DATA(cl, o);
    struct hioMessage * umsg = AllocMem (sizeof (struct hioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct MsgPort  * port = CreateMsgPort();
    struct hio_data *ud = HD(cl);

    if (umsg  && port)
    {
	port->mp_Flags = PA_SIGNAL;
	port->mp_SigTask = FindTask (NULL);

	umsg->Message.mn_ReplyPort = port;
	umsg->fd   = ((struct hioMsg *)msg)->hm_Filedesc;
	umsg->fd_type = vHidd_HostIO_Socket;
	umsg->mode = ((struct hioMsg *)msg)->hm_Mode;
	umsg->callback = ((struct hioMsg *)msg)->hm_CallBack;
	umsg->callbackdata = ((struct hioMsg *)msg)->hm_CallBackData;

	D(bug("HostIO::Wait() Sending msg fd=%ld mode=%ld to port %x\n", umsg->fd, umsg->mode, ud->hd_Port));

/*
kprintf("\tHostIO::Wait() Task %s (%x) waiting on port %x\n",FindTask(NULL)->tc_Node.ln_Name,FindTask(NULL),port);
*/
	PutMsg (ud->hd_Port, (struct Message *)umsg);
	WaitPort (port);
	GetMsg (port);

        DeleteMsgPort(port);

	D(bug("Get msg fd=%ld mode=%ld res=%ld\n", umsg->fd, umsg->mode, umsg->result));
	retval = umsg->result;
    }
    else
	retval = ENOMEM;

    if (umsg)
	FreeMem (umsg, sizeof (struct hioMessage));

    return retval;
}

/************************
**  HostIO::AsyncIO()  **
************************/
IPTR HIO__Hidd_HostIO__AsyncIO(OOP_Class *cl, OOP_Object *o, struct hioMsgAsyncIO *msg)
{
    IPTR retval = 0UL;
    struct hioMessage * umsg = AllocMem (sizeof (struct hioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct MsgPort  * port = msg->hm_ReplyPort;
    struct hio_data *ud = HD(cl);

    if (umsg)
    {
    	/* nlorentz: What action should be taken on reply of this message
	   should be the choice of the caller. (The caller might want
	   a signal instead of a softint)
	   
	port->mp_Flags   = PA_SOFTINT;
	
	*/

	umsg->Message.mn_ReplyPort = port;
	umsg->fd      = ((struct hioMsg *)msg)->hm_Filedesc;
	umsg->fd_type = ((struct hioMsg *)msg)->hm_Filedesc_Type;
	umsg->mode    = ((struct hioMsg *)msg)->hm_Mode;
	umsg->callback = NULL;
	umsg->callbackdata = NULL;

	D(bug("Sending msg fd=%ld mode=%ld to port %x\n", umsg->fd, umsg->mode, ud->hd_Port));

        /*
        ** Just send the message and leave
        ** When the message arrives on the port the user must free
        ** the message!
        */
	PutMsg (ud->hd_Port, (struct Message *)umsg);

    }
    else
	retval = ENOMEM;

    return retval;
}


/*****************************
**  HostIO::AbortAsyncIO()  **
*****************************/
VOID HIO__Hidd_HostIO__AbortAsyncIO(OOP_Class *cl, OOP_Object *o, struct hioMsgAbortAsyncIO *msg)
{
    struct hioMessage * umsg = AllocMem (sizeof (struct hioMessage), MEMF_CLEAR|MEMF_PUBLIC);
    struct hio_data *ud = HD(cl);
    struct MsgPort  * port = CreateMsgPort();

    if (umsg  && port)
    {
	umsg->Message.mn_ReplyPort = port;
	umsg->fd   = ((struct hioMsg *)msg)->hm_Filedesc;
	umsg->mode = vHidd_HostIO_Abort;

	PutMsg (ud->hd_Port, (struct Message *)umsg);

	WaitPort (port);
	GetMsg (port);

    }
    
    if (umsg)
	FreeMem (umsg, sizeof (struct hioMessage));

    if (port)
        DeleteMsgPort(port);
}

/*****************************
**  HostIO::OpenFile()      **
*****************************/
APTR HIO__Hidd_HostIO__OpenFile(OOP_Class *cl, OOP_Object *o, struct hioMsgOpenFile *msg)
{
    struct File_Handle *fh = AllocMem(sizeof(struct File_Handle), MEMF_ANY|MEMF_CLEAR);

    D(bug("[HostIO] OpenFile(\"%s\", 0x%04lX, %o)\n", msg->hm_FileName, msg->hm_Flags, msg->hm_Mode));
    if (fh) {
        ULONG access = 0;
        ULONG share = FILE_SHARE_READ;
        ULONG create = OPEN_EXISTING;
        ULONG flags = 0;
	ULONG error;

	if (msg->hm_Flags & O_RDONLY)
	    access = GENERIC_READ;
	if (msg->hm_Flags & O_WRONLY) {
	    access |= GENERIC_WRITE;
	    share = 0;
	}
	if (msg->hm_Flags & O_CREAT)
	    create = (msg->hm_Flags & O_EXCL) ? CREATE_NEW : OPEN_ALWAYS;
	else if (msg->hm_Flags & O_TRUNC)
	    create = CREATE_ALWAYS;
	if (msg->hm_Flags & O_DSYNC)
	    flags = FILE_FLAG_WRITE_THROUGH;
	if (msg->hm_Flags & O_NONBLOCK)
	    flags |= FILE_FLAG_OVERLAPPED;
	if (!msg->hm_Mode & S_IWUSR)
	    flags |= FILE_ATTRIBUTE_READONLY;
	/* TODO: process O_APPEND and set file pointer in OVERLAPPED structure */

	D(bug("[HostIO] Translated access: 0x%08lX, share: 0x%08lX, create: %u, flags: 0x%08lX\n", access, share, create, flags));
	Forbid();
	fh->handle = CreateFile(msg->hm_FileName, access, share, NULL, create, flags, NULL);
	error = GetLastError();
	Permit();
	SetError(error);

	if (fh->handle == INVALID_HANDLE_VALUE) {
	    D(bug("[HostIO] Error opening file\n"));
	    FreeMem(fh, sizeof(struct File_Handle));
	    fh = vHidd_HostIO_Invalid_Handle;
	}
    } else {
        fh = vHidd_HostIO_Invalid_Handle;
	SetStdError(ENOMEM);
    }
    return fh;
}

/*****************************
**  HostIO::CloseFile()      **
*****************************/
VOID HIO__Hidd_HostIO__CloseFile(OOP_Class *cl, OOP_Object *o, struct hioMsgCloseFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    ULONG error = 0;
    
    if (fh != vHidd_HostIO_Invalid_Handle) {

	Forbid();
	CloseHandle(fh->handle);
	error = GetLastError();
	Permit();
	FreeMem(fh, sizeof(struct File_Handle));
    }
    SetError(error);
}

/*****************************
**  HostIO::ReadFile()     **
*****************************/
IPTR HIO__Hidd_HostIO__ReadFile(OOP_Class *cl, OOP_Object *o, struct hioMsgReadFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    LONG retval = -1;
    ULONG err = 0;
    
    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	ULONG res;

	Forbid();
	res = ReadFile(fh->handle, msg->hm_Buffer, msg->hm_Count, &retval, &fh->io);
	err = GetLastError();
	Permit();
	if (!res)
	    /* EOF is an error in Windows but not an error in libc */
	    retval = (err == ERROR_HANDLE_EOF) ? 0 : -1;
    }
    SetError(err);    
    return retval;
}

/*****************************
**  HostIO::WriteFile()     **
*****************************/
IPTR HIO__Hidd_HostIO__WriteFile(OOP_Class *cl, OOP_Object *o, struct hioMsgWriteFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    LONG retval = -1;
    ULONG err = 0;
    
    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	ULONG res;

	Forbid();
	res = WriteFile(fh->handle, msg->hm_Buffer, msg->hm_Count, &retval, &fh->io);
	err = GetLastError();
	Permit();
	if (!res)
	    retval = -1;
    }
    SetError(err);    
    return retval;
}

/*****************************
**  HostIO::IOControlFile() **
*****************************/
IPTR HIO__Hidd_HostIO__IOControlFile(OOP_Class *cl, OOP_Object *o, struct hioMsgIOControlFile *msg)
{
    struct File_Handle *fh = msg->hm_FD;
    LONG retval = -1;
    ULONG err = 0;

    if (fh != vHidd_HostIO_Invalid_Handle)
    {
	ULONG res;

	Forbid();
    	res = DeviceIoControl(fh->handle, msg->hm_Request, msg->hm_Param, msg->hm_ParamLen, msg->hm_Output, msg->hm_OutputLen, &retval, &fh->io);
	err = GetLastError();
	Permit();
	if (!res)
	    retval = -1;
    }
    SetRawError(err);
    SetError(err);
    return retval;
}

/* This is the initialisation code for the HIDD class itself. */

static int HIO_Init(LIBBASETYPEPTR LIBBASE)
{
    struct Task     * newtask,
		    * task2 = NULL; /* keep compiler happy */
    struct newMemList nml;
    struct MemList  * ml;
    struct Interrupt * is;

    LIBBASE->hio_csd.hd_Port = CreateMsgPort();
    if(LIBBASE->hio_csd.hd_Port == NULL)
    {
	/* If you are not running from ROM, don't use Alert() */
	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return FALSE;
    }

    nml = MemTemplate;

    /*
	The original stack size was 8192, however some emulated systems
	require a large stack during signal handlers. FreeBSD in fact
	says that it requires 8192 just FOR the signal handler. I have
	changed this to AROS_STACKSIZE for that reason.
    */
    nml.nml_ME[1].me_Length = AROS_STACKSIZE;

    if (NewAllocEntry((struct MemList *)&nml, &ml, NULL))
    {
	newtask = ml->ml_ME[0].me_Addr;

	newtask->tc_Node.ln_Type = NT_TASK;
	newtask->tc_Node.ln_Pri  = 30;
	newtask->tc_Node.ln_Name = "HostIO.task";

	newtask->tc_SPReg   = NULL;
	newtask->tc_SPLower = ml->ml_ME[1].me_Addr;
	newtask->tc_SPUpper = (APTR)((IPTR)ml->ml_ME[1].me_Addr + AROS_STACKSIZE);

	newtask->tc_UserData = &LIBBASE->hio_csd;

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
	return FALSE;
    }

    LIBBASE->hio_csd.hd_WaitForIO = task2;

    LIBBASE->hio_csd.hd_Port->mp_Flags   = PA_SIGNAL;
    LIBBASE->hio_csd.hd_Port->mp_SigTask = task2;

    is=(struct Interrupt *)AllocMem(sizeof(struct Interrupt),MEMF_PUBLIC);
    if (!is)
    {
	Alert(AT_DeadEnd | AG_NoMemory | AN_Unknown);
	return FALSE;
    }
/*
    is->is_Code=(void (*)())&SigIO_IntServer;
    is->is_Data=(APTR)&LIBBASE->hio_csd;
    SetIntVector(INTB_DSKBLK,is);
*/
    return TRUE;
}

ADD2INITLIB(HIO_Init, 0)
