/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/hostlib.h>
#include "alsa_hostlib.h"

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
};

#define ALSA_NUM_FUNCS (sizeof(alsa_func_names) / sizeof(alsa_func_names[0]))

APTR HostLibBase;
struct alsa_func alsa_func;
static void * libasoundhandle;

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
        return FALSE;

    libasoundhandle = hostlib_load_so(LIBASOUND_SOFILE, alsa_func_names,
            ALSA_NUM_FUNCS, (void **)&alsa_func);

    if (!libasoundhandle)
        return FALSE;

    return TRUE;
}

VOID ALSA_HostLib_Cleanup()
{
    HostLib_Close(libasoundhandle, NULL);
}
