/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: i8259A stuff for native AROS
    Lang: english
*/

#include "irq.h"

#include <asm/io.h>
#include <linux/linkage.h>

/*
 * Dummy controller type for unused interrupts
 */
static void do_none(unsigned int irq, struct pt_regs * regs)
{
	/*
	 * we are careful. While for ISA irqs it's common to happen
	 * outside of any driver (think autodetection), this is not
	 * at all nice for PCI interrupts. So we are stricter and
	 * print a warning when such spurious interrupts happen.
	 * Spurious interrupts can confuse other drivers if the PCI
	 * IRQ line is shared.
	 *
	 * Such spurious interrupts are either driver bugs, or
	 * sometimes hw (chipset) bugs.
	 */
	kprintf("unexpected IRQ vector %d!\n",irq);
}
static void enable_none(unsigned int irq) { }
static void disable_none(unsigned int irq) { }

/* startup is the same as "enable", shutdown is same as "disable" */
#define startup_none	enable_none
#define shutdown_none	disable_none

struct hw_interrupt_type no_irq_type = {
	"none",
	startup_none,
	shutdown_none,
	do_none,
	enable_none,
	disable_none
};

/*
 * This is the 'legacy' 8259A Programmable Interrupt Controller,
 * present in the majority of PC/AT boxes.
 */

static void do_8259A_IRQ(unsigned int irq, struct pt_regs * regs);
static void enable_8259A_irq(unsigned int irq);
void disable_8259A_irq(unsigned int irq);

/* startup is the same as "enable", shutdown is same as "disable" */
#define startup_8259A_irq	enable_8259A_irq
#define shutdown_8259A_irq	disable_8259A_irq

static struct hw_interrupt_type i8259A_irq_type = {
	"XT-PIC",
	startup_8259A_irq,
	shutdown_8259A_irq,
	do_8259A_IRQ,
	enable_8259A_irq,
	disable_8259A_irq
};

/*
 * Controller mappings for all interrupt sources:
 */
irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned = { [0 ... NR_IRQS-1] = { 0, &no_irq_type, }};

/*
 * 8259A PIC functions to handle ISA devices:
 */

/*
 * This contains the irq mask for both 8259A irq controllers,
 */
static unsigned int cached_irq_mask = 0xffff;

#define __byte(x,y) 	(((unsigned char *)&(y))[x])
#define cached_21	(__byte(0,cached_irq_mask))
#define cached_A1	(__byte(1,cached_irq_mask))

/*
 * Not all IRQs can be routed through the IO-APIC, eg. on certain (older)
 * boards the timer interrupt is not connected to any IO-APIC pin, it's
 * fed to the CPU IRQ line directly.
 *
 * Any '1' bit in this mask means the IRQ is routed through the IO-APIC.
 * this 'mixed mode' IRQ handling costs nothing because it's only used
 * at IRQ setup time.
 */
unsigned long io_apic_irqs = 0;

/*
 * These have to be protected by the irq controller spinlock
 * before being called.
 */
void disable_8259A_irq(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	cached_irq_mask |= mask;
	if (irq & 8) {
		outb(cached_A1,0xA1);
	} else {
		outb(cached_21,0x21);
	}
}

static void enable_8259A_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);
	cached_irq_mask &= mask;
	if (irq & 8) {
		outb(cached_A1,0xA1);
	} else {
		outb(cached_21,0x21);
	}
}

void make_8259A_irq(unsigned int irq)
{
	disable_irq_nosync(irq);
	io_apic_irqs &= ~(1<<irq);
	irq_desc[irq].handler = &i8259A_irq_type;
	enable_irq(irq);
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
		inb(0xA1);	/* DUMMY */
		outb(cached_A1,0xA1);
		outb(0x62,0x20);	/* Specific EOI to cascade */
		outb(0x20,0xA0);
	} else {
		inb(0x21);	/* DUMMY */
		outb(cached_21,0x21);
		outb(0x20,0x20);
	}
}

