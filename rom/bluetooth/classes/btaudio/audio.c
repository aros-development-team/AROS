
/*
 *  AHI interface of the Bluetooth audio class: the mixed output is SBC encoded and sent
 *  to a Bluetooth audio sink (speaker, headphones) over AVDTP. Recording
 *  is not supported.
 */

#include <devices/ahi.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <stddef.h>

#include "btaudio.h"

#define dd ((struct BluetoothAudioData*) AudioCtrl->ahiac_DriverData)

AROS_LH2(ULONG, sub_AllocAudio,
	AROS_LHA(struct TagItem *, taglist, A1),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 5, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    struct BTAudioModeData *data = (APTR)GetTagData(AHIDB_Data, 0, taglist);
    struct BtaLink *unit;
    BOOL found = FALSE;

    if(!data || data->bmd_Size < sizeof(*data)) return AHISF_ERROR;
    ObtainSemaphore(&AHIsubBase->cls->lock);
    for(unit = (APTR)AHIsubBase->cls->units.lh_Head; unit->node.ln_Succ;
        unit = (APTR)unit->node.ln_Succ) {
        if(unit->attached && unit->modeid == data->bmd_ID && !unit->owner) {
            found = TRUE;
            break;
        }
    }
    if(found) {
        AudioCtrl->ahiac_DriverData = AllocVec(sizeof(struct BluetoothAudioData), MEMF_CLEAR | MEMF_PUBLIC);
        if(dd) {
            dd->unit = unit;
            dd->ahisubbase = AHIsubBase;
            dd->volume = 0x10000;
            ObtainSemaphore(&unit->lock);
            unit->owner = AudioCtrl;
            AudioCtrl->ahiac_MixFreq = unit->mixfreq;
            ReleaseSemaphore(&unit->lock);
        }
    }
    ReleaseSemaphore(&AHIsubBase->cls->lock);
    if(!found || !dd) return AHISF_ERROR;

    // 16 bit mixing only: SBC takes 16 bit samples.
    return (AHISF_KNOWSTEREO |
            AHISF_MIXING |
            AHISF_TIMING);
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, sub_FreeAudio,
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 6, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    if(AudioCtrl->ahiac_DriverData != NULL) {
        AHIsub_Stop(AHISF_PLAY, AudioCtrl);
        ObtainSemaphore(&AHIsubBase->cls->lock);
        ObtainSemaphore(&dd->unit->lock);
        dd->unit->owner = NULL;
        if(dd->unit->task) Signal(dd->unit->task, 1UL << dd->unit->wakesig);
        ReleaseSemaphore(&dd->unit->lock);
        ReleaseSemaphore(&AHIsubBase->cls->lock);
        FreeVec(AudioCtrl->ahiac_DriverData);
        AudioCtrl->ahiac_DriverData = NULL;
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, sub_Disable,
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 7, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    // V6 drivers do not have to preserve all registers

    Forbid();
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, sub_Enable,
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 8, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    // V6 drivers do not have to preserve all registers

    Permit();
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, sub_Start,
	AROS_LHA(ULONG, flags, D0),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 9, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    struct BluetoothAudioBase *BluetoothAudioBase = (struct BluetoothAudioBase *) AHIsubBase;

    AHIsub_Stop(flags, AudioCtrl);

    if(flags & AHISF_PLAY) {
        dd->mixbuffer = AllocVec(AudioCtrl->ahiac_BuffSize,
                                 MEMF_ANY | MEMF_PUBLIC);

        if(dd->mixbuffer == NULL) return AHIE_NOMEM;

        if(!bLinkPlay(BluetoothAudioBase, AudioCtrl)) {
            FreeVec(dd->mixbuffer);
            dd->mixbuffer = NULL;
            return AHIE_UNKNOWN;
        }
    }

    if(flags & AHISF_RECORD) {
        return AHIE_UNKNOWN;
    }

    return AHIE_OK;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, sub_Update,
	AROS_LHA(ULONG, flags, D0),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 10, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    // Empty function
    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, sub_Stop,
	AROS_LHA(ULONG, flags, D0),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 11, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    if(dd && (flags & AHISF_PLAY)) {
        bLinkRelease((struct BluetoothAudioBase *) AHIsubBase, AudioCtrl);

        FreeVec(dd->mixbuffer);
        dd->mixbuffer = NULL;
    }

    if(flags & AHISF_RECORD) {
        // Do nothing
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH5(IPTR, sub_GetAttr,
	AROS_LHA(ULONG, attribute, D0),
	AROS_LHA(LONG, argument, D1),
	AROS_LHA(IPTR, def, D2),
	AROS_LHA(struct TagItem *, taglist, A1),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 18, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    struct BTAudioModeData *data = (APTR)GetTagData(AHIDB_Data, 0, taglist);

    switch(attribute) {
    case AHIDB_Bits:
        return 16;

    case AHIDB_Frequencies:
        return 1;
    case AHIDB_Frequency:
        return argument == 0 && data ? data->bmd_Frequency : def;
    case AHIDB_Index:
        return 0;

    case AHIDB_Author:
        return (IPTR) "The AROS Development Team";

    case AHIDB_Copyright:
        return (IPTR) "(C) 2026 The AROS Development Team";

    case AHIDB_Version:
        return (IPTR) LibIDString;

    case AHIDB_Record:
        return FALSE;

    case AHIDB_Realtime:
        return TRUE;

    case AHIDB_MinOutputVolume:
        return 0x41;              // about -60 dB
    case AHIDB_MaxOutputVolume:
        return 0x10000;           // 0 dB: SBC has no headroom above full scale

    case AHIDB_Outputs:
        return 1;

    case AHIDB_Output:
        return (IPTR) "Bluetooth";

    default:
        return def;
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH3(ULONG, sub_HardwareControl,
	AROS_LHA(ULONG, attribute, D0),
	AROS_LHA(LONG, argument, D1),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 19, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    if(AudioCtrl->ahiac_DriverData == NULL) {
        return 0;
    }
    switch(attribute) {
    case AHIC_OutputVolume:
        // applied to the mixed samples before SBC encoding
        dd->volume = argument < 0 ? 0 : (argument > 0x10000 ? 0x10000 : (ULONG)argument);
        return TRUE;

    case AHIC_OutputVolume_Query:
        return dd->volume;
    }
    return 0;
    AROS_LIBFUNC_EXIT
}

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "btaudio.h"

AROS_LH5(ULONG, sub_SetVol,
	AROS_LHA(UWORD, channel, D0),
	AROS_LHA(Fixed, volume, D1),
	AROS_LHA(sposition, pan, D2),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	AROS_LHA(ULONG, flags, D3),
	struct BluetoothAudioBase *, AHIsubBase, 12, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, sub_SetFreq,
	AROS_LHA(UWORD, channel, D0),
	AROS_LHA(ULONG, freq, D1),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	AROS_LHA(ULONG, flags, D2),
	struct BluetoothAudioBase *, AHIsubBase, 13, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}

AROS_LH6(ULONG, sub_SetSound,
	AROS_LHA(UWORD, channel, D0),
	AROS_LHA(UWORD, sound, D1),
	AROS_LHA(ULONG, offset, D2),
	AROS_LHA(LONG, length, D3),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	AROS_LHA(ULONG, flags, D4),
	struct BluetoothAudioBase *, AHIsubBase, 14, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, sub_SetEffect,
	AROS_LHA(APTR, effect, A0),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 15, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}

AROS_LH4(ULONG, sub_LoadSound,
	AROS_LHA(UWORD, sound, D0),
	AROS_LHA(ULONG, type, D1),
	AROS_LHA(APTR, info, A0),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 16, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, sub_UnloadSound,
	AROS_LHA(UWORD, sound, D0),
	AROS_LHA(struct AHIAudioCtrlDrv *, AudioCtrl, A2),
	struct BluetoothAudioBase *, AHIsubBase, 17, btaudio_sub)
{
    AROS_LIBFUNC_INIT
    return AHIS_UNKNOWN;
    AROS_LIBFUNC_EXIT
}
