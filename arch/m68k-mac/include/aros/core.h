#ifndef AROS_CORE_H
#define AROS_CORE_H

/*
    (C) 2000 AROS - The Amiga Research OS
    $Id: core.h 16602 2003-03-05 00:54:09Z bergers $

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
        "\n\t"__ALIGN_STR"\n"                \
SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t"   \
	"/* move.b  #0x61,0xdddddebc*/\n\t"       \
        "movem.l %d0-%d7/%a0-%a6,-(%ssp)\n\t"\
        "move.l  %usp,%a0\n\t"               \
        "move.l  %a0,-(%ssp)\n\t"            \
        "move.l  %ssp,%a0\n\t"               \
        "pea     "#nr"\n\t"                  \
        "move.l  %a0,-(%ssp)\n\t"            \
        "bsr "SYMBOL_NAME_STR(do_IRQ)"\n\t"  \
        "add.l   #8,%ssp\n\t"                \
        "move.l  (%ssp)+,%a0\n\t"            \
        "move.l  %a0,%usp\n\t"               \
        "movem.l (%ssp)+,%d0-%d7/%a0-%a6\n\t"\
        "/*move.b  #0x41,0xdddddebc*/\n\t"       \
        "rte\n\t");


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
