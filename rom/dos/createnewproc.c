/*
    (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: Create a new process
    Lang: English
*/
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/dostags.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include "dos_intern.h"

static void KillCurrentProcess(void);
struct Process *AddProcess(struct Process *process, STRPTR argPtr,
ULONG argSize, APTR initialPC, APTR finalPC, struct DosLibrary *DOSBase);

static void freeLocalVars(struct Process *process, struct DosLibrary *DOSBase);

BOOL copyVars(struct Process *fromProcess, struct Process *toProcess);

void internal_ChildWait(struct Task *task);
void internal_ChildFree(APTR tid);

#include <aros/debug.h>

/* Temporary macro */
#define P(x) 
/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(struct Process *, CreateNewProc,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 83, Dos)

/*  FUNCTION
	Create a new process using the tagitem array.

    INPUTS
	tags - information on the new process.

    RESULT
	Pointer to the new process or NULL on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Allocated resources */
    struct Process  	    	*process = NULL;
    BPTR            	    	 input = 0, output = 0, ces = 0, curdir = 0, homedir = 0;
    STRPTR          	    	 stack = NULL, name = NULL, argptr = NULL;
    ULONG           	    	 namesize = 0, argsize = 0;
    struct MemList  	    	*memlist = NULL;
    struct CommandLineInterface *cli = NULL;
    struct Process  	    	*me = (struct Process *)FindTask(NULL);
    STRPTR          	    	 s;
    BPTR            	    	*oldpath, *newpath, *nextpath;

    /* TODO: NP_CommandName, NP_ConsoleTask, NP_NotifyOnDeath */

#define TAGDATA_NOT_SPECIFIED ~0ul

    struct TagItem defaults[]=
    {
    /* 0 */    { NP_Seglist 	, 0                 	    	},
    /* 1 */    { NP_Entry   	, (IPTR)NULL        	    	},
    /* 2 */    { NP_Input   	, TAGDATA_NOT_SPECIFIED       	},
    /* 3 */    { NP_CloseInput	, 1           	    	    	},
    /* 4 */    { NP_Output  	, TAGDATA_NOT_SPECIFIED    	},
    /* 5 */    { NP_CloseOutput , 1           	    	    	},
    /* 6 */    { NP_Error   	, TAGDATA_NOT_SPECIFIED    	},
    /* 7 */    { NP_CloseError	, 1           	    	    	},
    /* 8 */    { NP_CurrentDir	, TAGDATA_NOT_SPECIFIED    	},
    /* 9 */    { NP_StackSize	, AROS_STACKSIZE    	        },
    /*10 */    { NP_Name    	, (IPTR)"New Process" 	    	},
    /*11 */    { NP_Priority	, me->pr_Task.tc_Node.ln_Pri 	},
    /*12 */    { NP_Arguments	, (IPTR)NULL  	    	    	},
    /*13 */    { NP_Cli     	, 0           	    	    	},
    /*14 */    { NP_UserData	, (IPTR)NULL  	    	    	},
    /*15 */    { NP_ExitCode	, (IPTR)NULL  	    	    	},
    /*16 */    { NP_ExitData	, (IPTR)NULL  	    	    	},
    /*17 */    { NP_WindowPtr	, (IPTR)NULL  	    	    	}, /* Default: default public screen */
    /*18 */    { NP_CopyVars	, (IPTR)TRUE  	    	    	},
    /*19 */    { NP_Synchronous , (IPTR)FALSE 	    	    	},
    /*20 */    { NP_FreeSeglist , (IPTR)TRUE  	    	    	},
    /*21 */    { NP_HomeDir 	, TAGDATA_NOT_SPECIFIED     	},
	       { TAG_END    	, 0           	    	    	}
    };

    /* C has no exceptions. This is a simple replacement. */
