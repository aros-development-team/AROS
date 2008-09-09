/*
 * Console output in Windows seems to be protected via semaphores. SuspendThread() at the moment
 * of writing something to console leaves it in locked state. Attempt to write something to it from
 * timer interrupt thread leads to a deadlock. So we have to do this.
 */

#if DEBUG

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
    bug("SP=%08lx  FP=%08lx  PC=%08lx\n", ctx->Esp, ctx->Ebp, ctx->Eip);
    bug("R0=%08lx  R1=%08lx  R2=%08lx  R3=%08lx\n", ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
    bug("R4=%08lx  R5=%08lx  R6=%08lx\n", ctx->Edi, ctx->Esi, ctx->EFlags);
}

#endif
