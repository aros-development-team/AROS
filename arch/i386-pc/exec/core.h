/*
    Copyright © 2000, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Interrupt core, part of kernel.resource
    Lang: english
*/

#include <asm/irq.h>

/* Here are some macros used to build interrupt table in core file. */

#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define BUILD_COMMON_IRQ()                  \
__asm__(                                    \
        "\n"__ALIGN_STR"\n"                 \
        "common_interrupt:\n\t"             \
        SAVE_REGS                           \
        "call "SYMBOL_NAME_STR(do_IRQ)"\n\t"\
        "jmp Exec_ExitIntr\n");

#define BUILD_IRQ(nr)                       \
asmlinkage void IRQ_NAME(nr);               \
__asm__(                                    \
        "\n"__ALIGN_STR"\n"                 \
SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t"  \
        "pushl $"#nr"\n\t"                  \
        "jmp common_interrupt");
