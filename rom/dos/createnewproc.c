/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new process
    Lang: English
*/

#include <aros/debug.h>

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

#define SEGARRAY_LENGTH 6       /* Minimum needed for HUNK overlays */

static void DosEntry(void);
static void freeLocalVars(struct Process *process, struct DosLibrary *DOSBase);
static BPTR OpenNIL(struct DosLibrary *DOSBase);

BOOL copyVars(struct Process *fromProcess, struct Process *toProcess, struct DosLibrary * DOSBase);

void internal_ChildWait(struct Task *task, struct DosLibrary * DOSBase);
void internal_ChildFree(APTR tid, struct DosLibrary * DOSBase);

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
        It is possible to supply NP_Input, NP_Output and NP_Error tags
        with BNULL values. This is equal to NIL: handle, however if NP_Input
        is set to BNULL, NP_Arguments tag will not work. Arguments are
        passed to the process via input stream, and the stream needs
        to be a valid handle for this. This is original AmigaOS(tm) feature.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Allocated resources */
    struct Process              *process = NULL;
    BPTR                         input = 0, output = 0, ces = 0, curdir = 0, homedir = 0, segList, *segArray;
    STRPTR                       stack = NULL, name = NULL, argptr = NULL;
    ULONG                        namesize = 0, argsize = 0;
    struct MemList              *memlist = NULL;
    struct CommandLineInterface *cli = NULL;
    struct Process              *me = (struct Process *)FindTask(NULL);
    ULONG                        old_sig = 0;
    APTR                         entry;

    /* TODO: NP_CommandName */

#define TAGDATA_NOT_SPECIFIED ~0ul

    struct TagItem defaults[]=
    {
    /* 0 */    { NP_Seglist       , 0                           },
    /* 1 */    { NP_Entry         , (IPTR)NULL                  },
    /* 2 */    { NP_Input         , TAGDATA_NOT_SPECIFIED       },
    /* 3 */    { NP_CloseInput    , 1                           },
    /* 4 */    { NP_Output        , TAGDATA_NOT_SPECIFIED       },
    /* 5 */    { NP_CloseOutput   , 1                           },
    /* 6 */    { NP_Error         , TAGDATA_NOT_SPECIFIED       },
    /* 7 */    { NP_CloseError    , 1                           },
    /* 8 */    { NP_CurrentDir    , TAGDATA_NOT_SPECIFIED       },
    /* 9 */    { NP_StackSize     , AROS_STACKSIZE              },
    /*10 */    { NP_Name          , (IPTR)"New Process"         },
    /*11 */    { NP_Priority      , me->pr_Task.tc_Node.ln_Pri  },
    /*12 */    { NP_Arguments     , TAGDATA_NOT_SPECIFIED       },
    /*13 */    { NP_Cli           , 0                           },
    /*14 */    { NP_UserData      , (IPTR)NULL                  },
    /*15 */    { NP_ExitCode      , (IPTR)NULL                  },
    /*16 */    { NP_ExitData      , (IPTR)NULL                  },
    /*17 */    { NP_WindowPtr     , (IPTR)NULL                  }, /* Default: default public screen */
    /*18 */    { NP_CopyVars      , (IPTR)TRUE                  },
    /*19 */    { NP_Synchronous   , (IPTR)FALSE                 },
    /*20 */    { NP_FreeSeglist   , (IPTR)TRUE                  },
    /*21 */    { NP_HomeDir       , TAGDATA_NOT_SPECIFIED       },
    /*22 */    { NP_Path          , TAGDATA_NOT_SPECIFIED       }, /* Default: copy path from parent */
    /*23 */    { NP_NotifyOnDeath , (IPTR)FALSE                 },
    /*24 */    { NP_ConsoleTask   , TAGDATA_NOT_SPECIFIED       },
               { TAG_END          , 0                           }
    };

    /* C has no exceptions. This is a simple replacement. */
#define ERROR_IF(a)  if(a) goto error  /* Throw a generic error. */
#define ENOMEM_IF(a) if (a) goto enomem /* Throw out of memory. */

    D(bug("[createnewproc] Called from %s %s\n", __is_process(me) ? "Process" : "Task", me->pr_Task.tc_Node.ln_Name));

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

    D({
        int i;
        for (i = 0;  defaults[i].ti_Tag; i++)
            bug("'%s' %2d: %08x = %08x\n", defaults[10].ti_Data, i, defaults[i].ti_Tag, defaults[i].ti_Data);
      }
    );

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

    /*
     * We allocate from the 31bit area because SDL's thread
     * support requires that the thread ID be 32 bit.
     * !!! URGENT FIXME !!! SDL MUST BE FIXED!!! Consider using ETask->et_UniqueID for this.
     * Some architectures (Darwin 64-bit hosted) do not have MEMF_31BIT memory at all.
     * Additionally, it's horribly bad practice to support broken software this way.
     */
