/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_CPUFEATURE_H_
#define _ASM_CPUFEATURE_H_
#define X86_FEATURE_HYPERVISOR  (4*32+31)
#define boot_cpu_has(bit)       0
#define static_cpu_has(bit)     boot_cpu_has(bit)

#endif
