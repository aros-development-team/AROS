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
#include <proto/utility.h>
#include <proto/oop.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#include <aros/symbolsets.h>

#include "sdl_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

static OOP_AttrBase HiddKbdAB;

static struct OOP_ABDescr attrbases[] = {
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL,           NULL         }
};

static int sdl_kbdclass_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_kbdclass_init\n"));

    return OOP_ObtainAttrBases(attrbases);
}

static int sdl_kbdclass_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_kbdclass_expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(sdl_kbdclass_init , 0)
ADD2EXPUNGELIB(sdl_kbdclass_expunge, 0)

#define SDLGfxBase ((LIBBASETYPEPTR) cl->UserData)

OOP_Object *SDLKbd__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
    BOOL has_kbd_hidd = FALSE;
    struct kbddata *kbddata;

    D(bug("[sdl] SDLKbd::New\n"));

    ObtainSemaphoreShared(&LIBBASE->lock);
    if (LIBBASE->kbdhidd != NULL)
        has_kbd_hidd = TRUE;
    ReleaseSemaphore(&LIBBASE->lock);

    if (has_kbd_hidd) {
        D(bug("[sdl] keyboard hidd already present, can't make another one\n"));
        return NULL;
    }

    if ((o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg)) == NULL) {
        D(bug("[sdl] supermethod failed, bailing out\n"));
        return NULL;
    }

    kbddata = OOP_INST_DATA(cl, o);

    kbddata->callback = GetTagData(aHidd_Kbd_IrqHandler, NULL, msg->attrList);
    kbddata->callbackdata = GetTagData(aHidd_Kbd_IrqHandlerData, NULL, msg->attrList);

    ObtainSemaphore(&LIBBASE->lock);
    LIBBASE->kbdhidd = o;
    ReleaseSemaphore(&LIBBASE->lock);

    D(bug("[sdl] created keyboard hidd, callback 0x%08x, data 0x%08x\n", kbddata->callback, kbddata->callbackdata));

    return (OOP_Object *) o;
}

VOID SDLKbd__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
    D(bug("[sdl] SDLKbd::Dispose\n"));

    ObtainSemaphore(&LIBBASE->lock);
    LIBBASE->kbdhidd = NULL;
    ReleaseSemaphore(&LIBBASE->lock);

    OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
}

VOID SDLKbd__Hidd_SDLKbd__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_SDLKbd_HandleEvent *msg) {
    struct kbddata *kbddata = OOP_INST_DATA(cl, o);
    UWORD keycode;

    D(bug("[sdl] SDLKbd::HandleEvent\n"));

    D(bug("[sdl] %s event for sdl key 0x%04x\n", msg->e->key.state == SDL_PRESSED ? "PRESS" : "RELEASE",
                                                 msg->e->key.keysym.sym));

    keycode = LIBBASE->keycode[msg->e->key.keysym.sym];

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
