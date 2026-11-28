#ifndef KERNEL_IPIPORT_H_
#define KERNEL_IPIPORT_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: What the pc port supplies for rom/kernel/kernel_ipicall.c.
          See rom/kernel/kernel_ipicall.h for what each one must do.
*/

#define KRN_IPICALL
#define KRN_IPICALL_MAXCPUS     256

/* Request bits. Each has its own vector on pc (kernel_ipi.c). */
#define KRN_IPIF_CALL           (1 << 0)
#define KRN_IPIF_RESCHED        (1 << 1)

ULONG krnIPIPortThisCPU(void);
void krnIPIPortRaise(ULONG cpu, ULONG bits);
ULONG krnIPIPortIntrDisable(void);
void krnIPIPortIntrRestore(ULONG flags);
BOOL krnIPIPortCanServe(void);
void krnIPIPortServe(void);

#define krnIPIPortPause()       asm volatile("pause")

#endif /* KERNEL_IPIPORT_H_ */
