/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * This code probes Amiga hardware timings, and
 * configures SysBase with those settings.
 */

#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <hardware/cia.h>

#include <kernel_base.h>

static int Timer_Init(struct KernelBase *KernelBase)
{
    volatile struct CIA *ciaa = (struct CIA*)0xbfe001;
    UWORD todlo, todcnt;

    /*
     * Check powersupply tick rate.
     * No Disable() here because we are running without interrupts during early init.
     */
    ciaa->ciacra = 0x00;
    ciaa->ciatodhi = 0;
    ciaa->ciatodmid = 0;
    ciaa->ciatodlow = 0;
    ciaa->ciatalo = 0xff;
    ciaa->ciatahi = 0xff;
    todlo = ciaa->ciatodlow;
    while (todlo == ciaa->ciatodlow);
    ciaa->ciacra = 0x01;
    todlo = ciaa->ciatodlow;
    while (todlo == ciaa->ciatodlow);
    ciaa->ciacra = 0x00;
    todcnt = ~(((ciaa->ciatahi << 8) | ciaa->ciatalo) + 1);

    /*
     * 50Hz/60Hz ticks:
     * 50Hz PAL  14188
     * 60Hz NTSC 11932
     * 50Hz NTSC 14318
     * 60Hz PAL  11823
     */
    if (todcnt > 14188 + (14318 - 14188) / 2)
    	SysBase->PowerSupplyFrequency = 50;
    else if (todcnt <= 11823 + (11932 - 11823) / 2)
    	SysBase->PowerSupplyFrequency = 60;
    else if (todcnt > 14188 - (14188 - 11932) / 2)
    	SysBase->PowerSupplyFrequency = 50;
    else
    	SysBase->PowerSupplyFrequency = 60;

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)
