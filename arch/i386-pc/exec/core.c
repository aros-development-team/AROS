/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Interrupt core, part of kernel.resource
    Lang: english
*/

#include "core.h"
#include "traps.h"
//#include "include/machine.i"
#include <asm/io.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <string.h>

#define __text __attribute__((section(".text")))

struct view { unsigned char sign; unsigned char attr; };

extern unsigned int cached_irq_mask;
extern unsigned long io_apic_irqs;
extern struct irqDescriptor irq_desc[];
void handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqServer * is);

/*
 * Build all interrupt assemble code needed. We use some very ugly macros
 * which were taken from linux sources.
 */

BUILD_COMMON_IRQ()

#define BI(x,y) \
	BUILD_IRQ(x##y)

#define BUILD_16_IRQS(x) \
	BI(x,0) BI(x,1) BI(x,2) BI(x,3) \
	BI(x,4) BI(x,5) BI(x,6) BI(x,7) \
	BI(x,8) BI(x,9) BI(x,a) BI(x,b) \
	BI(x,c) BI(x,d) BI(x,e) BI(x,f)

/*
 * ISA PIC or low IO-APIC triggered (INTA-cycle or APIC) interrupts:
 * (these are usually mapped to vectors 0x20-0x30)
 */
BUILD_16_IRQS(0x0)

#undef BUILD_16_IRQS
#undef BI

#define IRQ(x,y) \
	(const void (*)(void))IRQ##x##y##_interrupt

#define IRQLIST_16(x) \
	IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
	IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
	IRQ(x,8), IRQ(x,9), IRQ(x,a), IRQ(x,b), \
	IRQ(x,c), IRQ(x,d), IRQ(x,e), IRQ(x,f)

const void (*interrupt[NR_IRQS])(void) __text = {
	IRQLIST_16(0x0),
};

#undef IRQ
#undef IRQLIST_16

/*
 * This is the 'legacy' 8259A Programmable Interrupt Controller,
 * present in the majority of PC/AT boxes.
 */

static void do_8259A_IRQ(unsigned int, struct pt_regs *);
static void enable_8259A_irq(unsigned int);
void        disable_8259A_irq(unsigned int);

#define startup_8259A_irq   enable_8259A_irq
#define shutdown_8259A_irq  disable_8259A_irq

static const struct irqController i8259_controller  =
{
    "XT-PIC",
    startup_8259A_irq,
    shutdown_8259A_irq,
    do_8259A_IRQ,
    enable_8259A_irq,
    disable_8259A_irq
};

#define __byte(x,y)     (((unsigned char *)&(y))[x])
#define cached_21       (__byte(0,cached_irq_mask))
#define cached_A1       (__byte(1,cached_irq_mask))

void disable_8259A_irq(unsigned int irq)
{
    cached_irq_mask |= (1 << irq);
    if (irq & 8)
    {
        outb(cached_A1, 0xa1);
    } else
    {
        outb(cached_21, 0x21);
    }
}

static void enable_8259A_irq(unsigned int irq)
{
    cached_irq_mask &= ~(1 << irq);
    if (irq & 8) {
        outb(cached_A1,0xA1);
    } else {
        outb(cached_21,0x21);
    }
}

/*
 * Careful! The 8259A is a fragile beast, it pretty
 * much _has_ to be done exactly like this (mask it
 * first, _then_ send the EOI, and the order of EOI
 * to the two 8259s is important!
 */
static inline void mask_and_ack_8259A(unsigned int irq)
{
    cached_irq_mask |= 1 << irq;
    if (irq & 8) {
//        inb(0xA1);	/* DUMMY */
        outb(cached_A1,0xA1);
        outb(0x62,0x20);	/* Specific EOI to cascade */
        outb(0x20,0xA0);
    } else {
//        inb(0x21);	/* DUMMY */
        outb(cached_21,0x21);
        outb(0x20,0x20);
    }
}

static void do_8259A_IRQ(unsigned int irq, struct pt_regs * regs)
{
    struct irqServer * iServer;
    struct irqDescriptor *desc = &irq_desc[irq];

    {
        unsigned int status;
        mask_and_ack_8259A(irq);
        status = desc->id_status & ~(IRQ_REPLAY | IRQ_WAITING);
        iServer = NULL;
        if (!(status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
            iServer = desc->id_server;
            status |= IRQ_INPROGRESS;
        }
        desc->id_status = status;
    }

    /* Exit early if we had no action or it was disabled */
    if (!iServer)
        return;

    handle_IRQ_event(irq, regs, iServer);

    {
        unsigned int status = desc->id_status & ~IRQ_INPROGRESS;
        desc->id_status = status;
        if (!(status & IRQ_DISABLED))
            enable_8259A_irq(irq);
    }
}

/*******************************************************************************
    Lowlevel IRQ functions used by each controller
*******************************************************************************/

void handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqServer * is)
{
	__cli();
	is->is_handler(irq, is->is_UserData, regs);
	__cli();
}

/*
 * Generic enable/disable code: this just calls
 * down into the PIC-specific version for the actual
 * hardware disable after having gotten the irq
 * controller lock. 
 */
void disable_irq_nosync(unsigned int irq)
{
	if (!irq_desc[irq].id_depth++) {
		irq_desc[irq].id_status |= IRQ_DISABLED;
		irq_desc[irq].id_handler->ic_disable(irq);
	}
}

/*
 * Synchronous version of the above, making sure the IRQ is
 * no longer running on any other IRQ..
 */
void disable_irq(unsigned int irq)
{
	disable_irq_nosync(irq);
}

void enable_irq(unsigned int irq)
{
    switch (irq_desc[irq].id_depth)
    {
	case 0: break;
	case 1:
	    irq_desc[irq].id_status &= ~IRQ_DISABLED;
	    irq_desc[irq].id_handler->ic_enable(irq);
	    /* fall throught */
	default:
		irq_desc[irq].id_depth--;
    }
}

/*
 * do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 */
asmlinkage void do_IRQ(struct pt_regs regs)
{	
	/* 
	 * We ack quickly, we don't want the irq controller
	 * thinking we're snobs just because some other CPU has
	 * disabled global interrupts (we have already done the
	 * INT_ACK cycles, it's too late to try to pretend to the
	 * controller that we aren't taking the interrupt).
	 *
	 * 0 return value means that this irq is already being
	 * handled by some other CPU. (or is disabled)
	 */

	irq_desc[regs.orig_eax].id_count++;
	irq_desc[regs.orig_eax].id_handler->ic_handle(regs.orig_eax, &regs);
#if 0
	{
	    int i;
	    for (i=0; i<16; i++)
	    {
		((struct view*)0xb8000)[64+i].attr = 0x1f;
		((struct view*)0xb8000)[64+i].sign = 
		    (irq_desc[i].id_count & 7) ? '0' + (irq_desc[i].id_count & 7):'.';
	    }
	}
#endif

}

extern struct ExecBase *SysBase;
#undef Elapsed
#undef AttnResched

static void VBL_handler(int i, void *user, struct pt_regs *regs)
{
    if (SysBase->Elapsed == 0)
    {
	SysBase->SysFlags |= 0x2000;
	SysBase->AttnResched |= 0x80;
    }
    else SysBase->Elapsed--;
}

static struct irqServer VBlank = { VBL_handler, "VBlank", NULL };

#define HZ	50
#define LATCH	((1193180 + 25)/50)

void irqSet(int irq, struct irqServer *is)
{
    if (is)
    {
        irq_desc[irq].id_server = is;
        irq_desc[irq].id_depth = 0;
        irq_desc[irq].id_status &= ~IRQ_DISABLED;
        irq_desc[irq].id_handler->ic_startup(irq);
    }
}

void irqSetup()
{
    int i;
    
    cached_irq_mask = 0xffff;
    io_apic_irqs = 0;

#if 0
    asm("rep\n\tstosb"
        :
        :"eax"(0),          /* Fill with 0 */
         "D"(&irq_desc),    /* irq_desc table */
         "ecx"(512/4));
#else
    bzero(&irq_desc, 512);
#endif

    for (i = 0; i<NR_IRQS; i++)
    {
        irq_desc[i].id_handler = &i8259_controller;
	irq_desc[i].id_status = IRQ_DISABLED;
	irq_desc[i].id_depth = 0;
	irq_desc[i].id_server = 0;
	
        set_intr_gate(FIRST_EXT_VECTOR + i, interrupt[i]);
    }
    
    outb_p(0x34,0x43);                  /* binary, mode 2, LSB/MSB, ch 0 */
    outb_p(LATCH & 0xff , 0x40);        /* LSB */
    outb(LATCH >> 8 , 0x40);            /* MSB */

    irq_desc[0].id_server = &VBlank;
    irq_desc[0].id_depth = 0;
    irq_desc[0].id_status &= ~IRQ_DISABLED;
    irq_desc[0].id_handler->ic_startup(0);
}

/*
 * Interrupts
 */

int sys_Cause(struct pt_regs);
int sys_Supervisor(struct pt_regs);
int sys_None(struct pt_regs regs) { return 0; }

asmlinkage int sys_ColdReboot(struct pt_regs regs)
{
    __asm__("jmp kernel_startup");
    return 0;
}

int (*sys_call_table[])(struct pt_regs) __text =
{
    sys_Cause,
    sys_ColdReboot,
    sys_Supervisor,
    sys_None
};
