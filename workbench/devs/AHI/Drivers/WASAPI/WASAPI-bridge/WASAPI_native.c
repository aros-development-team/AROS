/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.

    Desc: Host-side part of WASAPI audio driver.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <aros/irq.h>

#define COBJMACROS

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiosessiontypes.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

#include "WASAPI_common.h"

#define D(x)

#define 	CODEPAGE_ISO_8859_1  28591

#ifdef __x86_64__
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_GetDefaultAudioEndpoint(IMMDeviceEnumerator *dEnum,  EDataFlow eFlow, ERole eRole, IMMDevice **ppEndpoint)
{
    HRESULT retval;

    D(printf("[WASAPI:NATIVE] %s(0x%p, %d, %d, 0x%p)\n", __func__, dEnum, eFlow, eRole, ppEndpoint);)
    retval = IMMDeviceEnumerator_GetDefaultAudioEndpoint(dEnum, eFlow, eRole, ppEndpoint);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IMMDActivate(IMMDevice *device, REFIID iid, DWORD dwClsCtx, PROPVARIANT *pActivationParams, void **ppInterface)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p, %x, 0x%p, 0x%p)\n", __func__, device, iid, dwClsCtx, pActivationParams, ppInterface);)
    retval = IMMDevice_Activate(device, iid, dwClsCtx, pActivationParams, ppInterface);
    return retval;
}

volatile void __declspec(dllexport) __aros WASAPIAudio_IMMDRelease(IMMDevice *device)
{
    D(printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, device);)
    IMMDevice_Release(device);
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACGetService(IAudioClient *client, REFIID riid, void **ppv)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p, 0x%p)\n", __func__, client, riid, ppv);)
    retval = IAudioClient_GetService(client, riid, ppv);
    return retval;
}

volatile void __declspec(dllexport) __aros WASAPIAudio_DumpWaveFormat(WAVEFORMATEX *wFormat)
{
    D(
    printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, wFormat);
    printf("[WASAPI:NATIVE] %s: format %d\n", __func__, wFormat->wFormatTag);
    printf("[WASAPI:NATIVE] %s:     %d bits per sample\n", __func__, wFormat->wBitsPerSample);
    printf("[WASAPI:NATIVE] %s:     %d channels\n", __func__, wFormat->nChannels);
    printf("[WASAPI:NATIVE] %s:     %d samples per sec\n", __func__, wFormat->nSamplesPerSec);
    )
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACGetMixFormat(IAudioClient *client, WAVEFORMATEX **ppDeviceFormat)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p)\n", __func__, client, ppDeviceFormat);)
    retval = IAudioClient_GetMixFormat(client, ppDeviceFormat);
    if (retval >= 0)
        WASAPIAudio_DumpWaveFormat(*ppDeviceFormat);
    return retval;
}

volatile void __declspec(dllexport) __aros WASAPIAudio_InitWaveFormat(WAVEFORMATEX *MixFmt, WAVEFORMATEX *AudioFmt)
{
    AudioFmt->wFormatTag = WAVE_FORMAT_PCM; 
    AudioFmt->nChannels = 2; 
    AudioFmt->nSamplesPerSec = 44100L; 
    AudioFmt->nAvgBytesPerSec = 176400L; 
    AudioFmt->nBlockAlign = 4; 
    AudioFmt->wBitsPerSample = 16; 
    AudioFmt->cbSize = 0;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACGetBufferSize(IAudioClient *client, UINT32 *pNumBufferFrames)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p)\n", __func__, client, pNumBufferFrames);)
    retval = IAudioClient_GetBufferSize(client, pNumBufferFrames);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACGetCurrentPadding(IAudioClient *client, UINT32 *pNumPaddingFrames)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p)\n", __func__, client, pNumPaddingFrames);)
    retval = IAudioClient_GetCurrentPadding(client, pNumPaddingFrames);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACGetDevicePeriod(IAudioClient *client, REFERENCE_TIME *phnsDefaultDevicePeriod, REFERENCE_TIME *phnsMinimumDevicePeriod)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p, 0x%p)\n", __func__, client, phnsDefaultDevicePeriod, phnsMinimumDevicePeriod);)
    retval = IAudioClient_GetDevicePeriod(client, phnsDefaultDevicePeriod, phnsMinimumDevicePeriod);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACInitialize(IAudioClient *client, AUDCLNT_SHAREMODE ShareMode, DWORD StreamFlags, REFERENCE_TIME hnsBufferDuration, REFERENCE_TIME hnsPeriodicity, const WAVEFORMATEX *pFormat, LPCGUID AudioSessionGuid)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p)\n", __func__, client, pFormat);)
    retval = IAudioClient_Initialize(client, ShareMode, StreamFlags, hnsBufferDuration, hnsPeriodicity, pFormat, AudioSessionGuid);
        return retval;
}  

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACStart(IAudioClient *client)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, client);)
    retval = IAudioClient_Start(client);
    return retval;
}  

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IACStop(IAudioClient *client)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, client);)
    retval = IAudioClient_Stop(client);
    return retval;
}  

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IMMDOpenPropertyStore(IMMDevice *device, ULONG stgmAccess, IPropertyStore **ppProperties)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, %x, 0x%p)\n", __func__, device, stgmAccess, ppProperties);)
    retval = IMMDevice_OpenPropertyStore(device, stgmAccess, ppProperties);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IPSGetValue(IPropertyStore *props, REFPROPERTYKEY key, PROPVARIANT *pv)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p, 0x%p)\n", __func__, props, key, pv);)
    retval = IPropertyStore_GetValue(props, key, pv);
    return retval;
}

