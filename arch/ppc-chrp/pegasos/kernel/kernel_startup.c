#include <aros/kernel.h>
#include <proto/arossupport.h>

#include <bootconsole.h>
#include <string.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"

#define D(x) x

static char boot_stack[];
static void kernel_start(struct TagItem *bootMsg);

int __startup kernel_entry(struct TagItem *bootMsg, ULONG magic)
{
    if (magic == AROS_BOOT_MAGIC)
    {
	core_kick(bootMsg, kernel_start);
    }
    return -1;
}

void core_kick(struct TagItem *bootMsg, void *target)
{
    const struct TagItem *bss = LibFindTagItem(KRN_KernelBss, bootMsg);

    /* First clear .bss */
    if (bss)
    	__clear_bss((const struct KernelBSS *)bss->ti_Data);

    asm volatile("mr %%r1, %1; bctr"::"c"(target),"r"(boot_stack + STACK_SIZE - SP_OFFSET));
}

static void kernel_start(struct TagItem *bootMsg)
{
    const struct TagItem *tag = LibFindTagItem(KRN_CmdLine, bootMsg);
    char *serial_options = NULL;

    if (tag)
    {    
	char *opts = strcasestr((char *)tag->ti_Data, "debug=serial");

	if (opts)
	    serial_options = opts + 12;
    }

    /*
     * Pegasos defaults.
     * Machine's only serial port is configured as Serial2 (0x2F8, IRQ 3).
     * ISA I/O space is mapped at 0xFE000000.
     */
    IO_Base     = (void *)0xFE000000;
    Serial_Base = 0x02F8;
    serial_Init(serial_options);

    D(bug("AROS64 - The AROS Research OS, PegasosPPC version. Compiled %s\n", __DATE__));
    D(bug("[Kernel] kernel.resource entry: 0x%p\n", kernel_entry));
    D(bug("[Kernel] Boot stack: 0x%p - 0x%p\n", boot_stack, boot_stack + STACK_SIZE));

    for (;;);
}

/* Our boot-time stack */
static char boot_stack[STACK_SIZE] __attribute__((aligned(16)));
