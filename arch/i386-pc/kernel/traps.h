#ifndef _TRAPS_H
#define _TRAPS_H

/* Here are some macros used to build trap table in core file. */

#define TRAP_NAME2(nr) nr##_trap(void)
#define TRAP_NAME(nr) TRAP_NAME2(TRAP##nr)

#define BUILD_TRAP(nr) void TRAP_NAME(nr);

void set_intr_gate(unsigned int n, void *addr);
void set_system_gate(unsigned int n, void *addr);

void Init_Traps(struct PlatformData *data);

#endif /* _TRAPS_H */
