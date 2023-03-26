#ifndef KERNEL_CPU_X86_H
#define KERNEL_CPU_X86_H

#if _WORDSEIZE==32
typedef struct int_gate_32bit x86vectgate_t;
#else
typedef struct int_gate_64bit x86vectgate_t;
#endif

#endif /* KERNEL_CPU_X86_H */
