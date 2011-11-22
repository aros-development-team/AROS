#ifdef __AROS__

/* This was taken from Mingw32's winnt.h */

/* Context flags */
#define CONTEXT_AMD64		0x100000

#define CONTEXT_CONTROL		(CONTEXT_AMD64 | 0x1L)
#define CONTEXT_INTEGER		(CONTEXT_AMD64 | 0x2L)
#define CONTEXT_SEGMENTS	(CONTEXT_AMD64 | 0x4L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_AMD64 | 0x8L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)
#define CONTEXT_FULL		(CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)
#define CONTEXT_ALL		(CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS)

typedef struct _M128A
{
    UQUAD Low;
    QUAD  High;
} M128A;

typedef struct _XMM_SAVE_AREA32
{
    UWORD ControlWord;
    UWORD StatusWord;
    UBYTE TagWord;
    UBYTE Reserved1;
    UWORD ErrorOpcode;
    ULONG ErrorOffset;
    UWORD ErrorSelector;
    UWORD Reserved2;
    ULONG DataOffset;
    UWORD DataSelector;
    UWORD Reserved3;
    ULONG MxCsr;
    ULONG MxCsr_Mask;
    M128A FloatRegisters[8];
    M128A XmmRegisters[16];
    UBYTE Reserved4[96];
} XMM_SAVE_AREA32;

typedef struct _CONTEXT
{
    IPTR  P1Home;
    IPTR  P2Home;
    IPTR  P3Home;
    IPTR  P4Home;
    IPTR  P5Home;
    IPTR  P6Home;
    ULONG ContextFlags;
    ULONG MxCsr;
    UWORD SegCs;
    UWORD SegDs;
    UWORD SegEs;
    UWORD SegFs;
    UWORD SegGs;
    UWORD SegSs;
    ULONG EFlags;
    IPTR  Dr0;
    IPTR  Dr1;
    IPTR  Dr2;
    IPTR  Dr3;
    IPTR  Dr6;
    IPTR  Dr7;
    IPTR  Rax;
    IPTR  Rcx;
    IPTR  Rdx;
    IPTR  Rbx;
    IPTR  Rsp;
    IPTR  Rbp;
    IPTR  Rsi;
    IPTR  Rdi;
    IPTR  R8;
    IPTR  R9;
    IPTR  R10;
    IPTR  R11;
    IPTR  R12;
    IPTR  R13;
    IPTR  R14;
    IPTR  R15;
    IPTR  Rip;
    union {
	XMM_SAVE_AREA32 FltSave;
	struct {
	    M128A Header[2];
	    M128A Legacy[8];
	    M128A Xmm0;
	    M128A Xmm1;
	    M128A Xmm2;
	    M128A Xmm3;
	    M128A Xmm4;
	    M128A Xmm5;
	    M128A Xmm6;
	    M128A Xmm7;
	    M128A Xmm8;
	    M128A Xmm9;
	    M128A Xmm10;
	    M128A Xmm11;
	    M128A Xmm12;
	    M128A Xmm13;
	    M128A Xmm14;
	    M128A Xmm15;
	};
    };
    M128A VectorRegister[26];
    IPTR  VectorControl;
    IPTR  DebugControl;
    IPTR  LastBranchToRip;
    IPTR  LastBranchFromRip;
    IPTR  LastExceptionToRip;
    IPTR  LastExceptionFromRip;
} CONTEXT;

struct VectorContext
{
    M128A VectorRegister[26];
    IPTR  VectorControl;
    IPTR  DebugControl;
    IPTR  LastBranchToRip;
    IPTR  LastBranchFromRip;
    IPTR  LastExceptionToRip;
    IPTR  LastExceptionFromRip;
};

/* Complete context frame, with Windows private data */
struct AROSCPUContext
{
    struct ExceptionContext regs; /* Public portion		*/
    IPTR  PHome[6];		  /* Some Windows-specific data */
    struct VectorContext vec;
    ULONG LastError;		  /* LastError code		*/
};

/*
 * Common part of SAVEREGS and TRAP_SAVEREGS.
 * Saves CPU registers from CONTEXT in struct ExceptionContext.
 */
