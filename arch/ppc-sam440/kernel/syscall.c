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
        
        case SC_CAUSE:
            {
                struct ExecBase *SysBase = getSysBase();
                if (SysBase)
                    core_Cause(SysBase);
            }
            break;
        
        case SC_DISPATCH:
            core_Dispatch(ctx);
            break;
        
        case SC_SWITCH:
            core_Switch(ctx);
            break;
        
        case SC_SCHEDULE:
            core_Schedule(ctx);
            break;
        
        case SC_INVALIDATED:
        {
            char *start = (char*)((IPTR)ctx->gpr[4] & 0xffffffe0);
            char *end = (char*)(((IPTR)ctx->gpr[4] + ctx->gpr[5] + 31) & 0xffffffe0);
            char *ptr;
            
            for (ptr = start; ptr < end; ptr +=32)
            {
                asm volatile("dcbi 0,%0"::"r"(ptr));
            }
            asm volatile("sync");
        }
    }
    
    core_LeaveInterrupt(ctx);
}
