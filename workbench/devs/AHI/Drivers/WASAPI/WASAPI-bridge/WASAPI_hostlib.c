/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>

#include "WASAPI_hostlib.h"

#include <aros/symbolsets.h>

#include <proto/hostlib.h>

#define DEBUG 0
#include <aros/debug.h>

static const char *wasapi_ole32_func_names[] =
{
    "CoInitialize",
    "CoUninitialize",
    "CoCreateInstance"
};

#define WASAPI_OLE32_NUM_FUNCS (sizeof(wasapi_ole32_func_names) / sizeof(wasapi_ole32_func_names[0]))

static const char *wasapi_native_func_names[] =
{
    "WASAPIAudio_PropVariantInit",
    "WASAPIAudio_PropVariantClear",
    "WASAPIAudio_ReadPropValStrN",
    "WASAPIAudio_InitWaveFormat",
    "WASAPIAudio_GetDefaultAudioEndpoint",
    "WASAPIAudio_IMMDActivate",
    "WASAPIAudio_IMMDOpenPropertyStore",
    "WASAPIAudio_IMMDRelease",
    "WASAPIAudio_IACGetService",
    "WASAPIAudio_IACGetMixFormat",
    "WASAPIAudio_IACGetBufferSize",
    "WASAPIAudio_IACGetCurrentPadding",
    "WASAPIAudio_IACGetDevicePeriod",
    "WASAPIAudio_IACInitialize",
    "WASAPIAudio_IACStart",
    "WASAPIAudio_IACStop",
    "WASAPIAudio_IPSGetValue",
    "WASAPIAudio_IPSRelease",
    "WASAPIAudio_IARCGetBuffer",
    "WASAPIAudio_IARCReleaseBuffer",
    "WASAPIAudio_ISAVGetMasterVolume",
    "WASAPIAudio_ISAVSetMasterVolume"
};

#define WASAPIAUDIO_NUM_FUNCS (sizeof(wasapi_native_func_names) / sizeof(wasapi_native_func_names[0]))

struct WASAPI_OLE32_func OLE32_func;
struct WASAPIAudio_native_func WASAPIAudio_func;

APTR HostLibBase = NULL;
static void *olehandle = NULL;
static void *wasapiaudiohandle = NULL;

static void *hostlib_load_so(const char *sofile, const char **names, int nfuncs,
        void **funcptr)
{
    void *handle;
    char *err;
    int i;

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        bug("[WASAPI:BRIDGE] failed to open '%s': %s\n", sofile, err);
        return NULL;
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if (err != NULL) {
            bug("[WASAPI:BRIDGE] failed to get symbol '%s' (%s)\n", names[i], err);
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    return handle;
}

int WASAPI_HostLib_Init()
{
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    {
        bug("[WASAPI:BRIDGE] failed to open hostlib.resource\n");
        return FALSE;
    }

    olehandle = hostlib_load_so("Ole32.dll", wasapi_ole32_func_names,
            WASAPI_OLE32_NUM_FUNCS, (void **)&OLE32_func);
    if (!olehandle)
    {
        return FALSE;
    }

    wasapiaudiohandle = hostlib_load_so("Libs\\Host\\wasapiaudio.dll", wasapi_native_func_names,
            WASAPIAUDIO_NUM_FUNCS, (void **)&WASAPIAudio_func);
    if (!wasapiaudiohandle)
    {
        return FALSE;
    }

    return TRUE;
}

int WASAPI_HostLib_Cleanup()
{
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if (wasapiaudiohandle != NULL)
        HostLib_Close(wasapiaudiohandle, NULL);
    if (olehandle != NULL)
        HostLib_Close(olehandle, NULL);
    return 0;
}
