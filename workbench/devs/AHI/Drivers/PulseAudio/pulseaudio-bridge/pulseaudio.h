#ifndef AHI_Drivers_PulseAudio_bridge_alsa_h
#define AHI_Drivers_PulseAudio_bridge_alsa_h

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

BOOL PULSEA_Init();
VOID PULSEA_Cleanup();

VOID PULSEA_MixerInit(APTR *handle, APTR *elem, LONG *min, LONG *max);
VOID PULSEA_MixerCleanup(APTR handle);
LONG PULSEA_MixerGetVolume(APTR elem);
VOID PULSEA_MixerSetVolume(APTR elem, LONG volume);

APTR PULSEA_Open();
VOID PULSEA_DropAndClose(APTR handle);

BOOL PULSEA_SetHWParams(APTR handle, ULONG *rate);

LONG PULSEA_Write(APTR handle, APTR buffer, ULONG size);


#endif
