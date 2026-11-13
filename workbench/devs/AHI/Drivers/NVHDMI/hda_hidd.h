#ifndef AHI_Drivers_NVHDMI_hda_hidd_h
#define AHI_Drivers_NVHDMI_hda_hidd_h

/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2026 The AROS Dev Team.

All Rights Reserved.
*/

#include <exec/types.h>
#include <exec/interrupts.h>
#include <hidd/hda.h>

#include "DriverData.h"

#ifndef IRQTYPE
#define IRQTYPE NT_INTERRUPT
#endif

struct NVHDMIChip;

BOOL ahi_hda_init(struct DriverBase *AHIsubBase);
void ahi_hda_exit(void);

/* Find, claim and wrap every PCI HDA function matching displayAudio/
   vendor (vendor 0 matches any); fills controllers[] with up to max
   controller objects and returns how many were obtained. */
ULONG ahi_hda_obtain_controllers(BOOL displayAudio, UWORD vendor,
                                 APTR *controllers, ULONG max);
void ahi_hda_release_controller(APTR controller);

IPTR ahi_hda_get_attr(APTR controller, ULONG attr);

ULONG hda_command(APTR controller, ULONG cmd);

APTR hda_stream_alloc(APTR controller, ULONG direction,
                      struct Interrupt *streamInt);
BOOL hda_stream_setup(APTR controller, APTR stream, UWORD format,
                      ULONG bufferSize, ULONG bufferCount,
                      struct HDA_StreamInfo *info);
void hda_stream_start(APTR controller, APTR stream);
void hda_stream_stop(APTR controller, APTR stream);
void hda_stream_free(APTR controller, APTR stream);
void hda_stream_sync(APTR controller, APTR stream, ULONG index);

#endif /* AHI_Drivers_NVHDMI_hda_hidd_h */
