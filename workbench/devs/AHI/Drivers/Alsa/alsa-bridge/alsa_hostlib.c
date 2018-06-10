/*
    Copyright © 2015-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "alsa_hostlib.h"
#include <proto/hostlib.h>

#include <aros/debug.h>

#define LIBASOUND_SOFILE "libasound.so.2"

static const char *alsa_func_names[] =
{
    "snd_pcm_open",
    "snd_pcm_close",
    "snd_pcm_hw_params_malloc",
    "snd_pcm_hw_params_free",
    "snd_pcm_hw_params_any",
    "snd_pcm_hw_params_set_access",
    "snd_pcm_hw_params_set_format",
    "snd_pcm_hw_params_set_rate_near",
    "snd_pcm_hw_params_set_channels",
    "snd_pcm_hw_params",
    "snd_pcm_prepare",
    "snd_pcm_writei",
    "snd_pcm_avail_update",
    "snd_pcm_hw_params_get_buffer_size",
    "snd_pcm_hw_params_set_buffer_size",
    "snd_pcm_drop",

    "snd_mixer_open",
    "snd_mixer_close",
    "snd_mixer_load",
    "snd_mixer_attach",
    "snd_mixer_find_selem",
    "snd_mixer_selem_register",
    "snd_mixer_selem_id_malloc",
    "snd_mixer_selem_id_free",
    "snd_mixer_selem_id_set_index",
    "snd_mixer_selem_id_set_name",
    "snd_mixer_selem_get_playback_volume_range",
    "snd_mixer_selem_get_playback_volume",
    "snd_mixer_selem_set_playback_volume_all",
};

#define ALSA_NUM_FUNCS (sizeof(alsa_func_names) / sizeof(alsa_func_names[0]))

APTR HostLibBase;
struct alsa_func alsa_func;
static void *libasoundhandle;

static void *hostlib_load_so(const char *sofile, const char **names, int nfuncs,
        void **funcptr)
{
    void *handle;
    char *err;
    int i;

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        D(bug("[ALSA] failed to open '%s': %s\n", sofile, err));
        return NULL;
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if (err != NULL) {
            bug("[ALSA] failed to get symbol '%s' (%s)\n", names[i], err);
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    return handle;
}

BOOL ALSA_HostLib_Init()
{
    HostLibBase = OpenResource("hostlib.resource");

    if (!HostLibBase)
    {
        D(bug("[ALSA] failed to open hostlib.resource\n"));
        return FALSE;
    }

    libasoundhandle = hostlib_load_so(LIBASOUND_SOFILE, alsa_func_names,
            ALSA_NUM_FUNCS, (void **)&alsa_func);

    if (!libasoundhandle)
    {
        bug("[ALSA] failed to open "LIBASOUND_SOFILE"\n");
        return FALSE;
    }

    return TRUE;
}

VOID ALSA_HostLib_Cleanup()
{
    if (libasoundhandle != NULL)
        HostLib_Close(libasoundhandle, NULL);
}
