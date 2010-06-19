/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "gallium_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH1(struct pipe_screen *, CreatePipeScreen,

/*  SYNOPSIS */ 
      AROS_LHA(struct TagItem *, tagItem, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 5, Gallium)

/*  NAME
 
    FUNCTION
    Creates a gallium pipe screen.
 
    INPUTS
    tagItem - a pointer to tags to be used during creation.
 
    RESULT
    A valid pipe screen instance or NULL of creation was not succesfull.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct pipe_screen * screen = NULL;

    ObtainSemaphore(&GB(GalliumBase)->driversemaphore);

    /* TODO: validate gallium interface version that is passed in tagList
       with the driver gallium interface version */
    if (!GB(GalliumBase)->driver)
        GB(GalliumBase)->driver = SelectGalliumDriver();
    
    if(GB(GalliumBase)->driver)
    {
        /* Create screen */
        struct pHidd_Gallium_CreatePipeScreen cpsmsg;
        
        cpsmsg.mID = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_CreatePipeScreen);
        screen = (struct pipe_screen *)OOP_DoMethod(GB(GalliumBase)->driver, (OOP_Msg)&cpsmsg);
    }
    
    ReleaseSemaphore(&GB(GalliumBase)->driversemaphore);

    return screen;

    AROS_LIBFUNC_EXIT
}
