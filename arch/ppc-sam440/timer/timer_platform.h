/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef TIMER_PLATFORM_H
#define TIMER_PLATFORM_H

#include <kernel_base.h>

struct PlatformTimer
{
    uint32_t tbc_expected;
    uint32_t tbc_achieved;
    int32_t corr;
};

#define KernelBase      ((struct KernelBase *)(TimerBase)->tb_KernelBase)

struct TimerBase;
void TimerSetup(struct TimerBase *TimerBase, uint32_t waste);

#endif /* TIMER_PLATFORM_H */
