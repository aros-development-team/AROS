/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_HYPERVISOR_H_
#define _ASM_HYPERVISOR_H_

#include <linux/types.h>

enum x86_hypervisor_type {
    X86_HYPER_NATIVE = 0,
    X86_HYPER_VMWARE,
    X86_HYPER_MS_HYPERV,
    X86_HYPER_XEN_PV,
    X86_HYPER_XEN_HVM,
    X86_HYPER_KVM,
    X86_HYPER_JAILHOUSE,
    X86_HYPER_ACRN,
};

/* The hypervisor CPUID leaf names the vendor; "VMwareVMware" is VMware's. */
static inline bool hypervisor_is_type(enum x86_hypervisor_type type)
{
#if defined(__i386__) || defined(__x86_64__)
    u32 eax, ebx, ecx, edx;
    char sig[13];

    asm volatile("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (0x40000000), "c" (0));
    memcpy(sig, &ebx, 4);
    memcpy(sig + 4, &ecx, 4);
    memcpy(sig + 8, &edx, 4);
    sig[12] = 0;
    if (type == X86_HYPER_VMWARE)
        return strcmp(sig, "VMwareVMware") == 0;
    if (type == X86_HYPER_KVM)
        return strcmp(sig, "KVMKVMKVM") == 0;
    return false;
#else
    return false;
#endif
}

#endif
