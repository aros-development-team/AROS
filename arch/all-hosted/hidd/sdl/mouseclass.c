/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2010 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <exec/types.h>
#include <exec/semaphores.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <aros/symbolsets.h>

#ifdef __THROW
#undef __THROW
#endif
#ifdef __CONCAT
#undef __CONCAT
#endif

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

OOP_Object *SDLMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
    struct mousedata *mousedata;

    D(bug("[sdl] SDLMouse::New\n"));

    if ((o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg)) == NULL) {
        D(bug("[sdl] supermethod failed, bailing out\n"));
        return NULL;
    }

    mousedata = OOP_INST_DATA(cl, o);

    mousedata->callback = (APTR)GetTagData(aHidd_Mouse_IrqHandler, 0, msg->attrList);
    mousedata->callbackdata = (APTR)GetTagData(aHidd_Mouse_IrqHandlerData, 0, msg->attrList);

    D(bug("[sdl] created mouse hidd, callback 0x%08x, data 0x%08x\n", mousedata->callback, mousedata->callbackdata));

    return (OOP_Object *) o;
}

VOID SDLMouse__Hidd_SDLMouse__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_SDLMouse_HandleEvent *msg) {
    struct mousedata *mousedata = OOP_INST_DATA(cl, o);
    struct pHidd_Mouse_Event hiddev;

    D(bug("[sdl] SDLMouse::HandleEvent\n"));

    switch (msg->e->type) {
        case SDL_MOUSEMOTION:
            D(bug("[sdl] mouse moved to [%d,%d]\n", msg->e->motion.x, msg->e->motion.y));

            hiddev.type = vHidd_Mouse_Motion;
            hiddev.x = msg->e->motion.x;
            hiddev.y = msg->e->motion.y;
            hiddev.button = vHidd_Mouse_NoButton;

            if (mousedata->callback != NULL)
                mousedata->callback(mousedata->callbackdata, &hiddev);

            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            D(bug("[sdl] %s event for button %s\n", msg->e->button.state == SDL_PRESSED ? "PRESS" : "RELEASE",
                                                    msg->e->button.button == SDL_BUTTON_LEFT   ? "LEFT"   :
                                                    msg->e->button.button == SDL_BUTTON_RIGHT  ? "RIGHT"  :
                                                    msg->e->button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                                                                                                 "UNKNOWN"));

            hiddev.type = msg->e->button.state == SDL_PRESSED ? vHidd_Mouse_Press : vHidd_Mouse_Release;
            hiddev.x = msg->e->button.x;
            hiddev.y = msg->e->button.y;

            switch (msg->e->button.button) {
                case SDL_BUTTON_LEFT:
                    hiddev.button = vHidd_Mouse_Button1;
                    break;

                case SDL_BUTTON_RIGHT:
                    hiddev.button = vHidd_Mouse_Button2;
                    break;

                case SDL_BUTTON_MIDDLE:
                    hiddev.button = vHidd_Mouse_Button3;
                    break;

                default:
                    hiddev.button = vHidd_Mouse_NoButton;
                    break;
            }

            if (mousedata->callback != NULL)
                mousedata->callback(mousedata->callbackdata, &hiddev);

            break;
    }
}

VOID Hidd_SDLMouse_HandleEvent(OOP_Object *o, SDL_Event *e) {
    struct pHidd_SDLMouse_HandleEvent msg;
    static OOP_MethodID mid;

    if (!mid)
        mid = OOP_GetMethodID(IID_Hidd_SDLMouse, moHidd_SDLMouse_HandleEvent);

    msg.mID = mid;
    msg.e = e;

    OOP_DoMethod(o, (OOP_Msg) &msg);
}

static struct OOP_MethodDescr SDLMouse_Root_descr[] = {
    {(OOP_MethodFunc)SDLMouse__Root__New, moRoot_New},
    {NULL                               , 0         }
};
#define NUM_SDLMouse_Root_METHODS 1

static struct OOP_MethodDescr SDLMouse_Hidd_SDLMouse_descr[] = {
    {(OOP_MethodFunc)SDLMouse__Hidd_SDLMouse__HandleEvent, moHidd_SDLMouse_HandleEvent},
    {NULL, 0}
};
#define NUM_SDLMouse_Hidd_SDLMouse_METHODS 1

struct OOP_InterfaceDescr SDLMouse_ifdescr[] = {
    {SDLMouse_Root_descr         , IID_Root         , NUM_SDLMouse_Root_METHODS         },
    {SDLMouse_Hidd_SDLMouse_descr, IID_Hidd_SDLMouse, NUM_SDLMouse_Hidd_SDLMouse_METHODS},
    {NULL                        , NULL                                                 }
};
