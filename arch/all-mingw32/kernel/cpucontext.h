#ifdef __AROS__

/* This was taken from Mingw32's w32api/winnt.h */
#ifdef __i386__
#define SIZE_OF_80387_REGISTERS	80
#define CONTEXT_i386	0x10000
#define CONTEXT_i486	0x10000
#define CONTEXT_CONTROL	(CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER	(CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	(CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	(CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define MAXIMUM_SUPPORTED_EXTENSION  512
typedef struct _FLOATING_SAVE_AREA {
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
struct AROSCPUContext {
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
	ULONG	LastError;
};
#else
#error Unsupported CPU type
#endif

#else

struct AROSCPUContext
{
    CONTEXT regs;
    DWORD LastError;
};

#define kprintf printf

#endif

#ifdef __i386__
#define PRINT_CPUCONTEXT(ctx) \
	kprintf ("    ContextFlags: 0x%08lX\n" \
		 "    ESP=%08lx  EBP=%08lx  EIP=%08lx\n" \
		 "    EAX=%08lx  EBX=%08lx  ECX=%08lx  EDX=%08lx\n" \
		 "    EDI=%08lx  ESI=%08lx  EFLAGS=%08lx\n" \
	    , ctx->ContextFlags \
	    , ctx->Esp \
	    , ctx->Ebp \
	    , ctx->Eip \
	    , ctx->Eax \
	    , ctx->Ebx \
	    , ctx->Ecx \
	    , ctx->Edx \
	    , ctx->Edi \
	    , ctx->Esi \
	    , ctx->EFlags \
      );

#define PREPARE_INITIAL_CONTEXT(ctx, sp, pc) ctx->Ebp = 0;			 \
					     ctx->Eip = (IPTR)pc;		 \
					     ctx->Esp = (IPTR)sp;		 \
					     ctx->ContextFlags = CONTEXT_CONTROL;

#define REG_SAVE_VAR DWORD SegCS_Save, SegSS_Save

#define CONTEXT_INIT_FLAGS(ctx) (ctx)->ContextFlags = CONTEXT_FULL|CONTEXT_INTEGER|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS|CONTEXT_EXTENDED_REGISTERS

#define CONTEXT_SAVE_REGS(ctx)    SegCS_Save = (ctx)->SegCs; \
    			          SegSS_Save = (ctx)->SegSs

#define CONTEXT_RESTORE_REGS(ctx) (ctx)->SegCs = SegCS_Save; \
				  (ctx)->SegSs = SegSS_Save; \
				  (ctx)->ContextFlags &= CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_FLOATING_POINT|CONTEXT_EXTENDED_REGISTERS
#else
#error Unsupported CPU type
#endif
