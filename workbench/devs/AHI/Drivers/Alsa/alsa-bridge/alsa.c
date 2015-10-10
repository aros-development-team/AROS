/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "alsa.h"
#include "alsa_hostlib.h"

BOOL ALSA_Init()
{
    return ALSA_HostLib_Init();
}

VOID ALSA_Cleanup()
{
    ALSA_HostLib_Cleanup();
}

APTR ALSA_Open()
{
    snd_pcm_t * handle = NULL;


    if (ALSACALL(snd_pcm_open, &handle, "default",
            SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return NULL;

    return handle;
}

VOID ALSA_Close(APTR handle)
{
    if (handle)
    {
        ALSACALL(snd_pcm_close, handle);
    }
}

BOOL ALSA_SetHWParams(APTR handle, ULONG * rate)
{
    snd_pcm_hw_params_t *hw_params;
    LONG dir = 0; int r = 0;

    ALSACALL(snd_pcm_hw_params_malloc, &hw_params);
    ALSACALL(snd_pcm_hw_params_any, handle, hw_params);
    ALSACALL(snd_pcm_hw_params_set_access, handle, hw_params,
            SND_PCM_ACCESS_RW_INTERLEAVED);
    ALSACALL(snd_pcm_hw_params_set_format, handle, hw_params,
            SND_PCM_FORMAT_S16_LE);
    ALSACALL(snd_pcm_hw_params_set_channels, handle, hw_params, 2);
    r = ALSACALL(snd_pcm_hw_params_set_rate_near, handle, hw_params, rate, &dir);
    ALSACALL(snd_pcm_hw_params_set_buffer_size, handle, hw_params, 4096);

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
