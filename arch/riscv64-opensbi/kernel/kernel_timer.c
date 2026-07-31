/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: SBI timer tick for the opensbi-riscv64 target.

    Programs a periodic S-mode timer interrupt through the SBI TIME
    extension. The tick counter is the future scheduler heartbeat; for
    now it just counts.
*/

#include <inttypes.h>

#include <asm/cpu.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"

volatile uint64_t __timer_ticks;

static uint64_t tick_interval;

static inline uint64_t rdtime(void)
{
    uint64_t v;
    asm volatile("rdtime %0" : "=r"(v));
    return v;
}

void krnTimerInit(uint32_t timebase_hz, uint32_t tick_hz)
{
    tick_interval = timebase_hz / tick_hz;

    sbi_set_timer(rdtime() + tick_interval);
    csr_set(sie, SIE_STIE);
}

void krnTimerTick(void)
{
    __timer_ticks++;
    sbi_set_timer(rdtime() + tick_interval);
}
