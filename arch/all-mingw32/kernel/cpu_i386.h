#ifdef __AROS__

#include <aros/i386/cpucontext.h>

/* This was taken from Mingw32's w32api/winnt.h */

/* Context flags */
#define CONTEXT_i386		   0x10000
#define CONTEXT_i486		   0x10000
#define CONTEXT_CONTROL		   (CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER		   (CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	   (CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	   (CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	   (CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL		   (CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)

/* Some lengths */
#define MAXIMUM_SUPPORTED_EXTENSION 512

typedef struct _FLOATING_SAVE_AREA
{
	IPTR	ControlWord;  
	IPTR	StatusWord;
	IPTR	TagWord;
	IPTR	ErrorOffset;
	IPTR	ErrorSelector;
	IPTR	DataOffset;
	IPTR	DataSelector;
	UBYTE	RegisterArea[80];
	IPTR	Cr0NpxState;
} FLOATING_SAVE_AREA;

typedef struct _CONTEXT
{
	IPTR	ContextFlags;
	IPTR	Dr0;
	IPTR	Dr1;
	IPTR	Dr2;
	IPTR	Dr3;
	IPTR	Dr6;
	IPTR	Dr7;
	FLOATING_SAVE_AREA FloatSave;
	IPTR	SegGs;
	IPTR	SegFs;
	IPTR	SegEs;
	IPTR	SegDs;
	IPTR	Edi;
	IPTR	Esi;
	IPTR	Ebx;
	IPTR	Edx;
	IPTR	Ecx;
	IPTR	Eax;
	IPTR	Ebp;
	IPTR	Eip;
	IPTR	SegCs;
	IPTR	EFlags;
	IPTR	Esp;
	IPTR	SegSs;
	BYTE	ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;

/* On AROS side we save also LastError code */
struct AROSCPUContext
{
    struct ExceptionContext regs; /* Public portion */
    ULONG LastError;		  /* LastError code */
};

/*
 * Common part of SAVEREGS and TRAP_SAVEREGS.
 * Saves CPU registers from CONTEXT in struct ExceptionContext.
 */
#define SAVE_CPU(regs, ctx)			\
    ctx.Flags = 0;				\
    ctx.eax = regs->Eax;			\
    ctx.ebx = regs->Ebx;			\
    ctx.ecx = regs->Ecx;			\
    ctx.edx = regs->Edx;			\
    ctx.esi = regs->Esi;			\
    ctx.edi = regs->Edi;			\
    ctx.ebp = regs->Ebp;			\
    ctx.eip = regs->Eip;			\
    ctx.eflags = regs->EFlags;			\
    ctx.esp = regs->Esp;			\
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
    regs->Eax = ctx.eax;					\
    regs->Ebx = ctx.ebx;					\
    regs->Ecx = ctx.ecx;					\
    regs->Edx = ctx.edx;					\
    regs->Esi = ctx.esi;					\
    regs->Edi = ctx.edi;					\
    regs->Ebp = ctx.ebp;					\
    regs->Eip = ctx.eip;					\
    regs->EFlags = ctx.eflags;					\
    regs->Esp = ctx.esp;

/* Provide legacy 8087 context together with SSE one */
#define USE_LEGACY_8087

/* 8087 frame contains private portion (Cr0NpxState) */
#define SIZEOF_8087_FRAME sizeof(FLOATING_SAVE_AREA)

/*
 * Save the whole set of registers in the allocated context space.
 * Also saves FPU and SSE, if available.
 */
#define SAVEREGS(regs, ctx)									\
    SAVE_CPU(regs, ctx->regs);									\
    if (regs->ContextFlags & CONTEXT_FLOATING_POINT)						\
    {												\
	ctx->regs.Flags |= ECF_FPU;								\
	CopyMemQuick(&regs->FloatSave, ctx->regs.FPData, sizeof(FLOATING_SAVE_AREA));		\
    }												\
    if (regs->ContextFlags & CONTEXT_EXTENDED_REGISTERS)					\
    {												\
	ctx->regs.Flags |= ECF_FPX;								\
	CopyMemQuick(regs->ExtendedRegisters, ctx->regs.FXData, MAXIMUM_SUPPORTED_EXTENSION);	\
    }

/*
 * Restore complete set of registers.
 * Restores FPU and SSE only if they present both in CONTEXT and in struct ExceptionContext.
 * This is done because a debugger may want to modify one of frames. In this case it will
 * reset flag of the second frame (which is still unmodified).
 */
#define RESTOREREGS(regs, ctx)									\
{												\
    ULONG orig = regs->ContextFlags;								\
    RESTORE_CPU(regs, ctx->regs);								\
    if (ctx->regs.Flags & ECF_FPU)								\
    {												\
	regs->ContextFlags |= CONTEXT_FLOATING_POINT;						\
	CopyMemQuick(ctx->regs.FPData, &regs->FloatSave, sizeof(FLOATING_SAVE_AREA));		\
    }												\
    if ((ctx->regs.Flags & ECF_FPX) && (orig & CONTEXT_EXTENDED_REGISTERS))			\
    {												\
	regs->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;					\
	CopyMemQuick(ctx->regs.FXData, regs->ExtendedRegisters, MAXIMUM_SUPPORTED_EXTENSION);	\
    }												\
}

/*
 * Similar to SAVEREGS and RESTOREREGS, but actually copies only CPU part.
 * FPU and SSE are specified by references (frames format in host and AROS match).
 * This is for use within trap handling code.
 */
#define TRAP_SAVEREGS(src, dest)					\
    SAVE_CPU(src, dest)							\
    if (src->ContextFlags & CONTEXT_FLOATING_POINT)			\
    {									\
	dest.Flags |= ECF_FPU;						\
	dest.FPData = (struct FPUContext *)&src->FloatSave;		\
    }									\
    if (src->ContextFlags & CONTEXT_EXTENDED_REGISTERS)			\
    {									\
	dest.Flags |= ECF_FPX;						\
	dest.FXData = (struct FPXContext *)src->ExtendedRegisters;	\
    }

#define TRAP_RESTOREREGS(dest, src)				\
    RESTORE_CPU(dest, src);					\
    if (src.Flags & ECF_FPU)					\
	dest->ContextFlags |= CONTEXT_FLOATING_POINT;		\
    if (src.Flags & ECF_FPX)					\
	dest->ContextFlags |= CONTEXT_EXTENDED_REGISTERS;

/*
 * Realign and copy FPU portion from src to dest. It is supposed
 * that common part is already copied.
 */
#define COPY_FPU(src, dest)								\
{											\
    IPTR fpdata = (IPTR)(dest) + sizeof(struct AROSCPUContext);				\
    if ((src)->Flags & ECF_FPU)								\
    {											\
	(dest)->FPData = (struct FPUContext *)fpdata;					\
	fpdata += sizeof(FLOATING_SAVE_AREA);						\
	CopyMemQuick((src)->FPData, (dest)->FPData, sizeof(FLOATING_SAVE_AREA));	\
    }											\
    else										\
	(dest)->FPData = NULL;								\
    if ((src)->Flags & ECF_FPX)								\
    {											\
	fpdata = (fpdata + 15) & ~15;							\
	(dest)->FXData = (struct FPXContext *)fpdata;					\
	CopyMemQuick((src)->FXData, (dest)->FXData, MAXIMUM_SUPPORTED_EXTENSION);	\
    }											\
    else										\
	(dest)->FXData = NULL;								\
}

#define GET_SP(ctx) (void *)ctx->regs.esp

#define EXCEPTIONS_COUNT 18

#endif /* __AROS__ */

/* The following macros need to be visible on both Windows and AROS side */

#define PRINT_CPUCONTEXT(ctx) \
	bug ("    ContextFlags: 0x%08lX\n" \
		 "    ESP=%08lx  EBP=%08lx  EIP=%08lx\n" \
		 "    EAX=%08lx  EBX=%08lx  ECX=%08lx  EDX=%08lx\n" \
		 "    EDI=%08lx  ESI=%08lx  EFLAGS=%08lx\n" \
	    , (ctx)->ContextFlags \
	    , (ctx)->Esp \
	    , (ctx)->Ebp \
	    , (ctx)->Eip \
	    , (ctx)->Eax \
	    , (ctx)->Ebx \
	    , (ctx)->Ecx \
	    , (ctx)->Edx \
	    , (ctx)->Edi \
	    , (ctx)->Esi \
	    , (ctx)->EFlags \
      );

#define CONTEXT_INIT_FLAGS(ctx) (ctx)->ContextFlags = CONTEXT_FULL|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS|CONTEXT_EXTENDED_REGISTERS

#define PC(regs) regs->Eip
#define R0(regs) regs->Eax
