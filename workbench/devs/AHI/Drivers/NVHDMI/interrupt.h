#ifndef AHI_Drivers_NVHDMI_interrupt_h
#define AHI_Drivers_NVHDMI_interrupt_h

#include <config.h>

#include "DriverData.h"

#ifdef __AMIGAOS4__
void PlaybackInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct NVHDMIChip *dd);
#else
void PlaybackInterrupt(struct NVHDMIChip *dd);
#endif

#endif /* AHI_Drivers_NVHDMI_interrupt_h */
