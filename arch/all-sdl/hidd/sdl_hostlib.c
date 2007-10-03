/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/semaphores.h>

#include <proto/exec.h>
#include <proto/hostlib.h>

#include LC_LIBDEFS_FILE

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

void *sdl_handle = NULL;

struct sdl_funcs sdl_funcs;

static const char *sdl_func_names[] = {
    "SDL_strlcpy",
    "SDL_strlcat",
    "SDL_strrev",
    "SDL_strupr",
    "SDL_strlwr",
    "SDL_ltoa",
    "SDL_ultoa",
    "SDL_lltoa",
    "SDL_ulltoa",
    "SDL_iconv",
    "SDL_iconv_string",
    "SDL_SetError",
    "SDL_GetError",
    "SDL_ClearError",
    "SDL_Error",
    "SDL_CreateMutex",
    "SDL_mutexP",
    "SDL_mutexV",
    "SDL_DestroyMutex",
    "SDL_CreateSemaphore",
    "SDL_DestroySemaphore",
    "SDL_SemWait",
    "SDL_SemTryWait",
    "SDL_SemWaitTimeout",
    "SDL_SemPost",
    "SDL_SemValue",
    "SDL_CreateCond",
    "SDL_DestroyCond",
    "SDL_CondSignal",
    "SDL_CondBroadcast",
    "SDL_CondWait",
    "SDL_CondWaitTimeout",
    "SDL_CreateThread",
    "SDL_ThreadID",
    "SDL_GetThreadID",
    "SDL_WaitThread",
    "SDL_KillThread",
    "SDL_RWFromFile",
    "SDL_RWFromFP",
    "SDL_RWFromMem",
    "SDL_RWFromConstMem",
    "SDL_AllocRW",
    "SDL_FreeRW",
    "SDL_ReadLE16",
    "SDL_ReadBE16",
    "SDL_ReadLE32",
    "SDL_ReadBE32",
    "SDL_ReadLE64",
    "SDL_ReadBE64",
    "SDL_WriteLE16",
    "SDL_WriteBE16",
    "SDL_WriteLE32",
    "SDL_WriteBE32",
    "SDL_WriteLE64",
    "SDL_WriteBE64",
    "SDL_AudioInit",
    "SDL_AudioQuit",
    "SDL_AudioDriverName",
    "SDL_OpenAudio",
    "SDL_GetAudioStatus",
    "SDL_PauseAudio",
    "SDL_LoadWAV_RW",
    "SDL_FreeWAV",
    "SDL_BuildAudioCVT",
    "SDL_ConvertAudio",
    "SDL_MixAudio",
    "SDL_LockAudio",
    "SDL_UnlockAudio",
    "SDL_CloseAudio",
    "SDL_CDNumDrives",
    "SDL_CDName",
    "SDL_CDOpen",
    "SDL_CDStatus",
    "SDL_CDPlayTracks",
    "SDL_CDPlay",
    "SDL_CDPause",
    "SDL_CDResume",
    "SDL_CDStop",
    "SDL_CDEject",
    "SDL_CDClose",
    "SDL_HasRDTSC",
    "SDL_HasMMX",
    "SDL_HasMMXExt",
    "SDL_Has3DNow",
    "SDL_Has3DNowExt",
    "SDL_HasSSE",
    "SDL_HasSSE2",
    "SDL_HasAltiVec",
    "SDL_GetAppState",
    "SDL_EnableUNICODE",
    "SDL_EnableKeyRepeat",
    "SDL_GetKeyRepeat",
    "SDL_GetKeyState",
    "SDL_GetModState",
    "SDL_SetModState",
    "SDL_GetKeyName",
    "SDL_VideoInit",
    "SDL_VideoQuit",
    "SDL_VideoDriverName",
    "SDL_GetVideoSurface",
    "SDL_GetVideoInfo",
    "SDL_VideoModeOK",
    "SDL_ListModes",
    "SDL_SetVideoMode",
    "SDL_UpdateRects",
    "SDL_UpdateRect",
    "SDL_Flip",
    "SDL_SetGamma",
    "SDL_SetGammaRamp",
    "SDL_GetGammaRamp",
    "SDL_SetColors",
    "SDL_SetPalette",
    "SDL_MapRGB",
    "SDL_MapRGBA",
    "SDL_GetRGB",
    "SDL_GetRGBA",
    "SDL_CreateRGBSurface",
    "SDL_CreateRGBSurfaceFrom",
    "SDL_FreeSurface",
    "SDL_LockSurface",
    "SDL_UnlockSurface",
    "SDL_LoadBMP_RW",
    "SDL_SaveBMP_RW",
    "SDL_SetColorKey",
    "SDL_SetAlpha",
    "SDL_SetClipRect",
    "SDL_GetClipRect",
    "SDL_ConvertSurface",
    "SDL_UpperBlit",
    "SDL_LowerBlit",
    "SDL_FillRect",
    "SDL_DisplayFormat",
    "SDL_DisplayFormatAlpha",
    "SDL_CreateYUVOverlay",
    "SDL_LockYUVOverlay",
    "SDL_UnlockYUVOverlay",
    "SDL_DisplayYUVOverlay",
    "SDL_FreeYUVOverlay",
    "SDL_GL_LoadLibrary",
    "SDL_GL_GetProcAddress",
    "SDL_GL_SetAttribute",
    "SDL_GL_GetAttribute",
    "SDL_GL_SwapBuffers",
    "SDL_GL_UpdateRects",
    "SDL_GL_Lock",
    "SDL_GL_Unlock",
    "SDL_WM_SetCaption",
    "SDL_WM_GetCaption",
    "SDL_WM_SetIcon",
    "SDL_WM_IconifyWindow",
    "SDL_WM_ToggleFullScreen",
    "SDL_WM_GrabInput",
    "SDL_SoftStretch",
    "SDL_GetMouseState",
    "SDL_GetRelativeMouseState",
    "SDL_WarpMouse",
    "SDL_CreateCursor",
    "SDL_SetCursor",
    "SDL_GetCursor",
    "SDL_FreeCursor",
    "SDL_ShowCursor",
    "SDL_NumJoysticks",
    "SDL_JoystickName",
    "SDL_JoystickOpen",
    "SDL_JoystickOpened",
    "SDL_JoystickIndex",
    "SDL_JoystickNumAxes",
    "SDL_JoystickNumBalls",
    "SDL_JoystickNumHats",
    "SDL_JoystickNumButtons",
    "SDL_JoystickUpdate",
    "SDL_JoystickEventState",
    "SDL_JoystickGetAxis",
    "SDL_JoystickGetHat",
    "SDL_JoystickGetBall",
    "SDL_JoystickGetButton",
    "SDL_JoystickClose",
    "SDL_PumpEvents",
    "SDL_PeepEvents",
    "SDL_PollEvent",
    "SDL_WaitEvent",
    "SDL_PushEvent",
    "SDL_SetEventFilter",
    "SDL_GetEventFilter",
    "SDL_EventState",
    "SDL_LoadObject",
    "SDL_LoadFunction",
    "SDL_UnloadObject",
    "SDL_GetTicks",
    "SDL_Delay",
    "SDL_SetTimer",
    "SDL_AddTimer",
    "SDL_RemoveTimer",
    "SDL_Linked_Version",
    "SDL_Init",
    "SDL_InitSubSystem",
    "SDL_QuitSubSystem",
    "SDL_WasInit",
    "SDL_Quit",
};

