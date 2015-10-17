/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <alsa/asoundlib.h>

struct alsa_func
{
    int (*snd_pcm_open)(snd_pcm_t **pcmp, const char *name, snd_pcm_stream_t stream, int mode);
    int (*snd_pcm_close)(snd_pcm_t *pcm);
    int (*snd_pcm_hw_params_malloc)(snd_pcm_hw_params_t **ptr);
    void (*snd_pcm_hw_params_free)(snd_pcm_hw_params_t *obj);
    int (*snd_pcm_hw_params_any)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
    int (*snd_pcm_hw_params_set_access)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
    int (*snd_pcm_hw_params_set_format)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
    int (*snd_pcm_hw_params_set_rate_near)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
    int (*snd_pcm_hw_params_set_channels)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
    int (*snd_pcm_hw_params)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
    int (*snd_pcm_prepare)(snd_pcm_t *pcm);
    snd_pcm_sframes_t(*snd_pcm_writei)(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
    snd_pcm_sframes_t(*snd_pcm_avail_update)(snd_pcm_t *pcm);
    int (*snd_pcm_hw_params_get_buffer_size)(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val);
    int (*snd_pcm_hw_params_set_buffer_size)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val);
    int (*snd_pcm_drop)(snd_pcm_t *pcm);
};

extern struct alsa_func alsa_func;

#define ALSACALL(func,...) (alsa_func.func(__VA_ARGS__))

BOOL ALSA_HostLib_Init();
VOID ALSA_HostLib_Cleanup();
