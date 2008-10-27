/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <exec/semaphores.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <aros/symbolsets.h>

#include "sdl_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] = {
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL,           NULL         }
};

static int sdl_mouseclass_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_mouseclass_init\n"));

    return OOP_ObtainAttrBases(attrbases);
}

static int sdl_mouseclass_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_mouseclass_expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(sdl_mouseclass_init , 0)
ADD2EXPUNGELIB(sdl_mouseclass_expunge, 0)

#define SDLGfxBase ((LIBBASETYPEPTR) cl->UserData)

OOP_Object *SDLMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
    BOOL has_mouse_hidd = FALSE;
    struct mousedata *mousedata;

    D(bug("[sdl] SDLMouse::New\n"));

    ObtainSemaphoreShared(&LIBBASE->lock);
    if (LIBBASE->mousehidd != NULL)
        has_mouse_hidd = TRUE;
    ReleaseSemaphore(&LIBBASE->lock);

    if (has_mouse_hidd) {
        D(bug("[sdl] mouse hidd already present, can't make another one\n"));
        return NULL;
    }

    if ((o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg)) == NULL) {
        D(bug("[sdl] supermethod failed, bailing out\n"));
        return NULL;
    }

    mousedata = OOP_INST_DATA(cl, o);

    mousedata->callback = GetTagData(aHidd_Mouse_IrqHandler, NULL, msg->attrList);
    mousedata->callbackdata = GetTagData(aHidd_Mouse_IrqHandlerData, NULL, msg->attrList);

    ObtainSemaphore(&LIBBASE->lock);
    LIBBASE->mousehidd = o;
    ReleaseSemaphore(&LIBBASE->lock);

    D(bug("[sdl] created mouse hidd, callback 0x%08x, data 0x%08x\n", mousedata->callback, mousedata->callbackdata));

    return (OOP_Object *) o;
}

VOID SDLMouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
    D(bug("[sdl] SDLMouse::Dispose\n"));

    ObtainSemaphore(&LIBBASE->lock);
    LIBBASE->mousehidd = NULL;
    ReleaseSemaphore(&LIBBASE->lock);

    OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
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
