/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#ifndef CPU_RISCV_H_
#define CPU_RISCV_H_

#include <inttypes.h>

#define EXCEPTIONS_COUNT	1

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

#define ADDTIME(dest, src)			        \
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						                \
        (dest)->tv_secs++;			        \
        (dest)->tv_micro -= 1000000;		\
    }

#define goSuper() 0
#define goUser()

#undef krnSysCall
#define krnSysCall(n) \
    asm volatile ( \
    "\taddi a7, zero, %[swi_no]\n" \
    "\tecall\n" \
    : : [swi_no] "I" (n) : "ra");


#endif /* CPU_RISCV_H_ */
