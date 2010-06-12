/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "gallium_intern.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase   (SD(cl)->hiddGalliumAB)

static VOID HiddGalliumCreateDriverObject(struct galliumstaticdata * sd)
{
    /* Contains ugly hardcodes/hacks */
    
    /* 1. Let see if we can create hidd.gallium.nouveau object. This will
        only work if the nouveau.hidd is actually loaded and used */
    sd->driver = OOP_NewObject(NULL, "hidd.gallium.nouveau", NULL);
    if (sd->driver)
        return;

    /* 2. Everything else failed. Let's try loading softpipe */
    /* TODO: what if loadedDriverHidd was already set by previous attempt? */
    sd->loadedDriverHidd = OpenLibrary("softpipe.hidd", 4);
    if (sd->loadedDriverHidd)
        sd->driver = OOP_NewObject(NULL, "hidd.gallium.softpipe", NULL);
    
    /* Final check */
    if (!sd->driver)
        bug("[GalliumDriverFactory] Error - no driver implementation available\n");
}

OOP_Object * METHOD(GALLIUMDRIVERFACTORY, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

OOP_Object * METHOD(GALLIUMDRIVERFACTORY, Hidd_GalliumDriverFactory, GetDriver)
{
    /* Make sure this method is stateless and does lazy load of selected hidd.
       The selection of driver implementation (nouvea.hidd, softpipe.hidd)
       cannot happed during gallium.hidd initilization, because the driver
       implementation requires the gallium.hidd to be already loaded and
       initizalized */
    
    /* TODO: Use semaphore lock the operation */

    /* Create driver */
    if (!SD(cl)->driver)
        HiddGalliumCreateDriverObject(SD(cl));
    
    /* Check driver interface version in relation to client version. This check
       needs to happen for each request, not only for the first one! */
    if (SD(cl)->driver)
    {
        /* Check interface version */
        IPTR galliuminterfaceversion = 0;
        OOP_GetAttr(SD(cl)->driver, aHidd_Gallium_GalliumInterfaceVersion, &galliuminterfaceversion);
        
        /* Only matching version is allowed */
        if (msg->galliuminterfaceversion != (ULONG)galliuminterfaceversion)
        {
            bug("[GalliumDriverFactory] Error - client requested interface version %d while driver supplies version %d\n",
                msg->galliuminterfaceversion, galliuminterfaceversion);
            return NULL;
        }
    }
    
    return SD(cl)->driver;
}

