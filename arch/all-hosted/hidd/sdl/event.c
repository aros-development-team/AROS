/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (C) 2007 Robert Norris. All rights reserved.
 * Copyright (C) 2010-2017 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <aros/symbolsets.h>
#include <aros/libcall.h>

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/lists.h>
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#ifdef __THROW
#undef __THROW
#endif
#ifdef __CONCAT
#undef __CONCAT
#endif

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#define MAX_EVENTS (64)

AROS_INTH1(tick_handler, struct Task *,task)
{
    AROS_INTFUNC_INIT

    Signal(task, SIGBREAKF_CTRL_D);

    return FALSE;

    AROS_INTFUNC_EXIT
}


VOID sdl_event_task(struct Task *creator, ULONG sync, LIBBASETYPEPTR LIBBASE) {
    struct Interrupt tick_int;
    SDL_Event e[MAX_EVENTS];
    int nevents, i;

    D(bug("[sdl] event loop task starting up\n"));

    tick_int.is_Code         = (VOID_FUNC)tick_handler;
    tick_int.is_Data         = FindTask(NULL);
    tick_int.is_Node.ln_Name = "SDL event tick";
    tick_int.is_Node.ln_Pri  = 0;
    tick_int.is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_VERTB, &tick_int);

    D(bug("[sdl] event loop task running, signalling creator\n"));

    Signal(creator, sync);

    D(bug("[sdl] entering loop\n"));

    while (1) {
        Uint8 active = 1;

        Wait(SIGBREAKF_CTRL_D);

        SV(SDL_PumpEvents);
        if ((nevents = S(SDL_PeepEvents, e, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEEVENTMASK|SDL_KEYEVENTMASK|SDL_ACTIVEEVENTMASK)) > 0) {
            D(bug("[sdl] %d events pending\n", nevents));

            for (i = 0; i < nevents; i++) {
                switch (e[i].type) {
                    case SDL_ACTIVEEVENT:
                        if (e[i].active.state & SDL_APPINPUTFOCUS) {
                            active = e[i].active.gain;
                            D(bug("[sdl] Window active: %u\n", active));
                            if (active && LIBBASE->cb)
                                LIBBASE->cb(LIBBASE->cbdata, NULL);
                        }
                        break;
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        /* We report mouse events only if our window is active.
                           Some OSes (MS Windows) otherwise report mouse movements
                           for inactive windows too, this can confuse Intuition */
                        if (active) {
                            D(bug("[sdl] got mouse event, sending to mouse hidd\n"));

                            if (LIBBASE->mousehidd)
                                Hidd_Mouse_SDL_HandleEvent(LIBBASE->mousehidd, &e[i]);
                        }
                        break;

                    case SDL_KEYUP:
                    case SDL_KEYDOWN:
                        D(bug("[sdl] got keyboard event, sending to keyboard hidd\n"));

                        if (LIBBASE->kbdhidd)
                            Hidd_Kbd_SDL_HandleEvent(LIBBASE->kbdhidd, &e[i]);

                        break;
                }
            }
        }
    }
}

int sdl_event_init(LIBBASETYPEPTR LIBBASE) {
    struct Task *task;
    ULONG sync;

    D(bug("[sdl] creating event loop task\n"));

    sync = SIGF_BLIT;
    SetSignal(0, sync);

    if ((task = NewCreateTask(TASKTAG_PC, sdl_event_task,
        TASKTAG_STACKSIZE, AROS_STACKSIZE,
        TASKTAG_NAME, "sdl.hidd event task",
        TASKTAG_PRI, 50,
        TASKTAG_ARG1, (IPTR)FindTask(NULL),
        TASKTAG_ARG2, (IPTR)sync,
        TASKTAG_ARG3, (IPTR)LIBBASE,
        TAG_DONE)) == NULL)
    {
        D(bug("[sdl] new task creation failed\n"));
        return FALSE;
    }

    D(bug("[sdl] task created, waiting for it to start up\n"));

    Wait(sync);

    D(bug("[sdl] event loop task up and running\n"));

    LIBBASE->eventtask = task;
    
    return TRUE;
}

void sdl_event_expunge(LIBBASETYPEPTR LIBBASE)
{
    RemTask(LIBBASE->eventtask);
    LIBBASE->eventtask = NULL;
}
