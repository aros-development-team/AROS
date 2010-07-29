#include <proto/exec.h>

static int Kernel_Init(struct KernelBase *KernelBase)
{
    int i;

    for (i=0; i < EXCEPTIONS_COUNT; i++)
	NEWLIST(&kBase->kb_Exceptions[i]);

    for (i=0; i < IRQ_COUNT; i++)
        NEWLIST(&kBase->kb_Interrupts[i]);

    NEWLIST(&kBase->kb_Modules);
    InitSemaphore(&kBase->kb_ModSem);

    return 1;
}

ADD2INITLIB(Kernel_Init, 0)
