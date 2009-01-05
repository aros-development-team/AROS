/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Basic assumptions:

- buffered one-directional pipes
- both ends opened at once with FSA_PIPE
- FSA_OPEN and FSA_OPEN_FILE used only for duplicating locks (file name "")
- no support for multiple readers (however there may be multiple duplicated pipe ends)

*/

#define DEBUG 0
#include <aros/debug.h>

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
#include <clib/macros.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include <aros/asmcall.h>
#if defined(__GNUC__) || defined(__INTEL_COMPILER)
#include "xpipe_handler_gcc.h"
#endif

#include LC_LIBDEFS_FILE

#include <string.h>
#include <stddef.h>

AROS_UFP3(LONG, xpipeproc,
    AROS_UFPA(char *,argstr,A0),
    AROS_UFPA(ULONG,argsize,D0),
    AROS_UFPA(struct ExecBase *,SysBase,A6));

struct XPipeMessage
{
    struct Message msg;
    struct IOFileSys *iofs;
    LONG curlen;
};

struct XPipe
{
    struct Node node;
    struct List readers; /* List of XPipeEnd structures */
    struct List writers; /* List of XPipeEnd structures */
    struct List pendingreads;
    struct List pendingwrites;
    char buffer[4096];
    int bufstart;
    int bufend;
};

struct XPipeUnit
{
    struct Node node;
    struct List pipes;
};

struct XPipeEnd
{
    struct Node node;
    struct XPipe *pipe;
    ULONG mode;
    unsigned int bufpos; /* Used only for readers */
};

enum { NONE, PROCESS_READS, PROCESS_WRITES };
enum { NT_PIPEROOT, NT_PIPEEND };

static ULONG            SendRequest  (struct XPipeBase *xpipebase, struct IOFileSys *iofs, BOOL abort);

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR xpipebase)
{
    DOSBase =  (struct DosLibrary *)OpenLibrary("dos.library",39);

    if(DOSBase)
    {
	struct TagItem taglist[]=
	{
	 {NP_Entry,              (IPTR)xpipeproc},
	 {NP_Name, (IPTR)"xpipe.handler process"},
	 {NP_UserData,           (IPTR)xpipebase},
	 {TAG_DONE,                           0}
	};

	xpipebase->proc = CreateNewProc(taglist);

       	if (xpipebase->proc)
	    return TRUE;

        CloseLibrary((struct Library *)DOSBase);
    }

    return FALSE;
}

static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR xpipebase,
    struct IOFileSys *iofs,
    ULONG unitnum,
    ULONG flags
)
{
    struct XPipeUnit *un;

    /* Mark message as replied. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    un = AllocVec(sizeof(struct XPipeUnit), MEMF_PUBLIC | MEMF_CLEAR);
    if(un)
    {
	NEWLIST(&un->pipes);
	un->node.ln_Type = NT_PIPEROOT;
	iofs->IOFS.io_Unit=(struct Unit *)un;
        iofs->IOFS.io_Device=&xpipebase->device;
	return TRUE;
    }

    iofs->io_DosError=ERROR_NO_FREE_STORE;
    iofs->IOFS.io_Error=IOERR_OPENFAIL;

    return FALSE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR xpipebase,
    struct IOFileSys *iofs
)
{
    struct XPipeUnit *un = (struct XPipeUnit *)iofs->IOFS.io_Unit;

    if(!IsListEmpty(&un->pipes))
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return FALSE;
    }

    FreeVec(un);

    iofs->io_DosError=0;

    return TRUE;
}

static int GM_UNIQUENAME(Expunge)(LIBBASETYPEPTR xpipebase)
{
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    SendRequest(xpipebase, NULL, TRUE);

    /* Free all resources */
    CloseLibrary((struct Library *)xpipebase->dosbase);
    
    return TRUE;
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct XPipeBase *, xpipebase, 5, Xpipe)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    BOOL enqueued = FALSE;

    D(bug("[xpipe] COMMAND %d\n", iofs->IOFS.io_Command));
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	case FSA_OPEN_FILE:
	case FSA_PIPE:
        case FSA_EXAMINE:
	case FSA_READ:
	case FSA_WRITE:
	case FSA_CLOSE:
	case FSA_FILE_MODE:
	    error = SendRequest(xpipebase, iofs, FALSE);
	    enqueued = !error;
	    break;

	case FSA_SEEK:
	    error = ERROR_SEEK_ERROR;
	    break;
	case FSA_IS_FILESYSTEM:
	    iofs->io_Union.io_IS_FILESYSTEM.io_IsFilesystem = FALSE;
	    break;
	case FSA_CREATE_DIR:
        case FSA_DELETE_OBJECT:
	case FSA_SET_FILE_SIZE:
        case FSA_EXAMINE_ALL:
        case FSA_CREATE_HARDLINK:
        case FSA_CREATE_SOFTLINK:
        case FSA_RENAME:
	case FSA_EXAMINE_NEXT:
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

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(Expunge), 0)

