#include <asm/amcc440.h>
#include "kernel_intern.h"
#include "syscall.h"

extern char * __text_start;
extern char * __text_end;

void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self)
{
    struct KernelBase *KernelBase = getKernelBase();
    
//    D(bug("[KRN] SysCall handler. context @ %p SC=%d\n", ctx, ctx->gpr[3]));
    
    if ((char*)ctx->srr0 < &__text_start || (char*)ctx->srr0 >= &__text_end)
    {
        D(bug("[KRN] ERROR ERROR! SysCall issued directly outside kernel.resource!!!!!\n"));
        core_LeaveInterrupt(ctx);
    }
    
    switch (ctx->gpr[3])
    {
        case SC_CLI:
            ctx->srr1 &= ~MSR_EE;
            break;
        
        case SC_STI:
            ctx->srr1 |= MSR_EE;
            break;
        
        case SC_SUPERSTATE:
            ctx->srr1 &= ~MSR_PR;
            break;
        
        case SC_ISSUPERSTATE:
            if (ctx->srr1 & MSR_PR)
                ctx->gpr[3] = 0;
            else
                ctx->gpr[3] = 1;
            break;
    }
    
    core_LeaveInterrupt(ctx);
}
