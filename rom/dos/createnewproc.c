/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new process
    Lang: English
*/

#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
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

#define DEBUG 0
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
    ULONG                        old_sig = 0;

    /* TODO: NP_CommandName, NP_NotifyOnDeath */

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
    /*12 */    { NP_Arguments	  , (IPTR)-1    	    	},
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
    /*24 */    { NP_ConsoleTask   , TAGDATA_NOT_SPECIFIED       },
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

	    D(bug("[createnewproc] Parent stack: %u (0x%08X)\n", parentstack, parentstack));
	    if (parentstack > AROS_STACKSIZE)
	    {
	        defaults[9].ti_Data = parentstack;
	    }
	}
    }

    ApplyTagChanges(defaults, (struct TagItem *)tags);

    /*
     * If both the seglist and the entry are specified, make sure that the entry resides in the seglist
     * Disabled because it is not necessarily always true. At least current implementation of SystemTagList()
     * specifies seglist for the shell but uses own entry point.
     *
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
    } */

    process = (struct Process *)AllocMem(sizeof(struct Process),
					 MEMF_PUBLIC | MEMF_CLEAR);
    ENOMEM_IF(process == NULL);

    /* Do this early to ease implementation of failure code */
    NEWLIST((struct List *)&process->pr_LocalVars);

    /*
     * This assignment will explicitly convert stack size from IPTR to ULONG.
     * This is important on 64-bit machines. For example, let's have a tag item:
     * NP_StackSize, 8192
     * When this pair is put on stack, numbers are considered LONGs. But items on stack
     * must be aligned on 8-byte boundaries. So we skip some spacing:
     * movl   $0x2000,0x8(%rsp)
     * movl   $0x800003f3,(%rsp)
     * Note that since numbers are plain longs, they are loaded using movl operation.
     * This means that empty space at 0x4(%rsp) will contain trash (leftover from previous
     * stack usage).
     * So, if you then grab this value as IPTR, upper half of this value will contain trash.
     * This way 0x2000 may become something like 0x2000002000, and seriously spoil your life.
     * Yes, 64-bit systems appear to be strictly typed in such places.
     */
    process->pr_StackSize = defaults[9].ti_Data;
    /* We need a minimum stack to handle interrupt contexts */
    if (process->pr_StackSize < AROS_STACKSIZE)
    {
	process->pr_StackSize = AROS_STACKSIZE;
    }

    stack = AllocMem(process->pr_StackSize, MEMF_PUBLIC);
    ENOMEM_IF(stack == NULL);

    namesize = strlen((STRPTR)defaults[10].ti_Data) + 1;
    name = AllocMem(namesize, MEMF_PUBLIC);
    ENOMEM_IF(name == NULL);

    /* NP_Arguments */
    if (defaults[12].ti_Data != (IPTR)-1)
    {
    	CONST_STRPTR args = (CONST_STRPTR)defaults[12].ti_Data;

    	/* If NULL, then it was provided by the user,
    	 * so use the empty "" arg list
    	 */
    	if (args == NULL)
	{
    	    argptr = "";
    	    argsize = 0;
	} else {
	    argsize = strlen(args);
	    argptr  = (STRPTR)AllocVec(argsize+1, MEMF_PUBLIC);
	    ENOMEM_IF(argptr == NULL);
	    CopyMem(args, argptr, argsize+1);
	}
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

	cli->cli_DefaultStack = (process->pr_StackSize + CLI_DEFAULTSTACK_UNIT - 1) / CLI_DEFAULTSTACK_UNIT;

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

    process->pr_Task.tc_Node.ln_Type = NT_PROCESS;
    process->pr_Task.tc_Node.ln_Name = name;
    process->pr_Task.tc_Node.ln_Pri = defaults[11].ti_Data;
    process->pr_Task.tc_SPLower = stack;
    process->pr_Task.tc_SPUpper = stack + process->pr_StackSize - SP_OFFSET;

    D(bug("[createnewproc] Starting process %s\n", name));
    D(bug("[createnewproc] Stack: 0x%p - 0x%p\n", process->pr_Task.tc_SPLower, process->pr_Task.tc_SPUpper));

/*  process->pr_ReturnAddr; */
    NEWLIST(&process->pr_Task.tc_MemEntry);

    memlist->ml_NumEntries = 3;
    memlist->ml_ME[0].me_Addr = process;
    memlist->ml_ME[0].me_Length = sizeof(struct Process);
    memlist->ml_ME[1].me_Addr = stack;
    memlist->ml_ME[1].me_Length = process->pr_StackSize;
    memlist->ml_ME[2].me_Addr = name;
    memlist->ml_ME[2].me_Length = namesize;

    AddHead(&process->pr_Task.tc_MemEntry, &memlist->ml_Node);

    process->pr_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
    process->pr_MsgPort.mp_Flags = PA_SIGNAL;
    process->pr_MsgPort.mp_SigBit = SIGB_DOS;
    process->pr_MsgPort.mp_SigTask = process;

    NEWLIST(&process->pr_MsgPort.mp_MsgList);

    process->pr_SegList = (BPTR)defaults[0].ti_Data;
    process->pr_GlobVec = DOSBase->dl_GV;
    process->pr_StackBase = MKBADDR(process->pr_Task.tc_SPUpper);
    process->pr_Result2 = 0;
    process->pr_CurrentDir = (BPTR)defaults[8].ti_Data;
    process->pr_CIS = (BPTR)defaults[2].ti_Data;
    process->pr_COS = (BPTR)defaults[4].ti_Data;
    process->pr_CES = (BPTR)defaults[6].ti_Data;
    process->pr_Task.tc_UserData = (APTR)defaults[14].ti_Data;

    /* Inherit pr_ConsoleTask and pr_FileSystemTask from parent */
    if (defaults[24].ti_Data != TAGDATA_NOT_SPECIFIED)
    	process->pr_ConsoleTask = (struct MsgPort*)defaults[24].ti_Data;
    else if (__is_process(me))
    	process->pr_ConsoleTask = me->pr_ConsoleTask;
    if (__is_process(me))
    	process->pr_FileSystemTask = me->pr_FileSystemTask;
    else
    	process->pr_FileSystemTask = DOSBase->dl_Root->rn_BootProc;

    process->pr_CLI = MKBADDR(cli);

    /* Set the name of this program */
    internal_SetProgramName(cli, name, DOSBase);
    D(bug("[createnewproc] Calling internal_SetProgramName() with name = %s\n", name));

    process->pr_PktWait = NULL;
    process->pr_WindowPtr = (struct Window *)defaults[17].ti_Data;
    process->pr_HomeDir = (BPTR)defaults[21].ti_Data;
    process->pr_Flags = (defaults[3].ti_Data  ? PRF_CLOSEINPUT    : 0) |
		        (defaults[5].ti_Data  ? PRF_CLOSEOUTPUT   : 0) |
		        (defaults[7].ti_Data  ? PRF_CLOSEERROR    : 0) |
			(defaults[8].ti_Data  ? PRF_FREECURRDIR   : 0) |
		        (defaults[13].ti_Data ? PRF_FREECLI       : 0) |
	                (defaults[19].ti_Data ? PRF_SYNCHRONOUS   : 0) |
			(defaults[20].ti_Data ? PRF_FREESEGLIST   : 0) |
			(defaults[23].ti_Data ? PRF_NOTIFYONDEATH : 0) |
		        PRF_FREEARGS;
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

    if (argptr != NULL) {
        /*
         * Inject command arguments to the beginning of input handle. Guru Book mentions this.
         * This fixes for example AmigaOS' C:Execute.
         * This applies only to processes that set NP_Arguments,
         *  (even to NULL!)
         */
        D(bug("[createnewproc] argsize: %u argstr: %s\n", argsize, argptr));
        vbuf_inject(process->pr_CIS, argptr, argsize, DOSBase);
    }

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
	FreeMem(stack, process->pr_StackSize);
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
	
	/* We use the same strategy as in the ***Var() functions */
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
	   The Ralph Babel's guru book says that pr_ExitCode
 	   is passed the process return code in D0 and pr_ExitData in D1,
	   but the Matt Dillon's DICE C implementation of vfork shows that
	   those parameters are passed also on the stack. 	   
	 */
#ifdef __mc68000
	asm volatile (
		"move.l %0, %%d0\n"
		"move.l %1, %%d1\n"
		"move.l %2, %%a0\n"
		"move.l %%d0, %%sp@-\n"
		"move.l %%d1, %%sp@-\n"
		"jsr (%%a0)\n"
		"addq.l #8, %%sp\n"
		: /* No return values */
		: "g" (result), "g" (me->pr_ExitData), "g" (me->pr_ExitCode)
		: "d0", "d1", "a0", "a1"
	);
#else
	/* 
 	   The AROS macros for functions with register parameters don't
 	   support both register and stack parameters at once, so we use 
	   the stack only on non-m68k. This oughta be fixed somehow.
         */
 	me->pr_ExitCode(result, me->pr_ExitData);
#endif
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
    	BPTR *segarray = BADDR(me->pr_SegList);

    	if (segarray && segarray[3])
	    UnLoadSeg(segarray[3]);
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
