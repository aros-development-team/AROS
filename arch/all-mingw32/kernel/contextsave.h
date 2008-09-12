#ifdef __i386__
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
