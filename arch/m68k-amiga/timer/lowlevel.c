/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CIA timer.device
    Lang: english
*/
#include <exec/types.h>
#include <exec/execbase.h>
#include <hardware/cia.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/cia.h>

#include "timer_intern.h"

#define DEBUG 0
#include <aros/debug.h>

// convert timeval pair to 64-bit vblank or e-clock unit
// (hopefully 64-bit multiplication and division isn't too heavy..)
void convertunits(struct TimerBase *TimerBase, struct timeval *tr, int unit)
{
	if (unit == UNIT_VBLANK) {
		long long v = ((long long)tr->tv_secs) * 1000000 + tr->tv_micro;
		v /= TimerBase->tb_vblank_micros;
		tr->tv_secs = v >> 32;
		tr->tv_micro = v;
	} else if (unit == UNIT_MICROHZ) {
		long long v = ((long long)tr->tv_secs) * 1000000 + tr->tv_micro;
		v *= TimerBase->tb_cia_micros;
		v /= 1000000;
		tr->tv_secs = v >> 32;
		tr->tv_micro = v;		
	}
}

// dst++
void inc64(struct timeval *dst)
{
	dst->tv_micro++;
	if (dst->tv_micro == 0)
		dst->tv_secs++;
}
// dst += src
void add64(struct timeval *dst, struct timeval *src)
{
	ULONG old = dst->tv_micro;
	dst->tv_micro += src->tv_micro;
	if (old > dst->tv_micro)
		dst->tv_secs++;
	dst->tv_secs += src->tv_secs;
}
// true if tv1 == tv2
BOOL equ64(struct timeval *tv1, struct timeval *tv2)
{
	return tv1->tv_secs == tv2->tv_secs && tv1->tv_micro == tv2->tv_micro;
}
// return true if tv1 >= tv2
BOOL cmp64(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_secs > tv2->tv_secs)
		return TRUE;
	if (tv1->tv_secs == tv2->tv_secs && tv1->tv_micro >= tv2->tv_micro)
		return TRUE;
	return FALSE;
}
// return (larger - smaller) or 0xffffffff if result does not fit in ULONG
// larger >= smaller
ULONG sub64(struct timeval *larger, struct timeval *smaller)
{
	if (larger->tv_secs == smaller->tv_secs)
		return larger->tv_micro - smaller->tv_micro;
	if (larger->tv_secs == smaller->tv_secs + 1) {
		if (larger->tv_micro > smaller->tv_micro)
			return 0xffffffff;
		return (~smaller->tv_micro) + larger->tv_micro + 1;
	}
	return 0xffffffff;
}

// Disabled state assumed
void CheckTimer(struct TimerBase *TimerBase, ULONG unitnum)
{
	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	if (unitnum == UNIT_VBLANK) {
		TimerBase->tb_vblank_on = 1;
	} else if (unitnum == UNIT_MICROHZ) {
		if (!TimerBase->tb_cia_on) {
			// not active, kickstart it
			TimerBase->tb_cia_on = 1;
			TimerBase->tb_cia_count_started = 0;
			D(bug("UNIT_MICROHZ kickstarted\n"));
			SetICR(TimerBase->ciares, 0x82);
		} else {
			UBYTE lo, hi;
			// already active but new item was added to head
			for (;;) {
				hi = ciaa->ciatbhi;
				lo = ciaa->ciatblo;
				if (hi == ciaa->ciatbhi)
					break;
			}
			// how long have we already waited?
			TimerBase->tb_cia_count_started -= (hi << 8) | lo;
			// force interrupt now
			SetICR(TimerBase->ciares, 0x82);
			D(bug("UNIT_MICROHZ restarted\n"));
		}
	}
}
	
void GetEClock(struct TimerBase *TimerBase, struct EClockVal *ev)
{
	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	UBYTE lo, hi;
	ULONG evlo, evhi, old;
	UWORD diff, val;
	
	Disable();
	for (;;) {
		hi = ciaa->ciatahi;
		lo = ciaa->ciatalo;
		if (hi != ciaa->ciatahi)
			break;
		// lo wraparound, try again
	}
	val = (hi << 8) | lo;
	old = evlo = TimerBase->tb_eclock.ev_lo;
	evhi = TimerBase->tb_eclock.ev_hi;
	// pending interrupt?
	if (SetICR(TimerBase->ciares, 0) & 0x01) {
		TimerBase->tb_eclock_last = ECLOCK_BASE;
		diff = ECLOCK_BASE;
	} else {
		diff = 0;
	}
	diff += TimerBase->tb_eclock_last - val;
	evlo += diff;
	if (old > evlo)
		evhi++;
	ev->ev_lo = evlo;
	ev->ev_hi = evhi;
	TimerBase->tb_eclock_last = val;

	TimerBase->tb_eclock_to_usec += diff;
	if (TimerBase->tb_eclock_to_usec >= TimerBase->tb_eclock_rate) {
		TimerBase->tb_eclock_to_usec -= TimerBase->tb_eclock_rate;
		TimerBase->tb_CurrentTime.tv_secs++;
	}
	Enable();
}

