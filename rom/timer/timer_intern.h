/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
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
#ifndef HIDD_TIMER_H
#include <hidd/timer.h>
#endif
#ifndef DOS_BPTR_H
#include <dos/bptr.h>
#endif

#include <aros/system.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#define TL_VBLANK	0
#define TL_WAITVBL	1
#define TL_MICROHZ	2
#define TL_ECLOCK	3
#define TL_WAITECLOCK	4
#define NUM_LISTS	5

struct TimerBase
{
    /* Required by the system */
    struct Device	 tb_Device;

    struct IClass	*tb_TimerHIDD;
    ULONG		 tb_MiscFlags;		/* miscellaneous flags */
    struct timeval	 tb_CurrentTime;	/* system time */
    struct timeval	 tb_Elapsed;		/* Elapsed Time for VBlank */

    /* This is required for the vertical blanking stuff */
    struct Interrupt	 tb_VBlankInt;
    struct IClass	*tb_VBlankHIDD;		/* vblank hidd class */
    struct timeval	 tb_VBlankTime;		/* vblank interval */
    
    /* Lists for waiting vblank, waituntil, microhz, eclock, waiteclock */
    struct MinList	 tb_Lists[NUM_LISTS];
    
    UQUAD                tb_ticks_total;
    ULONG                tb_ticks_sec;
    ULONG                tb_ticks_elapsed;
    ULONG                tb_prev_tick;
    
    struct timerequest   tb_vblank_timerequest; /* For vblank emulation */
};

#define GetTimerBase(tb)	((struct TimerBase *)(tb))
#define GetDevice(tb)		((struct Device *)(tb))

#define TF_GO		(1L<<0)
#define TF_GotVBlank	(1L<<1)	/* HIDD used for VBlank units */
#define TF_GotEClock	(1L<<2)	/* HIDD used for other units */
