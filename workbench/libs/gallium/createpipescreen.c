/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"
#include <proto/utility.h>
#include <aros/debug.h>

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase GB(GalliumBase)->galliumAttrBase

/*****************************************************************************

    NAME */

      AROS_LH1(struct pipe_screen *, CreatePipeScreen,

/*  SYNOPSIS */ 
      AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 5, Gallium)

/*  FUNCTION
        Creates a gallium pipe screen.
 
    INPUTS
        tags - a pointer to tags to be used during creation.
 
    TAGS
        CPS_GalliumInterfaceVersion - Indicates a version of gallium interface
            that a client is expected to receive. The client expected version
            must ideally match with the version that the driver provides,
            because gallium interface is not backwards compatible. This tag is
            required. Unless otherwise needed, the value
            GALLIUM_INTERFACE_VERSION should be passed.
            See also CreatePipeScreenV.

    RESULT
        A valid pipe screen instance or NULL of creation was not succesfull.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct pipe_screen * screen = NULL;
    LONG requestedinterfaceversion = 
        GetTagData(CPS_GalliumInterfaceVersion, -1, tags);

    /* The tag is missing */
    if (requestedinterfaceversion == -1)
        return NULL;

    ObtainSemaphore(&GB(GalliumBase)->driversemaphore);

    if (!GB(GalliumBase)->driver)
        GB(GalliumBase)->driver = SelectGalliumDriver(requestedinterfaceversion, GalliumBase);
    
    if(GB(GalliumBase)->driver)
    {
        /* Validate driver gallium interface version */
        if (IsVersionMatching(requestedinterfaceversion, GB(GalliumBase)->driver, GalliumBase))
        {
            /* Create screen */
            struct pHidd_Gallium_CreatePipeScreen cpsmsg;
            
            cpsmsg.mID = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_CreatePipeScreen);
            screen = (struct pipe_screen *)OOP_DoMethod(GB(GalliumBase)->driver, (OOP_Msg)&cpsmsg);
        }
        else
        {
            bug("[gallium.library] Requested gallium interface version does not match selected driver\n");
        }
    }
    else
    {
        bug("[gallium.library] Failed to acquire a driver\n");
    }
    
    ReleaseSemaphore(&GB(GalliumBase)->driversemaphore);

    return screen;

    AROS_LIBFUNC_EXIT
}
