/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386unix version of PrepareContext().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <sigcore.h>
#include "etask.h"
#include "exec_util.h"

#include <aros/libcall.h>
#include <asm/ptrace.h>
#include <asm/segments.h>

#define Regs(t) ((struct pt_regs *)(GetIntETask(t)->iet_Context))

static ULONG *PrepareContext_Common(struct Task *task, APTR entryPoint, APTR fallBack,
    	    	    	    	    struct TagItem *tagList, struct ExecBase *SysBase)
{
    ULONG *regs;
    IPTR  *sp=(IPTR *)task->tc_SPReg;
    IPTR args[8] = {0};
    WORD numargs = 0;

    while(tagList)
    {
    	switch(tagList->ti_Tag)
	{
	    case TAG_MORE:
	    	tagList = (struct TagItem *)tagList->ti_Data;
		continue;
		
	    case TAG_SKIP:
	    	tagList += tagList->ti_Data;
		break;
		
	    case TAG_DONE:
	    	tagList = NULL;
    	    	break;
		
	    #define HANDLEARG(x) \
	    case TASKTAG_ARG ## x: \
	    	args[x - 1] = (IPTR)tagList->ti_Data; \
		if (x > numargs) numargs = x; \
		break;
		
	    HANDLEARG(1)
	    HANDLEARG(2)
	    HANDLEARG(3)
	    HANDLEARG(4)
	    HANDLEARG(5)
	    HANDLEARG(6)
	    HANDLEARG(7)
	    HANDLEARG(8)
	    	
	    #undef HANDLEARG
	}
	
	if (tagList) tagList++;
    }

    if (!(task->tc_Flags & TF_ETASK) )
        return NULL;

    GetIntETask (task)->iet_Context = AllocTaskMem (task
        , SIZEOF_ALL_REGISTERS
        , MEMF_PUBLIC|MEMF_CLEAR
    );

    if (!(regs = (ULONG*)GetIntETask (task)->iet_Context))
        return NULL;

    if (numargs)
    {
	while(numargs--)
	{
	    *--sp = args[numargs];
	}
    }

    /* Push fallBack address */

    *--sp = fallBack;
    

    /* We have to prepare whole context right now so Dispatch()
     * would work propertly */

    *regs++ = 0;		/* ebx */
    *regs++ = 0;		/* ecx */
    *regs++ = 0;		/* edx */
    *regs++ = 0;		/* esi */
    *regs++ = 0;		/* edi */
    *regs++ = 0;		/* ebp */
    *regs++ = 0;		/* eax */
    *regs++ = USER_DS;		/* xds */
    *regs++ = USER_DS;		/* xes */
    *regs++ = (ULONG)entryPoint;/* eip */
    *regs++ = USER_CS;		/* xcs */
    *regs++ = 0x3202;		/* eflags */
    *regs++ = (ULONG)sp;    	/* esp */
    *regs++ = USER_DS;		/* xss */

    task->tc_SPReg = sp;

    return regs;
}

AROS_LH4(BOOL, PrepareContext,
    AROS_LHA(struct Task *, task, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    AROS_LHA(struct TagItem *, tagList, A3),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    return PrepareContext_Common(task, entryPoint, fallBack, tagList, SysBase) ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
}

#warning This needs to be fixed/updated if the way library params are passed is changed

BOOL Exec_PrepareContext_FPU(struct Task *task, APTR entryPoint, APTR fallBack, struct TagItem *tagList, struct ExecBase *SysBase)
{
    ULONG *regs;
    BOOL  retval = FALSE;
    
    if ((regs = PrepareContext_Common(task, entryPoint, fallBack, tagList, SysBase)))    
    {    
    	UBYTE this_fpustate[SIZEOF_FPU_STATE];
	
	asm volatile("fnsave %1\n\t"
                     "fwait\n\t"
                     "fninit\n\t"
                     "fnsave %0\n\t"
                     "fwait\n\t"
                     "frstor %1\n\t"
                     "fwait\n\t" : "=m" (*regs), "=m" (this_fpustate) : : "memory");
	
	retval = TRUE;
    }
    
    return retval;

}

