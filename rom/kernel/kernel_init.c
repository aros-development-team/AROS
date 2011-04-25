#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <proto/arossupport.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_timer.h>

/* We have own bug(), so don't use aros/debug.h to avoid conflicts */
#define D(x)

/* Some globals we can't live without */
struct TagItem *BootMsg = NULL;
struct KernelBase *KernelBase = NULL;

void __clear_bss(const struct KernelBSS *bss)
{
    while (bss->addr) {
	bzero((void*)bss->addr, bss->len);
        bss++;
    }
}

static int Kernel_Init(struct KernelBase *kBase)
{
    int i;

    KernelBase = kBase;
    D(bug("[KRN] Kernel_Init(0x%p)\n", KernelBase));

    for (i=0; i < EXCEPTIONS_COUNT; i++)
	NEWLIST(&KernelBase->kb_Exceptions[i]);

    for (i=0; i < IRQ_COUNT; i++)
        NEWLIST(&KernelBase->kb_Interrupts[i]);

    D(bug("[KRN] Kernel_Init() done\n"));
    return 1;
}

ADD2INITLIB(Kernel_Init, 0)
