/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

BOOL ALSA_Init();
VOID ALSA_Cleanup();

APTR ALSA_Open();
VOID ALSA_Close(APTR handle);

BOOL ALSA_SetHWParams(APTR handle, ULONG * rate);

LONG ALSA_Write(APTR handle, APTR buffer, ULONG size);

VOID ALSA_Prepare(APTR handle);

LONG ALSA_Avail(APTR handle);
