/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.2  2001/07/15 20:16:38  falemagn
    Implemented named pipes. Actually there are ONLY named pipes. The standard AmigaOS PIPE: can be implemented assigning PIPE: to PIPEFS:namedpipe. pipe() support is about to come


    Desc: A PIPE filesystems, in which named and unnamed pipes can be created.
    Lang: English

    History:

    2001/07/14 falemagn created

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
#include "pipefs_handler_gcc.h"
#endif

#include <string.h>
#include <stddef.h>

//#define kprintf(x...)

static const char name[];
static const char version[];
static const APTR inittabl[4];
static void *const functable[];
struct pipefsbase *AROS_SLIB_ENTRY(init,pipefs_handler)();
void AROS_SLIB_ENTRY(open,pipefs_handler)();
BPTR AROS_SLIB_ENTRY(close,pipefs_handler)();
BPTR AROS_SLIB_ENTRY(expunge,pipefs_handler)();
int AROS_SLIB_ENTRY(null,pipefs_handler)();
void AROS_SLIB_ENTRY(beginio,pipefs_handler)();
LONG AROS_SLIB_ENTRY(abortio,pipefs_handler)();
static const char end;

AROS_UFP3(LONG, pipefsproc,
    AROS_UFPA(char *,argstr,A0),
    AROS_UFPA(ULONG,argsize,D0),
    AROS_UFPA(struct ExecBase *,SysBase,A6));

ULONG SendRequest(struct pipefsbase *pipefsbase, struct IOFileSys *iofs);


struct pipefsmessage
{
    struct Message    msg;
    struct IOFileSys *iofs;
    LONG              curlen;
};

struct dirnode
{
    struct Node node;
    struct dirnode         *parent;     /* Parent directory */
    ULONG                   numusers;
    struct SignalSemaphore  filesSem;
    struct List files;
};

struct filenode
{
    struct Node     node;
    struct dirnode *parent;
    ULONG           numusers;           /* Number of actual users of this pipe */
    ULONG           numwriters;         /* Num of actual writers */
    ULONG           numreaders;         /* Num of actual readers */
    struct List     pendingwrites;      /* List of pending write requestes */
    struct List     pendingreads;       /* List of pending read requestes */
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

const struct Resident pipefs_handler_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&pipefs_handler_resident,
    (APTR)&end,
    RTF_AUTOINIT,
    41,
    NT_DEVICE,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

static const char name[]="pipefs.handler";

static const char version[]="$VER: pipefs-handler 41.1 (8.6.96)\r\n";

static const APTR inittabl[4]=
{
    (APTR)sizeof(struct pipefsbase),
    (APTR)functable,
    NULL,
    &AROS_SLIB_ENTRY(init,pipefs_handler)
};

static void *const functable[]=
{
    &AROS_SLIB_ENTRY(open,pipefs_handler),
    &AROS_SLIB_ENTRY(close,pipefs_handler),
    &AROS_SLIB_ENTRY(expunge,pipefs_handler),
    &AROS_SLIB_ENTRY(null,pipefs_handler),
    &AROS_SLIB_ENTRY(beginio,pipefs_handler),
    &AROS_SLIB_ENTRY(abortio,pipefs_handler),
    (void *)-1
};

AROS_LH2(struct pipefsbase *, init,
AROS_LHA(struct pipefsbase *, pipefsbase, D0),
AROS_LHA(BPTR,              segList,  A0),
	 struct ExecBase *, sysBase, 0, pipefs_handler)
{
    AROS_LIBFUNC_INIT

    /* Store arguments */
    SysBase =  sysBase;
    DOSBase =  (struct DosLibrary *)OpenLibrary("dos.library",39);
    pipefsbase->seglist=segList;

    if(DOSBase)
    {
	struct TagItem taglist[]=
	{
	 {NP_Entry,              (IPTR)pipefsproc},
	 {NP_Name, (IPTR)"pipefs.handler process"},
	 {NP_UserData,           (IPTR)pipefsbase},
	 {TAG_DONE,                           0}
	};

	pipefsbase->proc = CreateNewProc(taglist);

       	if (pipefsbase->proc)
	    return pipefsbase;

        CloseLibrary((struct Library *)DOSBase);
    }

    return NULL;
    AROS_LIBFUNC_EXIT
}

AROS_LH3(void, open,
 AROS_LHA(struct IOFileSys *, iofs, A1),
 AROS_LHA(ULONG,              unitnum, D0),
 AROS_LHA(ULONG,              flags, D1),
	   struct pipefsbase *, pipefsbase, 1, pipefs_handler)
{
    AROS_LIBFUNC_INIT
    struct usernode *un;
    struct dirnode  *dn;

