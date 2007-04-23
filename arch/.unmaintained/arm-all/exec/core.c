/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Core of AROS.
    Lang: english
*/
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/ptrace.h>
#include <proto/exec.h>
#include <hidd/irq.h>

#include <aros/core.h>
#include <asm/irq.h>
#include <asm/registers.h>
#include <asm/cpu.h>

# define  DEBUG  1
# include <aros/debug.h>

#include "arm_exec_internal.h"

/*
 * Build all interrupt assmbly code needed. Derived from i386-native code.
 */
BUILD_IRQ(_Reset)
BUILD_IRQ(_Undef)
BUILD_IRQ(_Prefetch_Abort)
BUILD_IRQ(_Data_Abort)
BUILD_IRQ(_std)
BUILD_IRQ(_FIQ)

static void irqSetup(struct irqDescriptor *, struct ExecBase *);
static void handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqServer * is);


void do_IRQ(struct pt_regs * regs, LONG adjust);

BOOL init_core(struct ExecBase * SysBase)
{
	int rc = FALSE;
	SysBase->PlatformData = AllocMem(sizeof(struct PlatformData), 
	                                 MEMF_CLEAR|MEMF_PUBLIC);
	if (NULL != SysBase->PlatformData) {
		rc = TRUE;
		/*
		 * Now initialise the PlatformData structure.
		 */
		irqSetup(&PLATFORMDATA(SysBase)->irq_desc[0], SysBase);
		/*
		 * Activate the low-level (assembly) interrupt handlers 
		 */
		INSTALL_IRQ_HANDLER(VECTOR_SWI, _sys_swi_handler);
//		WREG_L(VECTOR_PREFETCH_ABORT) = 0xe25ef004;
		WREG_L(VECTOR_DATA_ABORT)     = 0xe25ef004; // NECESSARY!!!
		INSTALL_IRQ_HANDLER(VECTOR_IRQ, IRQ_std_interrupt);
		WREG_L(VECTOR_FIQ)            = 0xe25ef004;
		D(bug("Installed SWI Handler!\r\n"));
		D(bug("*0x08 = %x\n",RREG_L(0x08)));
		D(bug("*0x0c = %x\n",RREG_L(0x0c)));
		D(bug("*0x18 = %x\n",RREG_L(0x18)));
		D(bug("_sys_swi_handler  = %x\n",_sys_swi_handler));
		D(bug("IRQ_std_interrupt = %x\n",IRQ_std_interrupt));

		/* Start the timer */
		WREG_L(OSCR) = 0;
		WREG_L(OIER) = 0x1;
		/* allow this interrupt! */
		WREG_L(ICMR) = (1 << 26);
	}
	return rc;
}

static void do_db_IRQ(unsigned int irq,
                      unsigned int virq,
                      struct pt_regs * regs);
static void disable_db_irq(unsigned int irq);
static void enable_db_irq(unsigned int irq);

#define startup_db_irq   enable_db_irq
#define shutdown_db_irq  disable_db_irq

static struct irqController db_controller =
{
	"Dragonball",     // ic_Name
	startup_db_irq,   // ic_startup
	shutdown_db_irq,  // ic_shutdown
	do_db_IRQ,        // ic_handle
	enable_db_irq,    // ic_enable
	disable_db_irq    // ic_disable
};



static void disable_db_irq(unsigned int irq)
{
}

static void enable_db_irq(unsigned int virq)
{
	ULONG icmr = RREG_L(ICMR);
	/*
	 * On this processor I must clear the flags of those interrupts
	 * that I want to enable.
	 */
	switch (virq) {
		case vHidd_IRQ_Timer2:
//			imr &= ~(TMR2_F);
		break;
		case vHidd_IRQ_CustomD:
//			imr &= ~(INT0_F | INT1_F | INT2_F | INT3_F | INT4_F | INT5_F | INT6_F | INT7_F);
		break;
		case vHidd_IRQ_Serial1:
//			imr &= ~(UART1_F);
		break;
		case vHidd_IRQ_Touchscreen:
//			imr &= ~(PEN_F);
		break;
	}
	WREG_L(ICMR) = icmr;
}


static inline void mask_and_ack_dbirq(unsigned int irq)
{
}

