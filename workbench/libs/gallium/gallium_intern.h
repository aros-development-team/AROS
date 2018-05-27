/*
    Copyright © 2010-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GALLIUM_INTERN_H
#define GALLIUM_INTERN_H

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef GALLIUM_GALLIUM_H
#   include <gallium/gallium.h>
#endif

#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef PROTO_OOP_H
#   include <proto/oop.h>
#endif

#ifndef HIDD_GALLIUM_H
#   include <hidd/gallium.h>
#endif

#ifndef P_SCREEN_H
#   include <gallium/pipe/p_screen.h>
#endif

struct GalliumBase
{
    struct Library              galb_Lib;

    char                        *fallback;
    struct Library              *fallbackmodule;

    OOP_Class                   *basegallium;
    OOP_AttrBase                gfxAttrBase;
    OOP_AttrBase                galliumAttrBase;
    OOP_AttrBase                bmAttrBase;

    /* methods we use .. */
    OOP_MethodID                galliumMId_UpdateRect;
    OOP_MethodID                galliumMId_DisplayResource;

    struct SignalSemaphore      driversemaphore;
    struct Library              *drivermodule;
    OOP_Object                  *driver;
};

OOP_Object * SelectGalliumDriver(ULONG requestedinterfaceversion, struct Library * GalliumBase);
BOOL IsVersionMatching(ULONG version, OOP_Object * driver, struct Library * GalliumBase);

#define GB(lb)  ((struct GalliumBase *)lb)
#undef HiddGfxAttrBase
#define HiddGfxAttrBase      (GB(GalliumBase)->gfxAttrBase)
#undef HiddBitMapAttrBase
#define HiddBitMapAttrBase      (GB(GalliumBase)->bmAttrBase)
#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase      (GB(GalliumBase)->galliumAttrBase)

#endif
