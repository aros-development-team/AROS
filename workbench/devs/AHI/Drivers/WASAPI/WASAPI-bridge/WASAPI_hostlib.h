/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#ifndef WASAPI_HOSTLIB_H
#define WASAPI_HOSTLIB_H

#include <exec/types.h>

// Define some used windows types ...
typedef struct _GUID {
    ULONG  Data1;
    UWORD Data2;
    UWORD Data3;
    UBYTE  Data4[ 8 ];
} GUID;

typedef struct _PROPERTYKEY {
    GUID  fmtid;
    ULONG pid;
} PROPERTYKEY;

typedef GUID IID;
typedef GUID CLSID;

// Define used windows CLSID's ...
static const CLSID CLSID_MMDeviceEnumerator = {
    0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}
};
static CLSID *CLSID_MMDEVICEENUMERATOR = (CLSID *) &CLSID_MMDeviceEnumerator;

static const IID IID_IMMDeviceEnumerator = {
    0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}
};
static IID *IID_IMMDEVICEENUMERATOR = (IID *) &IID_IMMDeviceEnumerator;

static const IID IID_IAudioClient = {
    0x1cb9ad4c, 0xdbfa, 0x4c32, { 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2 }
};
static IID *IID_IAUDIOCLIENT   = (IID *) &IID_IAudioClient;

static const IID IID_IAudioRenderClient    = {
    0xf294acfc, 0x3146, 0x4483, {0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2}
};
static IID *IID_IAUDIORENDERCLIENT   = (IID *) &IID_IAudioRenderClient;

static const IID IID_ISimpleAudioVolume = {
    0x87ce5498, 0x68d6, 0x44e5, { 0x92, 0x15, 0x6d, 0xa4, 0x7e, 0xf8, 0x83, 0xd8 }
};
static IID *IID_ISIMPLEAUDIOVOLUME   = (IID *) &IID_ISimpleAudioVolume;

static const PROPERTYKEY PKEY_Device_FriendlyName = {
    { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, } }, 14
};
static PROPERTYKEY   *PKEY_DEVICE_FRIENDLYNAME   = (PROPERTYKEY *) &PKEY_Device_FriendlyName;

// Host API calls ..
struct WASAPI_OLE32_func {
    LONG(*CoInitialize)(void *pvR);
    void (*CoUninitialize)(void);
    LONG(*CoCreateInstance)(CLSID *rclsid, APTR pUnkOuter, ULONG dwClsContext, IID *riid, void *ppv);
};

struct WASAPIAudio_native_func {
    void (*WASAPIAudio_PropVariantInit)(APTR pv);
    void (*WASAPIAudio_PropVariantClear)(APTR pv);
    void (*WASAPIAudio_ReadPropValStrN)(APTR pv, char *strbuf, int len);
    void (*WASAPIAudio_InitWaveFormat)(APTR MixFmt, APTR AudioFmt);
    LONG(*WASAPIAudio_GetDefaultAudioEndpoint)(APTR dEnum,  UWORD eFlow, UWORD eRole, APTR ppEndpoint);
    LONG(*WASAPIAudio_IMMDActivate)(APTR device, APTR iid, ULONG dwClsCtx, APTR pActivationParams, APTR ppInterface);
    LONG(*WASAPIAudio_IMMDOpenPropertyStore)(APTR device, ULONG stgmAccess, APTR ppProperties);
    void (*WASAPIAudio_IMMDRelease)(APTR device);
    LONG(*WASAPIAudio_IACGetService)(APTR device, APTR riid, APTR ppv);
    LONG(*WASAPIAudio_IACGetMixFormat)(APTR client, APTR ppDeviceFormat);
    LONG(*WASAPIAudio_IACGetBufferSize)(APTR client, APTR pNumBufferFrames);
    LONG(*WASAPIAudio_IACGetCurrentPadding)(APTR client, APTR pNumPaddingFrames);
    LONG(*WASAPIAudio_IACGetDevicePeriod)(APTR client, APTR phnsDefaultDevicePeriod, APTR phnsMinimumDevicePeriod);
    LONG(*WASAPIAudio_IACInitialize)(APTR client, UWORD ShareMode, ULONG StreamFlags, UQUAD hnsBufferDuration,
                                     UQUAD hnsPeriodicity, APTR pFormat, APTR AudioSessionGuid);
    LONG(*WASAPIAudio_IACStart)(APTR client);
    LONG(*WASAPIAudio_IACStop)(APTR client);
    LONG(*WASAPIAudio_IPSGetValue)(APTR props, APTR key, APTR pv);
    void (*WASAPIAudio_IPSRelease)(APTR props);
    LONG(*WASAPIAudio_IARCGetBuffer)(APTR client, ULONG NumFramesRequested, APTR ppData);
    LONG(*WASAPIAudio_IARCReleaseBuffer)(APTR client, ULONG NumFramesWritten, ULONG dwFlags);
    LONG(*WASAPIAudio_ISAVGetMasterVolume)(APTR volume, float *pfLevel);
    LONG(*WASAPIAudio_ISAVSetMasterVolume)(APTR volume, float fLevel, APTR EventContext);
};

extern struct WASAPI_OLE32_func OLE32_func;
extern struct WASAPIAudio_native_func WASAPIAudio_func;

#define WASAPIOLECALL(func,...) (OLE32_func.func(__VA_ARGS__))
#define WASAPIAUDIOCALL(func,...) (WASAPIAudio_func.func(__VA_ARGS__))

int WASAPI_HostLib_Init();
int WASAPI_HostLib_Cleanup();

#endif
