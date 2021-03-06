/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include "alsa_hostlib.h"
#include "alsa.h"

#define CARDNAME    "default"
#define VOLUMENAME  "Master"

BOOL ALSA_Init()
{
    return ALSA_HostLib_Init();
}

VOID ALSA_Cleanup()
{
    ALSA_HostLib_Cleanup();
}

VOID ALSA_MixerInit(APTR * handle, APTR * elem, LONG * min, LONG * max)
{
    snd_mixer_t * _handle;
    snd_mixer_selem_id_t * _sid;
    snd_mixer_elem_t * _elem;

    *handle = NULL;
    *elem = NULL;

    ALSACALL(snd_mixer_open,&_handle, 0);
    ALSACALL(snd_mixer_attach,_handle, CARDNAME);
    ALSACALL(snd_mixer_selem_register,_handle, NULL, NULL);
    ALSACALL(snd_mixer_load,_handle);

    ALSACALL(snd_mixer_selem_id_malloc,&_sid);
    ALSACALL(snd_mixer_selem_id_set_index,_sid, 0);
    ALSACALL(snd_mixer_selem_id_set_name,_sid, VOLUMENAME);
    _elem = ALSACALL(snd_mixer_find_selem,_handle, _sid);

    if (_elem != NULL)
    {
        long a,b;
        ALSACALL(snd_mixer_selem_get_playback_volume_range,_elem, &a, &b);

        *handle = _handle;
        *elem   = _elem;
        *min = (LONG)a;
        *max = (LONG)b;
    }

    ALSACALL(snd_mixer_selem_id_free,_sid);
}

VOID ALSA_MixerCleanup(APTR handle)
{
    ALSACALL(snd_mixer_close,handle);
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
    snd_pcm_t * handle = NULL;


    if (ALSACALL(snd_pcm_open, &handle, CARDNAME,
            SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return NULL;

    return handle;
}

VOID ALSA_DropAndClose(APTR handle)
{
    if (handle)
    {
        ALSACALL(snd_pcm_drop, handle);
        ALSACALL(snd_pcm_close, handle);
    }
}

BOOL ALSA_SetHWParams(APTR handle, ULONG * rate)
{
    snd_pcm_hw_params_t * hw_params;
    LONG dir = 0; int r = 0;

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

    if (rc == -EPIPE)
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
    if (rc == -EPIPE)
        rc = ALSA_XRUN;

    return rc;
}
