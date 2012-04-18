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

#include <timer_intern.h>

#define DEBUG 0
#include <aros/debug.h>

// convert timeval pair to 64-bit e-clock unit or 32-bit vblank
void convertunits(struct TimerBase *TimerBase, struct timeval *tr, int unit)
{
	if (unit == UNIT_VBLANK) {
		ULONG v = tr->tv_secs * TimerBase->tb_vblank_rate;
		v += tr->tv_micro / TimerBase->tb_vblank_micros;
		tr->tv_secs = 0;
		tr->tv_micro = v;
	} else if (unit == UNIT_MICROHZ) {
		long long v = (long long)tr->tv_secs * TimerBase->tb_eclock_rate;
		v += ((long long)tr->tv_micro * TimerBase->tb_micro_eclock_mult) >> 15;
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

void addmicro(struct TimerBase *TimerBase, struct timeval *tv)
{
	ULONG old;
	UBYTE lo, hi;
	UWORD val;
	
        add64(tv, &TimerBase->tb_micro_count);

	if (!TimerBase->tb_micro_on)
		return;
	if (IsListEmpty(&TimerBase->tb_Lists[UNIT_MICROHZ])) {
		TimerBase->tb_micro_on = FALSE;
		return;
	}
	/* add (tb_micro_started - current counter value) */
	for (;;) {
		hi = *TimerBase->tb_micro_hi;
		lo = *TimerBase->tb_micro_lo;
		if (hi == *TimerBase->tb_micro_hi)
			break;
	}
	val = (hi << 8) | lo;

	if (val > TimerBase->tb_micro_started)
		val = TimerBase->tb_micro_started;
	else
		val = TimerBase->tb_micro_started - val;

    	old = tv->tv_micro;
	tv->tv_micro += val;
	if (old > tv->tv_micro)
		tv->tv_secs++;
}

// Disabled state assumed
void CheckTimer(struct TimerBase *TimerBase, UWORD unitnum)
{
	if (unitnum == UNIT_VBLANK) {
		TimerBase->tb_vblank_on = TRUE;
	} else if (unitnum == UNIT_MICROHZ) {
		if (!TimerBase->tb_micro_on) {
			// not active, kickstart it
			TimerBase->tb_micro_on = TRUE;
			TimerBase->tb_micro_started = 0;
			D(bug("ciaint_timer kickstarted\n"));
		} else {
			UBYTE lo, hi;
			UWORD val;
			// already active but new item was added to head
			*TimerBase->tb_micro_cr &= 0x40;
			*TimerBase->tb_micro_cr |= 0x08;
			hi = *TimerBase->tb_micro_hi;
			lo = *TimerBase->tb_micro_lo;
			val = (hi << 8) | lo;
			// how long have we already waited?
			TimerBase->tb_micro_started -= val;
			// force interrupt now
			D(bug("ciaint_timer restarted\n"));
		}
		SetICR(TimerBase->tb_micro_res, 0x80 | (1 << TimerBase->tb_micro_intbit));
	}
}

ULONG GetEClock(struct TimerBase *TimerBase)
{
	UBYTE lo, hi;
	ULONG diff, val;
	
	/* Disable() assumed */
	for (;;) {
		hi = *TimerBase->tb_eclock_hi;
		lo = *TimerBase->tb_eclock_lo;
		if (hi == *TimerBase->tb_eclock_hi)
			break;
		// lo wraparound, try again
	}
	diff = 0;
	// pending interrupt? Re-read counter */
	if (SetICR(TimerBase->tb_eclock_res, 0) & (1 << TimerBase->tb_eclock_intbit)) {
		diff = ECLOCK_BASE;
		for (;;) {
			hi = *TimerBase->tb_eclock_hi;
			lo = *TimerBase->tb_eclock_lo;
			if (hi == *TimerBase->tb_eclock_hi)
				break;
		}
	}
	val = (hi << 8) | lo;
	diff += ECLOCK_BASE - val;
	return diff;
}

AROS_UFH4(APTR, ciab_eclock,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	D(bug("eclock int\n"));

	// e-clock counter, counts full ECLOCK_BASE cycles
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

AROS_UFH4(APTR, ciaint_timer,
    AROS_UFHA(ULONG, dummy, A0),
    AROS_UFHA(struct TimerBase *, TimerBase, A1),
    AROS_UFHA(ULONG, dummy2, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct timerequest *tr, *next;
	ULONG old;

	D(bug("ciaint_timer\n"));

	if (TimerBase->tb_micro_on == FALSE)
		return 0;

	// we have counted tb_micro_started since last interrupt
	old = TimerBase->tb_micro_count.tv_micro;
	TimerBase->tb_micro_count.tv_micro += TimerBase->tb_micro_started;
	if (old > TimerBase->tb_micro_count.tv_micro)
		TimerBase->tb_micro_count.tv_secs++;

	Disable();

    	ForeachNodeSafe(&TimerBase->tb_Lists[UNIT_MICROHZ], tr, next) {
    		D(bug("%d/%d %d/%d\n", TimerBase->tb_micro_count.tv_secs, TimerBase->tb_micro_count.tv_micro, tr->tr_time.tv_secs, tr->tr_time.tv_micro));
    		if (cmp64(&TimerBase->tb_micro_count, &tr->tr_time)) {
	           	Remove((struct Node *)tr);
	           	tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
	           	tr->tr_node.io_Error = 0;
	           	ReplyMsg((struct Message *)tr);
	           	D(bug("ciaint_timer %x done\n", tr));
		} else {
			break; // first not finished, can stop searching
		}	
			
	}
	tr = (struct timerequest*)(((struct List *)(&TimerBase->tb_Lists[UNIT_MICROHZ]))->lh_Head);
	if (tr->tr_node.io_Message.mn_Node.ln_Succ) {
		ULONG newcount = sub64(&tr->tr_time, &TimerBase->tb_micro_count);
		D(bug("ciaint_timer newcount=%d\n", newcount));
		// longer than max CIA timer capacity?
		if (newcount > 0xffff)
			newcount = 0xffff;
		TimerBase->tb_micro_started = newcount;
		// reset control register, some badly behaving programs may have changed it
		// do not touch bit 6 because it is used by keyboard handshake and reset warning
		*TimerBase->tb_micro_cr &= 0x40;
		*TimerBase->tb_micro_cr |= 0x08;
		// reload new timer value (timer autostarts)
		*TimerBase->tb_micro_lo = (UBYTE)(newcount >> 0);
		*TimerBase->tb_micro_hi = (UBYTE)(newcount >> 8);
	} else {
		D(bug("ciaint_timer off\n"));
		// list is empty
		TimerBase->tb_micro_on = FALSE;
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

	if (TimerBase->tb_vblank_on == FALSE)
		return 0;
	inc64(&TimerBase->tb_vb_count);

	Disable();
    	ForeachNodeSafe(&TimerBase->tb_Lists[UNIT_VBLANK], tr, next) {
	    	if (cmp64(&TimerBase->tb_vb_count, &tr->tr_time)) {
	           	Remove((struct Node *)tr);
	           	tr->tr_time.tv_secs = tr->tr_time.tv_micro = 0;
	           	tr->tr_node.io_Error = 0;
           		ReplyMsg((struct Message *)tr);
	           	D(bug("vblank %x done\n", tr));
		} else {
			break; // first not finished, can stop searching
		}	
			
	}
	if (IsListEmpty(&TimerBase->tb_Lists[UNIT_VBLANK])) {
		TimerBase->tb_vblank_on = FALSE;
	}
	Enable();
	
	return 0;	

	AROS_USERFUNC_EXIT

}
