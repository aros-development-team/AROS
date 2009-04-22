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
    char * (*SDL_GetError) (void);
    char * (*SDL_VideoDriverName) (char *namebuf, int maxlen);
    SDL_Surface * (*SDL_GetVideoSurface) (void);
    const SDL_VideoInfo * (*SDL_GetVideoInfo) (void);
    SDL_Rect ** (*SDL_ListModes) (SDL_PixelFormat *format, Uint32 flags);
    SDL_Surface * (*SDL_SetVideoMode) (int width, int height, int bpp, Uint32 flags);
    void (*SDL_UpdateRect) (SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
    int (*SDL_SetColors) (SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);
    SDL_Surface * (*SDL_CreateRGBSurface) (Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
    SDL_Surface * (*SDL_CreateRGBSurfaceFrom) (void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
    void (*SDL_FreeSurface) (SDL_Surface *surface);
    int (*SDL_LockSurface) (SDL_Surface *surface);
    void (*SDL_UnlockSurface) (SDL_Surface *surface);
    int (*SDL_UpperBlit) (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
    int (*SDL_FillRect) (SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
    void (*SDL_WM_SetCaption) (const char *title, const char *icon);
    void (*SDL_WM_SetIcon) (SDL_Surface *icon, Uint8 *mask);
    int (*SDL_ShowCursor) (int toggle);
    void (*SDL_PumpEvents) (void);
    int (*SDL_PeepEvents) (SDL_Event *events, int numevents, SDL_eventaction action, Uint32 mask);
    const SDL_version * (*SDL_Linked_Version) (void);
    int (*SDL_Init) (Uint32 flags);
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