#define ERROR_IF(a)  if(a) goto error  /* Throw a generic error. */
#define ENOMEM_IF(a) if(a) goto enomem /* Throw out of memory. */

    /* Inherit the parent process' stacksize if possible */
    if (__is_process(me))
    {
	struct CommandLineInterface *cli = Cli();

	if (cli)
	{
    	    LONG parentstack = cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;

	    if (parentstack > AROS_STACKSIZE)
	    {
	        defaults[9].ti_Data = parentstack;
	    }
	}
    }

    ApplyTagChanges(defaults, tags);

    process = (struct Process *)AllocMem(sizeof(struct Process),
					 MEMF_PUBLIC | MEMF_CLEAR);
    ENOMEM_IF(process == NULL);

    /* Do this early to ease implementation of failure code */
    NEWLIST((struct List *)&process->pr_LocalVars);

    /* We need a minimum stack to handle interrupt contexts */
    if (defaults[9].ti_Data < AROS_STACKSIZE)
    {
	defaults[9].ti_Data = AROS_STACKSIZE;
    }

    stack = AllocMem(defaults[9].ti_Data, MEMF_PUBLIC);
    ENOMEM_IF(stack == NULL);

    s = (STRPTR)defaults[10].ti_Data;

    while (*s++);

    namesize = s - (STRPTR)defaults[10].ti_Data;

    name = AllocMem(namesize, MEMF_PUBLIC);
    ENOMEM_IF(name == NULL);

    /* NP_Arguments */
    s = (STRPTR)defaults[12].ti_Data;

    if (s != NULL)
    {
	while(*s++);

	argsize = s - (STRPTR)defaults[12].ti_Data;
	argptr  = (STRPTR)AllocVec(argsize, MEMF_PUBLIC);
	ENOMEM_IF(argptr == NULL);
    }

    memlist = AllocMem(sizeof(struct MemList) + 2*sizeof(struct MemEntry),
		       MEMF_ANY);
    ENOMEM_IF(memlist == NULL);

    if (defaults[13].ti_Data != NULL)
    {
	/* Don't forget to pass tags to AllocDosObject() */
	cli = (struct CommandLineInterface *)AllocDosObject(DOS_CLI, tags);
	ENOMEM_IF(cli == NULL);

	oldpath = NULL;
	cli->cli_DefaultStack = (defaults[9].ti_Data + CLI_DEFAULTSTACK_UNIT - 1) / CLI_DEFAULTSTACK_UNIT;

	if (__is_process(me))
	{
	    struct CommandLineInterface *oldcli = Cli();

	    if (oldcli != NULL)
	    {
		LONG oldlen = AROS_BSTR_strlen(oldcli->cli_Prompt);
		LONG newlen = GetTagData(ADO_PromptLen, 255, tags);

		oldpath = BADDR(oldcli->cli_CommandDir);

		CopyMem(BADDR(oldcli->cli_Prompt), BADDR(cli->cli_Prompt), (newlen<oldlen?newlen:oldlen) + 1);
	    }
	}

	newpath = &cli->cli_CommandDir;

	/* Add substitute for lock chain */
	while (oldpath != NULL)
	{
	    nextpath = AllocVec(2*sizeof(BPTR), MEMF_CLEAR);
	    ENOMEM_IF(nextpath == NULL);

	    newpath[0]  = MKBADDR(nextpath);
	    nextpath[1] = DupLock(oldpath[1]);
	    ERROR_IF(!nextpath[1]);

	    newpath = nextpath;
	    oldpath = BADDR(oldpath[0]);
	}
    }

    /* NP_Input */

    if (defaults[2].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
	if (__is_process(me))
	{
	    input = Open("NIL:", MODE_OLDFILE);
	    ERROR_IF(!input);

	    defaults[2].ti_Data = (IPTR)input;
	}
	else
	{
	    defaults[2].ti_Data = 0;
	}
    }

    /* NP_Output */

    if (defaults[4].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
	if (__is_process(me))
	{
	    output = Open("NIL:", MODE_NEWFILE);
	    ERROR_IF(!output);

	    defaults[4].ti_Data = (IPTR)output;
	}
	else
	{
	    defaults[4].ti_Data = 0;
	}
    }

    /* NP_Error */

    if (defaults[6].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
	if (__is_process(me))
	{
	    ces = Open("NIL:", MODE_NEWFILE);
	    ERROR_IF(!ces);

	    defaults[6].ti_Data = (IPTR)ces;
	}
	else
	{
	    defaults[6].ti_Data = 0;
	}
    }

    if (defaults[6].ti_Data) SetVBuf(defaults[6].ti_Data, NULL, BUF_NONE, -1);

    /* NP_CurrentDir */

    if (defaults[8].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
	if (__is_process(me))
	{
	    curdir = Lock("", SHARED_LOCK);
	    ERROR_IF(!curdir);

	    defaults[8].ti_Data = (IPTR)curdir;
	}
	else
	{
	    defaults[8].ti_Data = 0;
	}
    }
    
    /* NP_HomeDir */
    
    if (defaults[21].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
    	defaults[21].ti_Data = 0;
    	
    	if (__is_process(me))
	{
	    if (me->pr_HomeDir)
	    {
	    	homedir = DupLock(me->pr_HomeDir);
		ERROR_IF(!homedir);
		
		defaults[21].ti_Data = (IPTR)homedir;
	    }
	}
    }
    
    CopyMem((APTR)defaults[10].ti_Data, name, namesize);
    CopyMem((APTR)defaults[12].ti_Data, argptr, argsize);
    
    process->pr_Task.tc_Node.ln_Type = NT_PROCESS;
    process->pr_Task.tc_Node.ln_Name = name;
    process->pr_Task.tc_Node.ln_Pri = defaults[11].ti_Data;
    process->pr_Task.tc_SPLower = stack;
    process->pr_Task.tc_SPUpper = stack + defaults[9].ti_Data;

