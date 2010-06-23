/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"
#include <proto/oop.h>
#include <aros/debug.h>

/* TODO: Move this into libbase and close at library expunge */
static struct Library * GalliumHIDDBase = NULL;

OOP_Object * SelectGalliumDriver()
{
    OOP_Object * driver = NULL;

    /* 1. Let see if we can create hidd.gallium.nouveau object. This will
        only work if the nouveau.hidd is actually loaded and used */    
    driver = OOP_NewObject(NULL, "hidd.gallium.nouveau", NULL);
    if (driver)
        return driver;

    /* 2. Everything else failed. Let's try loading softpipe */
    if (!GalliumHIDDBase)
        GalliumHIDDBase = OpenLibrary("softpipe.hidd", 4);

    if (GalliumHIDDBase)
    {
        driver = OOP_NewObject(NULL, "hidd.gallium.softpipe", NULL);
        if (!driver)
        {
            /* Failed. Close library */
            CloseLibrary(GalliumHIDDBase);
            GalliumHIDDBase = NULL;
        }
    }

    /* Final check */
    if (!driver)
        bug("[gallium.library] ERROR - no driver available\n");

    return driver;
}
