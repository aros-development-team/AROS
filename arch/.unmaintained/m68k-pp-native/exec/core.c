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

/*
 * Build all interrupt assmbly code needed. Derived from i386-native code.
 */
BUILD_IRQ(0)
BUILD_IRQ(1)
BUILD_IRQ(2)
BUILD_IRQ(3)
BUILD_IRQ(4)
BUILD_IRQ(5)
BUILD_IRQ(6)
BUILD_IRQ(7)

static void irqSetup(struct irqDescriptor *, struct ExecBase *);
static void handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqServer * is);

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
		INSTALL_IRQ_HANDLER(IRQ_LEVEL6, IRQ6_interrupt);
		INSTALL_IRQ_HANDLER(IRQ_LEVEL5, IRQ5_interrupt);
		INSTALL_IRQ_HANDLER(IRQ_LEVEL4, IRQ4_interrupt);
		INSTALL_IRQ_HANDLER(IRQ_LEVEL3, IRQ3_interrupt);
		INSTALL_IRQ_HANDLER(IRQ_LEVEL2, IRQ2_interrupt);
		INSTALL_IRQ_HANDLER(IRQ_LEVEL1, IRQ1_interrupt);

//		WREG_L(0x100) = (ULONG)IRQ0_interrupt;
		WREG_L(0x104) = (ULONG)IRQ1_interrupt;
		WREG_L(0x108) = (ULONG)IRQ2_interrupt;
		WREG_L(0x10c) = (ULONG)IRQ3_interrupt;
		WREG_L(0x110) = (ULONG)IRQ4_interrupt;
		WREG_L(0x114) = (ULONG)IRQ5_interrupt;
		WREG_L(0x118) = (ULONG)IRQ6_interrupt;

		WREG_W(TCTL2) = 0x11;
		WREG_W(TCMP2) = 0x411a;
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
	ULONG imr = RREG_L(IMR);
	/*
	 * On this processor I must clear the flags of those interrupts
	 * that I want to enable.
	 */
	switch (virq) {
		case vHidd_IRQ_Timer:
			imr &= ~(TMR2_F);
		break;
		case vHidd_IRQ_HDD1:
			imr &= ~(INT0_F | INT1_F | INT2_F | INT3_F | INT4_F | INT5_F | INT6_F | INT7_F);
		break;
		case vHidd_IRQ_Serial1:
			imr &= ~(UART1_F);
		break;
		case vHidd_IRQ_Mouse:
			imr &= ~(PEN_F);
		break;
	}
	WREG_L(IMR) = imr;
}


static inline void mask_and_ack_dbirq(unsigned int irq)
{
}

static void do_db_IRQ(unsigned int irq, 
                      unsigned int virq,
                      struct pt_regs * regs)
{
	AROS_GET_SYSBASE
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
		} else {
			D(bug("IRQ server used!? %p (irq=%d,virq=%d)\n",
			      desc->id_server,
			      irq,
			      virq));
		}
		desc->id_status = status;
	}

	/* Exit early if we had no action or it was disabled */
	if (!iServer) {
		D(bug("No IRQ handler found!\n"));
		return;
	}
//	D(bug("Handling virq %d in server now!\n",virq));
	handle_IRQ_event(virq, regs, iServer);

	{
		unsigned int status = desc->id_status & ~IRQ_INPROGRESS;
		desc->id_status = status;
		if (!(status & IRQ_DISABLED))
			enable_db_irq(irq);
	}
}

/*******************************************************************************
    Lowlevel IRQ functions used by each controller
*******************************************************************************/

static void handle_IRQ_event(unsigned int virq, struct pt_regs * regs, struct irqServer * is)
{
	ULONG imr = RREG_L(IMR);
	WREG_L(IMR) = ~0;
	is->is_handler(virq, is->is_UserData, regs);
	WREG_L(IMR) = imr;
}



/*
 * Generic enable/disable code: this just calls
 * down into the PIC-specific version for the actual
 * hardware disable after having gotten the irq
 * controller lock. 
 */
