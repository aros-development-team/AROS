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

const struct Resident Exec_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&Exec_resident,
    (APTR)&LIBEND,
    RTF_SINGLETASK,
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

AROS_UFH3S(struct ExecBase *, GM_UNIQUENAME(init),
    AROS_UFHA(struct MemHeader *, mh, D0),
    AROS_UFHA(struct TagItem *, tagList, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct Task *t;
    struct MemList *ml;
    struct ExceptionContext *ctx;
    int i;

    /*
     * exec.library init routine is a little bit magic. The magic is that it
     * can be run twice.
     * First time it's run manually from kernel.resource's ROMTag scanner in order
     * to create initial ExecBase. This condition is determined by SysBase == NULL
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
    if (!SysBase)
    	return PrepareExecBase(mh, tagList);

    DINIT("exec.library init");

    /*
     * TODO: Amiga(tm) port may call PrepareExecBaseMove() here instead of hardlinking
     * it from within the boot code.
     */

    /*
     * kernel.resource is up and running and memory list is complete.
     * Complete boot task with ETask and CPU context.
     */
    t = SysBase->ThisTask;

    ml                        = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);
    t->tc_UnionETask.tc_ETask = AllocVec(sizeof(struct IntETask), MEMF_ANY|MEMF_CLEAR);
    ctx                       = KrnCreateContext();

    D(bug("[exec] MemList 0x%p, ETask 0x%p, CPU context 0x%p\n", ml, t->tc_UnionETask.tc_ETask, ctx));

    if (!ml || !t->tc_UnionETask.tc_ETask || !ctx)
    {
	DINIT("Not enough memory for first task");
	return NULL;
    }

    /*
     * Build a memory list for the task.
     * It doesn't include stack because it wasn't allocated by us.
     */
    ml->ml_NumEntries      = 1;
    ml->ml_ME[0].me_Addr   = t;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    AddHead(&t->tc_MemEntry, &ml->ml_Node);

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

    /*
     * This code returns, allowing more RTF_SINGLETASK modules to get initialized after us.
     * Kernel.resource's startup code has to InitCode(RTF_COLDSTART) itself.
     */
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
