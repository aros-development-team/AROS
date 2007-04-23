/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ servers for SA1110
    Lang: english
*/

#include <oop/oop.h>
#include <aros/asmcall.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <hidd/irq.h>
#include <utility/utility.h>
#include <hardware/intbits.h>

#include <exec/ptrace.h>
#include <asm/irq.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "irq.h"
#include <aros/core.h>

# define  DEBUG  1
# include <aros/debug.h>


#ifdef SysBase
#undef SysBase
#endif /* SysBase */

void global_server(int cpl, void *isd, struct pt_regs *);

struct irqServer irq_int     = { global_server, "irq: (all) "  , NULL};

void timer_interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw);

/*******************************************************************************
    Two special irq handlers. As we don't need these two interrupts we define
    here dummy handlers.
*******************************************************************************/

void no_action(int cpl, void *dev_id, struct pt_regs *regs, struct irq_staticdata *isd) { }


#define SysBase (isd->sysbase)

void init_Servers(struct irq_staticdata *isd)
{
	HIDDT_IRQ_Handler	*timer;

	irqSet(1,  &irq_int  , (void *)isd, SysBase);

	/* Install timer interrupt */
	timer = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
	if (timer) {
		timer->h_Node.ln_Name = "INT_VERTB emulator";
		timer->h_Node.ln_Type = NT_INTERRUPT;
		timer->h_Node.ln_Pri = 0;
		timer->h_Data = &SysBase->IntVects[INTB_VERTB];
		timer->h_Code = timer_interrupt;
	
		Enqueue((struct List *)&isd->irqlist[vHidd_IRQ_Timer], (struct Node *)timer);
	}
}

#undef SysBase

/*******************************************************************************
    This timer interrupt is used to keep compatibility with old Amiga software
*******************************************************************************/

void timer_interrupt(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
	struct IntVector *iv = irq->h_Data;

	if (iv->iv_Code)
	{
		/*  Call it. I call with all these parameters for a reason.

			In my `Amiga ROM Kernel Reference Manual: Libraries and
			Devices' (the 1.3 version), interrupt servers are called
			with the following 5 parameters.

			D1 - Mask of INTENAR and INTREQR
			A0 - 0xDFF000 (base of custom chips)
			A1 - Interrupt Data
			A5 - Interrupt Code vector
			A6 - SysBase

			It is quite possible that some code uses all of these, so
			I must supply them here. Obviously I will dummy some of these
			though.
		*/
		AROS_UFC5(void, iv->iv_Code,
		  AROS_UFCA(ULONG, 0, D1),
		  AROS_UFCA(ULONG, 0, A0),
		  AROS_UFCA(APTR, iv->iv_Data, A1),
		  AROS_UFCA(APTR, iv->iv_Code, A5),
		  AROS_UFCA(struct ExecBase *, hw->sysBase, A6));
	}
#if 0
	if (--SysBase->Elapsed == 0) {
		SysBase->SysFlags |= 0x2000;
		SysBase->AttnResched |= 0x80;
	}
#endif
}


/*******************************************************************************
    Global Interrupt Handler
    
    This piece of code translates real irq number to AROS specific irq id. This
    allows us to map system-dependent irqs (audio, ethernet and so on) into well
    defined ids.
*******************************************************************************/

#undef SysBase
#define SysBase (isd->sysbase)

HIDDT_IRQ_Id translation_table[] = {
	vHidd_IRQ_Timer,
	vHidd_IRQ_Keyboard,
/* 2 */	-1,
	vHidd_IRQ_Serial2,
	vHidd_IRQ_Serial1,
/* 5 */	-1,
	-1, //vHidd_IRQ_Floppy,
/* 7 */	-1,
	vHidd_IRQ_RTC,
/* 9 */	-1,
	-1,
	-1,
	-1, //vHidd_IRQ_Mouse,
/* 13 */-1,
	-1, //vHidd_IRQ_HDD1,
	-1  //vHidd_IRQ_HDD2
};


void global_server(int cpl, void *_isd, struct pt_regs *regs)
{
	struct irq_staticdata * isd = (struct irq_staticdata *)_isd;
	if (cpl >= 0 && cpl <= 15) {
		HIDDT_IRQ_Id          id;
		HIDDT_IRQ_HwInfo      hwinfo;
		HIDDT_IRQ_Handler    *handler;

		if (-1 == (id = translation_table[cpl]))
			return;

		hwinfo.Error = 0;	/* No errorcode */
		hwinfo.sysBase = isd->sysbase;

		/* Execute all installed handlers */
		ForeachNode(&isd->irqlist[id], handler) {
			handler->h_Code(handler, &hwinfo);
		}
	}
	/* Leave the interrupt. */
}
