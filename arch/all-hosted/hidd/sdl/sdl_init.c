/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2007-2009 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <proto/hostlib.h>
#include <proto/exec.h>

#define DEBUG 0
#include <aros/debug.h>

#include "sdl_intern.h"

#include LC_LIBDEFS_FILE

static int sdl_hidd_init(LIBBASETYPEPTR LIBBASE) {
    SDL_version cver, *rver;
    int i, ret;
    char *err;

    D(bug("[sdl] hidd init\n"));

    SDL_VERSION(&cver);
    rver = S(SDL_Linked_Version);

    kprintf("sdl.hidd: using SDL version %d.%d.%d\n", rver->major, rver->minor, rver->patch);

    if (cver.major != rver->major || cver.minor != rver->minor || cver.patch != rver->patch) {
        kprintf("WARNING: sdl.hidd was compiled against SDL version %d.%d.%d\n", cver.major, cver.minor, cver.patch);
        kprintf("         You may experience problems\n");
    }

    InitSemaphore(&LIBBASE->lock);

    /* start sdl. we don't catch any signals with a debug build as it plays
     * havoc with the debugger */
#if DEBUG
    ret = S(SDL_Init, SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
#else
    ret = S(SDL_Init, SDL_INIT_VIDEO);
#endif

    if (ret != 0) {
        D(bug("[sdl] couldn't initialise SDL\n"));
        return FALSE;
    }

    S(SDL_ShowCursor, SDL_DISABLE);
    
    return TRUE;
}

static int sdl_hidd_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] hidd expunge\n"));

    if (LIBBASE->sdl_handle)
        SV(SDL_Quit);

    return TRUE;
}

ADD2INITLIB(sdl_hidd_init, 1)
ADD2EXPUNGELIB(sdl_hidd_expunge, 1)
