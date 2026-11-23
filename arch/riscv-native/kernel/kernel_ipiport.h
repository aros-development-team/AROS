#ifndef KERNEL_IPIPORT_H_
#define KERNEL_IPIPORT_H_
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: What the native RISC-V ports supply for rom/kernel/kernel_ipicall.c.
          See rom/kernel/kernel_ipicall.h for what each one must do.
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#include <asm/riscv/csr.h>   /* rv_intr_disable/restore */
#include "kernel_riscv.h"

#define KRN_IPICALL
#define KRN_IPICALL_MAXCPUS     RISCV_MAXCPUS

/* IPI_CALL_HOOK (kernel_ipi.h): the IPI_* messages are the request bits here */
#define KRN_IPIF_CALL           0x040

void krnIPIPortRaise(ULONG cpu, ULONG bits);
BOOL krnIPIPortCanServe(void);
void krnIPIPortServe(void);

#define krnIPIPortThisCPU()             krnRVCPUNumber()
#define krnIPIPortIntrDisable()         rv_intr_disable()
#define krnIPIPortIntrRestore(flags)    rv_intr_restore(flags)
#define krnIPIPortPause()               krnRVPause()

#endif /* __AROSEXEC_SMP__ */

#endif /* KERNEL_IPIPORT_H_ */
