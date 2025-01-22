#ifndef PP_EXEC_INTERNAL_H
#define PP_EXEC_INTERNAL_H


#ifndef NO_PROTOS
/*
 * Assembly files don't want to see the prototypes
 */

void _sys_trap1_handler(void);
void sys_Dispatch(struct pt_regs * regs);


void switch_to_user_mode(void *, ULONG *);

struct ExecBase * PrepareExecBase(struct MemHeader *);

/*
 * Routines for detecting the memory size.
 */
struct MemHeader * detect_memory(void);
void detect_memory_rest(struct ExecBase * SysBase);

void dm_bus_error_handler(void);
void dm_addr_error_handler(void);

#endif


#define ADDRESS_ERROR_MARKER_ADDRESS	0x300


#endif