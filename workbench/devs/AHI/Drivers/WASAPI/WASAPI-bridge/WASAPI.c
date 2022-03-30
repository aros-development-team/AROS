/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include "WASAPI_hostlib.h"
#include "WASAPI.h"

#include "WASAPI_common.h"

#define CLSCTX_INPROC_SERVER                    0x1
#define CLSCTX_INPROC_HANDLER                   0x2
#define CLSCTX_LOCAL_SERVER                     0x4
#define CLSCTX_REMOTE_SERVER                    0x10
  
#define CLSCTX_ALL                              CLSCTX_INPROC_SERVER
//#define CLSCTX_ALL                            (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER)

#define eRender                                 0
#define eCapture                                1

#define eConsole                                0
#define eMultimedia                             1
#define eCommunications                         2

#define AUDCLNT_SHAREMODE_SHARED                0
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM      0x80000000
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000

#define STGM_READ                               0

#define PROPVARIANT_size                        16
#define WAVEFORMATEXTENSIBLE_size               40

// Enumerator..
APTR device_enumerator = NULL;
// Playback..
char *playback_namestr = NULL;
APTR playback_device = NULL;
APTR playback_props = NULL;
APTR playback_client = NULL;
APTR playback_render = NULL;
APTR playback_volume = NULL;
ULONG playback_framecount;

// Recording..
char *record_namestr = NULL;
APTR record_device = NULL;
APTR record_props = NULL;
APTR record_client = NULL;
ULONG record_framecount;
// Misc
APTR var = NULL;

