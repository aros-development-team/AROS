#include <asm/amcc440.h>
#include "kernel_intern.h"
#include "syscall.h"

void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    
    D(bug("[KRN] SysCall handler. context @ %p SC=%d\n", ctx, ctx->gpr[3]));
    
    switch (ctx->gpr[3])
    {
        case SC_CLI:
            ctx->srr1 &= ~MSR_EE;
            break;
        
        case SC_STI:
            ctx->srr1 |= MSR_EE;
            break;
    }
    
    core_LeaveInterrupt(ctx);
}
