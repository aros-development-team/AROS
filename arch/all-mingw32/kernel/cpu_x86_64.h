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

struct AROSCPUContext
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
    ULONG LastError;
};

#define SET_PC(ctx, addr) ctx->Rip = (IPTR)addr

#else

struct AROSCPUContext
{
    CONTEXT regs;
    ULONG LastError;
};

#define kprintf printf

#endif

#define GET_PC(ctx) (void *)ctx->Rip
#define GET_SP(ctx) (void *)ctx->Rsp

#define EXCEPTIONS_COUNT 19

#define PRINT_CPUCONTEXT(ctx) \
	kprintf ("    ContextFlags: 0x%08lX\n" \
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

#define PREPARE_INITIAL_FRAME(ctx, sp, pc) ctx->Rbp = 0;			 \
					     ctx->Rip = (IPTR)pc;		 \
					     ctx->Rsp = (IPTR)sp;		 \
					     ctx->ContextFlags = CONTEXT_CONTROL;

#define REG_SAVE_VAR UWORD SegCS_Save, SegSS_Save

#define CONTEXT_INIT_FLAGS(ctx) (ctx)->ContextFlags = CONTEXT_ALL

#define CONTEXT_SAVE_REGS(ctx)    SegCS_Save = (ctx)->SegCs; \
    			          SegSS_Save = (ctx)->SegSs

#define CONTEXT_RESTORE_REGS(ctx) (ctx)->SegCs = SegCS_Save; \
				  (ctx)->SegSs = SegSS_Save; \
				  (ctx)->ContextFlags &= CONTEXT_FULL