#if __WORDSIZE > 32
    process = AllocMem(sizeof(struct Process), MEMF_PUBLIC | MEMF_31BIT | MEMF_CLEAR);
    if (!process)
#endif
    process = AllocMem(sizeof(struct Process), MEMF_PUBLIC | MEMF_CLEAR);
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
    if (defaults[12].ti_Data != TAGDATA_NOT_SPECIFIED)
    {
        CONST_STRPTR args = (CONST_STRPTR)defaults[12].ti_Data;

        /* If NULL, then it was provided by the user,
         * so use the empty "" arg list
         */
        if (args == NULL)
        {
            args = "";
        }

        argsize = strlen(args);
        argptr  = (STRPTR)AllocVec(argsize+1, MEMF_PUBLIC);
        ENOMEM_IF(argptr == NULL);
        CopyMem(args, argptr, argsize+1);

        D(bug("[createnewproc] Arguments \"%s\"\n", argptr));
    }

    memlist = AllocMem(sizeof(struct MemList) + 2*sizeof(struct MemEntry),
                       MEMF_ANY);
    ENOMEM_IF(memlist == NULL);

    /* NP_Cli */
    if (defaults[13].ti_Data != 0)
    {
        BPTR oldpath = BNULL;
        
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

                oldpath = oldcli->cli_CommandDir;

                CopyMem(BADDR(oldcli->cli_Prompt), BADDR(cli->cli_Prompt), (newlen<oldlen?newlen:oldlen) + 1);
            }

            process->pr_CLI = MKBADDR(cli);
            addprocesstoroot(process, DOSBase);
        }


        if (defaults[22].ti_Data != TAGDATA_NOT_SPECIFIED)
        {
            cli->cli_CommandDir = (BPTR) defaults[22].ti_Data;
        }
        else
        {
            cli->cli_CommandDir = internal_CopyPath(oldpath, DOSBase);
        }
    }

    /* NP_Input */

    if (defaults[2].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
        input = OpenNIL(DOSBase);
        ERROR_IF(!input);

        defaults[2].ti_Data = (IPTR)input;
    }

    /* NP_Output */

    if (defaults[4].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
        output = OpenNIL(DOSBase);
        ERROR_IF(!output);

        defaults[4].ti_Data = (IPTR)output;
    }

    /* NP_Error */

    if (defaults[6].ti_Data == TAGDATA_NOT_SPECIFIED)
    {
        ces = OpenNIL(DOSBase);
        ERROR_IF(!ces);

        defaults[6].ti_Data = (IPTR)ces;
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

    D(bug("[createnewproc] Starting process %p '%s'\n", process, name));
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

    process->pr_SegList = BNULL;
    process->pr_GlobVec = ((struct DosLibrary *)DOSBase)->dl_GV;
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
    D(bug("[createnewproc] pr_ConsoleTask = %p\n", process->pr_ConsoleTask));
    D(bug("[createnewproc] pr_FileSystemTask = %p\n", process->pr_FileSystemTask));


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
                        (argptr               ? PRF_FREEARGS      : 0);
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

    /*
     * Allocate and fill in segArray.
     * Except for m68k, we don't have BCPL ABI, so this is a minimal leftover.
     * The main thing is 3rd member containing actual segList pointer.
     * Other values are just for convenience.
     */
    segList = (BPTR)defaults[0].ti_Data;
    entry   = (APTR)defaults[1].ti_Data;
    if (segList == BNULL) {
        segList = CreateSegList(entry);
        process->pr_Flags |= PRF_FREESEGLIST;
    } else if (entry == NULL) {
        entry = BADDR(segList) + sizeof(BPTR);
    }

    segArray = AllocVec(sizeof(BPTR) * (SEGARRAY_LENGTH+1), MEMF_ANY | MEMF_CLEAR);
    ENOMEM_IF(segArray == NULL);

    D(bug("[createnewproc] Creating SegArray %p, segList=%p\n", segArray, BADDR(segList)));
    segArray[0] = (BPTR)SEGARRAY_LENGTH;
    segArray[1] = (BPTR)-1;     /* 'system' segment */
    segArray[2] = (BPTR)-2;     /* 'dosbase' segment */
    segArray[3] = segList;      /* Program segment */

    process->pr_SegList = MKBADDR(segArray);

    /* If we have pr_Arguments *and* we have a Input,
     * then inject the arguments into the input stream.
     */
    if (process->pr_Arguments && process->pr_CIS)
    {
        D(bug("[createnewproc] Injecting %d bytes of arguments @%p into FileHandle @%p\n", argsize, process->pr_Arguments, BADDR(process->pr_CIS)));
        vbuf_inject(process->pr_CIS, process->pr_Arguments, argsize, DOSBase);
    }

    /* Do any last-minute SegList fixups */
    BCPL_Fixup(process);

    /* Abuse pr_Result2 to point to the *real* entry point */
    process->pr_Result2 = (SIPTR)entry;

    /* Use AddTask() instead of NewAddTask().
     * Blizzard SCSI Kit boot ROM plays SetFunction() tricks with
     * AddTask() and assumes it is called by a process early enough!
     */
    if (AddTask(&process->pr_Task, DosEntry, NULL))
    {
        /* Use defaults[19].ti_Data instead of testing against
         * (process->pr_Flags & PRF_SYNCHRONOUS).
         *
         * If the priority of the new process is higher than ours,
         * the new process can be scheduled, completed, and freed
         * before we get to this check - and we would then be
         * comparing via a pointer to a stale structure that
         * may have been overwritten.
         */
        if (defaults[19].ti_Data)
        {
             /* If we *are* synchronous, then 'process' is still valid.
              */
             SIPTR oldSignal = 0;
             struct FileHandle *fh = NULL;

            /* Migrate the CIS handle to the new process */
            if (IsInteractive(process->pr_CIS)) {
                fh = BADDR(process->pr_CIS);

                if (dopacket(&oldSignal, fh->fh_Type, ACTION_CHANGE_SIGNAL, (SIPTR)fh->fh_Arg1, (SIPTR)&process->pr_MsgPort, 0, 0, 0, 0, 0) == DOSFALSE)
                    oldSignal = 0;
            }

            D(bug("[createnewproc] Waiting for task to die...\n"));
            internal_ChildWait(&me->pr_Task, DOSBase);

            if (fh && oldSignal) {
                DoPkt(fh->fh_Type, ACTION_CHANGE_SIGNAL, (SIPTR)fh->fh_Arg1, oldSignal, 0, 0, 0);
            }
        }

        goto end;
    }

    /* Fall through */
enomem:

    if (process && process->pr_SegList)
        FreeVec(BADDR(process->pr_SegList));

    if (__is_process(me))
    {
        SetIoErr(ERROR_NO_FREE_STORE);
    }

    freeLocalVars(process, DOSBase);

error:

    D(bug("[createnewproc] Failed to create process\n"));

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

    FreeVec(argptr);

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
        D(bug("Freeing variable %s with value %s at %p\n",
                  varNode->lv_Node.ln_Name, varNode->lv_Value, varNode));
        FreeMem(varNode->lv_Value, varNode->lv_Len);
        Remove((struct Node *)varNode);
        FreeVec(varNode);
    }
}

