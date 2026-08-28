#ifndef _RISCV_TIMER_PLATFORM_H
#define _RISCV_TIMER_PLATFORM_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: opensbi-riscv64 timer device private structures
*/

#include <aros/platformtimer.h>

struct PlatformTimer
{
    struct KrnPlatformTimer *tb_KPT;        /* The kernel's timer, shared with us       */
    UQUAD                    tb_LastTime;   /* Time CSR value at the last EClock update */
    LONG                     tb_TimerIRQNum;
    struct timeval           tb_VBlankTime;
};

#endif /* _RISCV_TIMER_PLATFORM_H */
