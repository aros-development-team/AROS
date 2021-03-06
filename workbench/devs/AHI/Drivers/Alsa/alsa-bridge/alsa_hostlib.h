/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.
*/

#include <alsa/asoundlib.h>
#include <exec/types.h>

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

    int (*snd_mixer_open)(snd_mixer_t **mixer, int mode);
    int (*snd_mixer_close)(snd_mixer_t *mixer);
    int (*snd_mixer_load)(snd_mixer_t *mixer);
    int (*snd_mixer_attach)(snd_mixer_t *mixer, const char *name);
    snd_mixer_elem_t *(*snd_mixer_find_selem)(snd_mixer_t *mixer, const snd_mixer_selem_id_t *id);
    int (*snd_mixer_selem_register)(snd_mixer_t *mixer, struct snd_mixer_selem_regopt *options, snd_mixer_class_t **classp);
    int (*snd_mixer_selem_id_malloc)(snd_mixer_selem_id_t **ptr);
    void (*snd_mixer_selem_id_free)(snd_mixer_selem_id_t *obj);
    void (*snd_mixer_selem_id_set_index)(snd_mixer_selem_id_t *obj, unsigned int val);
    void (*snd_mixer_selem_id_set_name)(snd_mixer_selem_id_t *obj, const char *val);
    int  (*snd_mixer_selem_get_playback_volume_range)(snd_mixer_elem_t *elem, long *min, long *max);
    int  (*snd_mixer_selem_get_playback_volume)(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t channel, long *value);
    int  (*snd_mixer_selem_set_playback_volume_all)(snd_mixer_elem_t *elem, long value);
};

extern struct alsa_func alsa_func;

#define ALSACALL(func,...) (alsa_func.func(__VA_ARGS__))

BOOL ALSA_HostLib_Init();
VOID ALSA_HostLib_Cleanup();
