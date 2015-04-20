/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#define USE_VBLANK_INT
#define TICK_TIMER            1

struct PlatformTimer
{
    APTR tbp_BootLoaderBase;
    IPTR tbp_periiobase;
    struct timeval tbp_TickRate;	/* Our periodic timer interval   */
    unsigned int tbp_CLO;
    unsigned int tbp_CHI;
};

#undef KernelBase
#define KernelBase TimerBase->tb_KernelBase

#undef BootLoaderBase
#define BootLoaderBase TimerBase->tb_Platform.tbp_BootLoaderBase

#define ARM_PERIIOBASE TimerBase->tb_Platform.tbp_periiobase
#include <hardware/bcm2708.h>
