/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Create a new process
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include "dos_intern.h"

#define NEWLIST(l)                          \
((l)->lh_Head=(struct Node *)&(l)->lh_Tail, \
 (l)->lh_Tail=NULL,                         \
 (l)->lh_TailPred=(struct Node *)(l))

static void KillCurrentProcess(void);
struct Process *AddProcess(struct Process *process, STRPTR argPtr,
ULONG argSize, APTR initialPC, APTR finalPC, struct DosLibrary *DOSBase);

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
    struct Process *process=NULL;
    BPTR input=0, output=0, curdir=0;
    STRPTR stack=NULL, name=NULL, argptr=NULL;
    ULONG namesize, argsize=0;
    struct MemList *memlist=NULL;
    struct CommandLineInterface *cli=NULL;

    struct Process *me=(struct Process *)FindTask(NULL);
    STRPTR s;
    BPTR *oldpath, *newpath, *nextpath;

    struct TagItem defaults[]=
    {
    /* 0 */    { NP_Seglist,	   0 },
    /* 1 */    { NP_Entry,	   (IPTR)NULL },
    /* 2 */    { NP_Input,	   ~0ul },
    /* 3 */    { NP_CloseInput,    1 },
    /* 4 */    { NP_Output,	   ~0ul },
    /* 5 */    { NP_CloseOutput,   1 },
    /* 6 */    { NP_Error,	   0 },
    /* 7 */    { NP_CloseError,    1 },
    /* 8 */    { NP_CurrentDir,    ~0ul },
    /* 9 */    { NP_StackSize,	   AROS_STACKSIZE },
    /*10 */    { NP_Name,	   (IPTR)"New Process" },
    /*11 */    { NP_Priority,	   me->pr_Task.tc_Node.ln_Pri },
    /*12 */    { NP_Arguments,	   (IPTR)NULL },
    /*13 */    { NP_Cli,	   0 },
    /*14 */    { NP_UserData,	   (IPTR)NULL },
	{ TAG_END, 0 }
    };
    /* C has no exceptions. This is a simple replacement. */
#define ERROR_IF(a)  if(a) goto error  /* Throw a generic error. */
#define ENOMEM_IF(a) if(a) goto enomem /* Throw out of memory. */

    ApplyTagChanges(defaults,tags);

    process=(struct Process *)AllocMem(sizeof(struct Process),MEMF_PUBLIC|MEMF_CLEAR);
    ENOMEM_IF(process==NULL);
    stack=AllocMem(defaults[9].ti_Data,MEMF_PUBLIC);
    ENOMEM_IF(stack==NULL);
    s=(STRPTR)defaults[10].ti_Data;
    while(*s++)
	;
    namesize=s-(STRPTR)defaults[10].ti_Data;
    name=AllocMem(namesize,MEMF_PUBLIC);
    ENOMEM_IF(name==NULL);
    s=(STRPTR)defaults[12].ti_Data;
    if(s!=NULL)
    {
	while(*s++)
	    ;
	argsize=s-(STRPTR)defaults[12].ti_Data;
	argptr=(STRPTR)AllocVec(argsize,MEMF_PUBLIC);
	ENOMEM_IF(argptr==NULL);
    }
    memlist=AllocMem(sizeof(struct MemList)+2*sizeof(struct MemEntry),MEMF_ANY);
    ENOMEM_IF(memlist==NULL);
    if(defaults[13].ti_Data)
    {
	cli=(struct CommandLineInterface *)AllocDosObject(DOS_CLI,NULL);
	ENOMEM_IF(cli==NULL);
	oldpath=NULL;
	if(me->pr_Task.tc_Node.ln_Type==NT_PROCESS)
	{
	    struct CommandLineInterface *oldcli=Cli();
	    if(oldcli!=NULL)
		oldpath=BADDR(oldcli->cli_CommandDir);
	}
	newpath=&cli->cli_CommandDir;
	while(oldpath!=NULL)
	{
	    nextpath=AllocVec(2*sizeof(BPTR),MEMF_CLEAR);
	    ENOMEM_IF(nextpath==NULL);
	    newpath[0]=MKBADDR(nextpath);
	    nextpath[1]=DupLock(oldpath[1]);
	    ERROR_IF(!nextpath[1]);
	    newpath=nextpath;
	    oldpath=BADDR(oldpath[0]);
	}
    }
    if(defaults[2].ti_Data==~0ul)
    {
	input=Open("NIL:",MODE_OLDFILE);
	ERROR_IF(!input);
	defaults[2].ti_Data=(IPTR)input;
    }
    if(defaults[4].ti_Data==~0ul)
    {
	output=Open("NIL:",MODE_NEWFILE);
	ERROR_IF(!output);
	defaults[4].ti_Data=(IPTR)output;
    }
    if(defaults[8].ti_Data==~0ul)
    {
	if(me->pr_Task.tc_Node.ln_Type==NT_PROCESS)
	{
	    curdir=Lock("",SHARED_LOCK);
	    ERROR_IF(!curdir);
	    defaults[8].ti_Data=(IPTR)curdir;
	}else
	    defaults[8].ti_Data=0;
    }

    CopyMem((APTR)defaults[10].ti_Data,name,namesize);
    CopyMem((APTR)defaults[12].ti_Data,argptr,argsize);
    process->pr_Task.tc_Node.ln_Type=NT_PROCESS;
    process->pr_Task.tc_Node.ln_Name=name;
    process->pr_Task.tc_Node.ln_Pri=defaults[11].ti_Data;
    process->pr_Task.tc_SPLower=stack;
    process->pr_Task.tc_SPUpper=stack+defaults[9].ti_Data;


