/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang:
*/
#define AROS_ALMOST_COMPATIBLE

#include <exec/errors.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#ifdef __GNUC__
#include "pipe_handler_gcc.h"
#endif

#include <string.h>

//#define kprintf(x...)

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
struct pipebase *AROS_SLIB_ENTRY(init,pipe_handler)();
void AROS_SLIB_ENTRY(open,pipe_handler)();
BPTR AROS_SLIB_ENTRY(close,pipe_handler)();
BPTR AROS_SLIB_ENTRY(expunge,pipe_handler)();
int AROS_SLIB_ENTRY(null,pipe_handler)();
void AROS_SLIB_ENTRY(beginio,pipe_handler)();
LONG AROS_SLIB_ENTRY(abortio,pipe_handler)();
static const char end;

AROS_UFP3(LONG, pipeproc,
    AROS_UFPA(char *,argstr,A0),
    AROS_UFPA(ULONG,argsize,D0),
    AROS_UFPA(struct ExecBase *,SysBase,A6));

ULONG SendRequest(struct pipebase *pipebase, struct IOFileSys *iofs);


struct pipemessage
{
    struct Message    msg;
    struct IOFileSys *iofs;
    LONG              curlen;
};

struct filenode
{
    struct Node node;
    ULONG       numwriters;         /* Num of actual writers */
    ULONG       numreaders;         /* Num of actual readers */
    struct List pendingwrites;      /* List of pending write requestes */
    struct List pendingreads;       /* List of pending read requestes */
};

struct rootnode
{
    struct Node node;
    ULONG       numwriters;
    ULONG       numreaders;
    struct List pendingwrites;
    struct List pendingreads;
    struct SignalSemaphore filesSem;
    struct List files;
};
/*
   Abuse of pendingreads so that it's used as waiting list either for
   readers or writers before respectively a writer or a reader becomes
   available
*/
#define waitinglist pendingreads


struct usernode
{
    struct Node      node;
    struct filenode *fn;
    ULONG            mode;
};


int entry(void)
{
    /* If the handler was executed by accident return error code. */
    return -1;
}

const struct Resident pipe_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&pipe_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="pipe.handler";

static const char version[]="$VER: pipe-handler 41.1 (8.6.96)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct pipebase),
    (APTR)functable,
    NULL,
    &AROS_SLIB_ENTRY(init,pipe_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,pipe_handler),
    &AROS_SLIB_ENTRY(close,pipe_handler),
    &AROS_SLIB_ENTRY(expunge,pipe_handler),
    &AROS_SLIB_ENTRY(null,pipe_handler),
    &AROS_SLIB_ENTRY(beginio,pipe_handler),
    &AROS_SLIB_ENTRY(abortio,pipe_handler),
    (void *)-1
};

