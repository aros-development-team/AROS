#ifndef _TRAPS_H
#define _TRAPS_H

#include <asm/irq.h>
#include <asm/linkage.h>

/* Here are some macros used to build trap table in core file. */

#define TRAP_NAME2(nr) nr##_trap(void)
#define TRAP_NAME(nr) TRAP_NAME2(TRAP##nr)

#    define _stringify(x) #x
#    define stringify(x) _stringify(x)

#define BUILD_COMMON_TRAP() \
__asm__( \
        "\n"__ALIGN_STR"\n" \
        "common_trap:\n\t" \
        SAVE_REGS \
        "call "SYMBOL_NAME_STR(printException)"\n\t" \
        RESTORE_REGS \
        "popl %eax\n\t" \
        "movl 12(%esp),%esp\n\t"  /* Get and use user stack */ \
        "pushl %eax\n\t"   /* Exception number */ \
        "pushl $0\n\t"     /* "Return address" */ \
        "movl %esp,%ebx\n\t"   /* Hold the esp value before pushing! */ \
        "pushl $" stringify(USER_DS) "\n\t"    /* User SS */ \
        "pushl %ebx\n\t"       /* Stack frame */ \
        "pushl $0x3002\n\t"     /* IOPL:3 */ \
        "pushl $" stringify(USER_CS) "\n\t" \
        "pushl $" SYMBOL_NAME_STR(handleException) "\n\t" \
        "iret\n");

#define BUILD_TRAP(nr)                       \
asmlinkage void TRAP_NAME(nr);               \
__asm__(                                    \
        "\n"__ALIGN_STR"\n"                 \
SYMBOL_NAME_STR(TRAP) #nr "_trap:\n\t"  \
        "pushl $"#nr"\n\t"                  \
        "jmp common_trap");

void set_intr_gate(unsigned int n, void *addr);
void set_system_gate(unsigned int n, void *addr);

void Init_Traps(void);

#endif /* _TRAPS_H */
