/*
    Copyright 2010, The AROS Development Team. All rights reserved.
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
    struct SignalSemaphore      driversemaphore;
    struct Library              *drivermodule;
    OOP_Object                  *driver;
    OOP_AttrBase                galliumAttrBase;
};

OOP_Object * SelectGalliumDriver(ULONG requestedinterfaceversion, struct Library * GalliumBase);
BOOL IsVersionMatching(ULONG version, OOP_Object * driver, struct Library * GalliumBase);

#define GB(lb)  ((struct GalliumBase *)lb)

#endif