void disable_irq_nosync(unsigned int irq)
{
	AROS_GET_SYSBASE
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
	AROS_GET_SYSBASE
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


// !!! Move this into include file !!!
extern void sys_Dispatch(struct pt_regs * regs);
/*
 * Called from low level assembly code. Handles irq number 'irq'.
 */
/*asmlinkage*/ void do_IRQ(struct pt_regs * regs, ULONG irq)
{	
	AROS_GET_SYSBASE
	BOOL treated = FALSE;
	struct irqDescriptor * irq_desc = &PLATFORMDATA(SysBase)->irq_desc[irq];
	ULONG isr = RREG_L(ISR);
	
	/*
	 * Now the problem with this processor is that it multiplexes multiple
	 * interrupt sources over one IRQ. So I demultiplex them here by
	 * looking at the interrupt pending register depending on what irq
	 * level I have.
	 */

//	D(bug("isr=0x%x,irq=%d\n",isr,irq));
	switch (irq) {
		case 0:

		break;
		
		case 1:

		break;
		
		case 2:

		break;
		
		case 3:

		break;
		
		case 4:
			if (isr & TMR2_F) {
				volatile UWORD tstat2;
				treated = TRUE;
				/*
				 * Leave the following two lines as they are.
				 */
				tstat2 = RREG_W(TSTAT2);
				WREG_W(TSTAT2) = 0;
				/*
				 * Explicitly call the dispatcher here to get Multitasking
				 * going. Hm, might maybe want to put this into the chain
				 * of handlers...
				 */
//				D(bug("------------ Task SWITCH!\n"));
				sys_Dispatch(regs);
				irq_desc->id_count++;

				irq_desc->id_handler->ic_handle(irq,
				                                0, /* -> index of vHidd_IRQ_Timer2 in servers.c */
				                                regs);
			}

			if (isr & UART1_F) {
//				D(bug("-------------- UART IRQ!\n"));
				/* UART 1 */
				treated = TRUE;

				irq_desc->id_count++;
				irq_desc->id_handler->ic_handle(irq,
				                                4, /* -> index of vHidd_IRQ_Serial1 in servers.c */ 
				                                regs);
			}

			if (isr & WDT_F) {
				/* Watchdog timer */
			}

			if (isr & RTC_F) {
				/* real time clock */
			}

			if (isr & LCDC_F) {
				/* LCDC ??? */
			}

			if (isr & KB_F) {
				/* Keyboard */
			}
			
			if (isr & (INT0_F|INT1_F|INT2_F|INT3_F|INT4_F|INT5_F|INT6_F|INT7_F)) {
				/* Port D */
				D(bug("--------------- Keyboard IRQ!\n"));
				treated = TRUE;

				irq_desc->id_count++;
				irq_desc->id_handler->ic_handle(irq,
				                                1, /* -> index of vHidd_IRQ_CustomD */
				                                regs);
				
			}
		break;
		
		case 5:
			if (isr & PEN_F) {
//				D(bug("Pen IRQ!\n"));
				irq_desc->id_count++;
				irq_desc->id_handler->ic_handle(irq,
				                                12, /* index of vHidd_IRQ_Mouse in servers.c */
				                                regs);
				treated = TRUE;
			}
		break;
		
		case 6:
			if (isr & PWM1_F) {
				/* PWM */
			}

			if (isr & TMR1_F) {
				/* TMR1 */
			}
		break;
		
		case 7:
		break;
	}
	
	/*
	 * Check for the configurable IRQs here 
	 */
#if 0
	if ((isr & PWM2_F) && (irq == ((ilcr >> 4) & 0x07))) {
		
	} 
#endif
	 
	if (FALSE == treated) {
		D(bug("Untreated: irq=%d,isr=0x%x\n",irq,isr));
	}
}

static void VBL_handler(int i, void *user, struct pt_regs *regs)
{
	AROS_GET_SYSBASE;
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
