#define DEBUG 1

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <aros/symbolsets.h>

#include <proto/arossupport.h>

#include <utility/tagitem.h>

#include <strings.h>
#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"

static char boot_stack[];

kerncall void start32(struct TagItem *msg);

/* This is inside exec.library. We still cross-link... */
void exec_cinit(struct TagItem *tags, struct multiboot *mbinfo);

/*
 * Here the history starts. We are already in flat, 32bit mode. All protections
 * are off, CPU is working in Supervisor level (CPL0). Interrupts should
 * be disabled.
 *
 * Here we run on a stack provided by the bootstrap. We can perform calls, but we
 * don't know where it is placed, so we need to switch away from it ASAP.
 */
IPTR __startup kernel_entry(struct TagItem *bootMsg, ULONG magic, struct multiboot *mb)
{
    if (magic == AROS_BOOT_MAGIC)
	core_Kick(bootMsg, mb, exec_cinit);

    return -1;
}

/*
 * The second important place. We come here upon warm restart,
 * with stack pointer set to some safe place.
 */
void kernel_reboot(void)
{
    core_Kick(BootMsg, NULL, exec_cinit);
}

/*
 * This function actually runs the kickstart from the specified address.
 * Before doing this it clears .bss sections of all modules.
 */
void core_Kick(struct TagItem *msg, struct multiboot *mb, void *target)
{
    const struct TagItem *bss = LibFindTagItem(KRN_KernelBss, msg);

    /* First clear .bss */
    if (bss)
    	__clear_bss((const struct KernelBSS *)bss->ti_Data);

    /*
     * ... then switch to initial stack and jump to target address.
     * We set ebp to 0 and use call here in order to get correct stack traces
     * if the boot task crashes. Otherwise backtrace goes beyond this location
     * into memory areas with undefined contents.
     */
    asm volatile("movl	%2, %%esp     \n\t"
    		 "movl	$0, %%ebp     \n\t"
		 "pushl %1            \n\t"
		 "pushl %0            \n\t"
		 "cld                 \n\t"      /* At the startup it's very important   */
		 "cli                 \n\t"      /* to lock all interrupts. Both on the  */
		 "movb	$-1,%%al      \n\t"      /* CPU side and hardware side. We don't  */
		 "outb  %%al,$0x21    \n\t"      /* have proper structures in RAM yet.   */
		 "outb  %%al,$0xa1    \n\t"
    		 "call *%3\n"::"r"(msg), "r"(mb), "r"(boot_stack + STACK_SIZE), "r"(target));
}

/* Our boot-time stack */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));
