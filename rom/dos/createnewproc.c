/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>
#include <clib/utility_protos.h>
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
	#include <clib/dos_protos.h>

	__AROS_LH1(struct Process *, CreateNewProc,

/*  SYNOPSIS */
	__AROS_LA(struct TagItem *, tags, D1),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    /* Allocated resources */
    struct Process *process=NULL;
    BPTR input=0, output=0, curdir=0;
    STRPTR stack=NULL, name=NULL, argptr=NULL;
    ULONG namesize, argsize=0;
    struct MemList *memlist=NULL;
    struct CommandLineInterface *cli=NULL;

    struct Process *me=(struct Process *)FindTask(NULL);
    STRPTR s;

    struct TagItem defaults[]=
    {
        { NP_Seglist, 0 },
	{ NP_Entry, (ULONG)NULL },
	{ NP_Input, ~0ul },
	{ NP_CloseInput, 1 },
	{ NP_Output, ~0ul },
	{ NP_CloseOutput, 1 },
	{ NP_Error, 0 },
	{ NP_CloseError, 1 },
	{ NP_CurrentDir, ~0ul },
	{ NP_StackSize, 4000 },
	{ NP_Name, (ULONG)"New Process" },
	{ NP_Priority, me->pr_Task.tc_Node.ln_Pri },
	{ NP_Arguments, (ULONG)NULL },
	{ NP_Cli, 0 },
	{ TAG_END, 0 }
    };

    ApplyTagChanges(defaults,tags);
    
    process=(struct Process *)AllocMem(sizeof(struct Process),MEMF_PUBLIC|MEMF_CLEAR);
    stack=AllocMem(defaults[9].ti_Data,MEMF_PUBLIC);
    s=(STRPTR)defaults[10].ti_Data;
    while(*s++)
    	;
    namesize=s-(STRPTR)defaults[10].ti_Data;
    name=AllocMem(namesize,MEMF_PUBLIC);
    s=(STRPTR)defaults[12].ti_Data;
    if(s!=NULL)
    {
	while(*s++)
	    ;
	argsize=s-(STRPTR)defaults[12].ti_Data;
	argptr=(STRPTR)AllocVec(argsize,MEMF_PUBLIC);
    }
    memlist=AllocMem(sizeof(struct MemList)+2*sizeof(struct MemEntry),MEMF_ANY);
    if(defaults[2].ti_Data==~0ul)
        if((input=Open("NIL:",MODE_OLDFILE)))
            defaults[2].ti_Data=input;
    if(defaults[4].ti_Data==~0ul)
    	if((output=Open("NIL:",MODE_NEWFILE)))
    	    defaults[4].ti_Data=output;
    if(defaults[8].ti_Data==~0ul)
    {
        if(me->pr_Task.tc_Node.ln_Type==NT_PROCESS)
        {
            if((curdir=Lock("",SHARED_LOCK)))
                defaults[8].ti_Data=curdir;
        }else
            defaults[8].ti_Data=0;
    }
    if(defaults[13].ti_Data)
        cli=(struct CommandLineInterface *)AllocDosObject(DOS_CLI,NULL);

    if(process!=NULL&&stack!=NULL&&name!=NULL&&memlist!=NULL&&
       defaults[2].ti_Data!=~0ul&&defaults[4].ti_Data!=~0ul&&
       defaults[8].ti_Data!=~0ul&&(defaults[12].ti_Data==0||argptr!=NULL)&&
       (defaults[13].ti_Data==0||cli!=NULL))
    {
	CopyMem((APTR)defaults[10].ti_Data,name,namesize);
	CopyMem((APTR)defaults[12].ti_Data,argptr,argsize);
	process->pr_Task.tc_Node.ln_Type=NT_PROCESS;
	process->pr_Task.tc_Node.ln_Name=name;
	process->pr_Task.tc_Node.ln_Pri=defaults[11].ti_Data;
	process->pr_Task.tc_SPLower=stack;
	process->pr_Task.tc_SPUpper=stack+defaults[9].ti_Data;

/*	process->pr_ReturnAddr;		*/
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
	process->pr_SegList=defaults[0].ti_Data;
	process->pr_StackSize=defaults[9].ti_Data;
	process->pr_GlobVec=NULL;
	Forbid();
	process->pr_TaskNum=DOSBase->dl_ProcCnt++;
	Permit();
	process->pr_StackBase=(ULONG)process->pr_Task.tc_SPUpper;
	process->pr_Result2=0;
	process->pr_CurrentDir=defaults[8].ti_Data;
	process->pr_CIS=defaults[2].ti_Data;
	process->pr_COS=defaults[4].ti_Data;
	process->pr_CES=defaults[6].ti_Data;
/*	process->pr_ConsoleTask=;	*/
/*	process->pr_FileSystemTask=;	*/
	process->pr_CLI=MKBADDR(cli);
/*	process->pr_PktWait=;		*/
/*	process->pr_WindowPtr=;		*/
/*	process->pr_HomeDir=;		*/
	process->pr_Flags=(defaults[3].ti_Data?PRF_CLOSEINPUT:0)|
			  (defaults[5].ti_Data?PRF_CLOSEOUTPUT:0)|
			  (defaults[7].ti_Data?PRF_CLOSEERROR:0)|
			  (defaults[13].ti_Data?PRF_FREECLI:0)|
			  PRF_FREEARGS|PRF_FREESEGLIST|PRF_FREECURRDIR;
/*	process->pr_ExitCode=;		*/
/*	process->pr_ExitData=;		*/
	process->pr_Arguments=argptr;
	NEWLIST((struct List *)&process->pr_LocalVars);
	process->pr_ShellPrivate=0;
	
	if(AddProcess(process,argptr,argsize,defaults[0].ti_Data?
		      (APTR)BADDR(defaults[0].ti_Data+1):
		      (APTR)defaults[1].ti_Data,KillCurrentProcess,
		      DOSBase)!=NULL)
	    return process;
    }
    if(curdir)
        UnLock(curdir);
    if(output)
	Close(output);
    if(input)
	Close(input);
    if(memlist!=NULL)
	FreeMem(memlist,sizeof(struct MemList)+2*sizeof(struct MemEntry));
    if(argptr!=NULL)
	FreeVec(argptr);
    if(name!=NULL)
	FreeMem(name,namesize);
    if(stack!=NULL)
	FreeMem(stack,defaults[9].ti_Data);
    if(process!=NULL)
	FreeMem(process,sizeof(struct Process));
    if(me->pr_Task.tc_Node.ln_Type==NT_PROCESS)
	me->pr_Result2=ERROR_NO_FREE_STORE;
    return NULL;
    __AROS_FUNC_EXIT
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