#define SDL_NUM_FUNCS (199)

APTR HostLibBase;

struct SignalSemaphore sdl_lock;

static void *sdl_hostlib_load_so(const char *sofile, const char **names, int nfuncs, void **funcptr) {
    void *handle;
    char *err;
    int i;

    D(bug("[sdl] loading %d functions from %s\n", nfuncs, sofile));

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        kprintf("[sdl] couldn't open '%s': %s\n", sofile, err);
        return NULL;
    }

    for (i = 0; i < nfuncs; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if (err != NULL) {
            kprintf("[sdl] couldn't get symbol '%s' from '%s': %s\n");
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    D(bug("[sdl] done\n"));

    return handle;
}

static int sdl_hostlib_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] hostlib init\n"));

    InitSemaphore(&sdl_lock);

    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL) {
        kprintf("[sdl] couldn't open hostlib.resource\n");
        return FALSE;
    }

    if ((sdl_handle = sdl_hostlib_load_so(SDL_SOFILE, sdl_func_names, SDL_NUM_FUNCS, (void **) &sdl_funcs)) == NULL)
        return FALSE;

    return TRUE;
}

static int sdl_hostlib_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] hostlib expunge\n"));

    if (sdl_handle != NULL)
        HostLib_Close(sdl_handle, NULL);

    return TRUE;
}

ADD2INITLIB(sdl_hostlib_init, 0)
ADD2EXPUNGELIB(sdl_hostlib_expunge, 0)
