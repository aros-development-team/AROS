#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#include "intservers.h"

/*
 * Our default IntVectors.
 * This interrupt handler will send an interrupt to a series of queued
 * interrupt servers. Servers should return D0 != 0 (Z clear) if they
 * believe the interrupt was for them, and no further interrupts will
 * be called. This will only check the value in D0 for non-m68k systems,
 * however it SHOULD check the Z-flag on 68k systems.

 * Hmm, in that case I would have to separate it from this file in order
 * to replace it... TODO: this can be done after merging exec_init.c from
 * i386 and PPC native.
*/
AROS_UFH5(void, IntServer,
    AROS_UFHA(ULONG, intMask, D0),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(struct List *, intList, A1),
    AROS_UFHA(APTR, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct Interrupt * irq;

    ForeachNode(intList, irq)
    {
	if( AROS_UFC4(int, irq->is_Code,
		AROS_UFCA(struct Custom *, custom, A0),
		AROS_UFCA(APTR, irq->is_Data, A1),
		AROS_UFCA(APTR, irq->is_Code, A5),
		AROS_UFCA(struct ExecBase *, SysBase, A6)
	))
#ifdef __mc68000
	    ;
#else
	    break;
#endif
    }

    AROS_USERFUNC_EXIT
}

/* VBlankServer. The same as general purpose IntServer but also counts task's quantum */
AROS_UFH5(void, VBlankServer,
    AROS_UFHA(ULONG, intMask, D1),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(struct List *, intList, A1),
    AROS_UFHA(APTR, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    /* First decrease Elapsed time for current task */
    if (SysBase->Elapsed && (--SysBase->Elapsed == 0))
    {
        SysBase->SysFlags    |= SFF_QuantumOver;
        SysBase->AttnResched |= ARF_AttnSwitch;
    }

    /* Chain to the generic routine */
    AROS_UFC5NR(void, IntServer,
	      AROS_UFCA(ULONG, intMask, D1),
	      AROS_UFCA(struct Custom *, custom, A0),
	      AROS_UFCA(struct List *, intList, A1),
	      AROS_UFCA(APTR, intCode, A5),
	      AROS_UFCA(struct ExecBase *, SysBase, A6));

    AROS_USERFUNC_EXIT
}