    /* Get compiler happy */
    unitnum=flags=0;

    /* Mark Message as recently used. */
    iofs->IOFS.io_Message.mn_Node.ln_Type=NT_REPLYMSG;

    /* Build a fake usernode */
    un = AllocVec(sizeof(*un),MEMF_PUBLIC|MEMF_CLEAR);
    if(un)
    {
	dn = AllocVec(sizeof(*dn),MEMF_PUBLIC|MEMF_CLEAR);
	if (dn)
	{
	    dn->node.ln_Type = (UBYTE)ST_ROOT;
	    dn->node.ln_Name = "PIPEFS";
	    dn->parent = NULL;

	    NEWLIST(&dn->files);
	    InitSemaphore(&dn->filesSem);

	    un->fn = (struct filenode *)dn;

	    iofs->IOFS.io_Unit=(struct Unit *)un;
            iofs->IOFS.io_Device=&pipefsbase->device;

	    /* I have one more opener. */
            pipefsbase->device.dd_Library.lib_OpenCnt++;

	    pipefsbase->device.dd_Library.lib_Flags&=~LIBF_DELEXP;
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
	   struct pipefsbase *, pipefsbase, 2, pipefs_handler)
{
    AROS_LIBFUNC_INIT
    struct usernode *un;
    struct dirnode  *dn;

    un = (struct usernode *)iofs->IOFS.io_Unit;
    dn = (struct dirnode *)un->fn;

    if(!IsListEmpty(&dn->files))
    {
	iofs->io_DosError=ERROR_OBJECT_IN_USE;
	return 0;
    }

    /* Let any following attemps to use the device crash hard. */
    iofs->IOFS.io_Device=(struct Device *)-1;

    FreeVec(dn);
    FreeVec(un);

    iofs->io_DosError=0;

    /* I have one fewer opener. */
    if(!--pipefsbase->device.dd_Library.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(pipefsbase->device.dd_Library.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the device */
	    return expunge();
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct pipefsbase *, pipefsbase, 3, pipefs_handler)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(pipefsbase->device.dd_Library.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	pipefsbase->device.dd_Library.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    SendRequest(pipefsbase, NULL);

    /* Free all resources */
    CloseLibrary((struct Library *)pipefsbase->dosbase);

    /* Get rid of the device. Remove it from the list. */
    Remove(&pipefsbase->device.dd_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=pipefsbase->seglist;

    /* Free the memory. */
    FreeMem((char *)pipefsbase-pipefsbase->device.dd_Library.lib_NegSize,
	    pipefsbase->device.dd_Library.lib_NegSize+pipefsbase->device.dd_Library.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct pipefsbase *, pipefsbase, 4, pipefs_handler)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, beginio,
 AROS_LHA(struct IOFileSys *, iofs, A1),
	   struct pipefsbase *, pipefsbase, 5, pipefs_handler)
{
    AROS_LIBFUNC_INIT
    LONG error=0;
    BOOL enqueued = FALSE;

    kprintf("COMMAND %d\n", iofs->IOFS.io_Command);
    switch(iofs->IOFS.io_Command)
    {
	case FSA_OPEN:
	case FSA_OPEN_FILE:
        case FSA_EXAMINE:
	case FSA_READ:
	case FSA_WRITE:
	case FSA_CLOSE:
	    error = SendRequest(pipefsbase, iofs);
	    enqueued = !error;
	    break;

	case FSA_SEEK:
	    error = ERROR_SEEK_ERROR;
	    break;
        case FSA_SET_FILE_SIZE:
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
	   struct pipefsbase *, pipefsbase, 6, pipefs_handler)
{
    AROS_LIBFUNC_INIT
    /* Everything already done. */
    return 0;
    AROS_LIBFUNC_EXIT
}

ULONG SendRequest(struct pipefsbase *pipefsbase, struct IOFileSys *iofs)
{
    struct pipefsmessage *msg = AllocVec(sizeof(*msg), MEMF_PUBLIC);

    if (msg)
    {
        msg->msg.mn_Node.ln_Type = NT_MESSAGE;
	msg->msg.mn_Node.ln_Name = "PIPEFSMSG";
        msg->msg.mn_Length       = sizeof(struct pipefsmessage);
	msg->iofs                = iofs;

	if (iofs)
	{
	    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	    iofs->IOFS.io_Flags &= ~IOF_QUICK;
     	}

	PutMsg(&pipefsbase->proc->pr_MsgPort, (struct Message *)msg);

	return 0;
    }

    return ERROR_NO_FREE_STORE;
}

/* The helper process */

#define SendBack(msg)                        \
{                                            \
    ReplyMsg(&(msg)->iofs->IOFS.io_Message); \
    FreeVec(msg);                            \
}

static STRPTR SkipColon(STRPTR str)
{
    STRPTR oldstr = str;

    while(str[0])
        if (str++[0] == ':') return str;

    return oldstr;
}

/*
  Return the len of the first part in the path.

  EXAMPLE
    LenFirstPart("yyy/xxx") would return 3
    LenFirstPart("xxxx") would return 4
*/

static STRPTR StrDup(struct pipefsbase *pipefsbase, STRPTR str)
{
    size_t len = strlen(str)+1;
    STRPTR ret = AllocVec(len, MEMF_ANY);

    if (ret)
        CopyMem(str, ret, len);

    return ret;
}

static size_t LenFirstPart(STRPTR path)
{
    size_t len = 0;

    for (; path[0] && path[0] != '/'; path++, len++);

    return len;
}

static struct filenode *FindFile(struct dirnode **dn_ptr, STRPTR path)
{
    #define dn (*dn_ptr)
    size_t len;
    STRPTR nextpart;
    struct filenode *fn;

    while (path[0] == '/' && dn)
    {
        dn = dn->parent;
	path++;
    }

    if (!dn || (BYTE)dn->node.ln_Type <= 0) return NULL;

    len      = LenFirstPart(path);
    nextpart = &path[len];
    fn       = GetHead(&dn->files);

    kprintf("Searching for %.*S.\n", len, path);

    while (fn)
    {
	kprintf("Comparing %S with %.*S.\n", fn->node.ln_Name, len, path);
	if
	(
	    strlen(fn->node.ln_Name) == len               &&
	    strncasecmp(fn->node.ln_Name, path, len) == 0
	)
	{
	    break;
	}
        fn = GetSucc((struct Node *)fn);
    }

    if (fn)
    {
	if (nextpart[0] == '/') nextpart++;
        if (nextpart[0])
	{
	    if ((BYTE)fn->node.ln_Type <= 0)
            {
                kprintf("User wants %.*S to be a directory, but it's a file.\n", len, path);
	        dn = NULL;
	        fn = NULL;
            }
	    else
	    {
		dn = (struct dirnode *)fn;
		fn = FindFile(&dn, nextpart);
	    }
	}
    }

    return fn;

    #undef dn
}

static struct filenode *GetFile(struct pipefsbase *pipefsbase, struct IOFileSys *iofs)
{
    STRPTR filename       = SkipColon(iofs->io_Union.io_NamedFile.io_Filename);
    struct usernode *un   = (struct usernode *)iofs->IOFS.io_Unit;
    struct filenode *fn   = un->fn;
    ULONG            mode = iofs->io_Union.io_OPEN.io_FileMode;

    kprintf("User wants to open file %S.\n", filename);
    kprintf("Current directory is %S\n", fn->node.ln_Name);

    if (filename[0])
    {
	struct dirnode *dn = (struct dirnode *)fn;

	fn = FindFile(&dn, filename);
        if (!fn)
	{
	    kprintf("The file couldn't be found.\n");

	    if (dn && mode&FMF_CREATE)
	    {
		kprintf("But the user wants it to be created.\n");

		fn = AllocVec(sizeof(*fn), MEMF_PUBLIC|MEMF_CLEAR);
		if (fn)
		{
		    fn->node.ln_Name = StrDup(pipefsbase, FilePart(filename));

		    if (fn->node.ln_Name)
		    {
		        fn->node.ln_Type = (UBYTE)ST_PIPEFILE;

			NEWLIST(&fn->pendingwrites);
			NEWLIST(&fn->pendingreads);

			fn->parent = dn;

			AddTail(&dn->files, (struct Node *)fn);
			kprintf("New file created and added to the list\n");

		    	return fn;
		    }

		    FreeVec(fn);
		}

		kprintf("AllocVec Failed. No more memory available\n");
		iofs->io_DosError = ERROR_NO_FREE_STORE;
		return NULL;
            }
	    else
	    {
	        iofs->io_DosError = ERROR_OBJECT_NOT_FOUND;
	        return NULL;
	    }
        }
    }

    if ((BYTE)fn->node.ln_Type > 0 && mode&(FMF_WRITE|FMF_READ))
    {
	kprintf("The file is a directory, cannot be open for reading/writing\n");
	iofs->io_DosError = ERROR_OBJECT_WRONG_TYPE;
	return NULL;
    }

    return fn;
}

#undef SysBase
#ifndef kprintf
     struct ExecBase *SysBase;
#else
#    define SysBase _SysBase
#endif


AROS_UFH3(LONG, pipefsproc,
    AROS_UFHA(char *,argstr,A0),
    AROS_UFHA(ULONG,argsize,D0),
    AROS_UFHA(struct ExecBase *,_SysBase,A6))
{
    AROS_USERFUNC_INIT

    SysBase = _SysBase;

    struct Process       *me         = (struct Process *)FindTask(0);
    struct pipefsbase    *pipefsbase = me->pr_Task.tc_UserData;
    struct pipefsmessage *msg;
    struct usernode      *un;
    struct filenode      *fn;
    BOOL cont = TRUE;

    do
    {
    	WaitPort(&(me->pr_MsgPort));

	while
	(
	    (msg =(struct pipefsmessage *)GetMsg(&(me->pr_MsgPort))) &&
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
		    BOOL             stillwaiting;

		    kprintf("Command is OPEN\n");

		    fn = GetFile(pipefsbase, msg->iofs);
		    if (!fn)
		    {
			SendBack(msg);
			continue;
		    }

		    kprintf("File requested found.\n");
		    kprintf("The requested file is %s.\n",
		            (BYTE)fn->node.ln_Type <= 0  ?
			    "a pipe":
			    "a directory");

		    un = AllocVec(sizeof(*un), MEMF_PUBLIC);
		    if (!un)
		    {
		        msg->iofs->io_DosError = ERROR_NO_FREE_STORE;
			SendBack(msg);
			continue;
		    }

		    fn->numusers++;

		    un->mode = msg->iofs->io_Union.io_OPEN.io_FileMode;
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
			    struct pipefsmessage *msg;

			    kprintf("Finally there are enough readers and writers! "
			            "Wake up all of them\n");

			    while ((msg = (struct pipefsmessage *)RemHead(&fn->waitinglist)))
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
			struct pipefsmessage *msg;

			kprintf("There are no writers anymore. %s\n",
			        IsListEmpty(&fn->pendingreads) ?
				"There are no pending reads"   :
			        "Reply to all the waiting readers");
			while ((msg = (struct pipefsmessage *)RemHead(&fn->pendingreads)))
			{
			    msg->iofs->io_Union.io_READ_WRITE.io_Length =
			    msg->iofs->io_Union.io_READ_WRITE.io_Length - msg->curlen;
			    SendBack(msg);
			}
		    }
		    if (un->mode&FMF_READ && !fn->numreaders)
		    {
			struct pipefsmessage *msg;

			kprintf("There are no readers anymore. %s\n",
			        IsListEmpty(&fn->pendingwrites) ?
				"There are no pending writes"   :
			        "Reply to all the waiting writers");

			while ((msg = (struct pipefsmessage *)RemHead(&fn->pendingwrites)))
			{
			    msg->iofs->io_DosError = ERROR_BROKEN_PIPE;
			    SendBack(msg);
			}
		    }

		    un->fn->numusers--;
		    FreeVec(un);
		    SendBack(msg);

		    continue;
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

		    kprintf("Command is EXAMINE\n");
		    kprintf("Examining file %S\n", fn->node.ln_Name);

		    if (type > ED_OWNER)
    		    {
			kprintf("The user requested an invalid type\n");
			msg->iofs->io_DosError = ERROR_BAD_NUMBER;
			SendBack(msg);
			continue;
		    }

    		    next = (STRPTR)ead + sizes[type];
    		    end  = (STRPTR)ead + size;

		    if(next>end)  /* > is correct. Not >= */
		    {
			msg->iofs->io_DosError = ERROR_BUFFER_OVERFLOW;
			SendBack(msg);
			continue;
		    }

    		    msg->iofs->io_DirPos = (LONG)fn;

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
	    		    ead->ed_Type = (UBYTE)fn->node.ln_Type;

			/* Fall through */
			case ED_NAME:
	    		    ead->ed_Name = fn->node.ln_Name;
    		    }

		    ead->ed_Next = (struct ExAllData *)(((IPTR)next + AROS_PTRALIGN - 1) & ~(AROS_PTRALIGN - 1));

		    SendBack(msg);
		    continue;
		}
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
		struct pipefsmessage *rmsg = (struct pipefsmessage *)GetHead(&fn->pendingreads);
		struct pipefsmessage *wmsg = (struct pipefsmessage *)GetHead(&fn->pendingwrites);

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
		    Remove((struct Node *)wmsg);
		    SendBack(wmsg);
		}

		if (!rmsg->curlen)
		{
		    kprintf("Reader: finished its job. Removing it from the list.\n");
		    Remove((struct Node *)rmsg);
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