BPTR internal_CopyPath(BPTR boldpath, struct DosLibrary * DOSBase)
{
    BPTR *nextpath, path, *newpath, *oldpath;

    oldpath = BADDR(boldpath);

    for (newpath = &path; oldpath != NULL; newpath = nextpath, oldpath = BADDR(oldpath[0])) {
        /* NOTE: This memory allocation *must* match that which is
         *       done in C:Path!!!!
         */
        nextpath = AllocVec(2*sizeof(BPTR), MEMF_CLEAR);
        if (nextpath == NULL)
            break;

        *newpath = MKBADDR(nextpath);
        nextpath[1] = DupLock(oldpath[1]);
        if (nextpath[1] == BNULL)
            break;
    }

    *newpath = BNULL;

    return path;
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
            D(bug("Variable with name %s copied.\n", 
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

static void DosEntry(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG result;
    ULONG argSize = me->pr_Arguments ? strlen(me->pr_Arguments) : 0;
    APTR DOSBase;
    APTR initialPC;
    BPTR *segArray;
    BPTR cis, cos, ces;

    /* Save away our current streams */
    cis = me->pr_CIS;
    cos = me->pr_COS;
    ces = me->pr_CES;

    /* me->pr_Result2 contains our real entry point
     */
    initialPC = (APTR)me->pr_Result2;
    me->pr_Result2 = 0;

    D(bug("[DosEntry %p] is %synchronous\n", me, (me->pr_Flags & PRF_SYNCHRONOUS) ? "S" :"As"));

    segArray = BADDR(me->pr_SegList);
    if (initialPC == NULL)
        initialPC = BADDR(segArray[3]) + sizeof(BPTR);

    D(bug("[DosEntry %p] entry=%p, CIS=%p, COS=%p, argsize=%d, arguments=\"%s\"\n", me, initialPC, BADDR(me->pr_CIS), BADDR(me->pr_COS), argSize, me->pr_Arguments));

    /* Call entry point of our process, remembering stack in its pr_ReturnAddr */
    result = CallEntry(me->pr_Arguments, argSize, initialPC, me);

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

    /* Get our own private DOSBase. We'll need it for our
     * cleanup routines.
     *
     * We don't want to use the parent's DOSBase, since they
     * may have closed their handle long ago.
     *
     * With the current DOSBase implementation, this isn't a
     * big deal, but if DOSBase moved to a per-opener library
     * in the future, this would be a very subtle issue, so
     * we're going to plan ahead and do it right.
     */
    DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS);
    if (DOSBase == NULL) {
        D(bug("[DosEntry %p] Can't open DOS library\n", me));
        Alert(AT_DeadEnd | AG_OpenLib | AO_DOSLib);
    }

    D(bug("Deleting local variables\n"));

    /* Clean up */
    freeLocalVars(me, DOSBase);

    D(bug("Closing input stream\n"));

    if (me->pr_Flags & PRF_CLOSEINPUT)
    {
        Close(cis);
    }

    D(bug("Closing output stream\n"));

    if (me->pr_Flags & PRF_CLOSEOUTPUT)
    {
        Close(cos);
    }

    D(bug("Closing error stream\n"));

    if (me->pr_Flags & PRF_CLOSEERROR)
    {
        Close(ces);
    }

    D(bug("Freeing arguments\n"));

    if (me->pr_Flags & PRF_FREEARGS)
    {
        FreeVec(me->pr_Arguments);
    }

    D(bug("Unloading segment\n"));

    if (me->pr_Flags & PRF_FREESEGLIST)
    {
        BPTR *segarray = BADDR(me->pr_SegList);

        if (segarray && segarray[3])
            UnLoadSeg(segarray[3]);
    }

    FreeVec(BADDR(me->pr_SegList));

    if (me->pr_GlobVec && me->pr_GlobVec != ((struct DosLibrary *)DOSBase)->dl_GV) {
        D(bug("[DosEntry] Looks like someone screwed up %p's pr_GlobVec (%p != dl_GV of %p)\n", me, me->pr_GlobVec, ((struct DosLibrary *)DOSBase)->dl_GV));
        D(Alert(AT_DeadEnd | AN_FreeVec));
    }

    D(bug("Unlocking current dir\n"));

    if (me->pr_Flags & PRF_FREECURRDIR)
    {
        UnLock(me->pr_CurrentDir);
    }

    D(bug("Unlocking home dir\n"));
    UnLock(me->pr_HomeDir);

    D(bug("Freeing cli structure\n"));

    if (me->pr_Flags & PRF_FREECLI)
    {
        FreeDosObject(DOS_CLI, BADDR(me->pr_CLI));
        removefromrootnode(me, DOSBase);
        me->pr_CLI = BNULL;
    }

    /* Synchronous completion must be before
     * PRF_NOTIFYONDEATH, in case both were
     * enabled.
     */
    if (me->pr_Flags & PRF_SYNCHRONOUS) {
        D(bug("Calling ChildFree()\n"));
        /* ChildFree signals the parent */
        internal_ChildFree(me, DOSBase);
    }

    /* Notify of the child's death.
     */
    if (me->pr_Flags & PRF_NOTIFYONDEATH)
    {
        Signal(GetETask(me)->et_Parent, SIGF_CHILD);
    }

    CloseLibrary((APTR)DOSBase);
}

/*
 * This is a version of Open("NIL:") that works inside a task.
 */
static BPTR OpenNIL(struct DosLibrary *DOSBase)
{
    struct FileHandle *fh;

    if ((fh = (struct FileHandle *)AllocDosObject(DOS_FILEHANDLE,NULL))) {
        return (BPTR)handleNIL(ACTION_FINDINPUT, (SIPTR)MKBADDR(fh), (SIPTR)NULL, (SIPTR)NULL);
    }

    return BNULL;
}
