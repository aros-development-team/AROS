#ifndef _TRAPS_H
#define _TRAPS_H

/* These routines itself are in kernel.resource now */
void set_intr_gate(unsigned int n, void *addr);
void set_system_gate(unsigned int n, void *addr);

#endif /* _TRAPS_H */
