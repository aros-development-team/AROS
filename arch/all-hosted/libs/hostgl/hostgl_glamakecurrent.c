/*
    Copyright 2011-2015, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include "hostgl_ctx_manager.h"
#include "hostgl_support.h"


/*****************************************************************************

    NAME */

      void glAMakeCurrent(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Make the selected GL rendering context active.
 
    INPUTS
        ctx - GL rendering context to be made active for all following GL
                calls.
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    HostGL_Lock();

    D(bug("[HostGL] TASK: 0x%x, MAKE CURRENT 0x%x\n", FindTask(NULL), ((struct hostgl_context *)ctx)->glXctx));

    if (ctx)
    {
        GLAContext cur_ctx = HostGL_GetCurrentContext();

        if (ctx != cur_ctx)
        {
            /* Recalculate buffer dimensions */
            HostGLRecalculateBufferWidthHeight(ctx);

            /* Attach */
            HostGL_SetCurrentContext(ctx);
            HostGL_UpdateGlobalGLXContext();
        }
    }
    else
    {
        /* Detach */
        HostGL_SetCurrentContext(NULL);
        HostGL_UpdateGlobalGLXContext();
    }

    HostGL_UnLock();
}

