/*
    Copyright (C) 2015-2024, The AROS Development Team. All rights reserved.
*/

#include <signal.h>

#include "alsa_hostlib.h"
#include "alsa.h"

#define CARDNAME    "default"
#define VOLUMENAME  "Master"

/*****************************************************************************/

/*  This is an unfortunate HACK

    Depending on how sound system is configured on host, ALSA user space library
    can go directly to ALSA or go through user space sound servers like
    PulseAudio or PipeWire. The implementation of Alsa-over-PipeWire is specific
    as it starts three aditional threads. These threads inherit process signal
    mask from parent (which is AROS). As a results any of those threads can be
    invoked to handle a signal instead of AROS hosted kernel. On the other hand,
    AROS hosted kernel uses signals to drive interrupts. In effect a PipeWire
    thread can start executing AROS interrupt handler in parallel to AROS
    thread operating as usual, which quickly leads to either stack out of range
    (when PipeWire thread exists from interrupt and tries to schedule next
    task) or semaphore being called from supervisor (when AROS thread executes
    normal AROS code but supervisor flag was changed due to PipeWire thread
    starting to handle interrupt)

    There doesn't seem to be a way forbid child threads from inheriting parent
    signals. Suggested solution it to change parent signals during creation
    of child threads. This is what this hack do. In effect, for a short period
    of time it completly disables AROS kernel logic.

    The placement of these functions in code is adjusted to current
    (Ubuntu 24.04) implementation of thread creation in PipeWire. It is
    completly possible that future versions will change implementation and
    placement will have to be changed.
*/

static void _prepare_kernel_for_new_host_pthread(sigset_t *sigset)
{
    sigset_t _full;
    libc_func.sigfillset(&_full);
    libc_func.sigprocmask(SIG_SETMASK, &_full, sigset);
}

static void _restore_kernel_after_new_host_pthread(sigset_t *sigset)
{
    libc_func.sigprocmask(SIG_SETMASK, sigset, NULL);
}

/*****************************************************************************/

BOOL ALSA_Init()
{
    return ALSA_HostLib_Init();
}

VOID ALSA_Cleanup()
{
    ALSA_HostLib_Cleanup();
}

VOID ALSA_MixerInit(APTR *handle, APTR *elem, LONG *min, LONG *max)
{
    snd_mixer_t *_handle;
    snd_mixer_selem_id_t *_sid;
    snd_mixer_elem_t *_elem;
    sigset_t _current;

    *handle = NULL;
    *elem = NULL;

    _prepare_kernel_for_new_host_pthread(&_current);

    ALSACALL(snd_mixer_open, &_handle, 0);
    ALSACALL(snd_mixer_attach, _handle, CARDNAME);

    _restore_kernel_after_new_host_pthread(&_current);

    ALSACALL(snd_mixer_selem_register, _handle, NULL, NULL);
    ALSACALL(snd_mixer_load, _handle);

    ALSACALL(snd_mixer_selem_id_malloc, &_sid);
    ALSACALL(snd_mixer_selem_id_set_index, _sid, 0);
    ALSACALL(snd_mixer_selem_id_set_name, _sid, VOLUMENAME);
    _elem = ALSACALL(snd_mixer_find_selem, _handle, _sid);

    if(_elem != NULL) {
        long a, b;
        ALSACALL(snd_mixer_selem_get_playback_volume_range, _elem, &a, &b);

        *handle = _handle;
        *elem   = _elem;
        *min = (LONG)a;
        *max = (LONG)b;
    }

    ALSACALL(snd_mixer_selem_id_free, _sid);
}

VOID ALSA_MixerCleanup(APTR handle)
{
    ALSACALL(snd_mixer_close, handle);
}

LONG ALSA_MixerGetVolume(APTR elem)
{
    long _ret = 0;

    ALSACALL(snd_mixer_selem_get_playback_volume, elem,
             SND_MIXER_SCHN_FRONT_LEFT, &_ret);
    return (LONG)_ret;
}

VOID ALSA_MixerSetVolume(APTR elem, LONG volume)
{
    ALSACALL(snd_mixer_selem_set_playback_volume_all, elem, volume);
}

APTR ALSA_Open()
{
    snd_pcm_t *handle = NULL;
    int _ret;
    sigset_t _current;

    _prepare_kernel_for_new_host_pthread(&_current);
    _ret = ALSACALL(snd_pcm_open, &handle, CARDNAME, SND_PCM_STREAM_PLAYBACK, 0);
    _restore_kernel_after_new_host_pthread(&_current);

    if(_ret < 0) return NULL;

    return handle;
}

VOID ALSA_DropAndClose(APTR handle)
{
    if(handle) {
        ALSACALL(snd_pcm_drop, handle);
        ALSACALL(snd_pcm_close, handle);
    }
}

BOOL ALSA_SetHWParams(APTR handle, ULONG *rate)
{
    snd_pcm_hw_params_t *hw_params;
    LONG dir = 0;
    int r = 0;

    ALSACALL(snd_pcm_hw_params_malloc, &hw_params);
    ALSACALL(snd_pcm_hw_params_any, handle, hw_params);
    ALSACALL(snd_pcm_hw_params_set_access, handle, hw_params,
             SND_PCM_ACCESS_RW_INTERLEAVED);
    ALSACALL(snd_pcm_hw_params_set_format, handle, hw_params,
             SND_PCM_FORMAT_S16_LE);
    ALSACALL(snd_pcm_hw_params_set_channels, handle, hw_params, 2);
    r = ALSACALL(snd_pcm_hw_params_set_rate_near, handle, hw_params, rate, &dir);
    ALSACALL(snd_pcm_hw_params_set_buffer_size, handle, hw_params, 2048);

    ALSACALL(snd_pcm_hw_params, handle, hw_params);
    ALSACALL(snd_pcm_hw_params_free, hw_params);

    return (r >= 0);
}

LONG ALSA_Write(APTR handle, APTR buffer, ULONG size)
{
    LONG rc = ALSACALL(snd_pcm_writei, handle, buffer, (snd_pcm_uframes_t)size);

    if(rc == -EPIPE)
        rc = ALSA_XRUN;

    return rc;
}

VOID ALSA_Prepare(APTR handle)
{
    ALSACALL(snd_pcm_prepare, handle);
}

LONG ALSA_Avail(APTR handle)
{
    LONG rc = ALSACALL(snd_pcm_avail_update, handle);
    if(rc == -EPIPE)
        rc = ALSA_XRUN;

    return rc;
}
