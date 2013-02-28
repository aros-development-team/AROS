#ifndef AHI_Drivers_HDAudio_interrupt_h
#define AHI_Drivers_HDAudio_interrupt_h

#include <config.h>

#include "DriverData.h"

#ifdef __AMIGAOS4__
ULONG CardInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct HDAudioChip* dd);

void PlaybackInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct HDAudioChip* dd);

void RecordInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct HDAudioChip* dd);
#else

ULONG CardInterrupt(struct HDAudioChip* dd);

void PlaybackInterrupt(struct HDAudioChip* dd);

void RecordInterrupt(struct HDAudioChip* dd);

#endif

#endif /* AHI_Drivers_HDAudio_interrupt_h */
