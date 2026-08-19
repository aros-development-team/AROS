/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI Audio AHI Sub-Driver Main Implementation
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/ahi_sub.h>
#include <devices/ahi.h>

#include "DriverData.h"
#include "rpihdmi-hwaccess.h"

#define FREQUENCIES 4
static const ULONG frequencies[FREQUENCIES] = { 44100, 48000, 96000, 192000 };

/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG _AHIsub_AllocAudio(struct TagItem *tagList,
                         struct AHIAudioCtrlDrv *AudioCtrl,
                         struct DriverBase *AHIsubBase)
{
    struct RPiHDMIData *dd;

    (void)tagList;
    (void)AHIsubBase;

    dd = AllocMem(sizeof(struct RPiHDMIData), MEMF_PUBLIC | MEMF_CLEAR);
    if (!dd) {
        return AHIE_NOMEM;
    }

    dd->ahisubbase = (struct RPiHDMIBase *)AHIsubBase;
    AudioCtrl->ahiac_DriverData = (struct DriverData *)dd;

    return AHIE_OK;
}

/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void _AHIsub_FreeAudio(struct AHIAudioCtrlDrv *AudioCtrl,
                       struct DriverBase *AHIsubBase)
{
    struct RPiHDMIData *dd = (struct RPiHDMIData *)AudioCtrl->ahiac_DriverData;

    (void)AHIsubBase;

    if (dd) {
        rpihdmi_hw_cleanup(dd);
        FreeMem(dd, sizeof(struct RPiHDMIData));
        AudioCtrl->ahiac_DriverData = NULL;
    }
}

/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG _AHIsub_Start(ULONG flags,
                    struct AHIAudioCtrlDrv *AudioCtrl,
                    struct DriverBase *AHIsubBase)
{
    struct RPiHDMIData *dd = (struct RPiHDMIData *)AudioCtrl->ahiac_DriverData;

    (void)AHIsubBase;

    if (!dd) {
        return AHIE_UNKNOWN;
    }

    if (flags & AHISF_PLAY) {
        if (!rpihdmi_hw_init(dd, AudioCtrl->ahiac_MixFreq)) {
            return AHIE_UNKNOWN;
        }

        rpihdmi_hw_start_dma(dd);
    }

    return AHIE_OK;
}

/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void _AHIsub_Update(ULONG flags,
                    struct AHIAudioCtrlDrv *AudioCtrl,
                    struct DriverBase *AHIsubBase)
{
    (void)flags;
    (void)AudioCtrl;
    (void)AHIsubBase;
}

/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void _AHIsub_Stop(ULONG flags,
                  struct AHIAudioCtrlDrv *AudioCtrl,
                  struct DriverBase *AHIsubBase)
{
    struct RPiHDMIData *dd = (struct RPiHDMIData *)AudioCtrl->ahiac_DriverData;

    (void)AHIsubBase;

    if (dd && (flags & AHISF_PLAY)) {
        rpihdmi_hw_stop_dma(dd);
    }
}

/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

IPTR _AHIsub_GetAttr(ULONG attribute,
                     LONG argument,
                     IPTR def,
                     struct TagItem *taglist,
                     struct AHIAudioCtrlDrv *AudioCtrl,
                     struct DriverBase *AHIsubBase)
{
    size_t i;

    (void)taglist;
    (void)AudioCtrl;
    (void)AHIsubBase;

    switch (attribute) {
    case AHIDB_Bits:
        return 16;

    case AHIDB_Frequencies:
        return FREQUENCIES;

    case AHIDB_Frequency:
        if (argument >= 0 && argument < FREQUENCIES) {
            return (LONG)frequencies[argument];
        }
        return 48000;

    case AHIDB_Index:
        if (argument <= (LONG)frequencies[0]) {
            return 0;
        }
        if (argument >= (LONG)frequencies[FREQUENCIES - 1]) {
            return FREQUENCIES - 1;
        }
        for (i = 1; i < FREQUENCIES; i++) {
            if ((LONG)frequencies[i] > argument) {
                if ((argument - (LONG)frequencies[i - 1]) < ((LONG)frequencies[i] - argument)) {
                    return i - 1;
                } else {
                    return i;
                }
            }
        }
        return 0;

    case AHIDB_Author:
        return (IPTR)"Fabian Schmieder (@metaneutrons)";

    case AHIDB_Copyright:
        return (IPTR)"Copyright (C) 2026, The AROS Development Team. All rights reserved.";

    case AHIDB_Version:
        return (IPTR)"rpihdmi.audio 1.0";

    case AHIDB_Record:
        return FALSE;

    case AHIDB_Realtime:
        return TRUE;

    case AHIDB_Outputs:
        return 1;

    case AHIDB_Output:
        return (IPTR)"HDMI Digital Audio";

    default:
        return def;
    }
}

/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG _AHIsub_HardwareControl(ULONG attribute,
                              LONG argument,
                              struct AHIAudioCtrlDrv *AudioCtrl,
                              struct DriverBase *AHIsubBase)
{
    (void)attribute;
    (void)argument;
    (void)AudioCtrl;
    (void)AHIsubBase;
    return 0;
}
