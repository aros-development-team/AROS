/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_Envy24HT_interrupt_h
#define AHI_Drivers_Envy24HT_interrupt_h

#include <config.h>

#include "DriverData.h"

#ifdef __AMIGAOS4__
ULONG CardInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* dd);

void PlaybackInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* dd);

void RecordInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* dd);
#else

ULONG CardInterrupt(struct CardData* dd);

void PlaybackInterrupt(struct CardData* dd);

void RecordInterrupt(struct CardData* dd);

#endif

#endif /* AHI_Drivers_Envy24HT_interrupt_h */