/*  process->pr_ReturnAddr; */
    NEWLIST(&process->pr_Task.tc_MemEntry);
    memlist->ml_NumEntries=3;
    memlist->ml_ME[0].me_Addr=process;
    memlist->ml_ME[0].me_Length=sizeof(struct Process);
    memlist->ml_ME[1].me_Addr=stack;
    memlist->ml_ME[1].me_Length=defaults[9].ti_Data;
    memlist->ml_ME[2].me_Addr=name;
    memlist->ml_ME[2].me_Length=namesize;
    AddHead(&process->pr_Task.tc_MemEntry,&memlist->ml_Node);
    process->pr_MsgPort.mp_Node.ln_Type=NT_MSGPORT;
    process->pr_MsgPort.mp_Flags=PA_SIGNAL;
    process->pr_MsgPort.mp_SigBit=SIGB_DOS;
    process->pr_MsgPort.mp_SigTask=process;
    NEWLIST(&process->pr_MsgPort.mp_MsgList);
    process->pr_SegList=(BPTR)defaults[0].ti_Data;
    process->pr_StackSize=defaults[9].ti_Data;
    process->pr_GlobVec=NULL;
    Forbid();
    process->pr_TaskNum=DOSBase->dl_ProcCnt++;
    Permit();
    process->pr_StackBase=MKBADDR(process->pr_Task.tc_SPUpper);
    process->pr_Result2=0;
    process->pr_CurrentDir=(BPTR)defaults[8].ti_Data;
    process->pr_CIS=(BPTR)defaults[2].ti_Data;
    process->pr_COS=(BPTR)defaults[4].ti_Data;
    process->pr_CES=(BPTR)defaults[6].ti_Data;

    process->pr_Task.tc_UserData = (APTR)defaults[14].ti_Data;

/*  process->pr_ConsoleTask=; */
/*  process->pr_FileSystemTask=; */
    process->pr_CLI=MKBADDR(cli);
/*  process->pr_PktWait=; */
/*  process->pr_WindowPtr=; */
/*  process->pr_HomeDir=; */
    process->pr_Flags=(defaults[3].ti_Data?PRF_CLOSEINPUT:0)|
		      (defaults[5].ti_Data?PRF_CLOSEOUTPUT:0)|
		      (defaults[7].ti_Data?PRF_CLOSEERROR:0)|
		      (defaults[13].ti_Data?PRF_FREECLI:0)|
		      PRF_FREEARGS|PRF_FREESEGLIST|PRF_FREECURRDIR;
/*  process->pr_ExitCode=; */
/*  process->pr_ExitData=; */
    process->pr_Arguments=argptr;
    NEWLIST((struct List *)&process->pr_LocalVars);
    process->pr_ShellPrivate=0;

    if(AddProcess(process,argptr,argsize,defaults[0].ti_Data?
		  (BPTR *)BADDR(defaults[0].ti_Data)+1:
		  (BPTR *)defaults[1].ti_Data,KillCurrentProcess,
		  DOSBase)!=NULL)
	return process;

    /* Fall through */
enomem:
    if(me->pr_Task.tc_Node.ln_Type==NT_PROCESS)
	me->pr_Result2=ERROR_NO_FREE_STORE;
error:
    if (cli)
	FreeDosObject(DOS_CLI,cli);

    if (curdir)
	UnLock(curdir);

    if (output)
	Close(output);

    if (input)
	Close(input);

    if (argptr)
	FreeVec(argptr);

    if(memlist!=NULL)
	FreeMem(memlist,sizeof(struct MemList)+2*sizeof(struct MemEntry));

    if(name!=NULL)
	FreeMem(name,namesize);

    if(stack!=NULL)
	FreeMem(stack,defaults[9].ti_Data);

    if(process!=NULL)
	FreeMem(process,sizeof(struct Process));

    return NULL;
    AROS_LIBFUNC_EXIT
} /* CreateNewProc */

static void KillCurrentProcess(void)
{
    /* I need the global here because there is no local way to get it */
    extern struct DosLibrary *DOSBase;
    struct Process *me=(struct Process *)FindTask(NULL);

    if(me->pr_Flags&PRF_CLOSEINPUT)
	Close(me->pr_CIS);
    if(me->pr_Flags&PRF_CLOSEOUTPUT)
	Close(me->pr_COS);
    if(me->pr_Flags&PRF_CLOSEERROR)
	Close(me->pr_CES);
    if(me->pr_Flags&PRF_FREEARGS)
	FreeVec(me->pr_Arguments);
    if(me->pr_Flags&PRF_FREESEGLIST)
	UnLoadSeg(me->pr_SegList);
    if(me->pr_Flags&PRF_FREECURRDIR)
	UnLock(me->pr_CurrentDir);
    if(me->pr_Flags&PRF_FREECLI)
	FreeDosObject(DOS_CLI,BADDR(me->pr_CLI));
    RemTask(NULL);
}
