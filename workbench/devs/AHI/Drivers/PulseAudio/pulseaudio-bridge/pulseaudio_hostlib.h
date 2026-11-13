/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <exec/types.h>

struct pulse_simple_func {
    pa_simple *(*pa_simple_new)(const char *, const char *, pa_stream_direction_t,
                                const char *, const char *, const pa_sample_spec *,
                                const pa_channel_map *, const pa_buffer_attr *, int *);
    void (*pa_simple_free)(pa_simple *);
    int (*pa_simple_write)(pa_simple *, const void *, size_t, int *);
    int (*pa_simple_drain)(pa_simple *, int *);
};

struct pulse_func {
    pa_mainloop *(*pa_mainloop_new)(void);
    pa_mainloop_api *(*pa_mainloop_get_api)(pa_mainloop *);
    int (*pa_mainloop_iterate)(pa_mainloop *, int, int *);
    void (*pa_mainloop_free)(pa_mainloop *);
    pa_context *(*pa_context_new)(pa_mainloop_api *, const char *);
    int (*pa_context_connect)(pa_context *, const char *, pa_context_flags_t, const pa_spawn_api *);
    pa_context_state_t (*pa_context_get_state)(pa_context *);
    void (*pa_context_disconnect)(pa_context *);
    void (*pa_context_unref)(pa_context *);
    pa_operation *(*pa_context_get_sink_info_by_name)(pa_context *, const char *, pa_sink_info_cb_t, void *);
    pa_operation_state_t (*pa_operation_get_state)(pa_operation *);
    void (*pa_operation_unref)(pa_operation *);
    pa_volume_t (*pa_cvolume_avg)(const pa_cvolume *);
    pa_cvolume *(*pa_cvolume_set)(pa_cvolume *, unsigned, pa_volume_t);
    pa_operation *(*pa_context_set_sink_volume_by_index)(pa_context *, uint32_t, const pa_cvolume *,
            pa_context_success_cb_t, void *);
    void (*pa_context_set_state_callback)(pa_context *c, pa_context_notify_cb_t cb, void *userdata);
};

struct libc_func {
    int (*sigfillset)(sigset_t *set);
    int (*sigprocmask)(int how, const sigset_t *set, sigset_t *oldset);
    void *(*calloc)(size_t nmemb, size_t size);
    void (*free)(void *ptr);
};

extern struct pulse_simple_func pulse_simple_func;
extern struct pulse_func pulse_func;
extern struct libc_func libc_func;

#define PULSEASCALL(func, ...) (pulse_simple_func.func(__VA_ARGS__))
#define PULSEACALL(func, ...)  (pulse_func.func(__VA_ARGS__))
#define CLIBCALL(func, ...)  (libc_func.func(__VA_ARGS__))

BOOL PULSEA_HostLib_Init();
VOID PULSEA_HostLib_Cleanup();
