/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_PROCESSOR_H_
#define _ASM_PROCESSOR_H_

#include <linux/types.h>

#if defined(__i386__) || defined(__x86_64__)
/* Only the family is ever looked at (ttm: "is this at least a 486") */
#define X86_VENDOR_INTEL        0
#define X86_VENDOR_AMD          2
#define X86_VENDOR_UNKNOWN      0xff

struct cpuinfo_x86 {
    int x86;
    u8 x86_vendor;
    u8 x86_model;
    u32 x86_vfm;
};
static const struct cpuinfo_x86 boot_cpu_data = { .x86 = 6, .x86_vendor = X86_VENDOR_UNKNOWN };
#define cpu_data(cpu)           boot_cpu_data
#define VFM_MODEL(vfm)          ((vfm) & 0xff)
#endif

#endif
