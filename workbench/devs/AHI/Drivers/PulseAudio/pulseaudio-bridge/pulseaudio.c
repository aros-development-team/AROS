/*
    Copyright (C) 2025,
    The AROS Development Team. All rights reserved.
*/
#include <aros/debug.h>

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

#include "pulseaudio_hostlib.h"   /* New hostlib for PulseAudio */
#include "pulseaudio.h"

#define APP_NAME    "AROS"
#define STREAM_DESC "AROS Audio"
#define SINK_NAME   NULL /* default sink */

/*****************************************************************************/

static void _prepare_kernel_for_new_host_pthread(sigset_t *sigset)
{
    sigset_t _full;
    CLIBCALL(sigfillset, &_full);
    CLIBCALL(sigprocmask, SIG_SETMASK, &_full, sigset);
}

static void _restore_kernel_after_new_host_pthread(sigset_t *sigset)
{
    CLIBCALL(sigprocmask, SIG_SETMASK, sigset, NULL);
}

/*****************************************************************************/

BOOL PULSEA_Init(void)
{
    return PULSEA_HostLib_Init();
}

VOID PULSEA_Cleanup(void)
{
    PULSEA_HostLib_Cleanup();
}

/*
 * Mixer functions — these use the asynchronous libpulse API
 * to change the volume of the default sink.
 */
typedef struct {
    pa_mainloop *ml;
    pa_context  *ctx;
    pa_cvolume   vol;
    uint32_t     sink_idx;
} pulse_mixer_t;

static void _sink_info_cb(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
    pulse_mixer_t *pm = userdata;
    if(eol > 0 || !i) return;
    pm->sink_idx = i->index;
    pm->vol = i->volume;
}

/* Fallback implementation of pa_sw_volume_from_dB to avoid undefined-symbol
 * when runtime libpulse doesn't provide it.
 */

/* pa_volume_t is an unsigned integer type defined in pulse/volume.h.
 * PA_VOLUME_NORM and PA_VOLUME_MAX are available from the headers.
 */
pa_volume_t pa_sw_volume_from_dB(double f)
{
    /* Convert decibels (amplitude dB) to linear amplitude factor */
    double lin = pow(10.0, f / 20.0);

    /* Scale by PA_VOLUME_NORM (the library's "normal" volume) */
    double v = lin * (double)PA_VOLUME_NORM;

    if(v < 0.0) v = 0.0;
    if(v > (double)PA_VOLUME_MAX) v = (double)PA_VOLUME_MAX;

    /* Round to nearest integer pa_volume_t */
    return (pa_volume_t)(v + 0.5);
}

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata)
{
    pa_context_state_t state;
    int *pa_ready = userdata;
    state = PULSEACALL(pa_context_get_state, c);
    switch(state) {
    // These are just here for reference
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    default:
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        *pa_ready = 2;
        break;
    case PA_CONTEXT_READY:
        *pa_ready = 1;
        break;
    }
}

VOID PULSEA_MixerInit(APTR *handle, APTR *elem, LONG *min, LONG *max)
{
    pulse_mixer_t *pm = CLIBCALL(calloc, 1, sizeof(*pm));
    if(!pm) return;

    *handle = NULL;
    *elem   = NULL;
    *min = 0;
    *max = PA_VOLUME_UI_MAX;

    bug("[PAUDIO] %s: pm @ 0x%p\n", __func__, pm);

    int pa_ready = 0;
    sigset_t _current;
    _prepare_kernel_for_new_host_pthread(&_current);

    pm->ml = PULSEACALL(pa_mainloop_new);
    if(!pm->ml) goto fail;

    bug("[PAUDIO] %s: ml @ 0x%p\n", __func__, pm->ml);

    pm->ctx = PULSEACALL(pa_context_new, PULSEACALL(pa_mainloop_get_api, pm->ml), APP_NAME);
    if(!pm->ctx) goto fail_ml;

    bug("[PAUDIO] %s: ctx @ 0x%p\n", __func__, pm->ctx);

    PULSEACALL(pa_context_connect, pm->ctx, NULL, 0, NULL);

    bug("[PAUDIO] %s: waiting for context to become ready ..\n", __func__);

    PULSEACALL(pa_context_set_state_callback, pm->ctx, pa_state_cb, &pa_ready);

    while(pa_ready == 0) {
        PULSEACALL(pa_mainloop_iterate, pm->ml, 1, NULL);
    }

    bug("[PAUDIO] %s: obtaining default sync ..\n", __func__);

    pa_operation *op = PULSEACALL(pa_context_get_sink_info_by_name,
                                  pm->ctx,
                                  SINK_NAME ? SINK_NAME : "@DEFAULT_SINK@",
                                  _sink_info_cb,
                                  pm);

    bug("[PAUDIO] %s: op @ 0x%p\n", __func__, op);

    if(op) {
        while(PULSEACALL(pa_operation_get_state, op) == PA_OPERATION_RUNNING)
            PULSEACALL(pa_mainloop_iterate, pm->ml, 1, NULL);
        PULSEACALL(pa_operation_unref, op);
    }

    bug("[PAUDIO] %s: mixer ready\n", __func__);

    _restore_kernel_after_new_host_pthread(&_current);

    *handle = pm;
    *elem   = pm; /* same object */
    return;

fail_ml:
    PULSEACALL(pa_mainloop_free, pm->ml);
fail:
    CLIBCALL(free, pm);
    _restore_kernel_after_new_host_pthread(&_current);
    return;
}