AROS_LH1(LONG, abortio,
AROS_LHA(struct IOFileSys *, iofs, A1),
struct XPipeBase *, xpipebase, 6, Xpipe)
{
    AROS_LIBFUNC_INIT

    return SendRequest(xpipebase, iofs, TRUE);

    AROS_LIBFUNC_EXIT
}

static ULONG SendRequest(struct XPipeBase *xpipebase, struct IOFileSys *iofs, BOOL abort)
{
    struct XPipeMessage *msg = AllocVec(sizeof(*msg), MEMF_PUBLIC);

    if (msg)
    {
        msg->msg.mn_Node.ln_Type = NT_MESSAGE;
	msg->msg.mn_Node.ln_Name = "XPIPEMSG";
        msg->msg.mn_Length       = sizeof(struct XPipeMessage);
	msg->iofs                = iofs;
	msg->curlen              = abort;

	if (iofs)
	{
	    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	    iofs->IOFS.io_Flags &= ~IOF_QUICK;
     	}

	PutMsg(&xpipebase->proc->pr_MsgPort, (struct Message *)msg);

	return 0;
    }

    return ERROR_NO_FREE_STORE;
}

/* The helper process */

#define SendBack(msg, err)                   \
{                                            \
    msg->iofs->io_DosError = err;            \
    ReplyMsg(&(msg)->iofs->IOFS.io_Message); \
    FreeVec(msg);                            \
}

