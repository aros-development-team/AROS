#ifndef HIDD_TIMER_H
#define HIDD_TIMER_H

/*
    Copyright (C) 1997-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Definitions for the Timer HIDD system.
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DEVICES_TIMER_H
#   include <devices/timer.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif

#define TIMERHIDD "timer.hidd"

/* This union describes a certain time, either in seconds or nanoseconds
   (cv_Normalised), or as an absolute value (cv_Absolute). */
typedef struct HIDDT_ClockVal
{
    ULONG	cv_Format;		/* HIDDT_TimeFormat enum */
    union {
	struct EClockVal eclock;	/* 64 bits, hi and low */
	struct timeval   tval;		/* 32b seconds, nanosecs */
	UQUAD		 abs;		/* 64 bit, absolute */
    } cv_Value;

} HIDDT_ClockVal;

#define cv_Secs		cv_Value.tval.tv_secs
#define cv_Nanos	cv_Value.tval.tv_micro
#define cv_EClock	cv_Value.eclock
#define cv_Absolute	cv_Value.abs

/* Attributes for the Timer HIDD */
enum {
    HIDDA_TimerBase = HIDDA_Base + 0x02000,
    HIDDA_Timer_MinPeriod,	/* [..G] (HIDDT_ClockVal *) Min period */
    HIDDA_Timer_MaxPeriod,	/* [..G] (HIDDT_ClockVal *) Max period */
    HIDDA_Timer_IsFixed,	/* [..G] (BOOL) Is this clock fixed */
    HIDDA_Timer_IsClock,	/* [..G] (BOOL) Is this a clock-like timer */
    HIDDA_Timer_IsAlarm,	/* [..G] (BOOL) Is this an alarm-like timer */
    HIDDA_Timer_IsExternal,	/* [..G] (BOOL) This timer is external */
    HIDDA_Timer_Mode,		/* [ISG] (ULONG) Current timer mode */

    /* Attributes for Clock like devices */
    HIDDA_Timer_ClockBase = HIDDA_TimerBase + 0x100,
    HIDDA_Timer_IsNonVolatile,	/* [..G] (BOOL) This clock is non-volatile */

    /* Attributes for Alarm like devices */
    HIDDA_Timer_AlarmBase = HIDDA_TimerBase + 0x200,
    HIDDA_Timer_CountMode,	/* [.SG] (ULONG) Current counting mode */
    HIDDA_Timer_Hook,		/* [ISG] (struct Hook *) Callback hook */
    HIDDA_Timer_SigTask,	/* [ISG] (struct Task *) Task to signal */
    HIDDA_Timer_SigBit,		/* [ISG] (UBYTE) Signal bit to use */
    HIDDA_Timer_SoftInt,	/* [ISG] (struct Interrupt *) Software Int */
};

/* Values for HIDDA_Timer_Mode */
enum HIDDV_Timer_Mode {
    HIDDV_Timer_ClockMode,	/* Timer in clock mode */
    HIDDV_Timer_AlarmMode	/* Timer in alarm mode */
};

/* Values for HIDDA_Timer_CountMode */
enum {
    HIDDV_Timer_OneShot,	/* Timer in one-shot mode */
    HIDDV_Timer_Continuous	/* Timer in continuous mode */
};

/* Values describing a HIDDT_ClockVal structure */
enum HIDDT_TimeFormat {
    HIDDV_Timer_Normalised,	/* Value is normalised (cv_Secs/cv_Nano) */
    HIDDV_Timer_Absolute,	/* Value is absolute format (cv_Absolute) */
    HIDDV_Timer_EClock,		/* Value is like EClock (cv_EClock) */
    HIDDV_Timer_Internal,	/* Value is timer specific format (cv_Abs) */
};

/* Methods implemented by timers */
enum
{
    HIDDM_TimerBase = 0x80200,
    HIDDM_Timer_Reset,			/* Reset a timer */
    HIDDM_Timer_Freeze,			/* Freeze a timer */
    HIDDM_Timer_UnFreeze,		/* Unfreeze a timer */
    HIDDM_Timer_ConvertTime,		/* Convert between time formats */
    
    HIDDM_Timer_ClockBase = 0x80240,
    HIDDM_Timer_SetPeriod,		/* Set the period of a clock */
    HIDDM_Timer_GetPeriod,		/* Get the period of a clock */
    HIDDM_Timer_Set,			/* Set the value of a clock */
    HIDDM_Timer_Get,			/* Get the value of a clock */

    HIDDM_Timer_AlarmBase = 0x80280,
    HIDDM_Timer_Start,			/* Start a timer running */
    HIDDM_Timer_Stop,			/* Stop a timer running */
    HIDDM_Timer_SetInterval,		/* Set the interval of the alarm */
    HIDDM_Timer_GetInterval		/* Get the interval of the timer */
};

/* Message for HIDDM_Timer_Convert */
struct hTm_Convert {
    STACKULONG		MethodID;
    HIDDT_ClockVal	*htm_From;
    HIDDT_ClockVal	*htm_To;
};

#endif /* HIDD_TIMER_H */
