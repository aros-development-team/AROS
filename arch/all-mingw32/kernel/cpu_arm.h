#ifdef __AROS__

/*
 * This was taken from Mingw32ce's winnt.h
 * Ouch, ouch, ouch... We don't have VFP data... No floating point in AROS, sorry. :(
 */

/* Context flags */
#define CONTEXT_ARM     0x0000040
#define CONTEXT_CONTROL (CONTEXT_ARM | 0x00000001L)
#define CONTEXT_INTEGER (CONTEXT_ARM | 0x00000002L)
#define CONTEXT_FULL    (CONTEXT_CONTROL | CONTEXT_INTEGER)

typedef struct _CONTEXT
{
    ULONG ContextFlags;
    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;
    ULONG R12;
    ULONG Sp;
    ULONG Lr;
    ULONG Pc;
    ULONG Psr;
} CONTEXT;

/* Complete context frame, with Windows private data */
struct AROSCPUContext
{
    struct ExceptionContext regs; /* Public portion             */
    ULONG LastError;              /* LastError code             */
};

/*
 * Thank you Microsoft! :( You gave us no FPU. Because of this
 * our register save/restore functions are so simple...
 */

#define TRAP_SAVEREGS(regs, ctx)                       \
    ctx.Flags = 0;                                     \
    CopyMemQuick(&regs->R0, ctx.r, 17 * sizeof(ULONG))

#define TRAP_RESTOREREGS(regs, ctx)                    \
    regs->ContextFlags = CONTEXT_FULL;                 \
    CopyMemQuick(ctx.r, &regs->R0, 17 * sizeof(ULONG))

#define SAVEREGS(regs, ctx)    TRAP_SAVEREGS(regs, ctx->regs)
#define RESTOREREGS(regs, ctx) TRAP_RESTOREREGS(regs, ctx->regs)

#define COPY_FPU(src, dest)

#define GET_SP(ctx) (void *)ctx->regs.sp

#define EXCEPTIONS_COUNT 18

#endif /* __AROS__ */

#define PRINT_CPUCONTEXT(ctx)                               \
    bug ("    R0=%08lX  R1=%08lX  R2 =%08lX  R3 =%08lX\n"   \
         "    R4=%08lX  R5=%08lX  R6 =%08lX  R7 =%08lX\n"   \
         "    R8=%08lX  R9=%08lX  R10=%08lX  R11=%08lX\n"   \
         "    IP=%08lX  SP=%08lX  LR =%08lX  PC =%08lX\n"   \
         "    CPSR=%08lX\n"                                 \
            , (ctx)->R0 , (ctx)->R1, (ctx)->R2 , (ctx)->R3  \
            , (ctx)->R4 , (ctx)->R5, (ctx)->R6 , (ctx)->R7  \
            , (ctx)->R8 , (ctx)->R9, (ctx)->R10, (ctx)->R11 \
            , (ctx)->R12, (ctx)->Sp, (ctx)->Lr , (ctx)->Pc  \
            , (ctx)->Psr                                    \
        )

#define CONTEXT_INIT_FLAGS(ctx) (ctx)->ContextFlags = CONTEXT_FULL

#define PC(regs) regs->Pc
#define R0(regs) regs->R0
