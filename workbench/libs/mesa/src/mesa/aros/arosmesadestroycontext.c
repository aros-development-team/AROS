/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "state_tracker/st_public.h"
#include <proto/exec.h>
#include <proto/gallium.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */

      AROS_LH1(void, AROSMesaDestroyContext,

/*  SYNOPSIS */ 
      AROS_LHA(AROSMesaContext, amesa, A0),

/*  LOCATION */
      struct Library *, MesaBase, 7, Mesa)

/*  FUNCTION
        Destroys the GL rendering context and frees all resoureces.
 
    INPUTS
        amesa - pointer to GL rendering context. A NULL pointer will be
                ignored.
 
    RESULT
        The GL context is destroyed. Do no use it anymore.
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    SAVE_REG
    
    PUT_MESABASE_IN_REG

    /* Destroy a AROSMesa context */
    D(bug("[AROSMESA] AROSMesaDestroyContext(amesa @ %x)\n", amesa));

    if (amesa)
    {
        GLcontext * ctx = GET_GL_CTX_PTR(amesa);

        if (ctx)
        {
            GET_CURRENT_CONTEXT(cur_ctx);

            if (cur_ctx == ctx)
            {
                /* Unbind if current */
                st_make_current( NULL, NULL, NULL );
            }

            st_finish(ctx->st);

            st_destroy_context(ctx->st);
            
            AROSMesaDestroyFrameBuffer(amesa->framebuffer);
            AROSMesaDestroyVisual(amesa->visual);
            DestroyPipeScreen(amesa->pscreen);
            AROSMesaDestroyContext(amesa);

        }
    }
    
    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