/*  process->pr_ReturnAddr; */
    NEWLIST(&process->pr_Task.tc_MemEntry);

    memlist->ml_NumEntries = 3;
    memlist->ml_ME[0].me_Addr = process;
    memlist->ml_ME[0].me_Length = sizeof(struct Process);
    memlist->ml_ME[1].me_Addr = stack;
    memlist->ml_ME[1].me_Length = defaults[9].ti_Data;
    memlist->ml_ME[2].me_Addr = name;
    memlist->ml_ME[2].me_Length = namesize;

    AddHead(&process->pr_Task.tc_MemEntry, &memlist->ml_Node);

    process->pr_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
    process->pr_MsgPort.mp_Flags = PA_SIGNAL;
    process->pr_MsgPort.mp_SigBit = SIGB_DOS;
    process->pr_MsgPort.mp_SigTask = process;

    NEWLIST(&process->pr_MsgPort.mp_MsgList);

    process->pr_SegList = (BPTR)defaults[0].ti_Data;
    process->pr_StackSize = defaults[9].ti_Data;
    process->pr_GlobVec = NULL;	                   /* Unused BCPL crap */
    process->pr_StackBase = MKBADDR(process->pr_Task.tc_SPUpper);
    process->pr_Result2 = 0;
    process->pr_CurrentDir = (BPTR)defaults[8].ti_Data;
    process->pr_CIS = (BPTR)defaults[2].ti_Data;
    process->pr_COS = (BPTR)defaults[4].ti_Data;
    process->pr_CES = (BPTR)defaults[6].ti_Data;
    process->pr_Task.tc_UserData = (APTR)defaults[14].ti_Data;

