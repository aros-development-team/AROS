/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_PROCESSOR_H_
#define _ASM_PROCESSOR_H_

#if defined(__i386__) || defined(__x86_64__)
/* Only the family is ever looked at (ttm: "is this at least a 486") */
struct cpuinfo_x86 {
    int x86;
};
static const struct cpuinfo_x86 boot_cpu_data = { .x86 = 6 };
#endif

#endif
