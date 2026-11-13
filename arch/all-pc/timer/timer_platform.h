#ifndef _PC_TIMER_PLATFORM_H
#define _PC_TIMER_PLATFORM_H
/*
    Copyright (C) 2020-2026, The AROS Development Team. All rights reserved.

    Desc: PC timer device private structures and constants
*/

#include <aros/config.h>

struct PlatformTimer
{
    ULONG               tb_Flags;       /* See below..                                  */
    LONG	            tb_TimerIRQNum;	/* Timer IRQ number			                    */
    struct timeval      tb_VBlankTime;	/* Software-emulated periodic timer interval    */
    UWORD               tb_InitValue;
    UWORD               tb_ReloadValue;
    UWORD               tb_ReloadMin;
#if defined(__AROSEXEC_SMP__)
    volatile ULONG      tb_PITLock;
#endif
};

/* tb_Flags definitions */
#define PCTIMER_FLAGB_ENABLED   1
#define PCTIMER_FLAGF_ENABLED   (1 << PCTIMER_FLAGB_ENABLED)

#endif /* _PC_TIMER_PLATFORM_H */
