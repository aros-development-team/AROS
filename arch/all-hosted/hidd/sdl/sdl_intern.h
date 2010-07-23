/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2010 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#ifndef SDL_INTERN_H
#define SDL_INTERN_H 1

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <hidd/mouse.h>

#include "sdl_hostlib.h"

#define CLID_Hidd_SDLGfx    "hidd.gfx.sdl"
#define IID_Hidd_SDLGFX     "hidd.gfx.sdl"

struct gfxdata
{
    OOP_Object *shownbm;	/* Currently shown bitmap object */
    OOP_Object *framebuffer;	/* Framebuffer bitmap object */
};

#define IID_Hidd_SDLBitMap  "hidd.bitmap.sdl"

enum
{
    aoHidd_SDLBitMap_Surface,
    num_Hidd_SDLBitMap_Attrs
};

#define aHidd_SDLBitMap_Surface     (((ULONG) HiddSDLBitMapAttrBase) + aoHidd_SDLBitMap_Surface)

#define SDLBM_ATTR(id) ((id)-HiddSDLBitMapAttrBase)

struct bmdata {
    SDL_Surface     *surface;
    BOOL            is_onscreen;
};

#define IID_Hidd_SDLMouse   "hidd.mouse.sdl"

struct mousedata {
    VOID (*callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
};

enum {
    moHidd_SDLMouse_HandleEvent
};

struct pHidd_SDLMouse_HandleEvent {
    OOP_MethodID mID;
    SDL_Event *e;
};

VOID Hidd_SDLMouse_HandleEvent(OOP_Object *o, SDL_Event *e);

#define IID_Hidd_SDLKbd     "hidd.kbd.sdl"

struct kbddata {
    VOID (*callback)(APTR, UWORD);
    APTR callbackdata;
};

enum {
    moHidd_SDLKbd_HandleEvent
};

struct pHidd_SDLKbd_HandleEvent {
    OOP_MethodID mID;
    SDL_Event *e;
};

VOID Hidd_SDLMouse_HandleEvent(OOP_Object *o, SDL_Event *e);

struct sdlhidd
{
    APTR                    sdl_handle;

    OOP_Class               *gfxclass;
    OOP_Class               *bmclass;
    OOP_Class               *mouseclass;
    OOP_Class               *kbdclass;

    struct Task             *eventtask;
    /* Object instance would be a better place for this, but event handler task gets
       only pointer to this structure. Anyway there can be only one SDL display in
       the system, so this will do. */
    void (*cb)(void *data, void *bm);		/* Display activation callback function */
    void 		    *cbdata;		/* User data for activation callback    */

    OOP_Object              *mousehidd;
    OOP_Object              *kbdhidd;

    UBYTE                   keycode[SDLK_LAST];

    BOOL                    use_hwsurface;
    BOOL                    use_fullscreen;
};

#define LIBBASETYPEPTR struct sdlhidd *

/* these should be handled by some sort of configuration
 * and/or commandline switches */
#define CFG_WANT_FULLSCREEN (0)

/* Class descriptors */
extern struct OOP_InterfaceDescr SDLGfx_ifdescr[];
extern struct OOP_InterfaceDescr SDLBitMap_ifdescr[];
extern struct OOP_InterfaceDescr SDLMouse_ifdescr[];
extern struct OOP_InterfaceDescr SDLKbd_ifdescr[];

extern OOP_AttrBase MetaAttrBase;
extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HiddSDLBitMapAttrBase;

extern struct sdlhidd xsd;

void sdl_keymap_init(LIBBASETYPEPTR LIBBASE);
int sdl_hidd_init(LIBBASETYPEPTR LIBBASE);
int sdl_event_init(LIBBASETYPEPTR LIBBASE);
void sdl_event_expunge(LIBBASETYPEPTR LIBBASE);

#endif
