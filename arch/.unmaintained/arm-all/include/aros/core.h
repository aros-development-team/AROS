#ifndef AROS_CORE_H
#define AROS_CORE_H

/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Interrupt core, part of kernel.resource
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <asm/irq.h>
#include <asm/linkage.h>

/* Here are some macros used to build interrupt table in core file. */

#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define BUILD_IRQ(nr)                        \
/*asmlinkage*/ void IRQ_NAME(nr);            \
__asm__(                                     \
        "\n\t.text\n\t"                      \
        ".globl "SYMBOL_NAME_STR(IRQ) #nr "_interrupt\n\t"           \
        ".type  "SYMBOL_NAME_STR(IRQ) #nr "_interrupt, function\n\t" \
        "\n\t"__ALIGN_STR"\n"                \
SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t"   \
        "sub    sp, sp, #(17*4)\n\t"         \
        "stmia  sp, {r0-r12}\n\t"            \
        "add    r8, sp, #(13*4)\n\t"         \
        "stmia  r8, {sp,lr}^\n\t"            \
        "nop    \n\t"                        \
        "str    lr, [sp, #(15*4)]\n\t"       \
        "mrs	r8, spsr\n\t"                \
        "str	r8, [sp, #(16*4)]\n\t"       \
        "mov	r0,sp\n\t"                   \
        "mov	r1,#0\n\t"                   \
        "bl    "SYMBOL_NAME_STR(do_IRQ)"\n\t"\
        "nop	\n\t"                        \
        "ldr    r0, [sp, #(16*4)]\n\t"       \
        "msr	spsr,r0\n\t"                 \
        "ldr	lr, [sp, #(15*4)]\n\t"       \
        "ldmia  sp, {r0-lr}^\n\t"            \
        "nop	\n\t"                        \
        "add	sp, sp, #(17*4)\n\t"         \
        "subs   pc, lr, #4\n\t");

struct PlatformData
{
	struct irqDescriptor    irq_desc[NR_IRQS];
};

#define PLATFORMDATA(x) ((struct PlatformData *)(x)->PlatformData)

BOOL init_core(struct ExecBase *);
BOOL irqSet(int, struct irqServer *, void *, struct ExecBase *);

void disable_irq(unsigned int virq);
void  enable_irq(unsigned int virq);

#endif