#define SAVE_CPU(regs, ctx)			\
    ctx.Flags = 0;				\
    ctx.rax = regs->Rax;			\
    ctx.rbx = regs->Rbx;			\
    ctx.rcx = regs->Rcx;			\
    ctx.rdx = regs->Rdx;			\
    ctx.rsi = regs->Rsi;			\
    ctx.rdi = regs->Rdi;			\
    ctx.r8  = regs->R8;				\
    ctx.r9  = regs->R9;				\
    ctx.r10 = regs->R10;			\
    ctx.r11 = regs->R11;			\
    ctx.r12 = regs->R12;			\
    ctx.r13 = regs->R13;			\
    ctx.r14 = regs->R14;			\
    ctx.r15 = regs->R15;			\
    ctx.rbp = regs->Rbp;			\
    ctx.rip = regs->Rip;			\
    ctx.rflags = regs->EFlags;			\
    ctx.rsp = regs->Rsp;			\
    if (regs->ContextFlags & CONTEXT_SEGMENTS)	\
    {						\
        ctx.Flags |= ECF_SEGMENTS;		\
	ctx.ds = regs->SegDs;			\
	ctx.es = regs->SegEs;			\
	ctx.fs = regs->SegFs;			\
	ctx.gs = regs->SegGs;			\
	ctx.cs = regs->SegCs;			\
	ctx.ss = regs->SegSs;			\
    }

/* 
 * Restore CPU registers.
 * Does not restore segment registers because they are of private use
 * by Windows. We can't modify them.
 */
#define RESTORE_CPU(regs, ctx)					\
    regs->ContextFlags = CONTEXT_CONTROL|CONTEXT_INTEGER;	\
    regs->Rax = ctx.rax;					\
    regs->Rbx = ctx.rbx;					\
    regs->Rcx = ctx.rcx;					\
    regs->Rdx = ctx.rdx;					\
    regs->Rsi = ctx.rsi;					\
    regs->Rdi = ctx.rdi;					\
    regs->R8  = ctx.r8;						\
    regs->R9  = ctx.r9;						\
    regs->R10 = ctx.r10;					\
    regs->R11 = ctx.r11;					\
    regs->R12 = ctx.r12;					\
    regs->R13 = ctx.r13;					\
    regs->R14 = ctx.r14;					\
    regs->R15 = ctx.r15;					\
    regs->Rbp = ctx.rbp;					\
    regs->Rip = ctx.rip;					\
    regs->EFlags = ctx.rflags;					\
    regs->Rsp = ctx.rsp;

/*
 * Save the whole set of registers in the allocated context space.
 * Also saves FPU and SSE, if available.
 */
#define SAVEREGS(regs, ctx)								\
    SAVE_CPU(regs, ctx->regs);								\
    if (regs->ContextFlags & CONTEXT_FLOATING_POINT)					\
    {											\
	ctx->regs.Flags |= ECF_FPX;							\
	CopyMemQuick(&regs->FltSave, ctx->regs.FXData, sizeof(XMM_SAVE_AREA32));	\
    }											\
    CopyMemQuick(&regs->P1Home, ctx->PHome, 6 * sizeof(IPTR));				\
    CopyMemQuick(regs->VectorRegister, &ctx->vec, sizeof(struct VectorContext));

/*
 * Restore complete set of registers.
 * Restores SSE only if the corresponding flag is set in struct ExceptionContext.
 */
#define RESTOREREGS(regs, ctx)								\
    RESTORE_CPU(regs, ctx->regs);							\
    if (ctx->regs.Flags & ECF_FPX)							\
    {											\
	regs->ContextFlags |= CONTEXT_FLOATING_POINT;					\
	CopyMemQuick(ctx->regs.FXData, &regs->FltSave, sizeof(XMM_SAVE_AREA32));	\
	regs->MxCsr = regs->FltSave.MxCsr;						\
    }											\
    CopyMemQuick(ctx->PHome, &regs->P1Home, 6 * sizeof(IPTR));				\
    CopyMemQuick(&ctx->vec, regs->VectorRegister, sizeof(struct VectorContext));

