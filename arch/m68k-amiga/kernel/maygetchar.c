#include <aros/kernel.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#include <proto/kernel.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
    struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    extern int DebugMayGetChar(void);

    /* The implementation is entirely architecture-specific */
    return DebugMayGetChar();

    AROS_LIBFUNC_EXIT
}
