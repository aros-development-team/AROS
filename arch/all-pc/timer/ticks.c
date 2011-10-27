/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hardware management routines for IBM PC-AT timer
    Lang: english
*/

#include <asm/io.h>
#include <proto/exec.h>

#include "ticks.h"
#include "timer_macros.h"

/*
 * This code uses two channels of the PIT for simplicity:
 * Channel 0 - sends IRQ 0 on terminal count. We use it as alarm clock.
 * Channel 2 is used as EClock counter. It counts all the time and is never reloaded.
 *	     We initialize it in timer_init.c.
 */

/*
 * The math magic behing this is:
 *
 * 1. Theoretical value of microseconds is: tick * 1000000 / tb_eclock_rate.
 * 2. tb_eclock_rate is constant and equal to 1193180Hz (frequency of PIT's master quartz).
 * 3. tick2usec() is called much more frequently than usec2tick(). And multiplication is faster than division.
 *
 * So let's get rid of division:
 *
 * a) Multiply both divident and divisor of the initial equation by 0x100000000 (4294967296 in decimal). In asm this
 *    can be accomplished simply by using 64-bit math ops and using upper half instead of lower:
 *    usec = (tick * 1000000 * 0x100000000) / (1193180 * 0x100000000) = tick * (1000000 * 0x100000000 / 1193180) / 0x100000000.
 * b) Calculate the constant part in brackets: 1000000 * 4294967296 / 1193180 = 3599597124.
 * c) So: usec = tick * 3599597124 / 0x100000000 = (tick * 3599597124) >> 32.
 *
 *	(c) Michal Schulz.
 */
const ULONG TIMER_RPROK = 3599597124UL;

static inline ULONG tick2usec(ULONG tick)
{
    ULONG ret, rest;

    asm volatile("mull %3":"=d"(ret),"=a"(rest):"a"(TIMER_RPROK),"m"(tick));
    ret+=rest>>31;
    return ret;
}

static inline ULONG usec2tick(ULONG usec)
{
    /*
     * This is important!!! EAX must be set to zero at input.
     * Previously asm construction here was written as:
     *		asm volatile("movl $0,%%eax; divl %2":"=a"(ret),"+d"(usec):"m"(TIMER_RPROK));
     * On x86-64 this caused dereferencing a NULL pointer. gcc v4.5.2 generated the
     * following code:
     * 		mov    $0x0,%rax	; This is a symbol reference, not actual zero.
     *		mov    $0x0,%eax	; Here our asm begins. This is real zero.
     *		divl   (%rax)
     * I. e. is used RAX to store address of TIMER_RPROK without knowing about it
     * being clobbered by the asm sequence itself.
     * Adding :"eax" to the clobberlist caused "impossible constraint" error,
     * so i made EAX to be both input and output operand and initialize it to zero in C.
     */
    ULONG ret = 0;

    /* Actually this seems to be usec * 4294967296 / 3599597124 */
    asm volatile("divl %2":"+a"(ret),"+d"(usec):"m"(TIMER_RPROK));
    return ret;
}

/*
 * Calculate difference and add it to our current EClock value.
 * PIT counters actually count backwards. This is why everything here looks reversed.
 */
static void EClockAdd(ULONG time, struct TimerBase *TimerBase)
{
    ULONG diff = (TimerBase->tb_prev_tick - time);

    if (time > TimerBase->tb_prev_tick)
    {
    	/* Handle PIT rollover through 0xFFFF */
        diff += 0x10000;
    }

    /* Increment our time counters */
    TimerBase->tb_ticks_total += diff;
    INCTIME(TimerBase->tb_CurrentTime, TimerBase->tb_ticks_sec, diff);
    INCTIME(TimerBase->tb_Elapsed, TimerBase->tb_ticks_elapsed, diff);
}

void EClockUpdate(struct TimerBase *TimerBase)
{
    ULONG time;

    outb(CH0|ACCESS_LATCH, PIT_CONTROL);	/* Latch the current time value */
    time = ch_read(PIT_CH0);	    		/* Read out current 16-bit time */

    EClockAdd(time, TimerBase);			/* Increment our time counters	*/
    TimerBase->tb_prev_tick = time;		/* Remember last counter value as start of new interval */
}

void EClockSet(struct TimerBase *TimerBase)
{
    TimerBase->tb_ticks_sec   = usec2tick(TimerBase->tb_CurrentTime.tv_micro);
    TimerBase->tb_ticks_total = TimerBase->tb_ticks_sec + (UQUAD)TimerBase->tb_CurrentTime.tv_secs * TimerBase->tb_eclock_rate;

    outb(CH0|ACCESS_LATCH, PIT_CONTROL);	/* Latch the current time value */
    TimerBase->tb_prev_tick = ch_read(PIT_CH0);	/* Read out current 16-bit time */
}

void Timer0Setup(struct TimerBase *TimerBase)
{
    struct timeval time;
    ULONG delay = 23864;
    ULONG old_tick;
    struct timerequest *tr = (struct timerequest *)GetHead(&TimerBase->tb_Lists[TL_MICROHZ]);

    if (tr)
    {
        time.tv_micro = tr->tr_time.tv_micro;
        time.tv_secs  = tr->tr_time.tv_secs;

        EClockUpdate(TimerBase);
        SUBTIME(&time, &TimerBase->tb_Elapsed);

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

    /*
     * We are going to reload the counter. By this moment, some time has passed after the last EClockUpdate()pdate.
     * In order to keep up with the precision, we pick up this time here.
     */
    outb(CH0|ACCESS_LATCH, PIT_CONTROL);	/* Latch the current time value */
    old_tick = ch_read(PIT_CH0);	    	/* Read out current 16-bit time */

    outb(CH0|ACCESS_FULL|MODE_SW_STROBE, PIT_CONTROL);  /* Software strobe mode, 16-bit access	*/
    ch_write(delay, PIT_CH0);				/* Activate the new delay		*/

    /*
     * Now, when our new delay is already in progress, we can spend some time
     * on adding previous value to our time counters.
     */
    EClockAdd(old_tick, TimerBase);
    /* tb_prev_tick is used by EClockAdd(), so update it only now */
    TimerBase->tb_prev_tick = delay;
}
