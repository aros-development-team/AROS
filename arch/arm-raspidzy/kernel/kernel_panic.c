#include <aros/config.h>
#include <asm/cpu.h>

#include <stdio.h>

#include "kernel_base.h"
#include "kernel_debug.h"

void krnPanic(struct KernelBase *KernelBase,const char *fmt, ...)
{
    for (;;) HALT;
}
