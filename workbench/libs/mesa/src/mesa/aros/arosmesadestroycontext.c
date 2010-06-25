/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "state_tracker/st_public.h"
#include <proto/exec.h>
#include <proto/gallium.h>
#include <aros/debug.h>

AROS_LH1(void, AROSMesaDestroyContext,
    AROS_LHA(AROSMesaContext, amesa, A0),
    struct Library *, MesaBase, 7, Mesa)
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

