#ifndef _TIMER_INTERN_H
#define _TIMER_INTERN_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
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

#define ECLOCK_BASE 0x7000

struct TimerBase
{
    /* Required by the system */
    struct Device	 tb_Device;

    struct timeval	tb_CurrentTime;	/* system time */
    struct timeval	tb_lastsystime;

    struct MinList	 tb_Lists[NUM_LISTS];

    struct Resource *ciaares;
    struct Resource *ciabres;
    struct Interrupt ciainta;
    struct Interrupt ciaintb;
    struct Interrupt vbint;

    struct EClockVal tb_eclock;
    ULONG tb_eclock_rate;
    ULONG tb_eclock_to_usec;
    UWORD tb_eclock_last;

    struct timeval tb_vb_count;
    UWORD tb_vblank_rate;
    ULONG tb_vblank_micros;
    UWORD tb_vblank_on;

    ULONG tb_cia_count_started;
    struct timeval tb_cia_count;
    ULONG tb_cia_micros;
    UWORD tb_cia_on;	

};

#define GetTimerBase(tb)	((struct TimerBase *)(tb))
#define GetDevice(tb)		((struct Device *)(tb))

void GetEClock(struct TimerBase *TimerBase, struct EClockVal *ev);
void CheckTimer(struct TimerBase *TimerBase, ULONG unitnum);
BOOL cmp64(struct timeval *tv1, struct timeval *tv2);
ULONG sub64(struct timeval *larger, struct timeval *smaller);
void add64(struct timeval *dst, struct timeval *src);
void inc64(struct timeval *dst);
BOOL equ64(struct timeval *tv1, struct timeval *tv2);
void convertunits(struct TimerBase *TimerBase, struct timeval *tr, int unit);

#endif /* _TIMER_INTERN_H */
