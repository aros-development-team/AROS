/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
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
#include <intuition/intuition.h>
#include <aros/symbolsets.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include "dos_intern.h"
#include LC_LIBDEFS_FILE
#include <string.h>

static void DosEntry (STRPTR argPtr, ULONG argSize, APTR initialPC, struct DosLibrary *DOSBase);
static void freeLocalVars(struct Process *process, struct DosLibrary *DOSBase);

BOOL copyVars(struct Process *fromProcess, struct Process *toProcess, struct DosLibrary * DOSBase);

void internal_ChildWait(struct Task *task, struct DosLibrary * DOSBase);
void internal_ChildFree(APTR tid, struct DosLibrary * DOSBase);

#ifdef __mc68000

/* On m68k CPU we support old BCPL programs */

extern APTR BCPL_Setup(struct Process *me, BPTR segList, APTR entry, APTR DOSBase);
extern void BCPL_Cleanup(struct Process *me);

#else

static inline APTR BCPL_Setup(struct Process *process , BPTR segList, APTR entry, APTR DOSBase)
{
     /* this points to segarray, not seglist, check BCPL_Setup() and Guru Book */
    process->pr_SegList = segList;

    return entry ? entry : (BPTR *)BADDR(segList) + 1;
}

#define BCPL_Cleanup(pr)

#endif

#include <aros/debug.h>

