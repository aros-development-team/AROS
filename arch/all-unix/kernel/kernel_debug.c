#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include "hostinterface.h"

extern struct HostInterface *HostIFace;

/*
 * KernelBase is an optional parameter here. During
 * very early startup it can be NULL.
 */

int krnPutC(int chr, struct KernelBase *KernelBase)
{
    int r = HostIFace->KPutC(chr);

    AROS_HOST_BARRIER
    return r;
}