/*
 * Similar to SAVEREGS and RESTOREREGS, but actually copies only public CPU part.
 * SSE frame is specified by reference (frames format in host and AROS match).
 * This is for use within trap handling code.
 */
#define TRAP_SAVEREGS(src, dest)			\
    SAVE_CPU(src, dest)					\
    if (src->ContextFlags & CONTEXT_FLOATING_POINT)	\
    {							\
	dest.Flags |= ECF_FPX;				\
	dest.FXData = &src->FltSave;			\
    }

#define TRAP_RESTOREREGS(dest, src)			\
    RESTORE_CPU(dest, src);				\
    if (src.Flags & ECF_FPX)				\
    {							\
	dest->ContextFlags |= CONTEXT_FLOATING_POINT;	\
	dest->MxCsr = dest->FltSave.MxCsr;		\
    }

/*
 * Realign and copy FPU portion from src to dest. It is supposed
 * that common part is already copied.
 */
#define COPY_FPU(src, dest)								\
    if ((src)->Flags & ECF_FPX)								\
    {											\
        IPTR fpdata = (IPTR)(dest) + sizeof(struct AROSCPUContext);			\
	fpdata = (fpdata + 15) & ~15;							\
	(dest)->FXData = (struct FPXContext *)fpdata;					\
	CopyMemQuick((src)->FXData, (dest)->FXData, sizeof(struct FPXContext));		\
    }											\
    else										\
	(dest)->FXData = NULL;

#define GET_SP(ctx) (void *)ctx->regs.rsp

#define EXCEPTIONS_COUNT 18

#define PRINT_CPUCONTEXT(ctx) \
	bug ("    ContextFlags: 0x%08lX\n" \
		 "    RSP=%016lx  RBP=%016lx  RIP=%016lx\n" \
		 "    RAX=%016lx  RBX=%016lx  RCX=%016lx  RDX=%016lx\n" \
		 "    R08=%016lx  R09=%016lx  R10=%016lx  R11=%016lx\n" \
		 "    R12=%016lx  R13=%016lx  R14=%016lx  R15=%016lx\n" \
		 "    RDI=%016lx  RSI=%016lx  EFLAGS=%08lx\n" \
	    , (ctx)->ContextFlags \
	    , (ctx)->Rsp, (ctx)->Rbp, (ctx)->Rip \
	    , (ctx)->Rax, (ctx)->Rbx, (ctx)->Rcx, (ctx)->Rdx \
	    , (ctx)->R8 , (ctx)->R9 , (ctx)->R10, (ctx)->R11 \
	    , (ctx)->R12, (ctx)->R13, (ctx)->R14, (ctx)->R15 \
	    , (ctx)->Rdi, (ctx)->Rsi, (ctx)->EFlags \
      );

#else /* __AROS__ */

#define PRINT_CPUCONTEXT(ctx) \
	printf("    ContextFlags: 0x%08lX\n" \
		 "    RSP=%016I64x  RBP=%016I64x  RIP=%016I64x\n" \
		 "    RAX=%016I64x  RBX=%016I64x  RCX=%016I64x  RDX=%016I64x\n" \
		 "    R08=%016I64x  R09=%016I64x  R10=%016I64x  R11=%016I64x\n" \
		 "    R12=%016I64x  R13=%016I64x  R14=%016I64x  R15=%016I64x\n" \
		 "    RDI=%016I64x  RSI=%016I64x  EFLAGS=%08lx\n" \
	    , (ctx)->ContextFlags \
	    , (ctx)->Rsp, (ctx)->Rbp, (ctx)->Rip \
	    , (ctx)->Rax, (ctx)->Rbx, (ctx)->Rcx, (ctx)->Rdx \
	    , (ctx)->R8 , (ctx)->R9 , (ctx)->R10, (ctx)->R11 \
	    , (ctx)->R12, (ctx)->R13, (ctx)->R14, (ctx)->R15 \
	    , (ctx)->Rdi, (ctx)->Rsi, (ctx)->EFlags \
      );

#endif /* __AROS__ */

#define CONTEXT_INIT_FLAGS(ctx) (ctx)->ContextFlags = CONTEXT_ALL

#define PC(regs) regs->Rip
#define R0(regs) regs->Rax