/* Temporary macro */
#define P(x)
/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(struct Process *, CreateNewProc,

/*  SYNOPSIS */
	AROS_LHA(const struct TagItem *, tags, D1),

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Allocated resources */
    struct Process  	    	*process = NULL;
    BPTR            	    	 input = 0, output = 0, ces = 0, curdir = 0, homedir = 0;
    STRPTR          	    	 stack = NULL, name = NULL, argptr = NULL;
    ULONG           	    	 namesize = 0, argsize = 0;
    struct MemList  	    	*memlist = NULL;
    struct CommandLineInterface *cli = NULL;
    struct Process  	    	*me = (struct Process *)FindTask(NULL);
    STRPTR          	    	 s;
    ULONG                        old_sig = 0;

    /* TODO: NP_CommandName, NP_ConsoleTask, NP_NotifyOnDeath */

#define TAGDATA_NOT_SPECIFIED ~0ul

    struct TagItem defaults[]=
    {
    /* 0 */    { NP_Seglist 	  , 0                 	    	},
    /* 1 */    { NP_Entry   	  , (IPTR)NULL        	    	},
    /* 2 */    { NP_Input   	  , TAGDATA_NOT_SPECIFIED       },
    /* 3 */    { NP_CloseInput	  , 1           	    	},
    /* 4 */    { NP_Output  	  , TAGDATA_NOT_SPECIFIED    	},
    /* 5 */    { NP_CloseOutput   , 1           	    	},
    /* 6 */    { NP_Error   	  , TAGDATA_NOT_SPECIFIED    	},
    /* 7 */    { NP_CloseError	  , 1           	    	},
    /* 8 */    { NP_CurrentDir	  , TAGDATA_NOT_SPECIFIED    	},
    /* 9 */    { NP_StackSize	  , AROS_STACKSIZE    	        },
    /*10 */    { NP_Name    	  , (IPTR)"New Process" 	},
    /*11 */    { NP_Priority	  , me->pr_Task.tc_Node.ln_Pri 	},
    /*12 */    { NP_Arguments	  , (IPTR)NULL  	    	},
    /*13 */    { NP_Cli     	  , 0           	    	},
    /*14 */    { NP_UserData	  , (IPTR)NULL  	    	},
    /*15 */    { NP_ExitCode	  , (IPTR)NULL  	    	},
    /*16 */    { NP_ExitData	  , (IPTR)NULL  	    	},
    /*17 */    { NP_WindowPtr	  , (IPTR)NULL  	    	}, /* Default: default public screen */
    /*18 */    { NP_CopyVars	  , (IPTR)TRUE  	    	},
    /*19 */    { NP_Synchronous   , (IPTR)FALSE 	    	},
    /*20 */    { NP_FreeSeglist   , (IPTR)TRUE  	    	},
    /*21 */    { NP_HomeDir 	  , TAGDATA_NOT_SPECIFIED     	},
    /*22 */    { NP_Path          , TAGDATA_NOT_SPECIFIED       }, /* Default: copy path from parent */
    /*23 */    { NP_NotifyOnDeath , (IPTR)FALSE                 },
	       { TAG_END    	  , 0           	    	}
    };

    struct TagItem tasktags[] =
    {
    	{TASKTAG_ARG1, 0},
	{TASKTAG_ARG2, 0},
	{TASKTAG_ARG3, 0},
	{TASKTAG_ARG4, (IPTR)DOSBase},
	{TAG_DONE    , 0}
    };

    /* C has no exceptions. This is a simple replacement. */
#define ERROR_IF(a)  if(a) goto error  /* Throw a generic error. */
#define ENOMEM_IF(a) if (a) goto enomem /* Throw out of memory. */
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

    ApplyTagChanges(defaults, (struct TagItem *)tags);

    /* If both the seglist and the entry are specified, make sure that the entry resides in the seglist */
    if (defaults[0].ti_Data && defaults[1].ti_Data)
    {
        BPTR seg;

        for (seg = (BPTR) defaults[0].ti_Data; seg; seg = *(BPTR *)BADDR(seg))
        {
            if
            (
                (UBYTE *)defaults[1].ti_Data >= (UBYTE *)BADDR(seg) &&
                (UBYTE *)defaults[1].ti_Data <= ((UBYTE *)BADDR(seg) + *((ULONG *)BADDR(seg) - 1) - sizeof(BPTR))
            )
            {
                break;
            }
        }

        if (!seg)
            return NULL;
    }

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

    /* NP_Cli */
    if (defaults[13].ti_Data != 0)
    {
        BPTR *oldpath = NULL;
        
	/* Don't forget to pass tags to AllocDosObject() */
	cli = (struct CommandLineInterface *)AllocDosObject(DOS_CLI, (struct TagItem *)tags);
	ENOMEM_IF(cli == NULL);

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


	if (defaults[22].ti_Data != TAGDATA_NOT_SPECIFIED)
        {
            cli->cli_CommandDir = (BPTR) defaults[22].ti_Data;
        }
        else
        {
            BPTR *nextpath, 
                 *newpath = &cli->cli_CommandDir;
            
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

    if (defaults[6].ti_Data)
    {
/* unbuffered to conform to widespread clib behavior */
        SetVBuf((BPTR) defaults[6].ti_Data, NULL, BUF_NONE, -1);
    }

    /* NP_CurrentDir */

    if (defaults[8].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
	if (__is_process(me) && me->pr_CurrentDir)
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
    process->pr_Flags = (defaults[3].ti_Data  ? PRF_CLOSEINPUT    : 0) |
		        (defaults[5].ti_Data  ? PRF_CLOSEOUTPUT   : 0) |
		        (defaults[7].ti_Data  ? PRF_CLOSEERROR    : 0) |
		        (defaults[13].ti_Data ? PRF_FREECLI       : 0) |
	                (defaults[19].ti_Data ? PRF_SYNCHRONOUS   : 0) |
			(defaults[20].ti_Data ? PRF_FREESEGLIST   : 0) |
			(defaults[23].ti_Data ? PRF_NOTIFYONDEATH : 0) |
		        PRF_FREEARGS | PRF_FREECURRDIR;
    process->pr_ExitCode = (APTR)defaults[15].ti_Data;
    process->pr_ExitData = defaults[16].ti_Data;
    process->pr_Arguments = argptr;

    if ((BOOL)defaults[18].ti_Data)      /* NP_CopyVars */
    {
	BOOL res = copyVars(me, process, DOSBase);

	ENOMEM_IF(res == FALSE);
    }

    process->pr_ShellPrivate = 0;

    
    if (defaults[19].ti_Data)
    {
        me->pr_Flags |= PRF_WAITINGFORCHILD;
	
        old_sig = SetSignal(0L, SIGF_SINGLE) & SIGF_SINGLE; 
    }

    /* argsize variable includes trailing 0 byte, but is supposed
       not to. */

    if (argsize) argsize--;

    tasktags[0].ti_Data = (IPTR)argptr;
    tasktags[1].ti_Data = argsize;

    tasktags[2].ti_Data = (IPTR)BCPL_Setup(process, (BPTR)defaults[0].ti_Data, (APTR)defaults[1].ti_Data, DOSBase);
    if (!tasktags[2].ti_Data)
    	goto enomem;

    addprocesstoroot(process, DOSBase);

    if (NewAddTask(&process->pr_Task, DosEntry,	NULL, tasktags))
    {
	/* NP_Synchronous */
	if (defaults[19].ti_Data)
	{
	    P(kprintf("Calling ChildWait()\n"));
	    internal_ChildWait(FindTask(NULL), DOSBase);
	    P(kprintf("Returned from ChildWait()\n"));
	}

	goto end;
    }

    /* Fall through */
enomem:

    BCPL_Cleanup(process);

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

    if (homedir != BNULL)
    {
    	UnLock(homedir);
    }

    if (curdir != BNULL)
    {
	UnLock(curdir);
    }

    if (output != BNULL)
    {
	Close(output);
    }

    if (input != BNULL)
    {
	Close(input);
    }

    if (ces != BNULL)
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

	process = NULL;
    }

end:

    if (defaults[19].ti_Data)
        SetSignal(SIGF_SINGLE, old_sig); 

    return process;

    AROS_LIBFUNC_EXIT
} /* CreateNewProc */





static void freeLocalVars(struct Process *process, struct DosLibrary *DOSBase)
{
    struct LocalVar *varNode;
    struct Node     *tempNode;
    
    ForeachNodeSafe(&process->pr_LocalVars,
		    varNode, tempNode)
    {
	P(kprintf("Freeing variable %s with value %s at %p\n",
		  varNode->lv_Node.ln_Name, varNode->lv_Value, varNode));
	FreeMem(varNode->lv_Value, varNode->lv_Len);
	Remove((struct Node *)varNode);
	FreeVec(varNode);
    }
}


void internal_ChildWait(struct Task *task, struct DosLibrary * DOSBase)
{
    while (((struct Process *)task)->pr_Flags & PRF_WAITINGFORCHILD)
        Wait(SIGF_SINGLE);
}


void internal_ChildFree(APTR tid, struct DosLibrary * DOSBase)
{
    struct Task    *task   = (struct Task *)tid;
    struct Process *parent = (struct Process *)(GetETask(task)->et_Parent);

    D(bug("Awakening the parent task %p (called %s)\n", parent, parent->pr_Task.tc_Node.ln_Name));

    parent->pr_Flags &= ~PRF_WAITINGFORCHILD;
    Signal(&parent->pr_Task, SIGF_SINGLE);
}


BOOL copyVars(struct Process *fromProcess, struct Process *toProcess, struct DosLibrary * DOSBase)
{
    /* We must have variables to copy... */
    if (__is_process(fromProcess))
    {
	struct LocalVar *varNode;
	struct LocalVar *newVar;
	
	/* We use the same strategy as in the�***Var() functions */
	ForeachNode(&fromProcess->pr_LocalVars, varNode)
	{
	    LONG  copyLength = strlen(varNode->lv_Node.ln_Name) + 1 +
		sizeof(struct LocalVar);
	    
	    newVar = (struct LocalVar *)AllocVec(copyLength,
						 MEMF_PUBLIC | MEMF_CLEAR);
	    if (newVar == NULL)
		return FALSE;

	    CopyMem(varNode, newVar, copyLength);
	    newVar->lv_Node.ln_Name = (char *)newVar +
		sizeof(struct LocalVar);
	    P(kprintf("Variable with name %s copied.\n", 
		      newVar->lv_Node.ln_Name));
	    
            if (varNode->lv_Len)
            {
	        newVar->lv_Value = AllocMem(varNode->lv_Len, MEMF_PUBLIC);
	    
	        if (newVar->lv_Value == NULL)
	        {
		    /* Free variable node before shutting down */
		    FreeVec(newVar);
		
		    return FALSE;
	        }
	    
                CopyMem(varNode->lv_Value, newVar->lv_Value, varNode->lv_Len);
	    }

	    AddTail((struct List *)&toProcess->pr_LocalVars,
		    (struct Node *)newVar);
	}
    }

    return TRUE;
}

static void DosEntry (STRPTR argPtr, ULONG argSize, APTR initialPC, struct DosLibrary *DOSBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG result;

    /* Call entry point of our process, remembering stack in its pr_ReturnAddr */
    result = CallEntry(argPtr, argSize, initialPC, me);

    /* Call user defined exit function before shutting down. */
    if (me->pr_ExitCode != NULL)
    {
        /* 
	   The Ralph Bebel's guru book says that pr_ExitCode
 	   is passed the process return code in D0 and pr_ExitData in D1,
	   but the Matt Dillon's DICE C implementation of vfork shows that
	   those parameters are passed also on the stack. 
	   
	   The AROS macros for functions with register parameters don't
	   support both register and stack parameters at once, so we use 
	   the stack only. This oughta be fixed somehow.
        */
	me->pr_ExitCode(result, me->pr_ExitData);
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
#ifdef __mc68000
    	ULONG *segarray = BADDR(me->pr_SegList);

    	if (segarray[3])
	    UnLoadSeg(segarray[3]);
	segarray[3] = 0;
#else
	UnLoadSeg(me->pr_SegList);
#endif
    }
    BCPL_Cleanup(me);

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
	internal_ChildFree(me, DOSBase);
    }

    removefromrootnode(me, DOSBase);
}
