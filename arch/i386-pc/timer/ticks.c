//#include DEBUG 1

//#include <aros/debug.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <asm/io.h>

#include <proto/timer.h>

#include "ticks.h"

const ULONG TIMER_RPROK = 3599597124UL;

ULONG tick2usec(ULONG tick)
{
    ULONG ret, rest;
    asm volatile("mull %3":"=edx"(ret),"=eax"(rest):"eax"(TIMER_RPROK),"m"(tick));
    ret+=rest>>31;
    return ret;
}

ULONG usec2tick(ULONG usec)
{
    ULONG ret;
    asm volatile("movl $0,%%eax; divl %2":"=eax"(ret):"edx"(usec),"m"(TIMER_RPROK));
    return ret;
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    ULONG time, diff;

    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    /* Latch the current time value */
    Disable();
    outb(0x80, 0x43);
    /* Read out current 16-bit time */
    time = inb(0x42);
    time += inb(0x42) << 8;
    Disable();
    
    diff = (TimerBase->tb_prev_tick - time);

    if (time > TimerBase->tb_prev_tick)
        diff += 0x10000;
    
    TimerBase->tb_prev_tick = time;

    TimerBase->tb_ticks_total += diff;
    TimerBase->tb_ticks_sec += diff;
    TimerBase->tb_ticks_elapsed += diff;
    
    if (TimerBase->tb_ticks_sec >= 1193180) {
        TimerBase->tb_ticks_sec -= 1193180;
        TimerBase->tb_CurrentTime.tv_secs++;
    }

    if (TimerBase->tb_ticks_elapsed >= 1193180) {
        TimerBase->tb_ticks_elapsed -= 1193180;
        TimerBase->tb_Elapsed.tv_secs++;
    }
    
    TimerBase->tb_Elapsed.tv_micro = tick2usec(TimerBase->tb_ticks_elapsed);
    TimerBase->tb_CurrentTime.tv_micro = tick2usec(TimerBase->tb_ticks_sec);
}

void EClockSet(struct TimerBase *TimerBase)
{
    ULONG time;
    
    TimerBase->tb_ticks_sec = usec2tick(TimerBase->tb_CurrentTime.tv_micro);
    TimerBase->tb_ticks_total = TimerBase->tb_ticks_sec 
                                + (UQUAD)TimerBase->tb_CurrentTime.tv_secs * 1193180;

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
    struct timerequest *tr;
    
    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_WAITVBL]);

    if (tr)
    {    
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
        SubTime(&time, &TimerBase->tb_CurrentTime);
    
        if (time.tv_secs == 0)
        {
            if (time.tv_micro < 20000)
            {
                delay = usec2tick(time.tv_micro);
            }
        }
    }

    tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_VBLANK]);

    if (tr)
    {    
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
        SubTime(&time, &TimerBase->tb_Elapsed);
    
        if (time.tv_secs == 0)
        {
            if (time.tv_micro < 20000)
            {
                if (delay > usec2tick(time.tv_micro)) 
                    delay = usec2tick(time.tv_micro);
            }
        }
    }
    
    outb((inb(0x61) & 0xfd) | 1, 0x61); /* Enable the timer (set GATE on) */
    outb(0x34, 0x43);   /* Binary, mode 2, LSB&MSB */
    outb(delay & 0xff, 0x40);
    outb(delay >> 8, 0x40);  
}

