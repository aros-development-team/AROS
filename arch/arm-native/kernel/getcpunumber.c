/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(cpuid_t, KrnGetCPUNumber,
	 struct KernelBase *, KernelBase, 41, Kernel)
{
    AROS_LIBFUNC_INIT

    register unsigned int superSP;
    uint32_t tmp;

    asm volatile (
        "       stmfd   sp!, {lr}               \n"
        "       mov     r1, sp                  \n"
        "       swi     %[swi_no]               \n"
        "       mov     %[superSP], sp          \n"
        "       mov     sp, r1                  \n"
        "       ldmfd   sp!, {lr}               \n"
        : [superSP] "=r" (superSP)
        : [swi_no] "I" (6 /*SC_SUPERSTATE*/) : "r1"
    );

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));

    if (superSP)
    {
        asm volatile (
            "       stmfd   sp!, {lr}               \n"
            "       mov     r1, sp                  \n"
            "       mov     sp, %[superSP]          \n"
            "       cpsie   i, %[mode_user]         \n"
            "       mov     sp, r1                  \n"
            "       ldmfd   sp!, {lr}               \n"
            : : [superSP] "r" (superSP), [mode_user] "I" (CPUMODE_USER) : "r1" );
    }

    if (tmp & (2 << 30))
    {
        return (cpuid_t)(((tmp & 0xF00) >> 4) | (tmp & 0xF));
    }

    // Uniprocessor System
    return (cpuid_t)0;

    AROS_LIBFUNC_EXIT
}