VOID PULSEA_MixerCleanup(APTR handle)
{
    pulse_mixer_t *pm = handle;
    if(!pm) return;
    PULSEACALL(pa_context_disconnect, pm->ctx);
    PULSEACALL(pa_context_unref, pm->ctx);
    PULSEACALL(pa_mainloop_free, pm->ml);
    CLIBCALL(free, pm);
}

LONG PULSEA_MixerGetVolume(APTR elem)
{
    pulse_mixer_t *pm = elem;
    if(!pm) return 0;
    return (LONG)PULSEACALL(pa_cvolume_avg, &pm->vol);
}

VOID PULSEA_MixerSetVolume(APTR elem, LONG volume)
{
    pulse_mixer_t *pm = elem;
    if(!pm) return;
    PULSEACALL(pa_cvolume_set, &pm->vol, pm->vol.channels, (pa_volume_t)volume);
    pa_operation *op = PULSEACALL(pa_context_set_sink_volume_by_index, pm->ctx, pm->sink_idx, &pm->vol, NULL, NULL);
    if(op) {
        while(PULSEACALL(pa_operation_get_state, op) == PA_OPERATION_RUNNING)
            PULSEACALL(pa_mainloop_iterate, pm->ml, 1, NULL);
        PULSEACALL(pa_operation_unref, op);
    }
}

/*
 * Playback functions — these use libpulse-simple for synchronous streaming
 */
APTR PULSEA_Open(void)
{
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.rate   = 44100;
    ss.channels = 2;

    sigset_t _current;
    _prepare_kernel_for_new_host_pthread(&_current);

    int error = 0;
    pa_simple *s = PULSEASCALL(pa_simple_new,
                               NULL, /* server */
                               APP_NAME,
                               PA_STREAM_PLAYBACK,
                               SINK_NAME,
                               STREAM_DESC,
                               &ss,
                               NULL, /* channel map */
                               NULL, /* buffer attr */
                               &error);

    _restore_kernel_after_new_host_pthread(&_current);

    if(!s) {
        bug("[PulseA] %s: pa_simple_new failed error code %x\n", __func__, error);
    }
    return s;
}

VOID PULSEA_DropAndClose(APTR handle)
{
    if(handle) {
        PULSEASCALL(pa_simple_free, handle);
    }
}

BOOL PULSEA_SetHWParams(APTR handle, ULONG *rate)
{
    /* With libpulse-simple, format is fixed at open — can't change here.
       Return TRUE if handle is valid. */
    return (handle != NULL);
}

LONG PULSEA_Write(APTR handle, APTR buffer, ULONG size)
{
    if(!handle) return -1;
    if(!buffer || size == 0) {
        // No data provided, return zero frames written
        return 0;
    }

    int error = 0;
    if(PULSEASCALL(pa_simple_write, handle, buffer, size, &error) < 0) {
        if(error == PA_ERR_WOULD_BLOCK) {
            // Stream not ready to accept data yet — just return 0 frames, not failure
            return 0;
        }
        bug("[PulseA] %s: pa_simple_write failed error code %x\n", __func__, error);
        return -1; /* libpulse-simple doesn't expose ALSA-like XRUN codes */
    }

    /* return frames written (approx). size is bytes; S16 LE stereo => 4 bytes/frame */
    return (LONG)(size / (sizeof(int16_t) * 2));
}

#if (0)
VOID PULSEA_Prepare(APTR handle)
{
    /* No-op for PulseAudio simple API */
}

LONG PULSEA_Avail(APTR handle)
{
    /* libpulse-simple doesn't expose this — pretend buffer always ready */
    return 2048;
}
#endif
