#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

/*
 * KernelBase is an optional parameter here. During
 * very early startup it can be NULL.
 */

int krnPutC(int chr, struct KernelBase *KernelBase)
{
    /* The implementation is architecture-specific */
    return 1;
}