AROS_UFH4(APTR, cia_ciainta,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

	// e-clock counter, counts full ECLOCK_BASE cycles
	TimerBase->tb_eclock_last = ECLOCK_BASE;
	ULONG old = TimerBase->tb_eclock.ev_lo;
	TimerBase->tb_eclock.ev_lo += ECLOCK_BASE;
	if (old > TimerBase->tb_eclock.ev_lo)
		TimerBase->tb_eclock.ev_hi++;

	TimerBase->tb_eclock_to_usec += ECLOCK_BASE;
	if (TimerBase->tb_eclock_to_usec >= TimerBase->tb_eclock_rate) {
		TimerBase->tb_eclock_to_usec -= TimerBase->tb_eclock_rate;
		TimerBase->tb_CurrentTime.tv_secs++;
	}

	return 0;	

	AROS_USERFUNC_EXIT
}

AROS_UFH4(APTR, cia_ciaintb,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

	volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
	struct timerequest *tr, *next;
	ULONG old;

	D(bug("ciabint\n"));

	if (TimerBase->tb_cia_on == 0)
		return 0;

	// we have counted tb_cia_count_started since last interrupt
	old = TimerBase->tb_cia_count.tv_micro;
	TimerBase->tb_cia_count.tv_micro += TimerBase->tb_cia_count_started;
	if (old > TimerBase->tb_cia_count.tv_micro)
		TimerBase->tb_cia_count.tv_secs++;

	Disable();

    ForeachNodeSafe(&TimerBase->tb_Lists[UNIT_MICROHZ], tr, next) {
    	D(bug("%d/%d %d/%d\n", TimerBase->tb_cia_count.tv_secs, TimerBase->tb_cia_count.tv_micro, tr->tr_time.tv_secs, tr->tr_time.tv_micro));
    	if (cmp64(&TimerBase->tb_cia_count, &tr->tr_time)) {
           	Remove((struct Node *)tr);
           	tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
           	tr->tr_node.io_Error = 0;
           	ReplyMsg((struct Message *)tr);
           	D(bug("ciab done\n"));
		} else {
			break; // first not finished, can stop searching
		}	
			
	}
	tr = (struct timerequest*)(((struct List *)(&TimerBase->tb_Lists[UNIT_MICROHZ]))->lh_Head);
	if (tr->tr_node.io_Message.mn_Node.ln_Succ) {
		ULONG newcount = sub64(&tr->tr_time, &TimerBase->tb_cia_count);
		D(bug("newcount=%d\n", newcount));
		// longer than max CIA timer capacity?
		if (newcount > 0xffff)
			newcount = 0xffff;
		TimerBase->tb_cia_count_started = newcount;
		// reload new timer value (timer autostarts)
		ciaa->ciatblo = (UBYTE)(newcount >> 0);
		ciaa->ciatbhi = (UBYTE)(newcount >> 8);
	} else {
		D(bug("ciab off\n"));
		// list is empty
		TimerBase->tb_cia_on = 0;
	}

	Enable();

	return 0;	

	AROS_USERFUNC_EXIT
}


AROS_UFH4(APTR, cia_vbint,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
	
    struct timerequest *tr, *next;

	if (TimerBase->tb_vblank_on == 0)
		return 0;
	inc64(&TimerBase->tb_vb_count);

	Disable();
    ForeachNodeSafe(&TimerBase->tb_Lists[UNIT_VBLANK], tr, next) {
    	if (cmp64(&TimerBase->tb_vb_count, &tr->tr_time)) {
           	Remove((struct Node *)tr);
           	tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
           	tr->tr_node.io_Error = 0;
           	ReplyMsg((struct Message *)tr);
		} else {
			break; // first not finished, can stop searching
		}	
			
	}
	if (IsListEmpty(&TimerBase->tb_Lists[UNIT_VBLANK])) {
		TimerBase->tb_vblank_on = 0;
	}
	Enable();
	
	return 0;	

	AROS_USERFUNC_EXIT

}
