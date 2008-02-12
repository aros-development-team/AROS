#include <asm/amcc440.h>
#include "kernel_intern.h"
#include "syscall.h"

void __attribute__((noreturn)) syscall_handler(regs_t *ctx, uint8_t exception, void *self)
{
    _rkprintf("[KRN] Exception %d handler. Context @ %p\n", exception, ctx);

    _rkprintf("[KRN] SysCall handler. SC=%d\n", ctx->gpr[3]);
    
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
