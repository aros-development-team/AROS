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
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#ifdef __THROW
#undef __THROW
#endif
#ifdef __CONCAT
#undef __CONCAT
#endif

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

OOP_Object *SDLKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
    struct kbddata *kbddata;

    D(bug("[sdl] SDLKbd::New\n"));

    if ((o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg)) == NULL) {
        D(bug("[sdl] supermethod failed, bailing out\n"));
        return NULL;
    }

    kbddata = OOP_INST_DATA(cl, o);

    kbddata->callback = (APTR)GetTagData(aHidd_Kbd_IrqHandler, 0, msg->attrList);
    kbddata->callbackdata = (APTR)GetTagData(aHidd_Kbd_IrqHandlerData, 0, msg->attrList);

    D(bug("[sdl] created keyboard hidd, callback 0x%08x, data 0x%08x\n", kbddata->callback, kbddata->callbackdata));

    return (OOP_Object *) o;
}

VOID SDLKbd__Hidd_SDLKbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_SDLKbd_HandleEvent *msg) {
    struct kbddata *kbddata = OOP_INST_DATA(cl, o);
    UWORD keycode;

    D(bug("[sdl] SDLKbd::HandleEvent\n"));

    D(bug("[sdl] %s event for sdl key 0x%04x\n", msg->e->key.state == SDL_PRESSED ? "PRESS" : "RELEASE",
                                                 msg->e->key.keysym.sym));

    keycode = xsd.keycode[msg->e->key.keysym.sym];

    D(bug("[sdl] converted to keycode 0x%02x\n", keycode));

    if (msg->e->key.state == SDL_RELEASED)
        keycode |= IECODE_UP_PREFIX;

    if (kbddata->callback != NULL)
        kbddata->callback(kbddata->callbackdata, keycode);
}

VOID Hidd_SDLKbd_HandleEvent(OOP_Object *o, SDL_Event *e) {
    struct pHidd_SDLKbd_HandleEvent msg;
    static OOP_MethodID mid;

    if (!mid)
        mid = OOP_GetMethodID(IID_Hidd_SDLKbd, moHidd_SDLKbd_HandleEvent);

    msg.mID = mid;
    msg.e = e;

    OOP_DoMethod(o, (OOP_Msg) &msg);
}

static struct OOP_MethodDescr SDLKbd_Root_descr[] = {
    {(OOP_MethodFunc)SDLKbd__Root__New, moRoot_New},
    {NULL                             , 0         }
};
#define NUM_SDLKbd_Root_METHODS 1

static struct OOP_MethodDescr SDLKbd_Hidd_SDLKbd_descr[] = {
    {(OOP_MethodFunc)SDLKbd__Hidd_SDLKbd__HandleEvent, moHidd_SDLKbd_HandleEvent},
    {NULL, 0}
};
#define NUM_SDLKbd_Hidd_SDLKbd_METHODS 1

struct OOP_InterfaceDescr SDLKbd_ifdescr[] = {
    {SDLKbd_Root_descr       , IID_Root       , NUM_SDLKbd_Root_METHODS       },
    {SDLKbd_Hidd_SDLKbd_descr, IID_Hidd_SDLKbd, NUM_SDLKbd_Hidd_SDLKbd_METHODS},
    {NULL                    , NULL                                           }
};
