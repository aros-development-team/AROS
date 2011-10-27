/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: exec.library resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <hardware/intbits.h>

#include <aros/symbolsets.h>
#include <aros/system.h>
#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <aros/config.h>

#include <aros/debug.h>

#include <proto/arossupport.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "exec_debug.h"
#include "exec_intern.h"
#include "exec_util.h"
#include "etask.h"
#include "intservers.h"
#include "memory.h"

#include LC_LIBDEFS_FILE

static const UBYTE name[];
static const UBYTE version[];

/* This comes from genmodule */
extern const char LIBEND;

AROS_UFP3S(struct ExecBase *, GM_UNIQUENAME(init),
    AROS_UFPA(struct MemHeader *, mh, D0),
    AROS_UFPA(struct TagItem *, tagList, A0),
    AROS_UFPA(struct ExecBase *, sysBase, A6));

/*
 * exec.library ROMTag.
 *
 * It has RTF_COLDSTART level specified, however it actually runs at SINGLETASK
 * (no multitasking, incomplete boot task).
 * This is supposed to be the first COLDSTART resident to be executed. Its job is
 * to complete the boot task and enable multitasking (which actually means entering
 * COLDSTART level).
 * Such mechanism allows kernel.resource boot code to do some additional setup after
 * all SINGLETASK residents are run. Usually these are various lowlevel hardware resources
 * (like acpi.resource, efi.resource, etc) which can be needed for kernel.resource to
 * complete own setup. This helps to get rid of additional ROMTag hooks.
 * There's one more magic with this ROMTag: it's called twice. First time it's called manually
 * from within krnPrepareExecBase(), for initial ExecBase creation. This magic is described below.
 *
 * WARNING: the CPU privilege level must be set to user before calling InitCode(RTF_COLDSTART)!
 */
const struct Resident Exec_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Exec_resident,
    (APTR)&LIBEND,
    RTF_COLDSTART,
    VERSION_NUMBER,
    NT_LIBRARY,
    120,
    (STRPTR)name,
    (STRPTR)&version[6],
    &GM_UNIQUENAME(init),
};

static const UBYTE name[] = MOD_NAME_STRING;
static const UBYTE version[] = VERSION_STRING;

extern void debugmem(void);

THIS_PROGRAM_HANDLES_SYMBOLSETS
DEFINESET(INITLIB)
DEFINESET(PREINITLIB)

