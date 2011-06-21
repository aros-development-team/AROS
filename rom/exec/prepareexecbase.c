/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sets up the ExecBase a bit. (Mostly clearing).
    Lang:
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/kernel.h>
#include <clib/macros.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <aros/arossupportbase.h>
#include <aros/asmcall.h>

#include <string.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include LC_LIBDEFS_FILE
#include "etask.h"
#include "memory.h"
#include "exec_util.h"
#include "exec_debug.h"
#include "exec_intern.h"

#undef kprintf /* This can't be used in the code here */

extern void *LIBFUNCTABLE[];

extern struct Resident Exec_resident; /* Need this for lib_IdString */

extern void Exec_TrapHandler(ULONG trapNum);
AROS_LD3(ULONG, MakeFunctions,
	 AROS_LDA(APTR, target, A0),
	 AROS_LDA(CONST_APTR, functionArray, A1),
	 AROS_LDA(CONST_APTR, funcDispBase, A2),
         struct ExecBase *, SysBase, 15, Exec);

/* Default finaliser. */
static void Exec_TaskFinaliser(void)
{
    /* Get rid of current task. */
    RemTask(SysBase->ThisTask);
}

#undef kprintf
#undef rkprintf
#undef vkprintf

void _aros_not_implemented(char *X)
{
    kprintf("Unsupported function at offset -0x%h in %s\n",
	    ABS(*(WORD *)((&X)[-1]-2)),
	    ((struct Library *)(&X)[-2])->lib_Node.ln_Name);
}

struct Library *PrepareAROSSupportBase (struct MemHeader *mh)
{
    struct AROSSupportBase *AROSSupportBase;

    AROSSupportBase = Allocate(mh, sizeof(struct AROSSupportBase));

    AROSSupportBase->kprintf = (void *)kprintf;
    AROSSupportBase->rkprintf = (void *)rkprintf;
    AROSSupportBase->vkprintf = (void *)vkprintf;

    AROSSupportBase->StdOut = NULL;
    AROSSupportBase->DebugConfig = NULL;

    return (struct Library *)AROSSupportBase;
}

BOOL IsSysBaseValid(struct ExecBase *sysbase)
{
    if (sysbase == NULL)
	return FALSE;
#ifdef __mc68000
    if (((IPTR)sysbase) & 0x80000001)
	return FALSE;
#endif
    if (sysbase->ChkBase != ~(IPTR)sysbase)
	return FALSE;
    if (sysbase->SoftVer != VERSION_NUMBER)
	return FALSE;
    /* more tests? */
    return GetSysBaseChkSum(sysbase) == 0xffff;
}

UWORD GetSysBaseChkSum(struct ExecBase *sysbase)
{
     UWORD sum = 0;
     UWORD *p = (UWORD*)&sysbase->SoftVer;
     while (p <= &sysbase->ChkSum)
	sum += *p++;
     return sum;
}

void SetSysBaseChkSum()
{
     SysBase->ChkBase=~(IPTR)SysBase;
     SysBase->ChkSum = 0;
     SysBase->ChkSum = GetSysBaseChkSum(SysBase) ^ 0xffff;
}

/*
 *  PrepareExecBase() will initialize the ExecBase to default values.
 *  MemHeader and ExecBase itself will already be added to appropriate
 *  lists. You don't need to do this yourself.
 *
 *  WARNING: this routine intentionally sets up global SysBase.
 *  This is done because:
 *  1. PrepareAROSSupportBase() calls Allocate() which relies on functional SysBase
 *  2. After PrepareAROSSupportBase() it is possible to call debug output functions
 *     (kprintf() etc). Yes, KernelBase is not set up yet, but remember that kernel.resource
 *     may have patched functions in AROSSupportBase so that KernelBase is not needed there.
 *  3. Existing ports (at least UNIX-hosted and Windows-hosted) rely on the fact that SysBase is
 *     set up here.
 *
 *  Resume: please be extremely careful, study existing code, and think five times if you decide to
 *  change this. You WILL break existing ports if you do not modify their code accordingly. There's
 *  nothing really bad in the fact that global SysBase is touched here and changing this does not
 *  really win something.
 *						Pavel Fedin <pavel_fedin@mail.ru>
 */