static ULONG WASAPI_COMInit(void)
{
    ULONG retval = 0;
    LONG hr = 0;
    
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    hr = WASAPIOLECALL(CoInitialize, NULL);
    if (hr < 0)
    {
        bug("[WASAPI:BRIDGE] %s: COM initialization failed (result = %d)\n", __func__, hr);
        return retval;
    }

    hr = WASAPIOLECALL(CoCreateInstance, CLSID_MMDEVICEENUMERATOR, NULL,
            CLSCTX_ALL, IID_IMMDEVICEENUMERATOR, (void**)&device_enumerator);
    if (hr >= 0)
    {
        D(bug("[WASAPI:BRIDGE] %s: device enumerator @ 0x%p\n", __func__, device_enumerator);)

        hr = WASAPIAUDIOCALL(WASAPIAudio_GetDefaultAudioEndpoint, device_enumerator, eRender, eConsole, &playback_device);
        if (hr >= 0)
        {
            D(bug("[WASAPI:BRIDGE] %s: playback device @ 0x%p\n", __func__, playback_device);)
            retval = 1;
            if ((var = AllocVec(PROPVARIANT_size, MEMF_CLEAR)) != NULL)
            {
                D(bug("[WASAPI:BRIDGE] %s: var storage @ 0x%p\n", __func__, var);)
                WASAPIAUDIOCALL(WASAPIAudio_PropVariantInit, var);

                hr = WASAPIAUDIOCALL(WASAPIAudio_IMMDOpenPropertyStore, playback_device, STGM_READ, &playback_props);
                if (hr >= 0)
                {
                    D(bug("[WASAPI:BRIDGE] %s:           PropStore @ 0x%p\n", __func__, playback_props);)
                    if ((playback_namestr = AllocVec(DEVNAME_size, MEMF_CLEAR)) != NULL)
                    {
                        D(bug("[WASAPI:BRIDGE] %s: namestr @ 0x%p\n", __func__, var, playback_namestr);)

                        hr  = WASAPIAUDIOCALL(WASAPIAudio_IPSGetValue, playback_props, PKEY_DEVICE_FRIENDLYNAME, var);
                        if (hr >= 0)
                        {
                            D(bug("[WASAPI:BRIDGE] %s: Converting Friendly Name.. \n", __func__);)
                            WASAPIAUDIOCALL(WASAPIAudio_ReadPropValStrN, var, playback_namestr, DEVNAME_size);
                            D(bug("[WASAPI:BRIDGE] %s: Friendly Name = '%s'\n", __func__, playback_namestr);)
                        }
                        WASAPIAUDIOCALL(WASAPIAudio_PropVariantClear, var);
                    }
#if (0)
                    hr = WASAPIAUDIOCALL(WASAPIAudio_IPSGetValue, playback_props, PKEY_AUDIOENGINE_DEVICEFORMAT, var);
                    if (hr >= 0)
                    {
                        
                    }
                    WASAPIAUDIOCALL(WASAPIAudio_PropVariantClear, var);
#endif
                    WASAPIAUDIOCALL(WASAPIAudio_IPSRelease, playback_props);
                }
                FreeVec(var);
                var = NULL;
            }

            hr = WASAPIAUDIOCALL(WASAPIAudio_GetDefaultAudioEndpoint, device_enumerator, eCapture, eConsole, &record_device);
            if (hr >= 0)
            {
                D(bug("[WASAPI:BRIDGE] %s: recording device @ 0x%p\n", __func__, record_device);)
                record_framecount = 0;
                if ((var = AllocVec(PROPVARIANT_size, MEMF_CLEAR)) != NULL)
                {
                    D(bug("[WASAPI:BRIDGE] %s: var storage @ 0x%p\n", __func__, var);)
                    WASAPIAUDIOCALL(WASAPIAudio_PropVariantInit, var);

                    hr = WASAPIAUDIOCALL(WASAPIAudio_IMMDOpenPropertyStore, record_device, STGM_READ, &record_props);
                    if (hr >= 0)
                    {
                        D(bug("[WASAPI:BRIDGE] %s:           PropStore @ 0x%p\n", __func__, record_props);)
                        if ((record_namestr = AllocVec(DEVNAME_size, MEMF_CLEAR)) != NULL)
                        {
                            D(bug("[WASAPI:BRIDGE] %s: namestr @ 0x%p\n", __func__, var, record_namestr);)

                            hr  = WASAPIAUDIOCALL(WASAPIAudio_IPSGetValue, record_props, PKEY_DEVICE_FRIENDLYNAME, var);
                            if (hr >= 0)
                            {
                                D(bug("[WASAPI:BRIDGE] %s: Converting Friendly Name.. \n", __func__);)
                                WASAPIAUDIOCALL(WASAPIAudio_ReadPropValStrN, var, record_namestr, DEVNAME_size);
                                D(bug("[WASAPI:BRIDGE] %s: Friendly Name = '%s'\n", __func__, record_namestr);)
                            }
                            WASAPIAUDIOCALL(WASAPIAudio_PropVariantClear, var);
                        }
                        WASAPIAUDIOCALL(WASAPIAudio_IPSRelease, record_props);
                    }
                    FreeVec(var);
                    var = NULL;
                }
            }

        }
        else
        {
            bug("[WASAPI:BRIDGE] %s: failed to obtain audio endpoint (result = %d)\n", __func__, hr);
            WASAPIOLECALL(CoUninitialize);
        }
    }
    else
    {
        bug("[WASAPI:BRIDGE] %s: failed to create ole32 device enumerator (result = %d)\n", __func__, hr);
        WASAPIOLECALL(CoUninitialize);
    }

    bug("[WASAPI:BRIDGE] %s: COM components ready\n", __func__);
    return retval;
}

#define STACK_SIZE 100000
BOOL WASAPI_Init()
{
    BOOL retval;

    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if ((retval = WASAPI_HostLib_Init()))
    {
        struct StackSwapStruct sss;
        struct StackSwapArgs ssa;
        UBYTE *stack;

        D(bug("[WASAPI:BRIDGE] %s: HostLib interfaces initialized\n", __func__);)

        stack = AllocMem(STACK_SIZE, MEMF_ANY);
        if (stack == NULL)
            return FALSE;

        sss.stk_Lower = stack;
        sss.stk_Upper = stack + STACK_SIZE;
        sss.stk_Pointer = sss.stk_Upper;
        ssa.Args[0] = 0;

        retval = (BOOL)NewStackSwap(&sss, WASAPI_COMInit, &ssa);
    }

    D(bug("[WASAPI:BRIDGE] %s: Init complete\n", __func__);)

    return retval;
}

