/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cd32.h"
typedef uint32_t ULONG;
#define VOID void
struct CD32Unit { ULONG cu_IntEnable; };
static void CD32_IntDisable(struct CD32Unit *, ULONG);
static struct CD32Unit *active;
static ULONG hardware_mask, pending_bit;
static unsigned disable_depth, irq_count, inject;
static int in_irq;
static void deliver_interrupt(void)
{
    if (disable_depth || in_irq || irq_count || !(hardware_mask & pending_bit))
        return;
    irq_count++;
    in_irq = 1;
    CD32_IntDisable(active, pending_bit);
    in_irq = 0;
}
static void Disable(void) { disable_depth++; }
static void Enable(void)
{
    if (!disable_depth) abort();
    disable_depth--;
    deliver_interrupt();
}
static void writel(ULONG value, ULONG address)
{
    if (address != AKIKO_CDINTENA) abort();
    /* Interrupt after the shadow update and argument evaluation, before
     * the hardware store. Disabled interrupts remain pending until Enable. */
    if (inject) {
        inject = 0;
        deliver_interrupt();
    }
    hardware_mask = value;
}