/*  process->pr_ConsoleTask=; */
/*  process->pr_FileSystemTask=; */
    process->pr_CLI = MKBADDR(cli);

    /* Set the name of this program */
    internal_SetProgramName(cli, name, DOSBase);
    D(bug("Calling internal_SetProgramName() with name = %s\n", name));

    process->pr_PktWait = NULL;
    process->pr_WindowPtr = (struct Window *)defaults[17].ti_Data; 
    process->pr_HomeDir = (BPTR)defaults[21].ti_Data;
    process->pr_Flags = (defaults[3].ti_Data  ? PRF_CLOSEINPUT  : 0) |
		        (defaults[5].ti_Data  ? PRF_CLOSEOUTPUT : 0) |
		        (defaults[7].ti_Data  ? PRF_CLOSEERROR  : 0) |
		        (defaults[13].ti_Data ? PRF_FREECLI     : 0) |
	                (defaults[19].ti_Data ? PRF_SYNCHRONOUS : 0) |
			(defaults[20].ti_Data ? PRF_FREESEGLIST : 0) |
		        PRF_FREEARGS | PRF_FREECURRDIR;
    process->pr_ExitCode = (APTR)defaults[15].ti_Data; 
    process->pr_ExitData = defaults[16].ti_Data; 
    process->pr_Arguments = argptr;

    if ((BOOL)defaults[18].ti_Data)      /* NP_CopyVars */
    {
	BOOL res = copyVars(me, process);
	
	ENOMEM_IF(res == FALSE);
    }

    process->pr_ShellPrivate = 0;

    if (AddProcess(process, argptr, argsize, 
		   defaults[0].ti_Data ?
		   (BPTR *)BADDR(defaults[0].ti_Data) + 1 :
		   (BPTR *)defaults[1].ti_Data,
		   KillCurrentProcess, DOSBase) != NULL)
    {
	/* NP_Synchronous */
	if (defaults[19].ti_Data)
	{
	    P(kprintf("Calling ChildWait()\n"));
	    internal_ChildWait(FindTask(NULL));
	    P(kprintf("Returned from ChildWait()\n"));
	}

	return process;
    }

    /* Fall through */
enomem:
    if (__is_process(me))
    {
	SetIoErr(ERROR_NO_FREE_STORE);
    }

    freeLocalVars(process, DOSBase);

error:
    if (cli != NULL)
    {
	FreeDosObject(DOS_CLI, cli);
    }

    if (homedir != NULL)
    {
    	UnLock(homedir);
    }

    if (curdir != NULL)
    {
	UnLock(curdir);
    }

    if (output != NULL)
    {
	Close(output);
    }

    if (input != NULL)
    {
	Close(input);
    }

    if (ces != NULL)
    {
	Close(ces);
    }

    if (argptr != NULL)
    {
	FreeVec(argptr);
    }

    if (memlist != NULL)
    {
	FreeMem(memlist, sizeof(struct MemList) + 2*sizeof(struct MemEntry));
    }

    if (name != NULL)
    {
	FreeMem(name, namesize);
    }

    if (stack != NULL)
    {
	FreeMem(stack, defaults[9].ti_Data);
    }

    if (process != NULL)
    {
	FreeMem(process, sizeof(struct Process));
    }

    return NULL;

    AROS_LIBFUNC_EXIT
} /* CreateNewProc */



static void KillCurrentProcess(void)
{
    /* I need the global here because there is no local way to get it */
    extern struct DosLibrary *DOSBase;
    struct Process *me = (struct Process *)FindTask(NULL);

    /* Call user defined exit function before shutting down. */
    if (me->pr_ExitCode != NULL)
    {
	me->pr_ExitCode(me->pr_ExitData);
    }

    P(kprintf("Deleting local variables\n"));

    /* Clean up */
    freeLocalVars(me, DOSBase);

    P(kprintf("Closing input stream\n"));

    if (me->pr_Flags & PRF_CLOSEINPUT)
    {
	Close(me->pr_CIS);
    }

    P(kprintf("Closing output stream\n"));

    if (me->pr_Flags & PRF_CLOSEOUTPUT)
    {
	Close(me->pr_COS);
    }

    P(kprintf("Closing error stream\n"));

    if (me->pr_Flags & PRF_CLOSEERROR)
    {
	Close(me->pr_CES);
    }

    P(kprintf("Freeing arguments\n"));

    if (me->pr_Flags & PRF_FREEARGS)
    {
	FreeVec(me->pr_Arguments);
    }

    P(kprintf("Unloading segment\n"));

    if (me->pr_Flags & PRF_FREESEGLIST)
    {
	UnLoadSeg(me->pr_SegList);
    }

    P(kprintf("Unlocking current dir\n"));

    if (me->pr_Flags & PRF_FREECURRDIR)
    {
	UnLock(me->pr_CurrentDir);
    }

    P(kprintf("Unlocking home dir\n"));
    UnLock(me->pr_HomeDir);

    P(kprintf("Freeing cli structure\n"));

    if (me->pr_Flags & PRF_FREECLI)
    {
	FreeDosObject(DOS_CLI, BADDR(me->pr_CLI));
    }

    /* To implement NP_Synchronous and NP_NotifyOnDeath I need Child***()
       here */

    // if(me->pr_Flags & PRF_NOTIFYONDEATH)
    //     Signal(GetETask(me)->iet_Parent, SIGF_CHILD);

    if (me->pr_Flags & PRF_SYNCHRONOUS)
    {
	P(kprintf("Calling ChildFree()\n"));

	// ChildStatus(me);
	internal_ChildFree(me);
    }

    removefromrootnode(me, DOSBase);

    RemTask(NULL);
}


