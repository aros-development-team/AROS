/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include "pulseaudio_hostlib.h"
#include <proto/hostlib.h>

#include <aros/debug.h>

#define LIBPULSE_SIMPLE_SOFILE "libpulse-simple.so.0"
#define LIBPULSE_SOFILE        "libpulse.so.0"
#define LIBC_SOFILE      "libc.so.6"

static const char *pulse_simple_func_names[] = {
    "pa_simple_new",
    "pa_simple_free",
    "pa_simple_write",
    "pa_simple_drain"
};
#define PULSE_SIMPLE_NUM_FUNCS (sizeof(pulse_simple_func_names) / sizeof(pulse_simple_func_names[0]))
struct pulse_simple_func pulse_simple_func;
static void *libpulsesimplehandle;

static const char *pulse_func_names[] = {
    "pa_mainloop_new",
    "pa_mainloop_get_api",
    "pa_mainloop_iterate",
    "pa_mainloop_free",
    "pa_context_new",
    "pa_context_connect",
    "pa_context_get_state",
    "pa_context_disconnect",
    "pa_context_unref",
    "pa_context_get_sink_info_by_name",
    "pa_operation_get_state",
    "pa_operation_unref",
    "pa_cvolume_avg",
    "pa_cvolume_set",
    "pa_context_set_sink_volume_by_index",
    "pa_context_set_state_callback"
};
#define PULSE_NUM_FUNCS (sizeof(pulse_func_names) / sizeof(pulse_func_names[0]))
struct pulse_func pulse_func;
static void *libpulsehandle;

static const char *libc_func_names[] = {
    "sigfillset",
    "sigprocmask",
    "calloc",
    "free",
};

#define LIBC_NUM_FUNCS (sizeof(libc_func_names) / sizeof(libc_func_names[0]))
struct libc_func libc_func;
static void *libchandle;

APTR HostLibBase;

static void *hostlib_load_so(const char *sofile, const char **names, int nfuncs,
                             void **funcptr)
{
    void *handle;
    char *err;
    int i;

    if((handle = HostLib_Open(sofile, &err)) == NULL) {
        D(bug("[PULSEA] failed to open '%s': %s\n", sofile, err));
        return NULL;
    }

    for(i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if(err != NULL) {
            bug("[PULSEA] failed to get symbol '%s' (%s)\n", names[i], err);
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    return handle;
}

BOOL PULSEA_HostLib_Init()
{
    HostLibBase = OpenResource("hostlib.resource");

    if(!HostLibBase) {
        D(bug("[PULSEA] failed to open hostlib.resource\n"));
        return FALSE;
    }

    libpulsesimplehandle = hostlib_load_so(LIBPULSE_SIMPLE_SOFILE, pulse_simple_func_names,
                                           PULSE_SIMPLE_NUM_FUNCS, (void **)&pulse_simple_func);

    if(!libpulsesimplehandle) {
        bug("[PULSEA] failed to open " LIBPULSE_SIMPLE_SOFILE "\n");
        return FALSE;
    }

    libpulsehandle = hostlib_load_so(LIBPULSE_SOFILE, pulse_func_names,
                                     PULSE_NUM_FUNCS, (void **)&pulse_func);

    if(!libpulsehandle) {
        bug("[PULSEA] failed to open " LIBPULSE_SOFILE "\n");
        return FALSE;
    }

    libchandle = hostlib_load_so(LIBC_SOFILE, libc_func_names,
                                 LIBC_NUM_FUNCS, (void **)&libc_func);

    return TRUE;
}

VOID PULSEA_HostLib_Cleanup()
{
    if(libpulsesimplehandle != NULL)
        HostLib_Close(libpulsesimplehandle, NULL);

    if(libpulsehandle != NULL)
        HostLib_Close(libpulsehandle, NULL);

    if(libchandle != NULL)
        HostLib_Close(libchandle, NULL);
}