static void do_8259A_IRQ(unsigned int irq, struct pt_regs * regs)
{
	struct irqaction * action;
	irq_desc_t *desc = irq_desc + irq;

	{
		unsigned int status;
		mask_and_ack_8259A(irq);
		status = desc->status & ~(IRQ_REPLAY | IRQ_WAITING);
		action = NULL;
		if (!(status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
			action = desc->action;
			status |= IRQ_INPROGRESS;
		}
		desc->status = status;
	}

	/* Exit early if we had no action or it was disabled */
	if (!action)
		return;

	handle_IRQ_event(irq, regs, action);

	{
		unsigned int status = desc->status & ~IRQ_INPROGRESS;
		desc->status = status;
		if (!(status & IRQ_DISABLED))
			enable_8259A_irq(irq);
	}
}

/*
 * This builds up the IRQ handler stubs using some ugly macros in irq.h
 *
 * These macros create the low-level assembly IRQ routines that save
 * register context and call do_IRQ(). do_IRQ() then does all the
 * operations that are needed to keep the AT (or SMP IOAPIC)
 * interrupt-controller happy.
 */

BUILD_COMMON_IRQ()

#define BI(x,y) \
	BUILD_IRQ(##x##y)

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
	IRQ##x##y##_interrupt

#define IRQLIST_16(x) \
	IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
	IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
	IRQ(x,8), IRQ(x,9), IRQ(x,a), IRQ(x,b), \
	IRQ(x,c), IRQ(x,d), IRQ(x,e), IRQ(x,f)

static void (*interrupt[NR_IRQS])(void) = {
	IRQLIST_16(0x0),
};

#undef IRQ
#undef IRQLIST_16

/*
 * Special irq handlers.
 */

void no_action(int cpl, void *dev_id, struct pt_regs *regs) { }

/*
 * Note that on a 486, we don't want to do a SIGFPE on an irq13
 * as the irq is unreliable, and exception 16 works correctly
 * (ie as explained in the intel literature). On a 386, you
 * can't use exception 16 due to bad IBM design, so we have to
 * rely on the less exact irq13.
 *
 * Careful.. Not only is IRQ13 unreliable, but it is also
 * leads to races. IBM designers who came up with it should
 * be shot.
 */
 
static void math_error_irq(int cpl, void *dev_id, struct pt_regs *regs)
{
	outb(0,0xF0);
//	if (ignore_irq13 || !boot_cpu_data.hard_math)
//		return;
//	math_error();
}

static struct irqaction irq13 = { math_error_irq, 0, 0, "fpu", NULL, NULL };

/*
 * IRQ2 is cascade interrupt to second interrupt controller
 */

static struct irqaction irq2  = { no_action, 0, 0, "cascade", NULL, NULL};

/*
 * This should really return information about whether
 * we should do bottom half handling etc. Right now we
 * end up _always_ checking the bottom half, which is a
 * waste of time and is not what some drivers would
 * prefer.
 */
int handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqaction * action)
{
	do {
		__cli();
		action->handler(irq, action->dev_id, regs);
		action = action->next;
	} while (action);

	__cli();

	return 1;
}

/*
 * Generic enable/disable code: this just calls
 * down into the PIC-specific version for the actual
 * hardware disable after having gotten the irq
 * controller lock. 
 */
void disable_irq_nosync(unsigned int irq)
{
	if (!irq_desc[irq].depth++) {
		irq_desc[irq].status |= IRQ_DISABLED;
		irq_desc[irq].handler->disable(irq);
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
	switch (irq_desc[irq].depth) {
	case 1:
		irq_desc[irq].status &= ~IRQ_DISABLED;
		irq_desc[irq].handler->enable(irq);
		/* fall throught */
	default:
		irq_desc[irq].depth--;
		break;
	case 0:
		kprintf("enable_irq() unbalanced!\n");
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
	int irq = regs.orig_eax & 0xff; /* subtle, see irq.h */

	irq_desc[irq].count++;
	irq_desc[irq].handler->handle(irq, &regs);
}

int setup_x86_irq(unsigned int irq, struct irqaction * new)
{
	struct irqaction *old, **p;
	int shared = 0;

	/*
	 * The following block of code has to be executed atomically
	 */
	p = &irq_desc[irq].action;
	if ((old = *p) != NULL) {
		/* add new interrupt at end of irq queue */
		do {
			p = &old->next;
			old = *p;
		} while (old);
		shared = 1;
	}

	*p = new;

	if (!shared) {
		irq_desc[irq].depth = 0;
		irq_desc[irq].status &= ~IRQ_DISABLED;
		irq_desc[irq].handler->startup(irq);
	}
	return 0;
}

struct irqaction * free_irq(unsigned int irq, void *dev_id)
{
	struct irqaction * action, **p;

	if (irq >= NR_IRQS)
		return (void*)0;

	for (p = &irq_desc[irq].action; (action = *p) != NULL; p = &action->next) {
		if (action->dev_id != dev_id)
			continue;

		/* Found it - now free it */
		*p = action->next;

		if (!irq_desc[irq].action) {
			irq_desc[irq].status |= IRQ_DISABLED;
			irq_desc[irq].handler->shutdown(irq);
		}
		
		return action;
	}
	kprintf("Trying to free free IRQ%d\n",irq);
}

void init_ISA_irqs (void)
{
	int i;

	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = 0;
		irq_desc[i].depth = 0;

		if (i < 16) {
			/*
			 * 16 old-style INTA-cycle interrupts:
			 */
			irq_desc[i].handler = &i8259A_irq_type;
		} else {
			/*
			 * 'high' PCI IRQs filled in on demand
			 */
			irq_desc[i].handler = &no_irq_type;
		}
	}
}

#define _set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
	"movw %4,%%dx\n\t" \
	"movl %%eax,%0\n\t" \
	"movl %%edx,%1" \
	:"=m" (*((long *) (gate_addr))), \
	 "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
	:"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "3" ((char *) (addr)),"2" (KERNEL_CS << 16)); \
} while (0)


void set_intr_gate(unsigned int n, void *addr)
{
	_set_gate(8 + (n << 3),14,0,addr);
}

#define HZ	50
#define LATCH	((1193180 + 25)/50)

__initfunc(void init_IRQ(void))
{
	int i;

	init_ISA_irqs();

	/*
	 * Cover the whole vector space, no vector can escape
	 * us. (some of these will be overridden and become
	 * 'special' SMP interrupts)
	 */
	for (i = 0; i < NR_IRQS; i++) {
		int vector = FIRST_EXTERNAL_VECTOR + i;
		if (vector != SYSCALL_VECTOR) 
			set_intr_gate(vector, interrupt[i]);
	}

	/*
	 * Set the clock to 50 Hz, we already have a valid
	 * vector now:
	 */
	outb_p(0x34,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */

	setup_x86_irq(2, &irq2);
	setup_x86_irq(13, &irq13);
}

/*
 * Generic, controller-independent functions:
 */

int get_irq_list(char *buf)
{
	int i, j;
	struct irqaction * action;
	char *p = buf;

	p += sprintf(p, "           ");
	for (j=0; j<1; j++)
		p += sprintf(p, "CPU%d       ",j);
	*p++ = '\n';

	for (i = 0 ; i < NR_IRQS ; i++) {
		action = irq_desc[i].action;
		if (!action) 
			continue;
		p += sprintf(p, "%3d: ",i);
		p += sprintf(p, "%10u ", irq_decs[i].count);
		p += sprintf(p, " %14s", irq_desc[i].handler->typename);
		p += sprintf(p, "  %s", action->name);

		for (action=action->next; action; action = action->next) {
			p += sprintf(p, ", %s", action->name);
		}
		*p++ = '\n';
	}
	p += sprintf(p, "NMI: %10u\n", 0);
	return p - buf;
}
