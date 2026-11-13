#ifndef KERNEL_CPUFREQ_H
#define KERNEL_CPUFREQ_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: CPU frequency governor helpers for PC platforms
    Lang: english
*/

#include "kernel_base.h"
#include "kernel_intern.h"
#include "apic.h"

#define CPUFREQ_LOAD_HIGH ((ULONG)((70ULL << 32) / 100))
#define CPUFREQ_LOAD_LOW  ((ULONG)((20ULL << 32) / 100))
#define CPUFREQ_LEVELS_DEFAULT 4

void core_CPUFreqUpdate(struct PlatformData *pdata, apicid_t cpuNum);

#endif /* KERNEL_CPUFREQ_H */
