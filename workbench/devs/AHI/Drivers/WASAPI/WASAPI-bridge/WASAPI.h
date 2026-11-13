#ifndef AHI_Drivers_WASAPI_bridge_WASAPI_h
#define AHI_Drivers_WASAPI_bridge_WASAPI_h

/*
    Copyright © 2022, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>

#define WASAPI_XRUN   (-32)

BOOL WASAPI_Init();
VOID WASAPI_Cleanup();

VOID WASAPI_MixerInit(APTR *handle, APTR *elem, LONG *min, LONG *max);
VOID WASAPI_MixerCleanup(APTR handle);
LONG WASAPI_MixerGetVolume(APTR elem);
VOID WASAPI_MixerSetVolume(APTR elem, LONG volume);

APTR WASAPI_Open();
VOID WASAPI_DropAndClose(APTR handle);

BOOL WASAPI_SetHWParams(APTR handle, ULONG *rate);

IPTR WASAPI_Start(APTR handle);
IPTR WASAPI_Stop(APTR handle);

LONG WASAPI_Write(APTR handle, APTR buffer, ULONG size);

VOID WASAPI_Prepare(APTR handle);

LONG WASAPI_Avail(APTR handle);

#endif