AROS_LH2(struct pipebase *, init,
AROS_LHA(struct pipebase *, pipebase, D0),
AROS_LHA(BPTR,              segList,  A0),
	 struct ExecBase *, sysBase, 0, pipe_handler)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    SysBase =  sysBase;
    DOSBase =  (struct DosLibrary *)OpenLibrary("dos.library",39);
    pipebase->seglist=segList;

    if(DOSBase)
    {
	struct TagItem taglist[]=
	{
	 {NP_Entry,              (IPTR)pipeproc},
	 {NP_Name, (IPTR)"pipe.handler process"},
	 {NP_UserData,           (IPTR)pipebase},
	 {TAG_DONE,                           0}
	};

	pipebase->proc = CreateNewProc(taglist);

       	if (pipebase->proc)
	    return pipebase;

        CloseLibrary((struct Library *)DOSBase);
    }

    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct pipebase *, pipebase, 1, pipe_handler)
{
    AROS_LIBFUNC_INIT
    struct usernode *un;
    struct rootnode *rn;

    /* Get compiler happy */
    unitnum=flags=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    /* Build a fake usernode */
    un = AllocVec(sizeof(*un),MEMF_PUBLIC|MEMF_CLEAR);
    if(un)
    {
	rn = AllocVec(sizeof(*rn),MEMF_PUBLIC|MEMF_CLEAR);
	if (rn)
	{
	    rn->node.ln_Type = ST_ROOT;
	    rn->node.ln_Name = "Root PIPE";

	    NEWLIST(&rn->files);
	    NEWLIST(&rn->pendingwrites);
	    NEWLIST(&rn->pendingreads);
	    InitSemaphore(&rn->filesSem);

	    un->fn = (struct filenode *)rn;

	    iofs->IOFS.io_Unit=(struct Unit *)un;
            iofs->IOFS.io_Device=&pipebase->device;

	    /* I have one more opener. */
            pipebase->device.dd_Library.lib_OpenCnt++;

	    pipebase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
    	    iofs->IOFS.io_Error=0;

    	    return;
        }

	FreeVec(un);
    }

    iofs->io_DosError=ERROR_NO_FREE_STORE;

    iofs->IOFS.io_Error=IOERR_OPENFAIL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(BPTR, close,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct pipebase *, pipebase, 2, pipe_handler)
{
    AROS_LIBFUNC_INIT
    struct usernode *un;
    struct rootnode *rn;

    un = (struct usernode *)iofs->IOFS.io_Unit;
    rn = (struct rootnode *)un->fn;

    if(!(IsListEmpty(&rn->files) || rn->numwriters || rn->numreaders))
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return 0;
    }

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;

    FreeVec(rn);
    FreeVec(un);

    iofs->io_DosError=0;

    /* I have one fewer opener. */
    if(!--pipebase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(pipebase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct pipebase *, pipebase, 3, pipe_handler)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(pipebase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	pipebase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    SendRequest(pipebase, NULL);

    /* Free all resources */
    CloseLibrary((struct Library *)pipebase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&pipebase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=pipebase->seglist;

    /* Free the memory. */
    FreeMem((char *)pipebase-pipebase->device.dd_Library.lib_NegSize,
	    pipebase->device.dd_Library.lib_NegSize+pipebase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct pipebase *, pipebase, 4, pipe_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct pipebase *, pipebase, 5, pipe_handler)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    BOOL enqueued = FALSE;

    kprintf("COMMAND %d\n", iofs->IOFS.io_Command);
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	case FSA_OPEN_FILE:
	case FSA_READ:
	case FSA_WRITE:
	case FSA_CLOSE:
	    error = SendRequest(pipebase, iofs);
	    enqueued = !error;
	    break;

	case FSA_SEEK:
	    error = ERROR_SEEK_ERROR;
	    break;
        case FSA_SET_FILE_SIZE:
        case FSA_EXAMINE:
        case FSA_EXAMINE_NEXT:
        case FSA_EXAMINE_ALL:
        case FSA_CREATE_DIR:
        case FSA_CREATE_HARDLINK:
        case FSA_CREATE_SOFTLINK:
        case FSA_RENAME:
        case FSA_DELETE_OBJECT:
            error = ERROR_NOT_IMPLEMENTED;
            break;

	default:
	    error = ERROR_ACTION_NOT_KNOWN;
	    break;
    }

    /* Set error code */
    iofs->io_DosError=error;

    /* If the quick bit is not set and the request hasn't been redirected
       send the message to the port
    */
    if(!(iofs->IOFS.io_Flags&IOF_QUICK) && !enqueued)
	ReplyMsg(&iofs->IOFS.io_Message);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct pipebase *, pipebase, 6, pipe_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

ULONG SendRequest(struct pipebase *pipebase, struct IOFileSys *iofs)
{
    struct pipemessage *msg = AllocVec(sizeof(*msg), MEMF_PUBLIC);

    if (msg)
    {
        msg->msg.mn_Node.ln_Type = NT_MESSAGE;
	msg->msg.mn_Node.ln_Name = "PIPEMSG";
        msg->msg.mn_Length       = sizeof(struct pipemessage);
	msg->iofs                = iofs;

	if (iofs)
	{
	    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	    iofs->IOFS.io_Flags &= ~IOF_QUICK;
     	}

	kprintf(">>>> %d\n", msg->iofs->IOFS.io_Command);
	kprintf(">>>> %p\n", msg->iofs);
	PutMsg(&pipebase->proc->pr_MsgPort, (struct Message *)msg);

	return 0;
    }

    return ERROR_NO_FREE_STORE;
}

/* The helper process */

#undef SysBase
#ifndef kprintf
     struct ExecBase *SysBase;
#else
#    define SysBase _SysBase
#endif

#define SendBack(msg)                        \
{                                            \
    ReplyMsg(&(msg)->iofs->IOFS.io_Message); \
    FreeVec(msg);                            \
}

STRPTR SkipColon(STRPTR str)
{
    STRPTR oldstr = str;

    while(str[0])
        if (str++[0] == ':') return str;

    return oldstr;
}

struct filenode *GetPipe(struct rootnode *rn, STRPTR pipename, ULONG mode)
{
    return NULL;
}

AROS_UFH3(LONG, pipeproc,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,_SysBase,A6))
{
    AROS_USERFUNC_INIT

    SysBase = _SysBase;

    struct Process     *me = (struct Process *)FindTask(0);
    struct pipemessage *msg;
    struct usernode    *un;
    struct filenode    *fn;
    BOOL cont = TRUE;

    do
    {
    	WaitPort(&(me->pr_MsgPort));

	while
	(
	    (msg =(struct pipemessage *)GetMsg(&(me->pr_MsgPort))) &&
	    (cont = (msg->iofs != 0))
	)
	{
	    kprintf("Message received.\n");

	    un = (struct usernode *)msg->iofs->IOFS.io_Unit;
	    fn = un->fn;

	    switch (msg->iofs->IOFS.io_Command)
	    {
		case FSA_OPEN:
		    msg->iofs->io_Union.io_OPEN.io_FileMode &= ~(FMF_WRITE|FMF_READ);
		    /* Fall through */
		case FSA_OPEN_FILE:
		{
		    struct usernode *un;
		    ULONG            mode;
		    STRPTR           pipename;
		    BOOL             stillwaiting;

		    kprintf("Command is OPEN\n");

		    mode     = msg->iofs->io_Union.io_OPEN.io_FileMode;
		    pipename = SkipColon(msg->iofs->io_Union.io_NamedFile.io_Filename);

		    kprintf("User wants to open pipe \"%s\".\n", pipename);
		    kprintf("Parent dir is \"%s\"\n", fn->node.ln_Name);

		    if (pipename[0])
		    {
		        if
			(
			    (fn->node.ln_Type <= 0) ||
			    (fn = GetPipe((struct rootnode *)fn, pipename, mode)) == NULL
			)
			{
			    kprintf("The requested pipe couldn't be found\n");
			    msg->iofs->io_DosError = ERROR_OBJECT_NOT_FOUND;
			    SendBack(msg);
			    continue;
			}
		    }

		    kprintf("Pipe requested found.\n");

		    un = AllocVec(sizeof(*un), MEMF_PUBLIC);
		    if (!un)
		    {
		        msg->iofs->io_DosError = ERROR_NO_FREE_STORE;
			SendBack(msg);
			continue;
		    }

		    un->mode = mode;
		    un->fn   = fn;
		    msg->iofs->IOFS.io_Unit = (struct Unit *)un;

                    stillwaiting = !fn->numwriters || !fn->numreaders;

		    if (un->mode == FMF_MODE_OLDFILE) un->mode &= ~FMF_WRITE;
		    if (un->mode == FMF_MODE_NEWFILE) un->mode &= ~FMF_READ;

		    if (un->mode & FMF_READ)
		    {
			kprintf("User wants to read. ");
			fn->numreaders++;
			kprintf("There are %d readers at the moment\n", fn->numreaders);
		    }
		    if (un->mode & FMF_WRITE)
		    {
			kprintf("User wants to write. ");
		    	fn->numwriters++;
			kprintf("There are %d writers at the moment\n", fn->numwriters);
		    }

		    if (!fn->numwriters || !fn->numreaders)
		    {
			if (un->mode&(FMF_WRITE|FMF_READ))
			{
			    /*
			       If we're lacking of writers or readers
			       then add this message to a waiting list.
			    */
			    kprintf("There are no %s at the moment, so this %s must wait\n",
				     fn->numwriters?"readers":"writers",
				     fn->numwriters?"writer":"reader");

			    AddTail(&fn->waitinglist, (struct Node *)msg);
       			}
			else
			    SendBack(msg);
                    }
		    else
		    {
			if (stillwaiting)
		        {
		            /*
		              Else wake up all the ones that were still waiting
		            */
			    struct pipemessage *msg;

			    kprintf("Finally there are enough readers and writers! "
			            "Wake up all of them\n");

			    while ((msg = (struct pipemessage *)RemHead(&fn->waitinglist)))
			        SendBack(msg);
           		}
			SendBack(msg);
		    }

		    continue;
		}
		case FSA_CLOSE:
		    kprintf("Command is FSA_CLOSE\n");

		    if (un->mode & FMF_READ)
		    {
			kprintf("User was a reader. ");
			fn->numreaders--;
			kprintf("There are %d readers at the moment\n", fn->numreaders);
   		    }
		    if (un->mode & FMF_WRITE)
		    {
			kprintf("User was a writer. ");
			fn->numwriters--;
			kprintf("There are %d writers at the moment\n", fn->numwriters);
   		    }

		    if (un->mode&FMF_WRITE && !fn->numwriters)
		    {
			struct pipemessage *msg;

			kprintf("There are no writers anymore. %s\n",
			        IsListEmpty(&fn->pendingreads) ?
				"There are no pending reads"   :
			        "Reply to all the waiting readers");
			while ((msg = (struct pipemessage *)RemHead(&fn->pendingreads)))
			{
			    msg->iofs->io_Union.io_READ_WRITE.io_Length =
			    msg->iofs->io_Union.io_READ_WRITE.io_Length - msg->curlen;
			    SendBack(msg);
			}
		    }
		    if (un->mode&FMF_READ && !fn->numreaders)
		    {
			struct pipemessage *msg;

			kprintf("There are no readers anymore. %s\n",
			        IsListEmpty(&fn->pendingwrites) ?
				"There are no pending writes"   :
			        "Reply to all the waiting writers");

			while ((msg = (struct pipemessage *)RemHead(&fn->pendingwrites)))
			{
			    msg->iofs->io_DosError = ERROR_BROKEN_PIPE;
			    SendBack(msg);
			}
		    }

		    SendBack(msg);

		    continue;
		case FSA_WRITE:
		    kprintf("Command is FSA_WRITE. ");
		    if (!un->mode & FMF_WRITE)
		    {
		        kprintf("User doesn't have permission to write.\n");
			msg->iofs->io_DosError = ERROR_BAD_STREAM_NAME;
			SendBack(msg);
		        continue;
		    }
		    if (!fn->numreaders)
		    {
			kprintf("There are no more readers: PIPE BROKEN.\n");
			msg->iofs->io_DosError = ERROR_BROKEN_PIPE;
			SendBack(msg);
		        continue;
		    }
		    kprintf("Enqueing the message\n");
		    msg->curlen = msg->iofs->io_Union.io_READ_WRITE.io_Length;
		    AddTail(&fn->pendingwrites, (struct Node *)msg);
		    break;
		case FSA_READ:
		    kprintf("Command is FSA_READ. ");
		    if (!un->mode & FMF_READ)
		    {
		        kprintf("User doesn't have permission to read.\n");
			msg->iofs->io_DosError = ERROR_BAD_STREAM_NAME;
			SendBack(msg);
		        continue;
		    }
		    if (!fn->numwriters)
		    {
			kprintf("There's no data to read: send EOF\n");
			msg->iofs->io_Union.io_READ_WRITE.io_Length = 0;
			SendBack(msg);
		        continue;
		    }
		    kprintf("Enqueing the message\n");
		    msg->curlen = msg->iofs->io_Union.io_READ_WRITE.io_Length;
		    AddTail(&fn->pendingreads, (struct Node *)msg);
		    break;
	    }

	    while (!IsListEmpty(&fn->pendingwrites) && !IsListEmpty(&fn->pendingreads))
	    {
		struct pipemessage *rmsg = (struct pipemessage *)GetHead(&fn->pendingreads);
		struct pipemessage *wmsg = (struct pipemessage *)GetHead(&fn->pendingwrites);

		ULONG len = (rmsg->curlen > wmsg->curlen) ?
		             wmsg->curlen : rmsg->curlen;

	    	kprintf("Writer len = %d - Reader len = %d. Copying %d bytes\n",
		         wmsg->curlen, rmsg->curlen, len);

  		CopyMem
		(
        	    wmsg->iofs->io_Union.io_READ_WRITE.io_Buffer +
		    wmsg->iofs->io_Union.io_READ_WRITE.io_Length -
		    wmsg->curlen,

		    rmsg->iofs->io_Union.io_READ_WRITE.io_Buffer +
      		    rmsg->iofs->io_Union.io_READ_WRITE.io_Length -
		    rmsg->curlen,

		    len
		);

		wmsg->curlen -= len;
		rmsg->curlen -= len;

	        kprintf("Writer curlen is now %d - Reader curlen is now %d\n",
		         wmsg->curlen, rmsg->curlen);

		if (!wmsg->curlen)
		{
		    kprintf("Writer: finished its job. Removing it from the list.\n");
		    Remove(wmsg);
		    SendBack(wmsg);
		}

		if (!rmsg->curlen)
		{
		    kprintf("Reader: finished its job. Removing it from the list.\n");
		    Remove(rmsg);
		    SendBack(rmsg);
		}

	    }

	}
	kprintf("Coming back to wait for a new message\n");

    } while (cont);

    return 0;

    AROS_USERFUNC_EXIT
}

static const char end=0;
