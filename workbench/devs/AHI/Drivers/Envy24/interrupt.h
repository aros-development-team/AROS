/*
    Copyright © 2004-2014, Davy Wentzler. All rights reserved.
    Copyright © 2010-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AHI_Drivers_interrupt_h
#define AHI_Drivers_interrupt_h

#include <config.h>

#include "DriverData.h"

#ifdef __AMIGAOS4__
ULONG CardInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* dd);

void PlaybackInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* dd);

void RecordInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct CardData* dd);

#elif defined(__AROS__)
AROS_INTP(CardInterrupt);
AROS_INTP(PlaybackInterrupt);
AROS_INTP(RecordInterrupt);

#else

LONG CardInterrupt(struct CardData* card);

void PlaybackInterrupt(struct CardData* card);

void RecordInterrupt(struct CardData* card);

#endif

#endif /* AHI_Drivers_interrupt_h */
