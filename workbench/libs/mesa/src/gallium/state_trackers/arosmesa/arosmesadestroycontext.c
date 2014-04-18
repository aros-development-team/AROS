/*
    Copyright 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "arosmesa_funcs_gallium.h"
#include <proto/exec.h>
#include <proto/gallium.h>
#include <aros/debug.h>

void AROSMesaDestroyContext(AROSMesaContext amesa);

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

    AROSMesaDestroyContext(amesa);

    AROS_LIBFUNC_EXIT
}

void AROSMesaDestroyContext(AROSMesaContext amesa)
{
    /* Destroy a AROSMesa context */
    D(bug("[AROSMESA] AROSMesaDestroyContext(amesa @ %x)\n", amesa));

    if (amesa)
    {
        struct st_context_iface * ctx = amesa->st;

        if (ctx)
        {
            struct st_context_iface * cur_ctx = glstapi->get_current(glstapi);

            if (cur_ctx == ctx)
            {
                /* Unbind if current */
                amesa->st->flush(amesa->st, 0, NULL);
                glstapi->make_current(glstapi, NULL, NULL, NULL);
            }

            amesa->st->destroy(amesa->st);
            AROSMesaFreeFrameBuffer(amesa->framebuffer);
            AROSMesaFreeStManager(amesa->stmanager);
            glstapi->destroy(glstapi);
            AROSMesaFreeContext(amesa);
        }
    }
}
