#ifndef _TRAPS_H
#define _TRAPS_H

#include <asm/irq.h>
#include <asm/linkage.h>

/* Here are some macros used to build trap table in core file. */

#define TRAP_NAME2(nr) nr##_trap(void)
#define TRAP_NAME(nr) TRAP_NAME2(TRAP##nr)

#define BUILD_COMMON_TRAP()                  \
__asm__(                                     \
        "\n"__ALIGN_STR"\n"                  \
        "common_trap:\n\t"                   \
        SAVE_REGS                            \
        "call "SYMBOL_NAME_STR(do_TRAP)"\n\t"\
        RESTORE_REGS                         \
        "rte\n");

#define BUILD_TRAP(nr)                       \
asmlinkage void TRAP_NAME(nr);               \
__asm__(                                     \
        "\n"__ALIGN_STR"\n"                  \
SYMBOL_NAME_STR(TRAP) #nr "_trap:\n\t"       \
        "pushl $"#nr"\n\t"                   \
        "jmp common_trap");

void Init_Traps(void);

#endif /* _TRAPS_H */
