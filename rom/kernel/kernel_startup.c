/*
 * This is just a hack to get some valid value in kb_PageSize in generic code.
 * It is expected to be overriden in every port. kb_PageSize needs to be set
 * up by your CPU/MMU initialization code.
 */

#include <aros/symbolsets.h>
#include <exec/types.h>

#include "kernel_base.h"

static int mmu_Init(struct KernelBase *KernelBase)
{
    KernelBase->kb_PageSize = 4096;
    return TRUE;
}

ADD2INITLIB(mmu_Init, 0);
