#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <kernel_base.h>
#include <kernel_debug.h>
#include <kernel_tagitems.h>
#include <kernel_timer.h>

#define D(x)

/* Some globals we can't live without */
struct TagItem *BootMsg = NULL;
struct KernelBase *KernelBase = NULL;
#if AROS_MODULES_DEBUG
static struct MinList *Debug_ModList = NULL;
#endif

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

    NEWLIST(&KernelBase->kb_Modules);
#if AROS_MODULES_DEBUG
    Debug_ModList = &KernelBase->kb_Modules;
#endif
    InitSemaphore(&KernelBase->kb_ModSem);

    KernelBase->kb_KernelModules = (dbg_seg_t *)krnGetTagData(KRN_DebugInfo, 0, BootMsg);

    D(bug("[KRN] Kernel_Init() done\n"));
    return 1;
}

ADD2INITLIB(Kernel_Init, 0)
