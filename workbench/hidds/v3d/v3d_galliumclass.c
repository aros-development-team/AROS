/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    VideoCore VI (V3D) — Gallium HIDD class
*/

#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/gallium.h>

#include "v3d_intern.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase (SD(cl)->hiddGalliumAB)

/* Forward declaration — from Mesa V3D driver */
struct pipe_screen;
struct pipe_screen_config;
struct renderonly;
extern struct pipe_screen *v3d_screen_create(int fd,
    const struct pipe_screen_config *config, struct renderonly *ro);

/* Global V3DData pointer for the DRM shim to access */
struct V3DData *g_v3d_data = NULL;

/*
 * New — create instance of V3D Gallium HIDD.
 */
OOP_Object *HiddV3D__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    return o;
}

/*
 * CreatePipeScreen — create the Gallium pipe_screen for V3D.
 *
 * This is called by mesa3dgl.library when an OpenGL context is created.
 * We return a real V3D pipe_screen backed by hardware.
 */
OOP_Object *HiddV3D__Hidd_Gallium__CreatePipeScreen(OOP_Class *cl, OOP_Object *o,
                                                     struct pHidd_Gallium_CreatePipeScreen *msg)
{
    struct V3DData *sd = SD(cl);

    D(bug("[V3D] CreatePipeScreen\n"));

    if (!sd->powered) {
        D(bug("[V3D] GPU not powered\n"));
        return NULL;
    }

    /* Set global pointer for DRM shim */
    g_v3d_data = sd;

    /*
     * Create the Mesa V3D pipe_screen.
     * fd=0 is a dummy — our v3d_ioctl override doesn't use it.
     * config=NULL — no driconf on AROS.
     * ro=NULL — no renderonly (we own the display).
     */
    struct pipe_screen *screen = v3d_screen_create(0, NULL, NULL);

    if (!screen) {
        D(bug("[V3D] v3d_screen_create failed\n"));
        return NULL;
    }

    D(bug("[V3D] pipe_screen created at %p\n", screen));

    return (OOP_Object *)screen;
}

/*
 * DisplayResource — present a rendered frame to the screen.
 */
VOID HiddV3D__Hidd_Gallium__DisplayResource(OOP_Class *cl, OOP_Object *o,
                                             struct pHidd_Gallium_DisplayResource *msg)
{
    /* TODO: Blit from V3D render target to vc4gfx framebuffer */
    D(bug("[V3D] DisplayResource\n"));
}
