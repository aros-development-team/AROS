/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "gallium.h"

#undef HiddGalliumBaseDriverAttrBase
#define HiddGalliumBaseDriverAttrBase   (SD(cl)->hiddGalliumBaseDriverAB)

static BOOL HiddGalliumIsLibLoaded(STRPTR libname)
{
    struct Library * library = NULL;
    
    Forbid();
    
    for(library = (struct Library *)SysBase->LibList.lh_Head; library->lib_Node.ln_Succ != NULL;
            library = (struct Library *)library->lib_Node.ln_Succ)
    {
        if (strcmp(library->lib_Node.ln_Name, libname) == 0)
        {
            Permit();
            return TRUE;
        }
    }
    
    Permit();
    return FALSE;
}

static VOID HiddGalliumLoadDriverHidd(struct galliumstaticdata * sd)
{
    /* This function is designed to contain all the ugly hardcodes */

    /* Try loading nouveau.hidd */
    if (!HiddGalliumIsLibLoaded("nvidia.hidd") && !HiddGalliumIsLibLoaded("nouveau2d.hidd"))
    {
        /* nvidia.hidd is not loaded so we might try loading nouveau.hidd */
        if (!sd->loadedDriverHidd)
        {
            sd->loadedDriverHidd = OpenLibrary("nouveau.hidd", 3);
            if (sd->loadedDriverHidd)
            {
                /* If the nouveau.hidd loaded, it means compatible nvidia card
                   was found */
                sd->loadedDriverClassName = "hidd.gallium.nouveaudriver";
            }
        }
    }

    /* Last option - softpipe.hidd */
    if (!sd->loadedDriverHidd)
    {
        sd->loadedDriverHidd = OpenLibrary("softpipe.hidd", 3);
        if (sd->loadedDriverHidd)
        {
            sd->loadedDriverClassName = "hidd.gallium.softpipedriver";
        }
    }
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

    /* Load driver hidd */
    if (!SD(cl)->loadedDriverHidd)
        HiddGalliumLoadDriverHidd(SD(cl));

    /* Create driver from selected driver implementation */
    if (!SD(cl)->driver)
    {
        if (SD(cl)->loadedDriverHidd)
            SD(cl)->driver = OOP_NewObject(NULL, SD(cl)->loadedDriverClassName, NULL);
        else
            bug("[GalliumDriverFactory] Error - no driver implementation available\n");
    }
    
    /* Check driver interface version in relation to client version. This check
       needs to happen for each request, not only for the first one! */
    if (SD(cl)->driver)
    {
        /* Check interface version */
        IPTR galliuminterfaceversion = 0;
        OOP_GetAttr(SD(cl)->driver, aHidd_GalliumBaseDriver_GalliumInterfaceVersion, &galliuminterfaceversion);
        
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

