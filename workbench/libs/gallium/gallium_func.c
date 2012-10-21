/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"
#include <proto/oop.h>
#include <aros/debug.h>

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase GB(GalliumBase)->galliumAttrBase

BOOL IsVersionMatching(ULONG version, OOP_Object * driver, struct Library * GalliumBase)
{
    IPTR galliuminterfaceversion = 0;

    if ((!driver) || (!GalliumBase))
        return FALSE;

    OOP_GetAttr(driver, aHidd_Gallium_GalliumInterfaceVersion, &galliuminterfaceversion);
    
    return (version == (ULONG)galliuminterfaceversion);
}

OOP_Object * SelectGalliumDriver(ULONG requestedinterfaceversion, struct Library * GalliumBase)
{
    OOP_Object * driver = NULL;

    /* 1. Let see if we can create hidd.gallium.nouveau object. This will
        only work if the nouveau.hidd is actually loaded and used */    
    driver = OOP_NewObject(NULL, "hidd.gallium.nouveau", NULL);
    if (driver)
    {
        if (IsVersionMatching(requestedinterfaceversion, driver, GalliumBase))
            return driver;
        else
        {
            /* Failed version check */
            OOP_DisposeObject(driver);
            driver = NULL;
        }
    }

    /* 2. Nouveau fails,try the next best...*/ 
    driver = OOP_NewObject(NULL, "hidd.gallium.i915", NULL);
    if (driver)
    {
        if (IsVersionMatching(requestedinterfaceversion, driver, GalliumBase)){
            return driver;
        }
        else
        {
            /* Failed version check */
            OOP_DisposeObject(driver);
            driver = NULL;
        }
    }

    /* 3. Everything else failed. Let's try loading softpipe */
    if (!GB(GalliumBase)->drivermodule)
        GB(GalliumBase)->drivermodule = OpenLibrary("softpipe.hidd", 9);

    if (GB(GalliumBase)->drivermodule)
    {
        driver = OOP_NewObject(NULL, "hidd.gallium.softpipe", NULL);
        if (driver)
        {
            if (IsVersionMatching(requestedinterfaceversion, driver, GalliumBase))
                return driver;
            else
            {
                /* Failed version check */
                OOP_DisposeObject(driver);
                driver = NULL;
            }            
        }
        
        /* Failed. Close library */
        CloseLibrary(GB(GalliumBase)->drivermodule);
        GB(GalliumBase)->drivermodule = NULL;
    }

    return NULL;
}
