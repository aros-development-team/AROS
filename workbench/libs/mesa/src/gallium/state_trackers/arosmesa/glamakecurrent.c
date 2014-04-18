/*
    Copyright 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>

void glAMakeCurrent(GLAContext ctx);

/*****************************************************************************

    NAME */

      AROS_LH1(void, AROSMesaMakeCurrent,

/*  SYNOPSIS */ 
      AROS_LHA(APTR, amesa, A0),

/*  LOCATION */
      struct Library *, MesaBase, 8, Mesa)

/*  FUNCTION
        Make the selected GL rendering context active.
 
    INPUTS
        amesa - GL rendering context to be made active for all following GL
                calls.
 
    RESULT
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    glAMakeCurrent(amesa);

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */

      void glAMakeCurrent(

/*  SYNOPSIS */
      GLAContext ctx)

/*  FUNCTION
        Make the selected GL rendering context active.

    INPUTS
        amesa - GL rendering context to be made active for all following GL
                calls.

    RESULT

    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    struct arosmesa_context * amesa = (struct arosmesa_context *)ctx;

    if (amesa)
    {
        struct st_context_iface * cur_ctx = glstapi->get_current(glstapi);
        
        if (amesa->st != cur_ctx)
        {
            /* Recalculate buffer dimensions */
            AROSMesaRecalculateBufferWidthHeight(amesa);

            /* Attach */
            glstapi->make_current(glstapi, amesa->st, 
                &amesa->framebuffer->base, &amesa->framebuffer->base);
        }            
    }
    else
    {
        /* Detach */
        glstapi->make_current(glstapi, NULL, NULL, NULL);
    }
}
