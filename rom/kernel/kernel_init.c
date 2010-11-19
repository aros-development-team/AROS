#include <aros/debug.h>
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

/*
 * Private exec.library include, needed for MEMCHUNK_TOTAL.
 * TODO: may be bring it out to public includes ?
 */
#include "memory.h"

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

APTR krnAllocBootMem(struct MemHeader *mh, ULONG size)
{
    APTR ret = mh->mh_First;

    size = (size + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1);

    mh->mh_First          = (struct MemChunk *)(ret + size);
    mh->mh_First->mc_Next = NULL;
    mh->mh_Free           = mh->mh_First->mc_Bytes = mh->mh_Free - size;

    return ret;
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
    InitSemaphore(&KernelBase->kb_ModSem);

    KernelBase->kb_KernelModules = (dbg_seg_t *)krnGetTagData(KRN_DebugInfo, 0, BootMsg);

    D(bug("[KRN] Kernel_Init() done\n"));
    return 1;
}

ADD2INITLIB(Kernel_Init, 0)