struct ExecBase *PrepareExecBase(struct MemHeader *mh, struct TagItem *msg)
{
    ULONG negsize = 0;
    ULONG totalsize, i;
    VOID  **fp = LIBFUNCTABLE;
    char *args;
    APTR ColdCapture = NULL, CoolCapture = NULL, WarmCapture = NULL;
    APTR KickMemPtr = NULL, KickTagPtr = NULL, KickCheckSum = NULL;
    struct Task *t, tmptask;
    struct MemList *ml;
    IPTR tmpstack[4];

    /*
     * Copy reset proof pointers if old SysBase is valid.
     * Additional platform-specific code is needed in order to test
     * address validity. This routine should zero out SysBase if it is invalid.
     */
    if (IsSysBaseValid(SysBase))
    {
	ColdCapture  = SysBase->ColdCapture;
    	CoolCapture  = SysBase->CoolCapture;
    	WarmCapture  = SysBase->WarmCapture;
    	KickMemPtr   = SysBase->KickMemPtr; 
    	KickTagPtr   = SysBase->KickTagPtr;
    	KickCheckSum = SysBase->KickCheckSum;
    }

    /* Calculate the size of the vector table */
    while (*fp++ != (VOID *) -1) negsize += LIB_VECTSIZE;
    
    /* Align library base */
    negsize = AROS_ALIGN(negsize);
    
    /* Allocate memory for library base */
    totalsize = negsize + sizeof(struct IntExecBase);
    SysBase = (struct ExecBase *)((UBYTE *)stdAlloc(mh, totalsize, MEMF_CLEAR, NULL) + negsize);

    /* Initialize temperaroy task and stack.
       End of stack may be used during function calling
    */
    SysBase->ThisTask = &tmptask;
    tmptask.tc_SPLower = tmpstack;
    tmptask.tc_SPUpper = tmpstack + sizeof(tmpstack);

#ifdef HAVE_PREPAREPLATFORM
    /* Setup platform-specific data */
    if (!Exec_PreparePlatform(&PD(SysBase), msg))
	return NULL;
#endif

    /* Setup function vectors */
    AROS_CALL3(ULONG, AROS_SLIB_ENTRY(MakeFunctions, Exec),
	      AROS_UFCA(APTR, SysBase, A0),
	      AROS_UFCA(CONST_APTR, LIBFUNCTABLE, A1),
	      AROS_UFCA(CONST_APTR, NULL, A2),
	      struct ExecBase *, SysBase);

    /* Bring back saved values (or NULLs) */
    SysBase->ColdCapture = ColdCapture;
    SysBase->CoolCapture = CoolCapture;
    SysBase->WarmCapture = WarmCapture;
    SysBase->KickMemPtr = KickMemPtr;
    SysBase->KickTagPtr = KickTagPtr;
    SysBase->KickCheckSum = KickCheckSum;

    SysBase->LibNode.lib_Node.ln_Type = NT_LIBRARY;
    SysBase->LibNode.lib_Node.ln_Pri  = -100;
    SysBase->LibNode.lib_Node.ln_Name = (char *)Exec_resident.rt_Name;
    SysBase->LibNode.lib_IdString     = (char *)Exec_resident.rt_IdString;
    SysBase->LibNode.lib_Version      = VERSION_NUMBER;
    SysBase->LibNode.lib_Revision     = REVISION_NUMBER;
    SysBase->LibNode.lib_OpenCnt      = 1;
    SysBase->LibNode.lib_NegSize      = negsize;
    SysBase->LibNode.lib_PosSize      = sizeof(struct IntExecBase);
    SysBase->LibNode.lib_Flags        = 0;

    NEWLIST(&SysBase->MemList);
    SysBase->MemList.lh_Type = NT_MEMORY;
    ADDHEAD(&SysBase->MemList, &mh->mh_Node);
    
    NEWLIST(&SysBase->ResourceList);
    SysBase->ResourceList.lh_Type = NT_RESOURCE;
    
    NEWLIST(&SysBase->DeviceList);
    SysBase->DeviceList.lh_Type = NT_DEVICE;

    NEWLIST(&SysBase->IntrList);
    SysBase->IntrList.lh_Type = NT_INTERRUPT;

    NEWLIST(&SysBase->LibList);
    SysBase->LibList.lh_Type = NT_LIBRARY;
    ADDHEAD(&SysBase->LibList, &SysBase->LibNode.lib_Node);

