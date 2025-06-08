/*
 * x86 feature check
 *
 * Copyright (C) 2021-2025 The AROS Dev Team
 * 
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#if defined(__i386__) || defined(__amd64__)
#include "x86.h"
#include "zutil.h"

int ZLIB_INTERNAL x86_cpu_enable_simd = 0;

#define cpuid(num) \
    do { asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(num)); } while(0)

void x86_check_features(void)
{
    int eax, ebx, ecx, edx;

    cpuid(0x00000001);

    if ((edx & 0x4000000) &&
        (ecx & 0x100000) && (ecx & 0x2))
        x86_cpu_enable_simd = 1;
}
#endif