volatile void __declspec(dllexport) __aros WASAPIAudio_IPSRelease(IPropertyStore *props)
{
    D(printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, props);)
    IPropertyStore_Release(props);
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_ISAVGetMasterVolume(ISimpleAudioVolume *volume, float *pfLevel)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p)\n", __func__, volume, pfLevel);)
    retval = ISimpleAudioVolume_GetMasterVolume(volume, pfLevel);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_ISAVSetMasterVolume(ISimpleAudioVolume *volume, float fLevel, LPCGUID EventContext)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, %f, 0x%p)\n", __func__, volume, fLevel, EventContext);)
    retval = ISimpleAudioVolume_SetMasterVolume(volume, fLevel, EventContext);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IARCGetBuffer(IAudioRenderClient *client, UINT32 NumFramesRequested, BYTE   **ppData)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, %d 0x%p)\n", __func__, client, NumFramesRequested, ppData);)
    retval = IAudioRenderClient_GetBuffer(client, NumFramesRequested, ppData);
    return retval;
}

volatile HRESULT __declspec(dllexport) __aros WASAPIAudio_IARCReleaseBuffer(IAudioRenderClient *client, UINT32 NumFramesWritten, DWORD  dwFlags)
{
    HRESULT retval;
    D(printf("[WASAPI:NATIVE] %s(0x%p, %d, 0x%p)\n", __func__, client, NumFramesWritten, dwFlags);)
    retval = IAudioRenderClient_ReleaseBuffer(client, NumFramesWritten, dwFlags);
    return retval;
}


/* PROPVARIANT support */
volatile void __declspec(dllexport) __aros WASAPIAudio_PropVariantInit(PROPVARIANT *pv)
{
    D(printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, pv);)
    PropVariantInit(pv);
}

volatile void __declspec(dllexport) __aros WASAPIAudio_PropVariantClear(PROPVARIANT *pv)
{
    D(printf("[WASAPI:NATIVE] %s(0x%p)\n", __func__, pv);)
    PropVariantClear(pv);
}

volatile void __declspec(dllexport) __aros WASAPIAudio_ReadPropValStrN(PROPVARIANT *pv, char *strbuf, int len)
{
    D(printf("[WASAPI:NATIVE] %s(0x%p, 0x%p)\n", __func__, pv, strbuf);)

    strbuf[0] = (char)0;
    if (pv->vt == VT_LPWSTR)
    {
        // Unicode
        D(printf("[WASAPI:NATIVE] %s: pwszVal = 0x%p\n", __func__, pv->pwszVal);)
        //wprintf(L"'%s'\n", pv->pwszVal);

        WideCharToMultiByte(CODEPAGE_ISO_8859_1, 0, pv->pwszVal, -1, strbuf, len, NULL, NULL);
    }
    else if (pv->vt == VT_LPSTR)
    {
        //ANSI (in system default codepage)
        D(printf("[WASAPI:NATIVE] %s: pszVal = 0x%p\n", __func__, pv->pszVal);)
    }
}
