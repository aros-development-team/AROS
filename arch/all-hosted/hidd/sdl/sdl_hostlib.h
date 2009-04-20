/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2007-2009 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#define timeval sys_timeval
#include "SDL_platform.h"
#include "SDL_config.h"
#undef HAVE_CTYPE_H
#undef HAVE_ICONV_H
#undef HAVE_ICONV
#include "SDL.h"
#undef timeval

#include <exec/semaphores.h>

struct sdl_funcs {
    void (*SDL_SetError) (const char *fmt, ...);
    char * (*SDL_GetError) (void);
    void (*SDL_ClearError) (void);
    void (*SDL_Error) (SDL_errorcode code);
    SDL_mutex * (*SDL_CreateMutex) (void);
    int (*SDL_mutexP) (SDL_mutex *mutex);
    int (*SDL_mutexV) (SDL_mutex *mutex);
    void (*SDL_DestroyMutex) (SDL_mutex *mutex);
    SDL_sem * (*SDL_CreateSemaphore) (Uint32 initial_value);
    void (*SDL_DestroySemaphore) (SDL_sem *sem);
    int (*SDL_SemWait) (SDL_sem *sem);
    int (*SDL_SemTryWait) (SDL_sem *sem);
    int (*SDL_SemWaitTimeout) (SDL_sem *sem, Uint32 ms);
    int (*SDL_SemPost) (SDL_sem *sem);
    Uint32 (*SDL_SemValue) (SDL_sem *sem);
    SDL_cond * (*SDL_CreateCond) (void);
    void (*SDL_DestroyCond) (SDL_cond *cond);
    int (*SDL_CondSignal) (SDL_cond *cond);
    int (*SDL_CondBroadcast) (SDL_cond *cond);
    int (*SDL_CondWait) (SDL_cond *cond, SDL_mutex *mut);
    int (*SDL_CondWaitTimeout) (SDL_cond *cond, SDL_mutex *mutex, Uint32 ms);
    SDL_Thread * (*SDL_CreateThread) (int ( *fn)(void *), void *data);
    Uint32 (*SDL_ThreadID) (void);
    Uint32 (*SDL_GetThreadID) (SDL_Thread *thread);
    void (*SDL_WaitThread) (SDL_Thread *thread, int *status);
    void (*SDL_KillThread) (SDL_Thread *thread);
    SDL_RWops * (*SDL_RWFromFile) (const char *file, const char *mode);
    SDL_RWops * (*SDL_RWFromFP) (FILE *fp, int autoclose);
    SDL_RWops * (*SDL_RWFromMem) (void *mem, int size);
    SDL_RWops * (*SDL_RWFromConstMem) (const void *mem, int size);
    SDL_RWops * (*SDL_AllocRW) (void);
    void (*SDL_FreeRW) (SDL_RWops *area);
    Uint16 (*SDL_ReadLE16) (SDL_RWops *src);
    Uint16 (*SDL_ReadBE16) (SDL_RWops *src);
    Uint32 (*SDL_ReadLE32) (SDL_RWops *src);
    Uint32 (*SDL_ReadBE32) (SDL_RWops *src);
    Uint64 (*SDL_ReadLE64) (SDL_RWops *src);
    Uint64 (*SDL_ReadBE64) (SDL_RWops *src);
    int (*SDL_WriteLE16) (SDL_RWops *dst, Uint16 value);
    int (*SDL_WriteBE16) (SDL_RWops *dst, Uint16 value);
    int (*SDL_WriteLE32) (SDL_RWops *dst, Uint32 value);
    int (*SDL_WriteBE32) (SDL_RWops *dst, Uint32 value);
    int (*SDL_WriteLE64) (SDL_RWops *dst, Uint64 value);
    int (*SDL_WriteBE64) (SDL_RWops *dst, Uint64 value);
    int (*SDL_AudioInit) (const char *driver_name);
    void (*SDL_AudioQuit) (void);
    char * (*SDL_AudioDriverName) (char *namebuf, int maxlen);
    int (*SDL_OpenAudio) (SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
    SDL_audiostatus (*SDL_GetAudioStatus) (void);
    void (*SDL_PauseAudio) (int pause_on);
    SDL_AudioSpec * (*SDL_LoadWAV_RW) (SDL_RWops *src, int freesrc, SDL_AudioSpec *spec, Uint8 **audio_buf, Uint32 *audio_len);
    void (*SDL_FreeWAV) (Uint8 *audio_buf);
    int (*SDL_BuildAudioCVT) (SDL_AudioCVT *cvt, Uint16 src_format, Uint8 src_channels, int src_rate, Uint16 dst_format, Uint8 dst_channels, int dst_rate);
    int (*SDL_ConvertAudio) (SDL_AudioCVT *cvt);
    void (*SDL_MixAudio) (Uint8 *dst, const Uint8 *src, Uint32 len, int volume);
    void (*SDL_LockAudio) (void);
    void (*SDL_UnlockAudio) (void);
    void (*SDL_CloseAudio) (void);
    int (*SDL_CDNumDrives) (void);
    const char * (*SDL_CDName) (int drive);
    SDL_CD * (*SDL_CDOpen) (int drive);
    CDstatus (*SDL_CDStatus) (SDL_CD *cdrom);
    int (*SDL_CDPlayTracks) (SDL_CD *cdrom, int start_track, int start_frame, int ntracks, int nframes);
    int (*SDL_CDPlay) (SDL_CD *cdrom, int start, int length);
    int (*SDL_CDPause) (SDL_CD *cdrom);
    int (*SDL_CDResume) (SDL_CD *cdrom);
    int (*SDL_CDStop) (SDL_CD *cdrom);
    int (*SDL_CDEject) (SDL_CD *cdrom);
    void (*SDL_CDClose) (SDL_CD *cdrom);
    SDL_bool (*SDL_HasRDTSC) (void);
    SDL_bool (*SDL_HasMMX) (void);
    SDL_bool (*SDL_HasMMXExt) (void);
    SDL_bool (*SDL_Has3DNow) (void);
    SDL_bool (*SDL_Has3DNowExt) (void);
    SDL_bool (*SDL_HasSSE) (void);
    SDL_bool (*SDL_HasSSE2) (void);
    SDL_bool (*SDL_HasAltiVec) (void);
    Uint8 (*SDL_GetAppState) (void);
    int (*SDL_EnableUNICODE) (int enable);
    int (*SDL_EnableKeyRepeat) (int delay, int interval);
    void (*SDL_GetKeyRepeat) (int *delay, int *interval);
    Uint8 * (*SDL_GetKeyState) (int *numkeys);
    SDLMod (*SDL_GetModState) (void);
    void (*SDL_SetModState) (SDLMod modstate);
    char * (*SDL_GetKeyName) (SDLKey key);
    int (*SDL_VideoInit) (const char *driver_name, Uint32 flags);
    void (*SDL_VideoQuit) (void);
    char * (*SDL_VideoDriverName) (char *namebuf, int maxlen);
    SDL_Surface * (*SDL_GetVideoSurface) (void);
    const SDL_VideoInfo * (*SDL_GetVideoInfo) (void);
    int (*SDL_VideoModeOK) (int width, int height, int bpp, Uint32 flags);
    SDL_Rect ** (*SDL_ListModes) (SDL_PixelFormat *format, Uint32 flags);
    SDL_Surface * (*SDL_SetVideoMode) (int width, int height, int bpp, Uint32 flags);
    void (*SDL_UpdateRects) (SDL_Surface *screen, int numrects, SDL_Rect *rects);
    void (*SDL_UpdateRect) (SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
    int (*SDL_Flip) (SDL_Surface *screen);
    int (*SDL_SetGamma) (float red, float green, float blue);
    int (*SDL_SetGammaRamp) (const Uint16 *red, const Uint16 *green, const Uint16 *blue);
    int (*SDL_GetGammaRamp) (Uint16 *red, Uint16 *green, Uint16 *blue);
    int (*SDL_SetColors) (SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);
    int (*SDL_SetPalette) (SDL_Surface *surface, int flags, SDL_Color *colors, int firstcolor, int ncolors);
    Uint32 (*SDL_MapRGB) (SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b);
    Uint32 (*SDL_MapRGBA) (SDL_PixelFormat *format, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void (*SDL_GetRGB) (Uint32 pixel, SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b);
    void (*SDL_GetRGBA) (Uint32 pixel, SDL_PixelFormat *fmt, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a);
    SDL_Surface * (*SDL_CreateRGBSurface) (Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
    SDL_Surface * (*SDL_CreateRGBSurfaceFrom) (void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
    void (*SDL_FreeSurface) (SDL_Surface *surface);
    int (*SDL_LockSurface) (SDL_Surface *surface);
    void (*SDL_UnlockSurface) (SDL_Surface *surface);
    SDL_Surface * (*SDL_LoadBMP_RW) (SDL_RWops *src, int freesrc);
    int (*SDL_SaveBMP_RW) (SDL_Surface *surface, SDL_RWops *dst, int freedst);
    int (*SDL_SetColorKey) (SDL_Surface *surface, Uint32 flag, Uint32 key);
    int (*SDL_SetAlpha) (SDL_Surface *surface, Uint32 flag, Uint8 alpha);
    SDL_bool (*SDL_SetClipRect) (SDL_Surface *surface, const SDL_Rect *rect);
    void (*SDL_GetClipRect) (SDL_Surface *surface, SDL_Rect *rect);
    SDL_Surface * (*SDL_ConvertSurface) (SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags);
    int (*SDL_UpperBlit) (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
    int (*SDL_LowerBlit) (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
    int (*SDL_FillRect) (SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
    SDL_Surface * (*SDL_DisplayFormat) (SDL_Surface *surface);
    SDL_Surface * (*SDL_DisplayFormatAlpha) (SDL_Surface *surface);
    SDL_Overlay * (*SDL_CreateYUVOverlay) (int width, int height, Uint32 format, SDL_Surface *display);
    int (*SDL_LockYUVOverlay) (SDL_Overlay *overlay);
    void (*SDL_UnlockYUVOverlay) (SDL_Overlay *overlay);
    int (*SDL_DisplayYUVOverlay) (SDL_Overlay *overlay, SDL_Rect *dstrect);
    void (*SDL_FreeYUVOverlay) (SDL_Overlay *overlay);
    int (*SDL_GL_LoadLibrary) (const char *path);
    void * (*SDL_GL_GetProcAddress) (const char* proc);
    int (*SDL_GL_SetAttribute) (SDL_GLattr attr, int value);
    int (*SDL_GL_GetAttribute) (SDL_GLattr attr, int* value);
    void (*SDL_GL_SwapBuffers) (void);
    void (*SDL_GL_UpdateRects) (int numrects, SDL_Rect* rects);
    void (*SDL_GL_Lock) (void);
    void (*SDL_GL_Unlock) (void);
    void (*SDL_WM_SetCaption) (const char *title, const char *icon);
    void (*SDL_WM_GetCaption) (char **title, char **icon);
    void (*SDL_WM_SetIcon) (SDL_Surface *icon, Uint8 *mask);
    int (*SDL_WM_IconifyWindow) (void);
    int (*SDL_WM_ToggleFullScreen) (SDL_Surface *surface);
    SDL_GrabMode (*SDL_WM_GrabInput) (SDL_GrabMode mode);
    int (*SDL_SoftStretch) (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
    Uint8 (*SDL_GetMouseState) (int *x, int *y);
    Uint8 (*SDL_GetRelativeMouseState) (int *x, int *y);
    void (*SDL_WarpMouse) (Uint16 x, Uint16 y);
    SDL_Cursor * (*SDL_CreateCursor) (Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
    void (*SDL_SetCursor) (SDL_Cursor *cursor);
    SDL_Cursor * (*SDL_GetCursor) (void);
    void (*SDL_FreeCursor) (SDL_Cursor *cursor);
    int (*SDL_ShowCursor) (int toggle);
    int (*SDL_NumJoysticks) (void);
    const char * (*SDL_JoystickName) (int device_index);
    SDL_Joystick * (*SDL_JoystickOpen) (int device_index);
    int (*SDL_JoystickOpened) (int device_index);
    int (*SDL_JoystickIndex) (SDL_Joystick *joystick);
    int (*SDL_JoystickNumAxes) (SDL_Joystick *joystick);
    int (*SDL_JoystickNumBalls) (SDL_Joystick *joystick);
    int (*SDL_JoystickNumHats) (SDL_Joystick *joystick);
    int (*SDL_JoystickNumButtons) (SDL_Joystick *joystick);
    void (*SDL_JoystickUpdate) (void);
    int (*SDL_JoystickEventState) (int state);
    Sint16 (*SDL_JoystickGetAxis) (SDL_Joystick *joystick, int axis);
    Uint8 (*SDL_JoystickGetHat) (SDL_Joystick *joystick, int hat);
    int (*SDL_JoystickGetBall) (SDL_Joystick *joystick, int ball, int *dx, int *dy);
    Uint8 (*SDL_JoystickGetButton) (SDL_Joystick *joystick, int button);
    void (*SDL_JoystickClose) (SDL_Joystick *joystick);
    void (*SDL_PumpEvents) (void);
    int (*SDL_PeepEvents) (SDL_Event *events, int numevents, SDL_eventaction action, Uint32 mask);
    int (*SDL_PollEvent) (SDL_Event *event);
    int (*SDL_WaitEvent) (SDL_Event *event);
    int (*SDL_PushEvent) (SDL_Event *event);
    void (*SDL_SetEventFilter) (SDL_EventFilter filter);
    SDL_EventFilter (*SDL_GetEventFilter) (void);
    Uint8 (*SDL_EventState) (Uint8 type, int state);
    void * (*SDL_LoadObject) (const char *sofile);
    void * (*SDL_LoadFunction) (void *handle, const char *name);
    void (*SDL_UnloadObject) (void *handle);
    Uint32 (*SDL_GetTicks) (void);
    void (*SDL_Delay) (Uint32 ms);
    int (*SDL_SetTimer) (Uint32 interval, SDL_TimerCallback callback);
    SDL_TimerID (*SDL_AddTimer) (Uint32 interval, SDL_NewTimerCallback callback, void *param);
    SDL_bool (*SDL_RemoveTimer) (SDL_TimerID t);
    const SDL_version * (*SDL_Linked_Version) (void);
    int (*SDL_Init) (Uint32 flags);
    int (*SDL_InitSubSystem) (Uint32 flags);
    void (*SDL_QuitSubSystem) (Uint32 flags);
    Uint32 (*SDL_WasInit) (Uint32 flags);
    void (*SDL_Quit) (void);
};

extern struct sdl_funcs sdl_funcs;

#define SDL_SOFILE "libSDL.so"
#define SDL_DLLFILE "SDL.dll"

#define S(name, ...) \
    ({ \
        Forbid(); \
        typeof (sdl_funcs.name) __sdlret = sdl_funcs.name(__VA_ARGS__); \
        Permit(); \
        __sdlret; \
    })

#define SV(name, ...) \
    do { \
        Forbid(); \
        sdl_funcs.name(__VA_ARGS__); \
        Permit(); \
    } while (0)
