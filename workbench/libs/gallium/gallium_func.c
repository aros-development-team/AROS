/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"
#include <proto/oop.h>

static struct Library * GalliumHIDDBase = NULL;

OOP_Object * SelectGalliumDriver()
{
    /* TODO: Correclty implement driver selection. Add safety checks */
    OOP_Object * driver = NULL;
    
    if (!GalliumHIDDBase)
        GalliumHIDDBase = OpenLibrary("softpipe.hidd", 4);

    if (GalliumHIDDBase)
        driver = OOP_NewObject(NULL, "hidd.gallium.softpipe", NULL);

    return driver;
}
