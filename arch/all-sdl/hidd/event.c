/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
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

#include "sdl_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

#define MAX_EVENTS (64)

AROS_UFH4(ULONG, tick_handler,
          AROS_UFHA(ULONG,             dummy, A0),
          AROS_UFHA(struct Task,       *task, A1),
          AROS_UFHA(ULONG,             dummy2, A5),
          AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT

    Signal(task, SIGBREAKF_CTRL_D);

    return 0;

    AROS_USERFUNC_EXIT
}


VOID sdl_event_task(struct Task *creator, BYTE sync, LIBBASETYPEPTR LIBBASE) {
    struct Interrupt tick_int;
    SDL_Event e[MAX_EVENTS];
    int nevents, i;

    D(bug("[sdl] event loop task starting up\n"));

    tick_int.is_Code         = (APTR) &tick_handler;
    tick_int.is_Data         = FindTask(NULL);
    tick_int.is_Node.ln_Name = "SDL event tick";
    tick_int.is_Node.ln_Pri  = 0;
    tick_int.is_Node.ln_Type = NT_INTERRUPT;
    AddIntServer(INTB_TIMERTICK, &tick_int);

    D(bug("[sdl] event loop task running, signalling creator\n"));

    Signal(creator, 1 << sync);

    D(bug("[sdl] entering loop\n"));

    while (1) {
        Wait(SIGBREAKF_CTRL_D);

        SV(SDL_PumpEvents);
        if ((nevents = S(SDL_PeepEvents, e, MAX_EVENTS, SDL_GETEVENT, SDL_MOUSEEVENTMASK | SDL_KEYEVENTMASK)) > 0) {
            D(bug("[sdl] %d events pending\n", nevents));

            for (i = 0; i < nevents; i++) {
                switch (e[i].type) {
                    case SDL_MOUSEMOTION:
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        D(bug("[sdl] got mouse event, sending to mouse hidd\n"));

                        ObtainSemaphoreShared(&LIBBASE->lock);
                        if (LIBBASE->mousehidd)
                            Hidd_SDLMouse_HandleEvent(LIBBASE->mousehidd, &e[i]);
                        ReleaseSemaphore(&LIBBASE->lock);
                        
                        break;

                    case SDL_KEYUP:
                    case SDL_KEYDOWN:
                        D(bug("[sdl] got keyboard event, sending to keyboard hidd\n"));

                        ObtainSemaphoreShared(&LIBBASE->lock);
                        if (LIBBASE->kbdhidd)
                            Hidd_SDLMouse_HandleEvent(LIBBASE->kbdhidd, &e[i]);
                        ReleaseSemaphore(&LIBBASE->lock);

                        break;
                }
            }
        }
    }
}

static int sdl_event_init(LIBBASETYPEPTR LIBBASE) {
    struct Task *task;
    APTR stack;
    BYTE sync;

    D(bug("[sdl] creating event loop task\n"));

    if ((task = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR)) == NULL) {
        D(bug("[sdl] couldn't allocate task memory\n"));
        return NULL;
    }

    if ((stack = AllocMem(AROS_STACKSIZE, MEMF_PUBLIC)) == NULL) {
        D(bug("[sdl] couldn't allocate task stack memory\n"));
        FreeMem(task, sizeof(struct Task));
        return NULL;
    }

    task->tc_Node.ln_Type = NT_TASK;
    task->tc_Node.ln_Name = "sdl.hidd event task";
    task->tc_Node.ln_Pri  = 50;

    NEWLIST(&task->tc_MemEntry);

    task->tc_SPLower = stack;
    task->tc_SPUpper = (UBYTE *) stack + AROS_STACKSIZE;

#if AROS_STACK_GROWS_DOWNWARDS
    task->tc_SPReg = (UBYTE *) task->tc_SPUpper - SP_OFFSET;
#else
    task->tc_SPReg = (UBYTE *) task->tc_SPLower + SP_OFFSET;
#endif

    sync = SIGBREAKF_CTRL_C;

    if (NewAddTask(task, sdl_event_task, NULL, TAGLIST(TASKTAG_ARG1, FindTask(NULL),
                                                       TASKTAG_ARG2, sync,
                                                       TASKTAG_ARG3, LIBBASE)) == NULL) {
        D(bug("[sdl] new task creation failed\n"));
        FreeMem(stack, AROS_STACKSIZE);
        FreeMem(task, sizeof(struct Task));
        return NULL;
    }

    D(bug("[sdl] task created, waiting for it to start up\n"));

    Wait(1 << sync);

    D(bug("[sdl] event loop task up and running\n"));

    LIBBASE->eventtask = task;
}

static int sdl_event_expunge(LIBBASETYPEPTR LIBBASE) {
    if (LIBBASE->eventtask == NULL)
        return TRUE;

    RemTask(LIBBASE->eventtask);
    LIBBASE->eventtask = NULL;
}

ADD2INITLIB(sdl_event_init, 1)
ADD2EXPUNGELIB(sdl_event_expunge, 1)
