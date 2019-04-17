/*
    Copyright © 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/utility.h>
#include <proto/intuition.h>

#include <hidd/gfx.h>

#include <stdio.h>

#include "gallium_intern.h"

#undef HiddGalliumAttrBase
#define HiddGalliumAttrBase GB(GalliumBase)->galliumAttrBase

/*****************************************************************************

    NAME */

      AROS_LH1(struct pipe_screen *, CreatePipeScreen,

/*  SYNOPSIS */ 
      AROS_LHA(PipeHandle_t, pipe, A0),

/*  LOCATION */
      struct Library *, GalliumBase, 7, Gallium)

/*  FUNCTION
        Creates a gallium pipe screen.

    INPUTS
        pipe - a pipe handle created using CreatePipe().

    RESULT
        A valid pipe screen instance or NULL if creation was not successful.

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct pHidd_Gallium_CreatePipeScreen cpsmsg;
    struct pipe_screen                          *screen = NULL;

    if (pipe)
    {
        D(bug("[gallium.library] %s: creating pipe_screen...\n", __PRETTY_FUNCTION__));

        cpsmsg.mID = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_CreatePipeScreen);
        screen = (struct pipe_screen *)OOP_DoMethod((OOP_Object *)pipe, (OOP_Msg)&cpsmsg);
    }
    else
    {
        bug("[gallium.library] no pipe specified!\n");
    }

    return screen;

    AROS_LIBFUNC_EXIT
}

