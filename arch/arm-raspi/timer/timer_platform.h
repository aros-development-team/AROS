/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/bcm2835.h>

//#define USE_VBLANK_INT
#define TICK_TIMER            1

struct PlatformTimer
{
    struct timeval tb_TickRate;	/* Our periodic timer interval   */
    unsigned int tb_cs;
    unsigned int tb_CLO;
    unsigned int tb_CHI;
};