    NEWLIST(&SysBase->PortList);
    SysBase->PortList.lh_Type = NT_MSGPORT;

    NEWLIST(&SysBase->TaskReady);
    SysBase->TaskReady.lh_Type = NT_TASK;

    NEWLIST(&SysBase->TaskWait);
    SysBase->TaskWait.lh_Type = NT_TASK;

    NEWLIST(&SysBase->SemaphoreList);
    SysBase->TaskWait.lh_Type = NT_SEMAPHORE;

    NEWLIST(&SysBase->ex_MemHandlers);

    for (i = 0; i < 5; i++)
    {
	NEWLIST(&SysBase->SoftInts[i].sh_List);
	SysBase->SoftInts[i].sh_List.lh_Type = NT_INTERRUPT;
    }

    NEWLIST(&PrivExecBase(SysBase)->ResetHandlers);
    NEWLIST(&PrivExecBase(SysBase)->AllocMemList);

    InitSemaphore(&PrivExecBase(SysBase)->MemListSem);
    InitSemaphore(&PrivExecBase(SysBase)->LowMemSem);

    SysBase->SoftVer        = VERSION_NUMBER;

    SysBase->MaxLocMem      = (IPTR)mh->mh_Upper;

    SysBase->Quantum        = 4;

    SysBase->TaskTrapCode   = Exec_TrapHandler;
    SysBase->TaskExceptCode = NULL;
    SysBase->TaskExitCode   = Exec_TaskFinaliser;
    SysBase->TaskSigAlloc   = 0xFFFF;
    SysBase->TaskTrapAlloc  = 0;

    /* Default frequencies */
    SysBase->VBlankFrequency = 50;
    SysBase->PowerSupplyFrequency = 1;

    /* Parse some arguments from command line */
    args = (char *)LibGetTagData(KRN_CmdLine, 0, msg);
    if (args)
    {
    	char *opts;

	/*
	 * Enable mungwall before the first AllocMem().
	 * Yes, we have actually already called stdAlloc() once
	 * in order to allocate memory for SysBase itself, however
	 * this is not a real problem because it is never going
	 * to be freed.
	 *
	 * We store mungwall setting in private flags because it must not be
	 * switched at runtime (or hard crash will happen).
	 */
	opts = strstr(args, "mungwall");
	if (opts)
	    PrivExecBase(SysBase)->IntFlags = EXECF_MungWall;

	opts = strstr(args, "stacksnoop");
	if (opts)
	    PrivExecBase(SysBase)->IntFlags = EXECF_StackSnoop;


	/*
	 * Parse system runtime debug flags.
	 * These are public. In future they will be editable by prefs program.
	 * However in order to be able to turn them on during early startup,
	 * we apply them also here.
	 */
	opts = strstr(args, "sysdebug=");
	if (opts)
	    SysBase->ex_DebugFlags = ParseFlags(&opts[9], ExecFlagNames);
    }

    SysBase->DebugAROSBase = PrepareAROSSupportBase(mh);

    SetSysBaseChkSum();

    /* Create boot task */
    ml = (struct MemList *)AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);
    t  = (struct Task *)   AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR);
    if( !ml || !t )
    {
	DINIT("ERROR: Cannot create Boot Task!");
	Alert( AT_DeadEnd | AG_NoMemory | AN_ExecLib );
    }

    D(bug("[exec] Boot task: MemList 0x%p, task 0x%p\n", ml, t));

    ml->ml_NumEntries = 1;
    ml->ml_ME[0].me_Addr = t;
    ml->ml_ME[0].me_Length = sizeof(struct Task);

    NEWLIST(&t->tc_MemEntry);

    AddHead(&t->tc_MemEntry,&ml->ml_Node);

    t->tc_Node.ln_Name = "Boot Task";
    t->tc_Node.ln_Type = NT_TASK;
    t->tc_Node.ln_Pri = 0;
    t->tc_State = TS_RUN;
    t->tc_SigAlloc = 0xFFFF;
    t->tc_SPLower = AllocMem(AROS_STACKSIZE, MEMF_ANY);
    t->tc_SPUpper = t->tc_SPLower + AROS_STACKSIZE;

    SysBase->ThisTask = t;
    SysBase->Elapsed = SysBase->Quantum;

    return SysBase;
}
