#ifndef PP_EXEC_INTERNAL_H
#define PP_EXEC_INTERNAL_H


#ifndef NO_PROTOS

#include <exec/types.h>
/*
 * Assembly files don't want to see the prototypes
 */

void sys_Dispatch(struct pt_regs * regs, LONG adjust);
void _sys_SetSR(struct pt_regs * regs, LONG adjust);
void _sys_swi_handler(void);

void switch_to_user_mode(void *, ULONG *);

struct ExecBase * PrepareExecBase(struct MemHeader *);

/*
 * Routines for detecting the memory size.
 */
struct MemHeader * detect_memory(void);
void detect_memory_rest(struct ExecBase * SysBase);

void dm_data_abort_handler(void);

void set_sp_mode(void *, ULONG mode);

/* Only temporarily for testing */
ULONG getsr(void);
ULONG getr9(void);
ULONG getsp(void);
ULONG get_cp15_r0(void);
ULONG get_cp15_r1(void);
ULONG get_cp15_r2(void);

#endif


#define DATA_ABORT_MARKER_ADDRESS	0x300


#endif