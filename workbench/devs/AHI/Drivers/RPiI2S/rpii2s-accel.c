/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Raspberry Pi I2S Audio Acceleration Stubs
*/

#include <config.h>
#include <devices/ahi.h>
#include <libraries/ahi_sub.h>
#include "DriverData.h"
#include "library.h"
/* No hardware acceleration */

ULONG
_AHIsub_SetVol(UWORD channel,
               Fixed volume,
               sposition pan,
               struct AHIAudioCtrlDrv *AudioCtrl,
               ULONG flags,
               struct DriverBase *AHIsubBase)
{
    return AHIS_UNKNOWN;
}

ULONG
_AHIsub_SetFreq(
    UWORD channel, ULONG freq, struct AHIAudioCtrlDrv *AudioCtrl, ULONG flags, struct DriverBase *AHIsubBase)
{
    return AHIS_UNKNOWN;
}

ULONG
_AHIsub_SetSound(UWORD channel,
                 UWORD sound,
                 ULONG offset,
                 LONG length,
                 struct AHIAudioCtrlDrv *AudioCtrl,
                 ULONG flags,
                 struct DriverBase *AHIsubBase)
{
    return AHIS_UNKNOWN;
}

ULONG
_AHIsub_SetEffect(APTR effect, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    return AHIS_UNKNOWN;
}

ULONG
_AHIsub_LoadSound(UWORD sound, ULONG type, APTR info, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    return AHIS_UNKNOWN;
}

ULONG
_AHIsub_UnloadSound(UWORD sound, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    return AHIS_UNKNOWN;
}