static void do_db_IRQ(unsigned int irq, 
                      unsigned int virq,
                      struct pt_regs * regs)
{
	struct irqServer     * iServer;
	struct irqDescriptor * desc = &PLATFORMDATA(SysBase)->irq_desc[irq];

//	D(bug("In do_db_IRQ(irq=%d,virq=%d)\n",irq,virq));
	{
		unsigned int status;
		mask_and_ack_dbirq(irq);
		status = desc->id_status & ~(IRQ_REPLAY | IRQ_WAITING);
		iServer = NULL;
		if (!(status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
			iServer = desc->id_server;
			status |= IRQ_INPROGRESS;
//			D(bug("Marked IRQ as INPROGRESS!\n"));
		} else {
			D(bug("IRQ server used!? %p status=0x%x (irq=%d,virq=%d)\n",
			      desc->id_server,
			      status,
			      irq,
			      virq));
			if (status & IRQ_INPROGRESS) {
				D(bug("IRQ in progress!?!?!\n"));
			} else {
				D(bug("IRQ disabled!?!?!\n"));
			}
		}
		desc->id_status = status;
	}

	/* Exit early if we had no action or it was disabled */
	if (!iServer) {
		D(bug("No IRQ handler found!\n"));
		return;
	} else {
//		D(bug("IRQ server at %x\n",iServer));
	}
//	D(bug("Handling virq %d in server now!\n",virq));
	handle_IRQ_event(virq, regs, iServer);

	{
		unsigned int status = desc->id_status & ~IRQ_INPROGRESS;
		desc->id_status = status;
//		D(bug("UNMarked IRQ as INPROGRESS!\n"));
		if (!(status & IRQ_DISABLED))
			enable_db_irq(irq);
	}
}

/*******************************************************************************
    Lowlevel IRQ functions used by each controller
*******************************************************************************/

static void handle_IRQ_event(unsigned int virq, struct pt_regs * regs, struct irqServer * is)
{
	ULONG icmr = RREG_L(ICMR);
	WREG_L(ICMR) = 0;
	is->is_handler(virq, is->is_UserData, regs);
	WREG_L(ICMR) = icmr;
}



/*
 * Generic enable/disable code: this just calls
 * down into the PIC-specific version for the actual
 * hardware disable after having gotten the irq
 * controller lock. 
 */
void disable_irq_nosync(unsigned int irq)
{
	if (!PLATFORMDATA(SysBase)->irq_desc[irq].id_depth++) {
		PLATFORMDATA(SysBase)->irq_desc[irq].id_status |= IRQ_DISABLED;
		PLATFORMDATA(SysBase)->irq_desc[irq].id_handler->ic_disable(irq);
	}
}

/*
 * Synchronous version of the above, making sure the IRQ is
 * no longer running on any other IRQ..
 */
void disable_irq(unsigned int virq)
{
	disable_db_irq(virq);
#if 0
	disable_irq_nosync(irq);
#endif
}

void enable_irq(unsigned int virq)
{
	enable_db_irq(virq);
#if 0
	struct irqDescriptor * irq_desc = PLATFORMDATA(SysBase)->irq_desc;
	switch (irq_desc[irq].id_depth) {
		case 0: break;
		case 1:
			irq_desc[irq].id_status &= ~IRQ_DISABLED;
			irq_desc[irq].id_handler->ic_enable(virq);
			/* fall throught */
		default:
			irq_desc[irq].id_depth--;
	}
#endif
}


/*
 * Called from low level assembly code. Handles irq number 'irq'.
 * 'irq' on the ARM implementation is ignored.
 */
void do_IRQ(struct pt_regs * regs, LONG adjust)
{
	BOOL treated = FALSE;
	struct irqDescriptor * irq_desc = &PLATFORMDATA(SysBase)->irq_desc[1];
	ULONG icip = RREG_L(ICIP);
#if 0
	D(bug("In do_IRQ !!!!\n"));
	D(bug("Entering! @regs=%x, icip=%x, lr_svc=%x, spsr=%x, r0=%x\n",
	      &regs,
	      RREG_L(ICIP),
	      regs->lr_svc,
	      regs->cpsr,
	      regs->r0));
#endif
	/*
	 * Now the problem with this processor is that it multiplexes multiple
	 * interrupt sources over one IRQ. So I demultiplex them here by
	 * looking at the interrupt pending register depending on what irq
	 * level I have.
	 */

	/* OS Timer 0 */
	if (icip & (1 << 26)) {
//		D(bug("------------ Task SWITCH!\n"));
		treated = TRUE;
		/*
		 * Explicitly call the dispatcher here to get Multitasking
		 * going. Hm, might maybe want to put this into the chain
		 * of handlers...
		 */
		sys_Dispatch(regs, adjust);

		irq_desc->id_count++;
		irq_desc->id_handler->ic_handle(1,
		                                0, /* -> index of vHidd_IRQ_Timer2 in servers.c */
		                                regs);
		WREG_L(OSSR) = 1;
		WREG_L(OSCR) = 0;
	}

	/* UART */
	if (icip & (1 << 17)) {
//		D(bug("-------------- UART IRQ!\n"));
		/* UART 1 */
		treated = TRUE;

		irq_desc->id_count++;
		irq_desc->id_handler->ic_handle(1,
		                                4, /* -> index of vHidd_IRQ_Serial1 in servers.c */ 
		                                regs);
	}

	if (FALSE == treated) {
		D(bug("Untreated: icip=0x%x\n",icip));
	}
	/*
	 * Reset all flags
	 */
	WREG_L(ICIP) = icip;
#if 0
	D(bug("Leaving! @regs=%x, icip=%x, lr_svc=%x, spsr=%x, r0=%x\n",
	      &regs,
	      RREG_L(ICIP),
	      regs->lr_svc,
	      regs->cpsr,
	      regs->r0));
#endif
}


static void VBL_handler(int i, void *user, struct pt_regs *regs)
{
	if (SysBase->Elapsed == 0) {
		SysBase->SysFlags |= 0x2000;
		SysBase->AttnResched |= 0x80;
	} else {
		SysBase->Elapsed--;
	}
//	D(bug("In VBL handler!\n"));
}

static struct irqServer VBlank = { VBL_handler, "VBlank", NULL };


static void irqSetup(struct irqDescriptor irq_desc[], struct ExecBase * SysBase)
{
	ULONG i;
	
	for (i = 0; i<NR_IRQS; i++) {
		irq_desc[i].id_handler = &db_controller;
		irq_desc[i].id_status  = IRQ_DISABLED;
		irq_desc[i].id_depth   = 0;
		irq_desc[i].id_server  = 0;
	}

	irqSet(0, &VBlank, NULL, SysBase);
	irq_desc[0].id_server = &VBlank;
	irq_desc[0].id_depth = 0;
	irq_desc[0].id_status &= ~IRQ_DISABLED;
	irq_desc[0].id_handler->ic_startup(0);
}


BOOL irqSet(int irq, struct irqServer *is, void * isd, struct ExecBase * SysBase)
{
	BOOL rc = FALSE;
	if (is) {
		struct irqServer * _is = AllocMem(sizeof(struct irqServer),
		                                  MEMF_PUBLIC|MEMF_CLEAR);
		if (NULL != _is) {
			rc = TRUE;
			_is->is_handler  = is->is_handler;
			_is->is_name     = is->is_name;
			_is->is_UserData = isd;
			PLATFORMDATA(SysBase)->irq_desc[irq].id_server  = _is;
			PLATFORMDATA(SysBase)->irq_desc[irq].id_depth   = 0;
			PLATFORMDATA(SysBase)->irq_desc[irq].id_status &= ~IRQ_DISABLED;
			PLATFORMDATA(SysBase)->irq_desc[irq].id_handler->ic_startup(irq);
		}
	}
	return rc;
}


/*
 * Interrupts
 */

LONG sys_Cause(struct pt_regs);
LONG sys_Supervisor(struct pt_regs regs) {return 0;}
LONG sys_None(struct pt_regs regs) { return 0; }

LONG sys_ColdReboot(struct pt_regs regs)
{
//	__asm__("jmp kernel_startup");
	return 0;
}


LONG (*sys_call_table[])(struct pt_regs) =
{
	sys_Cause,
	sys_ColdReboot,
	sys_Supervisor,
	sys_None
};
