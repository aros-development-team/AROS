/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2007-2011 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/semaphores.h>

#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

struct sdl_funcs sdl_funcs;

static const char *sdl_func_names[] = {
    "SDL_GetError",
    "SDL_VideoDriverName",
    "SDL_GetVideoSurface",
    "SDL_GetVideoInfo",
    "SDL_ListModes",
    "SDL_SetVideoMode",
    "SDL_UpdateRect",
    "SDL_SetColors",
    "SDL_CreateRGBSurface",
    "SDL_CreateRGBSurfaceFrom",
    "SDL_FreeSurface",
    "SDL_LockSurface",
    "SDL_UnlockSurface",
    "SDL_UpperBlit",
    "SDL_FillRect",
    "SDL_WM_SetCaption",
    "SDL_WM_SetIcon",
    "SDL_ShowCursor",
    "SDL_PumpEvents",
    "SDL_PeepEvents",
    "SDL_Linked_Version",
    "SDL_Init",
    "SDL_Quit",
    NULL
};

APTR HostLibBase;

static void *sdl_hostlib_load_so(const char *sofile, const char **names, void **funcptr) {
    void *handle;
    char *err;
    int i;

    D(bug("[sdl] loading functions from %s\n", sofile));

    if ((handle = HostLib_Open(sofile, &err)) == NULL) {
        kprintf("[sdl] couldn't open '%s': %s\n", sofile, err);
        return NULL;
    }

    for (i = 0; names[i]; i++) {
        funcptr[i] = HostLib_GetPointer(handle, names[i], &err);
        if (err != NULL) {
            kprintf("[sdl] couldn't get symbol '%s' from '%s': %s\n", names[i], sofile, err);
            HostLib_FreeErrorStr(err);
            HostLib_Close(handle, NULL);
            return NULL;
        }
    }

    D(bug("[sdl] done\n"));

    return handle;
}

int sdl_hostlib_init(LIBBASETYPEPTR LIBBASE)
{
    STRPTR LibraryFile = SDL_SOFILE;
    APTR KernelBase;
    const char *arch;

    D(bug("[sdl] hostlib init\n"));

    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL) {
        kprintf("[sdl] couldn't open hostlib.resource\n");
        return FALSE;
    }

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
        return FALSE;

    arch = (const char *)KrnGetSystemAttr(KATTR_Architecture);
    D(bug("[sdl] Host operating system: %s\n", arch));

    if (!strcmp(arch, "mingw32-i386")) 
        LibraryFile = SDL_DLLFILE;

    if ((LIBBASE->sdl_handle = sdl_hostlib_load_so(LibraryFile, sdl_func_names, (void **) &sdl_funcs)) == NULL)
        return FALSE;

    return TRUE;
}

int sdl_hostlib_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] hostlib expunge\n"));

    if (LIBBASE->sdl_handle != NULL)
        HostLib_Close(LIBBASE->sdl_handle, NULL);

    return TRUE;
}
