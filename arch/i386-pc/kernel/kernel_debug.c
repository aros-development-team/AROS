#include "kernel_base.h"
#include "kernel_debug.h"

/* At the moment this wraps to old exec.library function */

void Putc(char chr);

int krnPutC(int c, struct KernelBase *KernelBase)
{
    Putc(c);
    return 1;
}
