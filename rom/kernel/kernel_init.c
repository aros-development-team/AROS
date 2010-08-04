#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <string.h>

#include <kernel_base.h>
#include <kernel_init.h>
#include <kernel_debug.h>

#define D(x)

void __clear_bss(struct KernelBSS *bss)
{
    while (bss->addr) {
	bzero((void*)bss->addr, bss->len);
        bss++;
    }
}

static int Kernel_Init(struct KernelBase *KernelBase)
{
    int i;

    D(bug("[KRN] Kernel_Init(0x%p)\n", KernelBase));

    for (i=0; i < EXCEPTIONS_COUNT; i++)
	NEWLIST(&KernelBase->kb_Exceptions[i]);

    for (i=0; i < IRQ_COUNT; i++)
        NEWLIST(&KernelBase->kb_Interrupts[i]);

    NEWLIST(&KernelBase->kb_Modules);
    InitSemaphore(&KernelBase->kb_ModSem);

    D(bug("[KRN] Kernel_Init() done\n"));
    return 1;
}

ADD2INITLIB(Kernel_Init, 0)
