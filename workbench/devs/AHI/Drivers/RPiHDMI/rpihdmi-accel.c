/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI Audio AHI Hardware Mixing Stubs
*/

#include <exec/types.h>
#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"

ULONG _AHIsub_SetVol(UWORD channel,
                     Fixed volume,
                     sposition pan,
                     struct AHIAudioCtrlDrv *AudioCtrl,
                     ULONG flags,
                     struct DriverBase *AHIsubBase)
{
    (void)channel;
    (void)volume;
    (void)pan;
    (void)AudioCtrl;
    (void)flags;
    (void)AHIsubBase;
    return AHIS_UNKNOWN;
}

ULONG _AHIsub_SetFreq(UWORD channel,
                      ULONG freq,
                      struct AHIAudioCtrlDrv *AudioCtrl,
                      ULONG flags,
                      struct DriverBase *AHIsubBase)
{
    (void)channel;
    (void)freq;
    (void)AudioCtrl;
    (void)flags;
    (void)AHIsubBase;
    return AHIS_UNKNOWN;
}

ULONG _AHIsub_SetSound(UWORD channel,
                       UWORD sound,
                       ULONG offset,
                       LONG length,
                       struct AHIAudioCtrlDrv *AudioCtrl,
                       ULONG flags,
                       struct DriverBase *AHIsubBase)
{
    (void)channel;
    (void)sound;
    (void)offset;
    (void)length;
    (void)AudioCtrl;
    (void)flags;
    (void)AHIsubBase;
    return AHIS_UNKNOWN;
}

ULONG _AHIsub_SetEffect(APTR effect,
                        struct AHIAudioCtrlDrv *AudioCtrl,
                        struct DriverBase *AHIsubBase)
{
    (void)effect;
    (void)AudioCtrl;
    (void)AHIsubBase;
    return AHIS_UNKNOWN;
}

ULONG _AHIsub_LoadSound(UWORD sound,
                        ULONG type,
                        APTR info,
                        struct AHIAudioCtrlDrv *AudioCtrl,
                        struct DriverBase *AHIsubBase)
{
    (void)sound;
    (void)type;
    (void)info;
    (void)AudioCtrl;
    (void)AHIsubBase;
    return AHIS_UNKNOWN;
}

ULONG _AHIsub_UnloadSound(UWORD sound,
                          struct AHIAudioCtrlDrv *AudioCtrl,
                          struct DriverBase *AHIsubBase)
{
    (void)sound;
    (void)AudioCtrl;
    (void)AHIsubBase;
    return AHIS_UNKNOWN;
}
