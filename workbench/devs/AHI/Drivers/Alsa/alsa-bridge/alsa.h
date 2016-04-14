#ifndef AHI_Drivers_Alsa_bridge_alsa_h
#define AHI_Drivers_Alsa_bridge_alsa_h

/*
    Copyright © 2015-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#define ALSA_XRUN   (-32)

BOOL ALSA_Init();
VOID ALSA_Cleanup();

VOID ALSA_MixerInit(APTR * handle, APTR * elem, LONG * min, LONG * max);
VOID ALSA_MixerCleanup(APTR handle);
LONG ALSA_MixerGetVolume(APTR elem);
VOID ALSA_MixerSetVolume(APTR elem, LONG volume);

APTR ALSA_Open();
VOID ALSA_DropAndClose(APTR handle);

BOOL ALSA_SetHWParams(APTR handle, ULONG * rate);

LONG ALSA_Write(APTR handle, APTR buffer, ULONG size);

VOID ALSA_Prepare(APTR handle);

LONG ALSA_Avail(APTR handle);

#endif