LONG DuplicatePipeEnd(struct XPipeEnd **oldend, ULONG mode)
{
    struct XPipeEnd *newend;
    struct XPipe *pipe = (*oldend)->pipe;
    
    if ((newend = AllocVec(sizeof(struct XPipeEnd), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) 
    {
        return ERROR_NO_FREE_STORE;;
    }
    
    newend->node.ln_Type = NT_PIPEEND;
    newend->mode = mode;
    newend->pipe = (*oldend)->pipe;
    newend->bufpos = (*oldend)->bufpos;

    if (mode & FMF_WRITE)
    {
	D(bug("[xpipe] Cloned pipe end is a writer\n"));
	ADDTAIL(&pipe->writers, newend);
    }
    if (mode & FMF_READ)
    {
	D(bug("[xpipe] Cloned pipe end is a reader\n"));
	ADDTAIL(&pipe->readers, newend);
    }
    
    *oldend = newend;
    return 0;
}

ULONG ReadWouldBlock (struct XPipe *pipe, ULONG length)
{
    int curlen;
    int numwriters;
    ListLength (&pipe->writers, numwriters);
    
    /* If there are no writers, we won't block */
    if(numwriters == 0)
	return FALSE;
    
    if(pipe->bufstart <= pipe->bufend)
	curlen = pipe->bufend - pipe->bufstart;
    else
	curlen = (pipe->bufend + sizeof(pipe->buffer) - pipe->bufstart) % sizeof(pipe->buffer);
    
    if(length > curlen)
	return TRUE;
    else
	return FALSE;
}

ULONG WriteWouldBlock (struct XPipe *pipe, ULONG length)
{
    int curlen;
    if(pipe->bufstart <= pipe->bufend)
	curlen = pipe->bufend - pipe->bufstart;
    else
	curlen = (pipe->bufend + sizeof(pipe->buffer) - pipe->bufstart) % sizeof(pipe->buffer);
    
    if(sizeof(pipe->buffer) - curlen - 1 < length)
	return TRUE;
    else
	return FALSE;
}

LONG ReadFromPipe (struct XPipe *pipe, APTR buffer, ULONG length)
{
    D(bug("[xpipe] ReadFromPipe(%p, %p, %d)\n", pipe, buffer, length));
    D(bug("[xpipe] buffer before read from %d to %d\n", pipe->bufstart, pipe->bufend));
    if(pipe->bufstart <= pipe->bufend)
    {
	/* read bytes from between bufstart and bufend */
	int bytestoread = MIN((pipe->bufend - pipe->bufstart), length);
	CopyMem((APTR)((IPTR)pipe->buffer + pipe->bufstart), buffer, bytestoread);
	/* advance buffer */
	pipe->bufstart += bytestoread;
	D(bug("[xpipe] buffer after read from %d to %d\n", pipe->bufstart, pipe->bufend));
	return bytestoread;
    }
    else
    {
	/* read bytes from bufstart to the end of the buffer */
	int bytestoread1 = MIN((sizeof(pipe->buffer) - pipe->bufstart), length);
	CopyMem((APTR)((IPTR)pipe->buffer + pipe->bufstart), buffer, bytestoread1);
	pipe->bufstart = (pipe->bufstart + bytestoread1) % sizeof(pipe->buffer);
	if(bytestoread1 == length)
	{
	    D(bug("[xpipe] buffer after read from %d to %d\n", pipe->bufstart, pipe->bufend));
	    return length;
	}
	/*  and if it's not enough then from the beginning of the buffer to
	    bufend */
	int bytestoread2 = MIN(pipe->bufend, (length - bytestoread1));
	CopyMem(pipe->buffer, (APTR)((IPTR) buffer + bytestoread1), bytestoread2);
	pipe->bufstart = bytestoread2;
	D(bug("[xpipe] buffer after read from %d to %d\n", pipe->bufstart, pipe->bufend));
	return bytestoread1 + bytestoread2;
    }
}

LONG WriteToPipe (struct XPipe *pipe, APTR buffer, ULONG length)
{
    D(bug("[xpipe] WriteToPipe(%p, %p, %d)\n", pipe, buffer, length));
    D(bug("[xpipe] buffer before write from %d to %d\n", pipe->bufstart, pipe->bufend));
    if(pipe->bufstart > pipe->bufend)
    {
	/* write between bufend and bufstart, left one byte */
	int bytestowrite = MIN((pipe->bufstart - pipe->bufend - 1), length);
	CopyMem(buffer, (APTR)((IPTR)pipe->buffer + pipe->bufend), bytestowrite);
	/* advance buffer */
	pipe->bufend += bytestowrite;
	D(bug("[xpipe] buffer after write from %d to %d\n", pipe->bufstart, pipe->bufend));
	return bytestowrite;
    }
    else
    {
	/* write bytes from bufend to the end of the buffer */
	int bytestowrite1 = MIN((sizeof(pipe->buffer) - pipe->bufend), length);
	/* if there's no space left at the beginning then make sure we leave
	   at least one byte free at the end */
	if(pipe->bufstart == 0 && bytestowrite1 + pipe->bufend == sizeof(pipe->buffer)) bytestowrite1--;
	CopyMem(buffer, (APTR)((IPTR)pipe->buffer + pipe->bufend), bytestowrite1);
	pipe->bufend = (pipe->bufend + bytestowrite1) % sizeof(pipe->buffer);

	D(bug("[xpipe] wrote %d bytes in first part\n", bytestowrite1));
	/* skip the second part if there's no space free at the beginning */
	if(bytestowrite1 == length || pipe->bufstart == 0)
	{
	    D(bug("[xpipe] buffer after write from %d to %d\n", pipe->bufstart, pipe->bufend));
	    return bytestowrite1;
	}
	/*  and if it's not enough then from the beginning of the buffer to
	    bufstart - 1 (one byte free left) */
	int bytestowrite2 = MIN(pipe->bufstart - 1, (length - bytestowrite1));
	D(bug("[xpipe] wrote %d bytes in second part\n", bytestowrite2));
	CopyMem((APTR)((IPTR) buffer + bytestowrite1), pipe->buffer, bytestowrite2);
	pipe->bufend = bytestowrite2;
	D(bug("[xpipe] buffer after write from %d to %d\n", pipe->bufstart, pipe->bufend));
	return bytestowrite1 + bytestowrite2;
    }
}

/* Pump data from write request buffers to read request buffers through 
   the pipe buffer */
void pump(struct XPipe *pipe, int operation)
{
    while(operation != NONE)
    {
        switch(operation)
        {
	    case PROCESS_READS:
            {
        	D(bug("[xpipe] Processing pending reads\n"));
		operation = NONE;
		/* Ok, we have some new data, use them to continue with pending reads */
		struct XPipeMessage *readmsg;
		struct Node *tempnode;
		ForeachNodeSafe (&pipe->pendingreads, readmsg, tempnode)
		{
		    struct XPipeEnd *reader = (struct XPipeEnd*) readmsg->iofs->IOFS.io_Unit;
		    struct XPipe *pipe = reader->pipe;
		    int nread = readmsg->curlen;
		
		    readmsg->curlen += ReadFromPipe (
			pipe, 
			(APTR) ((IPTR) readmsg->iofs->io_Union.io_READ_WRITE.io_Buffer + readmsg->curlen),
			readmsg->iofs->io_Union.io_READ_WRITE.io_Length - readmsg->curlen
		    );
		    nread = readmsg->curlen - nread;
		    D(bug("[xpipe] managed to pump %d bytes from pipe (%d from requested %d)\n", nread, readmsg->curlen, readmsg->iofs->io_Union.io_READ_WRITE.io_Length));
		    if(nread > 0)
		    {
			/* We managed to free some buffer space, now we can
			   write some data */
			operation = PROCESS_WRITES;
		    }
		
		    if(readmsg->iofs->io_Union.io_READ_WRITE.io_Length == readmsg->curlen)
		    {
			D(bug("[xpipe] completed read request %p\n", readmsg));
			/* hooray, we managed to read all data */
			Remove ((struct Node*) readmsg);
			SendBack (readmsg, 0);
		    }
		    else
		    {
			/* Since we didn't manage to complete this request,
			   there's no point in processing other */
                        break;
		    }
		}
		break;
            }
	    case PROCESS_WRITES:
            {
        	D(bug("[xpipe] Processing pending writes\n"));
		operation = NONE;
		/* Ok, we have some new data, use them to continue with pending reads */
		struct XPipeMessage *writemsg;
		struct Node *tempnode;
		ForeachNodeSafe (&pipe->pendingwrites, writemsg, tempnode)
		{
		    struct XPipeEnd *writer = (struct XPipeEnd*) writemsg->iofs->IOFS.io_Unit;
		    struct XPipe *pipe = writer->pipe;
		    int nwrote = writemsg->curlen;
		
		    writemsg->curlen += WriteToPipe (
			pipe, 
			(APTR) ((IPTR) writemsg->iofs->io_Union.io_READ_WRITE.io_Buffer + writemsg->curlen),
			writemsg->iofs->io_Union.io_READ_WRITE.io_Length - writemsg->curlen
		    );
		    nwrote = writemsg->curlen - nwrote;
		    D(bug("[xpipe] managed to pump %d bytes to pipe (%d from requested %d)\n", nwrote, writemsg->curlen, writemsg->iofs->io_Union.io_READ_WRITE.io_Length));
		    if(nwrote > 0)
		    {
			/* We managed to write some data, now we can process
			   some pending reads. */
			operation = PROCESS_READS;
		    }
	
		    if(writemsg->iofs->io_Union.io_READ_WRITE.io_Length == writemsg->curlen)
		    {
			D(bug("[xpipe] completed write request %p\n", writemsg));
			/* hooray, we managed to write all data */
			Remove ((struct Node*) writemsg);
			SendBack (writemsg, 0);
		    }
		    else
		    {
			/* Since we didn't manage to complete this request,
			   there's no point in processing other */
                        break;
		    }
		}
		break;
            }
	}
    }
}

AROS_UFH3(LONG, xpipeproc,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,SysBase,A6))
{
    AROS_USERFUNC_INIT

    struct Process       *me;
    struct XPipeBase     *xpipebase;
    struct XPipeMessage  *msg;
    struct Node          *pn;
    BOOL cont = TRUE;

    me         = (struct Process *)FindTask(0);
    xpipebase = me->pr_Task.tc_UserData;


    do
    {
    	WaitPort (&(me->pr_MsgPort));

	while
	(
	    (msg =(struct XPipeMessage *)GetMsg (&(me->pr_MsgPort))) &&
	    (cont = (msg->iofs != 0))
	)
	{
	    D(bug("[xpipe] Message received.\n"));

	    pn = (struct Node *)msg->iofs->IOFS.io_Unit;

	    switch (msg->iofs->IOFS.io_Command)
	    {
		case FSA_OPEN:
		{
		    D(bug("[xpipe] Cmd is FSA_OPEN\n"));

                    if(pn->ln_Type != NT_PIPEEND)
                    {
                        SendBack (msg, ERROR_OBJECT_NOT_FOUND);
                        break;                	
                    }

		    if (msg->iofs->io_Union.io_OPEN.io_Filename[0])
		    {
			SendBack (msg, ERROR_OBJECT_NOT_FOUND);
			break;
		    }

		    D(bug("[xpipe] Cloning pipe end: %p with mode \n", msg->iofs->IOFS.io_Unit, msg->iofs->io_Union.io_OPEN.io_FileMode));

	            LONG error = DuplicatePipeEnd (
			(struct XPipeEnd**) &msg->iofs->IOFS.io_Unit, 
			msg->iofs->io_Union.io_OPEN.io_FileMode
		    );

	            D(bug("[xpipe] Cloned pipe end: %p\n", msg->iofs->IOFS.io_Unit));

	            SendBack (msg, error);
		    break;
		}
		case FSA_OPEN_FILE:
		{
	            D(bug("[xpipe] Cmd is FSA_OPEN_FILE\n"));

                    if(pn->ln_Type != NT_PIPEEND)
                    {
                        SendBack (msg, ERROR_OBJECT_NOT_FOUND);
                        break;                	
                    }

	            if (msg->iofs->io_Union.io_OPEN_FILE.io_Filename[0])
		    {
			SendBack (msg, ERROR_OBJECT_NOT_FOUND);
			break;
		    }

	            D(bug("[xpipe] Cloning pipe end: %p with mode %d\n", msg->iofs->IOFS.io_Unit, msg->iofs->io_Union.io_OPEN_FILE.io_FileMode));

		    LONG error = DuplicatePipeEnd (
			(struct XPipeEnd**) &msg->iofs->IOFS.io_Unit, 
			msg->iofs->io_Union.io_OPEN_FILE.io_FileMode
		    );
		    D(bug("[xpipe] Cloned pipe end: %p\n", msg->iofs->IOFS.io_Unit));
		    SendBack (msg, error);
		    break;
		}	       
                case FSA_PIPE: 
                {
                    struct XPipeEnd *reader, *writer;
                    struct XPipe *pipe;
                    struct XPipeUnit *un;

                    D(bug("[xpipe] Cmd is FSA_PIPE\n"));
                    
                    if(pn->ln_Type != NT_PIPEROOT)
                    {
                        SendBack (msg, ERROR_OBJECT_WRONG_TYPE);
                        break;                	
                    }
                    un = (struct XPipeUnit*) pn;

                    if ((reader = AllocVec (sizeof(struct XPipeEnd), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) 
                    {
                        SendBack (msg, ERROR_NO_FREE_STORE);
                        break;
                    }
                    reader->node.ln_Type = NT_PIPEEND;
                    reader->mode = FMF_READ;
                    reader->bufpos = 0;
                    
                    if ((writer = AllocVec (sizeof(struct XPipeEnd), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) 
                    {
                	FreeVec (reader);
                        SendBack (msg, ERROR_NO_FREE_STORE);
                        break;
                    }
                    writer->node.ln_Type = NT_PIPEEND;
                    writer->mode = FMF_WRITE;
                    writer->bufpos = 0;
                    
                    if ((pipe = AllocVec (sizeof(struct XPipe), MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
                    {
                	FreeVec (reader);
                	FreeVec (writer);
                	SendBack (msg, ERROR_NO_FREE_STORE);
                	break;
                    }
                    reader->pipe = pipe;
                    writer->pipe = pipe;
                    NEWLIST (&pipe->readers);
                    NEWLIST (&pipe->pendingreads);
                    ADDTAIL (&pipe->readers, reader);
                    NEWLIST (&pipe->writers);
                    NEWLIST (&pipe->pendingwrites);
                    ADDTAIL (&pipe->writers, writer);
                    ADDTAIL (&un->pipes, pipe);
                    
                    D(bug("[xpipe] Opened pipe with read end: %p and write end: %p\n", reader, writer));

                    msg->iofs->IOFS.io_Unit = (struct Unit *) reader;
                    msg->iofs->io_Union.io_PIPE.io_Writer = (struct Unit *) writer;

                    SendBack (msg, 0);

                    break;
                }
                    
		case FSA_CLOSE:
		{
		    D(bug("[xpipe] Cmd is FSA_CLOSE\n"));

                    if(pn->ln_Type != NT_PIPEEND)
                    {
                        SendBack (msg, ERROR_OBJECT_NOT_FOUND);
                        break;                	
                    }
		    struct XPipeEnd *pipeend = (struct XPipeEnd*) pn;
		    struct XPipe *pipe = pipeend->pipe;

		    D(bug("[xpipe] Closing pipe end %p\n", pipeend));

		    Remove ((struct Node*) pipeend);
		    FreeVec (pipeend);
		    
		    int numreaders, numwriters;
		    ListLength (&pipe->readers, numreaders);
		    ListLength (&pipe->writers, numwriters);
		    
		    /* If all writing ends are closed we have EOF, so finish all pending read requests */
		    if(numwriters == 0)
		    {
			D(bug("[xpipe] Processing pending reads\n"));
			struct XPipeMessage *readmsg;
			struct Node *tempnode;
			ForeachNodeSafe (&pipe->pendingreads, readmsg, tempnode)
			{
			    D(bug("[xpipe] Pending read msg %p\n", readmsg));
			    readmsg->iofs->io_Union.io_READ_WRITE.io_Length = readmsg->curlen;
			    Remove((struct Node *)readmsg);
			    SendBack (readmsg, 0);
			}
		    }
		    
		    /* If there are no pipe ends left, close the pipe */
		    if (numreaders == 0 && numwriters == 0)
		    {
			D(bug("[xpipe] No ends left, closing the pipe\n"));
			Remove ((struct Node*) pipe);
			FreeVec (pipe);
		    }

		    SendBack (msg, 0);
		    break;
		}
		case FSA_EXAMINE:
   		{
		    struct ExAllData  *ead        = msg->iofs->io_Union.io_EXAMINE.io_ead;
                    const ULONG        type       = msg->iofs->io_Union.io_EXAMINE.io_Mode;
                    const ULONG        size       = msg->iofs->io_Union.io_EXAMINE.io_Size;
                    STRPTR             next, end;

                    static const ULONG sizes[]=
		    {
			0,
			offsetof(struct ExAllData,ed_Type),
    			offsetof(struct ExAllData,ed_Size),
    			offsetof(struct ExAllData,ed_Prot),
    			offsetof(struct ExAllData,ed_Days),
    			offsetof(struct ExAllData,ed_Comment),
    			offsetof(struct ExAllData,ed_OwnerUID),
    			sizeof(struct ExAllData)
    		    };

		    D(bug("[xpipe] Cmd is EXAMINE\n"));

		    if (type > ED_OWNER)
    		    {
			D(bug("[xpipe] The user requested an invalid type\n"));
			SendBack (msg, ERROR_BAD_NUMBER);
			break;
		    }

    		    next = (STRPTR)ead + sizes[type];
    		    end  = (STRPTR)ead + size;

		    if(next>end)  /* > is correct. Not >= */
		    {
			SendBack (msg, ERROR_BUFFER_OVERFLOW);
			break;
		    }

		    switch(type)
		    {
        		case ED_OWNER:
	    		ead->ed_OwnerUID = 0;
	    		ead->ed_OwnerGID = 0;

			/* Fall through */
        		case ED_COMMENT:
	    		    ead->ed_Comment = NULL;

			/* Fall through */
        		case ED_DATE:
	    		    ead->ed_Days  = 0;
			    ead->ed_Mins  = 0;
	    		    ead->ed_Ticks = 0;

			/* Fall through */
        		case ED_PROTECTION:
	    		    ead->ed_Prot = 0;

			/* Fall through */
        		case ED_SIZE:
	    		    ead->ed_Size = 0;

			/* Fall through */
        		case ED_TYPE:
	    		    ead->ed_Type = ST_PIPEFILE;

			/* Fall through */
			case ED_NAME:
	  		{
	  		    ead->ed_Name = next;
			    ead->ed_Name[0] = '\0';
			}
    		    }

		    ead->ed_Next = NULL;

		    SendBack (msg, 0);
		    break;
		}
		case FSA_WRITE:
		{
		    D(bug("[xpipe] Cmd is FSA_WRITE.\n"));
		    
                    if(pn->ln_Type != NT_PIPEEND)
                    {
                        SendBack (msg, ERROR_OBJECT_NOT_FOUND);
                        break;                	
                    }
		    struct XPipeEnd *writer = (struct XPipeEnd *) pn;
		    
		    D(bug("[xpipe] Writer end %p\n", writer));
		    struct XPipe *pipe = writer->pipe;
		    int numreaders;
		    int length = msg->iofs->io_Union.io_READ_WRITE.io_Length;
		    D(bug("[xpipe] length is %d.\n", length));
		    
		    if (!(writer->mode & FMF_WRITE))
		    {
		        D(bug("[xpipe] User tried to write to the wrong end of the pipe.\n"));
			SendBack (msg, ERROR_WRITE_PROTECTED);
		        break;
		    }
		    
		    ListLength (&pipe->readers, numreaders);
		    if (numreaders == 0)
		    {
			D(bug("[xpipe] There are no open read ends: PIPE BROKEN.\n"));
			SendBack (msg, ERROR_BROKEN_PIPE);
		        break;
		    }
		    
		    if (WriteWouldBlock (pipe, length))
		    {
			if(writer->mode & FMF_NONBLOCK)
			{
			    D(bug("[xpipe] There is not enough space and the pipe is in nonblocking mode, so return EWOULDBLOCK\n"));
			    SendBack (msg, ERROR_WOULD_BLOCK);
			    break;
			}

			/* Write as much as we can, enqueue the request and reply when it's finished */
			D(bug("[xpipe] Enqueing the message\n"));
			AddTail (&pipe->pendingwrites, (struct Node *)msg);
		    }

		    msg->curlen = WriteToPipe (
			pipe, 
			msg->iofs->io_Union.io_READ_WRITE.io_Buffer,
			length
		    );
		    D(bug("[xpipe] Wrote %d bytes from requested %d\n", msg->curlen, length));
		    
		    if(length == msg->curlen)
		    {
			/* Managed to write everything */
			SendBack(msg, 0);
		    }

		    if(msg->curlen != 0)
			pump(pipe, PROCESS_READS);

		    break;
		}
		case FSA_READ:
		{
		    D(bug("[xpipe] Cmd is FSA_READ.\n"));

                    if(pn->ln_Type != NT_PIPEEND)
                    {
                        SendBack (msg, ERROR_OBJECT_NOT_FOUND);
                        break;                	
                    }
		    struct XPipeEnd *reader = (struct XPipeEnd *) pn;

		    D(bug("[xpipe] Reader end %p\n", reader));
		    struct XPipe *pipe = reader->pipe;
		    int numwriters;
		    ListLength(&pipe->writers, numwriters);
		    int length = msg->iofs->io_Union.io_READ_WRITE.io_Length;
		    D(bug("[xpipe] length is %d.\n", length));
		    
		    if (!(reader->mode & FMF_READ))
		    {
		        D(bug("[xpipe] User tried to read from the wrong end of the pipe.\n"));
			SendBack(msg, ERROR_READ_PROTECTED);
		        break;
		    }
		        
		    if (ReadWouldBlock (pipe, length))
		    {
			if(reader->mode & FMF_NONBLOCK)
			{
			    D(bug("[xpipe] There is not enough data to read and the pipe is in nonblocking mode, so return EWOULDBLOCK\n"));
			    SendBack(msg, ERROR_WOULD_BLOCK);
			    break;
			}

			/* Read as much as we can, enqueue the request and reply when it's finished */
			D(bug("[xpipe] Enqueing the message\n"));
			AddTail (&pipe->pendingreads, (struct Node *)msg);
		    }

		    msg->curlen = ReadFromPipe (
			pipe, 
			(APTR) msg->iofs->io_Union.io_READ_WRITE.io_Buffer,
			length
		    );	    
		    D(bug("[xpipe] Read %d bytes\n", msg->curlen));

		    if(length == msg->curlen || numwriters == 0)
		    {
			/* Managed to read everything or there are no more writers (EOF) */
			msg->iofs->io_Union.io_READ_WRITE.io_Length = msg->curlen;
			SendBack (msg, 0);
		    }
		    
		    if(msg->curlen != 0)
			pump(pipe, PROCESS_WRITES);

		    break;
		}
	    }
	}
    } while (cont);

    return 0;

    AROS_USERFUNC_EXIT
}
