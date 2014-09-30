/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/dos.h>
#include <stdio.h>

APTR HostLibBase;

int (*SDL_Init) (ULONG flags);
void * (*SDL_SetVideoMode) (int width, int height, int bpp, ULONG flags);
void (*SDL_Quit) (void);

int main(int argc, char **argv) {
    void *handle;
    char *err;

    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL) {
        fprintf(stderr, "can't open hostlib.resource\n");
        return 1;
    }

    if ((handle = HostLib_Open("libSDL.so", &err)) == NULL) {
        fprintf(stderr, "can't open sdl: %s\n", err);
        return 1;
    }

    SDL_Init = HostLib_GetPointer(handle, "SDL_Init", NULL);
    SDL_SetVideoMode = HostLib_GetPointer(handle, "SDL_SetVideoMode", NULL);
    SDL_Quit = HostLib_GetPointer(handle, "SDL_Quit", NULL);

    SDL_Init(0x20);
    SDL_SetVideoMode(640, 480, 16, 0);

    Delay(250);

    SDL_Quit();

    HostLib_Close(handle, NULL);

    return 0;
}