static void freeLocalVars(struct Process *process, struct DosLibrary *DOSBase)
{
    struct LocalVar *varNode;
    struct Node     *tempNode;
    
    ForeachNodeSafe((struct List *)&process->pr_LocalVars,
		    (struct Node *)varNode, tempNode)
    {
	P(kprintf("Freeing variable %s with value %s at %p\n",
		  varNode->lv_Node.ln_Name, varNode->lv_Value, varNode));
	FreeMem(varNode->lv_Value, varNode->lv_Len);
	Remove((struct Node *)varNode);
	FreeVec(varNode);
    }
}


void internal_ChildWait(struct Task *task)
{
    task->tc_State = TS_WAIT;
    Reschedule(task);
    Switch();
}


void internal_ChildFree(APTR tid)
{
    struct Task *task = (struct Task *)tid;

    // Parent may now run again
    ((struct Task *)(GetETask(task)->et_Parent))->tc_State = TS_READY;

    kprintf("Setting parent task %p (called %s) to TS_READY\n",
	    ((struct Task *)(GetETask(task)->et_Parent)),
	    task->tc_Node.ln_Name);

    // This is OK to do as we know that the parent is blocked
    Forbid();
    Remove((struct Node *)(GetETask(task)->et_Parent));
    Permit();

    Reschedule(((struct Task *)(GetETask(task)->et_Parent)));
    Switch();
}


BOOL copyVars(struct Process *fromProcess, struct Process *toProcess)
{
    /* We must have variables to copy... */
    if (__is_process(fromProcess))
    {
	struct LocalVar *varNode;
	struct LocalVar *newVar;
	
	/* We use the same strategy as in the�***Var() functions */
	ForeachNode((struct List *)&fromProcess->pr_LocalVars, 
		    (struct Node *)varNode)
	{
	    LONG  copyLength = strlen(varNode->lv_Node.ln_Name) + 1 +
		sizeof(struct LocalVar);
	    
	    newVar = (struct LocalVar *)AllocVec(copyLength,
						 MEMF_PUBLIC | MEMF_CLEAR);
	    if (newVar == NULL)
	    {
		return FALSE;
	    }
	    
	    CopyMem(varNode, newVar, copyLength);
	    newVar->lv_Node.ln_Name = (char *)newVar +
		sizeof(struct LocalVar);
	    P(kprintf("Variable with name %s copied.\n", 
		      newVar->lv_Node.ln_Name));
	    
	    newVar->lv_Value = AllocMem(varNode->lv_Len, MEMF_PUBLIC);
	    
	    if (newVar->lv_Value == NULL)
	    {
		/* Free variable node before shutting down */
		FreeVec(newVar);
		
		return FALSE;
	    }
	    
	    CopyMem(varNode->lv_Value, newVar->lv_Value, varNode->lv_Len);
	    
	    AddTail((struct List *)&toProcess->pr_LocalVars,
		    (struct Node *)newVar);
	}
    }

    return TRUE;
}