AROS_UFH3S(struct ExecBase *, GM_UNIQUENAME(init),
    AROS_UFHA(struct MemHeader *, mh, D0),
    AROS_UFHA(struct TagItem *, tagList, A0),
    AROS_UFHA(struct ExecBase *, origSysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct Task *t;
/*  struct MemList *ml; */
    struct ExceptionContext *ctx;
    int i;

    /*
     * exec.library init routine is a little bit magic. The magic is that it
     * can be run twice.
     * First time it's run manually from kernel.resource's ROMTag scanner in order
     * to create initial ExecBase. This condition is determined by origSysBase == NULL
     * passed to this function. In this case the routine expects two more arguments:
     * mh      - an initial MemHeader in which ExecBase will be constructed.
     * tagList - boot information passed from the bootstrap. It is used to parse
     *           certain command-line arguments.
     *
     * Second time it's run as part of normal modules initialization sequence, at the
     * end of all RTS_SINGLETASK modules. At this moment we already have a complete
     * memory list and working kernel.resource. Now the job is to complete the boot task
     * structure, and start up multitasking.
     */
    if (!origSysBase)
    	return PrepareExecBase(mh, tagList);

    DINIT("exec.library init");

    /*
     * Call platform-specific pre-init code (if any). Return values are not checked.
     * Note that Boot Task is still incomplete here, and there's no multitasking yet.
     *
     * TODO: Amiga(tm) port may call PrepareExecBaseMove() here instead of hardlinking
     * it from within the boot code.
     *
     * NOTE: All functions will be passed origSysBase value. This is the original
     * ExecBase pointer in case if it was moved. The new pointer will be in global
     * SysBase then.
     */
    set_call_libfuncs(SETNAME(PREINITLIB), 1, 0, origSysBase);

    /*
     * kernel.resource is up and running and memory list is complete.
     * Global SysBase is set to its final value.
     * Complete boot task with ETask and CPU context.
     */
    t = SysBase->ThisTask;

    t->tc_UnionETask.tc_ETask = AllocVec(sizeof(struct IntETask), MEMF_ANY|MEMF_CLEAR);
    ctx                       = KrnCreateContext();

    D(bug("[exec] ETask 0x%p, CPU context 0x%p\n", t->tc_UnionETask.tc_ETask, ctx));

    if (!t->tc_UnionETask.tc_ETask || !ctx)
    {
	DINIT("Not enough memory for first task");
	return NULL;
    }

    /*
     * Build a memory list for the task.
     * It doesn't include stack because it wasn't allocated by us.
     */
    /*
     * Temporarily disabled because of transition to MMU support.
     * PrepareExecBase() can't use AllocMem(), so it uses Allocate()
     * in order to create boot task. And Allocate()'d memory can't
     * be FreeMem()ed because of either mungwall or MMU-based AllocMem().
     * In the light of ABIv1 reimplementation boot task can be moved back here.
     *
    ml = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);
    if (!ml)
    {
    	DINIT("Not enough memory for boot task's MemList");
    	return NULL;
    }

    ml->ml_NumEntries      = 1;
    ml->ml_ME[0].me_Addr   = t;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    AddHead(&t->tc_MemEntry, &ml->ml_Node);
    */

    InitETask(t, t->tc_UnionETask.tc_ETask);

    /*
     * These adjustments need to be done after InitETask():
     * 1. Set et_Parent to NULL, InitETask() will set it to FindTask(NULL). At this moment
     *    we have SysBase->ThisTask already set to incomplete "boot task".
     * 2. Set TF_ETASK in tc_Flags. If it will be already set, InitETask() will try to add
     *    a new ETask into children list of parent ETask (i. e. ourselves).
     */
    t->tc_UnionETask.tc_ETask->et_Parent = NULL;
    t->tc_Flags = TF_ETASK;

    GetIntETask(t)->iet_Context = ctx;

    /* Install the interrupt servers. Again, do it here because allocations are needed. */
    for (i=0; i < 16; i++)
    {
	struct Interrupt *is;

	if (i != INTB_SOFTINT)
	{	
	    struct SoftIntList *sil;

	    is = AllocMem(sizeof(struct Interrupt) + sizeof(struct SoftIntList), MEMF_CLEAR|MEMF_PUBLIC);
	    if (is == NULL)
	    {
		DINIT("ERROR: Cannot install Interrupt Servers!");
		Alert( AT_DeadEnd | AN_IntrMem );
	    }

	    sil = (struct SoftIntList *)((struct Interrupt *)is + 1);

	    if (i == INTB_VERTB)
		is->is_Code = &VBlankServer;
	    else
		is->is_Code = &IntServer;
	    is->is_Data = sil;
	    NEWLIST((struct List *)sil);
	    SetIntVector(i,is);
	}
	else
	{
	    struct Interrupt * is;

	    is = AllocMem(sizeof(struct Interrupt), MEMF_CLEAR|MEMF_PUBLIC);
	    if (NULL == is)
	    {
		DINIT("Error: Cannot install SoftInt Handler!\n");
		Alert( AT_DeadEnd | AN_IntrMem );
	    }

	    is->is_Node.ln_Type = NT_INTERRUPT;
	    is->is_Node.ln_Pri = 0;
	    is->is_Node.ln_Name = "SW Interrupt Dispatcher";
	    is->is_Data = NULL;
	    is->is_Code = (void *)SoftIntDispatch;
	    SetIntVector(i,is);
	}
    }

    /* We now start up the interrupts */
    Permit();
    Enable();

    D(debugmem());

    /* Call platform-specific init code (if any) */
    set_call_libfuncs(SETNAME(INITLIB), 1, 1, SysBase);

    /* Multitasking is on. Call CoolCapture. */
    if (SysBase->CoolCapture)
    {
    	DINIT("Calling CoolCapture at 0x%p", SysBase->CoolCapture);

	AROS_UFC1(void, SysBase->CoolCapture,
            AROS_UFCA(struct Library *, (struct Library *)SysBase, A6));
    }

    /* Done. Following the convention, we return our base pointer. */
    return SysBase;

    AROS_USERFUNC_EXIT
}

AROS_PLH1(struct ExecBase *, open,
    AROS_LHA(ULONG, version, D0),
    struct ExecBase *, SysBase, 1, Exec)
{
    AROS_LIBFUNC_INIT

    /* I have one more opener. */
    SysBase->LibNode.lib_OpenCnt++;
    return SysBase;
    AROS_LIBFUNC_EXIT
}

AROS_PLH0(BPTR, close,
    struct ExecBase *, SysBase, 2, Exec)
{
    AROS_LIBFUNC_INIT

    /* I have one fewer opener. */
    SysBase->LibNode.lib_OpenCnt--;
    return 0;
    AROS_LIBFUNC_EXIT
}