VOID WASAPI_Cleanup()
{
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if (playback_device)
    {
        WASAPIAUDIOCALL(WASAPIAudio_IMMDRelease, playback_device);
    }

    WASAPI_HostLib_Cleanup();
}

VOID WASAPI_MixerInit(APTR * handle, APTR * volctrl, LONG * min, LONG * max)
{
    UQUAD duration;
    APTR    audiofmt, mixfmt;
    LONG    hr;

    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    hr = WASAPIAUDIOCALL(WASAPIAudio_IMMDActivate, playback_device, IID_IAUDIOCLIENT, CLSCTX_ALL, NULL, &playback_client);
    if (hr < 0)
    {
        bug("[WASAPI:BRIDGE] %s: failed to activate audio playback endpoint (result = %d)\n", __func__, hr);
        return;
    }

    *handle = playback_client;
    D(bug("[WASAPI:BRIDGE] %s: Playback IAudioClient @ 0x%p\n", __func__, playback_client);)

    if (record_device)
    {
        hr = WASAPIAUDIOCALL(WASAPIAudio_IMMDActivate, record_device, IID_IAUDIOCLIENT, CLSCTX_ALL, NULL, &record_client);
        if (hr < 0)
        {
            bug("[WASAPI:BRIDGE] %s: failed to activate audio record endpoint (result = %d)\n", __func__, hr);
        }
        else
        {
            D(bug("[WASAPI:BRIDGE] %s: Record IAudioClient @ 0x%p\n", __func__, record_client);)
        }
    }

    hr = WASAPIAUDIOCALL(WASAPIAudio_IACGetMixFormat, playback_client, &mixfmt);
    if ((audiofmt = AllocVec(WAVEFORMATEXTENSIBLE_size, MEMF_CLEAR)) != NULL)
    {
        D(bug("[WASAPI:BRIDGE] %s: WAVEFORMATEXTENSIBLE @ 0x%p\n", __func__, audiofmt);)
        WASAPIAUDIOCALL(WASAPIAudio_InitWaveFormat, mixfmt, audiofmt);
    }
    else
        audiofmt = mixfmt;

    hr = WASAPIAUDIOCALL(WASAPIAudio_IACGetDevicePeriod, playback_client, NULL, &duration);
    if (hr >= 0)
    {
        hr = WASAPIAUDIOCALL(WASAPIAudio_IACInitialize, playback_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY, duration, 0, audiofmt, NULL);
        if (hr >= 0)
        {
            D(bug("[WASAPI:BRIDGE] %s: IAudioClient Initialized\n", __func__);)
            hr = WASAPIAUDIOCALL(WASAPIAudio_IACGetService, playback_client, IID_ISIMPLEAUDIOVOLUME, &playback_volume);
            if (hr >= 0)
            {
                D(bug("[WASAPI:BRIDGE] %s: ISimpleAudioVolume @ 0x%p\n", __func__, playback_volume);)
                *volctrl = playback_volume;
            }
            else
            {
                bug("[WASAPI:BRIDGE] %s: failed to obtain ISimpleAudioVolume (result = %d)\n", __func__, hr);
            }
        }
        else
        {
            bug("[WASAPI:BRIDGE] %s: failed to initialize playback IAudioClient (result = %d)\n", __func__, hr);
        }
    }
    else
    {
        bug("[WASAPI:BRIDGE] %s: failed to determine minimum device period (result = %d)\n", __func__, hr);
    }

    D(bug("[WASAPI:BRIDGE] %s: Mixer Initialised\n", __func__);)
}

VOID WASAPI_MixerCleanup(APTR handle)
{
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    // TODO : Cleanup
}

LONG WASAPI_MixerGetVolume(APTR volctrl)
{
    LONG _ret = 0;
    LONG hr;
    float volume;

    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if (volctrl)
    {
        hr = WASAPIAUDIOCALL(WASAPIAudio_ISAVGetMasterVolume, volctrl, &volume);
        if (hr >= 0)
        {
            // TODO : Convert returned volume to something AHI can use.
        }
    }

    return (LONG)_ret;
}

