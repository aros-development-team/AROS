/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: The machine ID registers, asked of the SBI.

    mvendorid, marchid and mimpid are M-mode CSRs, unreadable from the
    supervisor mode this system runs in; the SBI base extension answers
    for them.
*/

#include <exec/types.h>

#include "processor_arch_intern.h"

#define SBI_EXT_BASE                0x10
#define SBI_BASE_GET_MVENDORID      4
#define SBI_BASE_GET_MARCHID        5
#define SBI_BASE_GET_MIMPID         6

static UQUAD sbi_base_get(unsigned long fid)
{
    register unsigned long r_a0 asm("a0") = 0;
    register unsigned long r_a1 asm("a1") = 0;
    register unsigned long r_a6 asm("a6") = fid;
    register unsigned long r_a7 asm("a7") = SBI_EXT_BASE;

    asm volatile("ecall"
                 : "+r"(r_a0), "+r"(r_a1)
                 : "r"(r_a6), "r"(r_a7)
                 : "memory");

    /* a0 is the error, a1 the value */
    return (r_a0 == 0) ? (UQUAD)r_a1 : 0;
}

void Processor_PlatformReadIDs(UQUAD *vendor, UQUAD *archid, UQUAD *impl)
{
    *vendor = sbi_base_get(SBI_BASE_GET_MVENDORID);
    *archid = sbi_base_get(SBI_BASE_GET_MARCHID);
    *impl   = sbi_base_get(SBI_BASE_GET_MIMPID);
}
