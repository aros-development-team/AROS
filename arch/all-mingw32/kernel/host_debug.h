/*
 * Console output in Windows seems to be protected via semaphores. SuspendThread() at the moment
 * of writing something to console leaves it in locked state. Attempt to write something to it from
 * timer interrupt thread leads to a deadlock. So we have to do this.
 */

#if DEBUG
#define D(x) x

static inline void bug(char *format, ...)
{
    va_list args;
    char buf[256];

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    OutputDebugString(buf);
    va_end(args);
}

static inline void PrintCPUContext(CONTEXT *ctx)
{
    bug("ContextFlags=0x%08lX\n", ctx->ContextFlags);
    bug("ESP=%08lx  EFP=%08lx  EIP   =%08lx\n", ctx->Esp, ctx->Ebp, ctx->Eip);
    bug("EAX=%08lx  EBX=%08lx  ECX   =%08lx  EDX=%08lx\n", ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
    bug("EDI=%08lx  ESI=%08lx  EFLAGS=%08lx\n", ctx->Edi, ctx->Esi, ctx->EFlags);
}

#else
#define D(x)
#endif
