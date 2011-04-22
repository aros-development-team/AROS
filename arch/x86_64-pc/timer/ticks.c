/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware management routines for IBM PC-AT timer
    Lang: english
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <asm/io.h>

#include "ticks.h"
#include "timer_macros.h"

const ULONG TIMER_RPROK = 3599597124UL;

static inline ULONG tick2usec(ULONG tick)
{
    ULONG ret, rest;

    asm volatile("mull %3":"=d"(ret),"=a"(rest):"a"(TIMER_RPROK),"m"(tick));
    ret+=rest>>31;
    return ret;
}

static ULONG usec2tick(ULONG usec)
{
    ULONG ret;

//  gcc 4.3.1 with -O2: Following doesn't work properly, probably because it
//                      doesn't tell about edx trashing.
//
//    asm volatile("movl $0,%%eax; divl %2":"=a"(ret):"d"(usec),"m"(TIMER_RPROK));
//
    asm volatile("movl $0,%%eax; divl %2":"=a"(ret),"+d"(usec):"m"(TIMER_RPROK));
    return ret;
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    ULONG time, diff;

    Disable();
    
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    /* Latch the current time value */

    outb(0x80, 0x43);
    /* Read out current 16-bit time */
    time = inb(0x42);
    time += inb(0x42) << 8;

    diff = (TimerBase->tb_prev_tick - time);

    if (time > TimerBase->tb_prev_tick)
        diff += 0x10000;

    TimerBase->tb_prev_tick = time;

    /* Increment our time counters */
    TimerBase->tb_ticks_total += diff;
    INCTIME(TimerBase->tb_CurrentTime, TimerBase->tb_ticks_sec, diff);
    INCTIME(TimerBase->tb_Elapsed, TimerBase->tb_ticks_elapsed, diff);

    Enable();
}

void EClockSet(struct TimerBase *TimerBase)
{
    ULONG time;
    
    TimerBase->tb_ticks_sec   = usec2tick(TimerBase->tb_CurrentTime.tv_micro);
    TimerBase->tb_ticks_total = TimerBase->tb_ticks_sec + (UQUAD)TimerBase->tb_CurrentTime.tv_secs * TimerBase->tb_eclock_rate;

    /* Latch the current time value */
    outb(0x80, 0x43);
    /* Read out current 16-bit time */
    time = inb(0x42);
    time += inb(0x42) << 8;
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */

    TimerBase->tb_prev_tick = time;
}

void Timer0Setup(struct TimerBase *TimerBase)
{
    struct timeval time;
    ULONG delay = 23864;
    struct timerequest *tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_MICROHZ]);

    if (tr)
    {
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
        SUBTIME(&time, &TimerBase->tb_CurrentTime);
    
    	if ((LONG)time.tv_secs < 0)
	{
	    delay = 0;
	}
        else if (time.tv_secs == 0)
        {
            if (time.tv_micro < 20000)
            {
                delay = usec2tick(time.tv_micro);
            }
        }
    }

    if (delay < 2) delay = 2;
    
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    outb(0x38, 0x43);   /* Binary, mode 4, LSB&MSB */
    outb(delay & 0xff, 0x40);
    outb(delay >> 8, 0x40);  
}
