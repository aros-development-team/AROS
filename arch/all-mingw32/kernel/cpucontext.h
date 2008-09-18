struct AROSCPUContext
{
    CONTEXT regs;
    DWORD LastError;
};

#ifdef __i386__
#define REG_SAVE_VAR DWORD SegCS_Save, SegSS_Save

#define CONTEXT_INIT_FLAGS(ctx) (ctx)->ContextFlags = CONTEXT_FULL|CONTEXT_INTEGER|CONTEXT_FLOATING_POINT|CONTEXT_DEBUG_REGISTERS|CONTEXT_EXTENDED_REGISTERS

#define CONTEXT_SAVE_REGS(ctx)    SegCS_Save = (ctx)->SegCs; \
    			          SegSS_Save = (ctx)->SegSs

#define CONTEXT_RESTORE_REGS(ctx) (ctx)->SegCs = SegCS_Save; \
				  (ctx)->SegSs = SegSS_Save; \
				  (ctx)->ContextFlags &= CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_FLOATING_POINT|CONTEXT_EXTENDED_REGISTERS

#define PRINT_CPUCONTEXT(ctx) \
	printf ("    ContextFlags: 0x%08lX\n" \
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

#else
#error Unsupported CPU type
#endif
