#ifndef _HIDD_IRQ_H
#define _HIDD_IRQ_H

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Include for the irq system HIDD.
    Lang: English.
*/

#include <linux/linkage.h>
#include <asm/init.h>

#include <asm/segments.h>
#include <hidd/irq.h>
#include <exec/lists.h>

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

/***** IRQ system HIDD *******************/

/* IDs */
#define IID_Hidd_IRQ        "hidd.bus.irq"
#define CLID_Hidd_IRQ       "hidd.bus.irq"

/* misc */

struct irq_staticdata
{
    struct Library	*oopbase;
    struct Library	*utilitybase;
    struct ExecBase	*sysbase;
    Class		*irqclass;
    
    struct List		irqlist[vHidd_IRQ_NumIRQ];    
    ULONG		transtable[16];
};

Class *init_irqclass  ( struct irq_staticdata * );
VOID free_irqclass  ( struct irq_staticdata * );

#define ISD(cl) ((struct irq_staticdata *)cl->UserData)

#define OOPBase		((struct Library *)ISD(cl)->oopbase)
#define UtilityBase	((struct Library *)ISD(cl)->utilitybase)
#define SysBase		(ISD(cl)->sysbase)

/*******************************************************************************
    Common things which are not present inside AROS
*******************************************************************************/

/* interrupt control.. */
#define __sti() __asm__ __volatile__ ("sti": : :"memory")
#define __cli() __asm__ __volatile__ ("cli": : :"memory")

/* this struct defines the way the registers are stored on the 
   stack during an interrupt and system call (SoftIRQ?). */

struct pt_regs {
	long edi;
	long esi;
	long ebp;
	long dummy;
	long ebx;
	long edx;
	long ecx;
	long eax;
	long orig_eax;
	long eip;
	int  xcs;
	long eflags;
//	long esp;	<-- These two will be defined when MP is done!
//	int  xss;
};

/*******************************************************************************
    These macros are used to SAVE/RESTORE stack inside interrupt.
    They also are used to emulate supervisor mode.
    
    I will remove this section as soon as AROS gets a kind of MP.
*******************************************************************************/

#define SAVE_ALL \
	"cld\n\t" \
	"pushal\n\t" \
	"movl %esp,esp\n\t"

#define RESTORE_ALL \
	"movl esp,%esp\n\t" \
	"popal\n\t" \
	"addl $4,%esp\n\t"

#define SUPER \
	"cmpb $0,supervisor\n\t" \
	"jne 1f\n\t" \
	"movl %esp,usp\n\t" \
	"movl ssp,%esp\n\t" \
	"movb $1,supervisor\n" \
	"1:\t"

#define USER \
	"cmpb $0,supervisor\n\t" \
	"je 1f\n\t" \
	"movl %esp,ssp\n\t" \
	"movl usp,%esp\n\t" \
	"movb $0,supervisor\n" \
	"1:\t"

/*
 * IDT vectors usable for external interrupt sources start
 * at 0x20:
 */
#define FIRST_EXTERNAL_VECTOR	0x20

#define SYSCALL_VECTOR		0x80

/*******************************************************************************
    IRQ hardware section
*******************************************************************************/

#define NR_IRQS 224

/*
 * Interrupt controller descriptor. This is all we need
 * to describe about the low-level hardware.
 */
struct hw_interrupt_type {
	const char * typename;
	void (*startup)(unsigned int irq);
	void (*shutdown)(unsigned int irq);
	void (*handle)(unsigned int irq, struct pt_regs * regs);
	void (*enable)(unsigned int irq);
	void (*disable)(unsigned int irq);
};

extern struct hw_interrupt_type no_irq_type;

struct irqaction {
	void (*handler)(int, void *, struct pt_regs *, struct irq_staticdata *);
	const char *name;
	void *dev_id;
	struct irqaction *next;
};

/*
 * This is the "IRQ descriptor", which contains various information
 * about the irq, including what kind of hardware handling it has,
 * whether it is disabled etc etc.
 *
 * Pad this out to 32 bytes for cache and indexing reasons.
 */
typedef struct {
    unsigned int                status;	/* IRQ status - IRQ_INPROGRESS, IRQ_DISABLED */
    struct hw_interrupt_type    *handler;   /* handle/enable/disable functions */
    struct irqaction            *action;    /* IRQ action list */
    unsigned int                depth;      /* Disable depth for nested irq disables */
    unsigned int                count;      /* IRQ counter */
    unsigned int unused[3];
} irq_desc_t;

/*
 * IRQ line status.
 */
#define IRQ_INPROGRESS	1	/* IRQ handler active - do not enter! */
#define IRQ_DISABLED	2	/* IRQ disabled - do not enter! */
#define IRQ_PENDING	4	/* IRQ pending - replay on enable */
#define IRQ_REPLAY	8	/* IRQ has been replayed but not acked yet */
#define IRQ_AUTODETECT	16	/* IRQ is being autodetected */
#define IRQ_WAITING	32	/* IRQ not yet seen - for autodetection */


/*
 * Macros used to build interrupt table
 */

#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define BUILD_COMMON_IRQ() \
__asm__( \
	"\n" __ALIGN_STR"\n" \
	"common_interrupt:\n\t" \
	SAVE_ALL \
	"call "SYMBOL_NAME_STR(do_IRQ)"\n\t" \
	RESTORE_ALL \
	USER \
	"iret");

/*
 * subtle. orig_eax is used by the signal code to distinct between
 * system calls and interrupted 'random user-space'. Thus we have
 * to put a negative value into orig_eax here. (the problem is that
 * both system calls and IRQs want to have small integer numbers in
 * orig_eax, and the syscall code has won the optimization conflict ;)
 */
#define BUILD_IRQ(nr) \
asmlinkage void IRQ_NAME(nr); \
__asm__( \
"\n"__ALIGN_STR"\n" \
SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t" \
	SUPER \
	"pushl $"#nr"-256\n\t" \
	"jmp common_interrupt");

extern irq_desc_t irq_desc[NR_IRQS];
extern void handle_IRQ_event(unsigned int, struct pt_regs *, struct irqaction *);
extern int setup_x86_irq(unsigned int, struct irqaction *);
extern void no_action(int cpl, void *dev_id, struct pt_regs *regs, struct irq_staticdata *isd);
extern void mask_irq(unsigned int irq);
extern void unmask_irq(unsigned int irq);
extern void disable_8259A_irq(unsigned int irq);
extern void make_8259A_irq(unsigned int irq);

extern unsigned long io_apic_irqs;

extern void disable_irq(unsigned int);
extern void disable_irq_nosync(unsigned int);
extern void enable_irq(unsigned int);

#endif /* _HIDD_IRQ_H */
