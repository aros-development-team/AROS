#ifndef _TRAPS_H
#define _TRAPS_H

void set_intr_gate(unsigned int n, void *addr);
void set_system_gate(unsigned int n, void *addr);

void Init_Traps(void);

#endif /* _TRAPS_H */