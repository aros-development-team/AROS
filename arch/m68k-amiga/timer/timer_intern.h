#ifndef _TIMER_INTERN_H
#define _TIMER_INTERN_H

/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal information about the timer.device and HIDD's
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif
#ifndef EXEC_INTERRUPTS_H
#include <exec/interrupts.h>
#endif
#ifndef EXEC_IO_H
#include <exec/io.h>
#endif
#ifndef EXEC_DEVICES_H
#include <exec/devices.h>
#endif
#ifndef DEVICES_TIMER_H
#include <devices/timer.h>
#endif

#define NUM_LISTS	2

#define ECLOCK_BASE 0x10000

struct TimerBase
{
    /* Required by the system */
    struct Device tb_Device;

    struct timeval tb_CurrentTime;	/* system time */
    struct timeval tb_Elapsed;
    struct timeval tb_lastsystime;

    struct MinList tb_Lists[NUM_LISTS];

    struct Resource *tb_cia[2];

    struct Resource *tb_eclock_res;
    volatile struct CIA *tb_eclock_cia;
    volatile UBYTE *tb_eclock_cr, *tb_eclock_lo, *tb_eclock_hi;
    struct Interrupt tb_ciaint_eclock;
    UWORD tb_eclock_intbit;
    struct EClockVal tb_eclock;
    ULONG tb_eclock_rate;
    ULONG tb_eclock_to_usec;

    struct Resource *tb_micro_res;
    volatile struct CIA *tb_micro_cia;
    volatile UBYTE *tb_micro_cr, *tb_micro_lo, *tb_micro_hi;
    struct Interrupt tb_ciaint_timer;
    UWORD tb_micro_intbit;
    ULONG tb_micro_started;
    struct timeval tb_micro_count;
    ULONG tb_micro_micros;
    BOOL tb_micro_on;

    struct Interrupt tb_vbint;
    struct timeval tb_vb_count;
    UWORD tb_vblank_rate;
    UWORD tb_vblank_micros;
    BOOL tb_vblank_on;

    ULONG tb_eclock_micro_mult;
    UWORD tb_micro_eclock_mult;
    ULONG  lastsystimetweak;
};

ULONG GetEClock(struct TimerBase *TimerBase);
void CheckTimer(struct TimerBase *TimerBase, UWORD unitnum);
void addmicro(struct TimerBase *TimerBase, struct timeval *tv);
BOOL cmp64(struct timeval *tv1, struct timeval *tv2);
ULONG sub64(struct timeval *larger, struct timeval *smaller);
void add64(struct timeval *dst, struct timeval *src);
void inc64(struct timeval *dst);
BOOL equ64(struct timeval *tv1, struct timeval *tv2);
void convertunits(struct TimerBase *TimerBase, struct timeval *tr, int unit);

#endif /* _TIMER_INTERN_H */