VOID WASAPI_MixerSetVolume(APTR volctrl, LONG volume)
{
    LONG hr;
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if (volctrl)
    {
        // TODO : Convert AHI volume to something WASAPI can use.
        hr = WASAPIAUDIOCALL(WASAPIAudio_ISAVSetMasterVolume, volctrl, volume, NULL);
        if (hr >= 0)
        {
        }
    }
}

APTR WASAPI_Open()
{
    APTR handle = NULL;
    LONG hr;

    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if (playback_client)
    {
        WASAPIAUDIOCALL(WASAPIAudio_IACGetBufferSize, playback_client, &playback_framecount);
        D(bug("[WASAPI:BRIDGE] %s: playback buffer size = %d frames\n", __func__, playback_framecount);)

        hr = WASAPIAUDIOCALL(WASAPIAudio_IACGetService, playback_client, IID_IAUDIORENDERCLIENT, &playback_render);
        if (hr >= 0)
        {
            D(bug("[WASAPI:BRIDGE] %s: IAudioRenderClient @ 0x%p\n", __func__, playback_render);)
            handle = playback_render;
        }
        else
        {
            bug("[WASAPI:BRIDGE] %s: failed to obtain IAudioRenderClient (result = %d)\n", __func__, hr);
        }
    }

    return handle;
}

VOID WASAPI_DropAndClose(APTR handle)
{
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    if (handle)
    {
        // TODO : Cleanup
    }
}

IPTR WASAPI_Start(APTR handle)
{
    LONG hr;
    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    hr = WASAPIAUDIOCALL(WASAPIAudio_IACStart, handle);
    if (hr >= 0)
        return 1;
    return 0;
}

IPTR WASAPI_Stop(APTR handle)
{
    LONG hr;

    D(bug("[WASAPI:BRIDGE] %s()\n", __func__);)

    hr = WASAPIAUDIOCALL(WASAPIAudio_IACStop, handle);
    if (hr >= 0)
        return 1;
    return 0;
}

BOOL WASAPI_SetHWParams(APTR handle, ULONG * rate)
{
    D(bug("[WASAPI:BRIDGE] %s(0x%p, %d)\n", __func__, handle, *rate);)

    LONG dir = 0; int r = 0;

    // TODO : Configure playback using AHI defined params.

    return (r >= 0);
}

LONG WASAPI_Write(APTR handle, APTR buffer, ULONG size)
{
    LONG rc, hr;
    APTR data;

    D(bug("[WASAPI:BRIDGE] %s(0x%p, 0x%p, %d)\n", __func__, handle, buffer, size);)

    hr = WASAPIAUDIOCALL(WASAPIAudio_IARCGetBuffer, handle, size, &data);
    if (hr >= 0)
    {
        D(bug("[WASAPI:BRIDGE] %s: IARC Buffer @ 0x%p\n", __func__, data);)
        CopyMem(buffer, data, (size << 2));

        hr = WASAPIAUDIOCALL(WASAPIAudio_IARCReleaseBuffer, handle, size, 0);
        if (hr < 0)
        {
            bug("[WASAPI:BRIDGE] %s: ReleaseBuffer returned error %d\n", __func__, hr);
        }
        rc = size;
    }
    else
    {
        bug("[WASAPI:BRIDGE] %s: Failed to obtain IARC Buffer\n", __func__);
        rc = WASAPI_XRUN;
    }

    return rc;
}

VOID WASAPI_Prepare(APTR handle)
{
    D(bug("[WASAPI:BRIDGE] %s(0x%p)\n", __func__, handle);)

    // TODO:
}

LONG WASAPI_Avail(APTR handle)
{
    ULONG frames_used;
    LONG rc, hr;

    D(bug("[WASAPI:BRIDGE] %s(0x%p)\n", __func__, handle);)

    hr = WASAPIAUDIOCALL(WASAPIAudio_IACGetCurrentPadding, playback_client, &frames_used);
    if (hr >= 0)
    {
        rc = playback_framecount - frames_used;
        D(bug("[WASAPI:BRIDGE] %s: %d available\n", __func__, rc);)
    }
    else
        rc = WASAPI_XRUN;

    return rc;
}
