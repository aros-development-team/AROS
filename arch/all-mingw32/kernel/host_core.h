#include "kernel_cpu.h"

#ifndef __AROS__
#define IPTR ULONG_PTR
#endif

#define SLEEP_MODE_OFF     0
#define SLEEP_MODE_PENDING 1
#define SLEEP_MODE_ON      2

/* AROS-side functions to be called by host-side DLL */
struct CoreInterface
{
    int (*HandleTrap)(unsigned int num, IPTR *args, CONTEXT *regs, ULONG *LastErrorPtr);
    void (*HandleIRQ)(unsigned int num, CONTEXT *regs, ULONG *LastErrorPtr);
};
