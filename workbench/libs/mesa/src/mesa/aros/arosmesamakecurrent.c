/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */

      AROS_LH1(void, AROSMesaMakeCurrent,

/*  SYNOPSIS */ 
      AROS_LHA(AROSMesaContext, amesa, A0),

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

    SAVE_REG
    
    PUT_MESABASE_IN_REG

    if (amesa)
    {
        struct st_context_iface * cur_ctx = amesa->stapi->get_current(amesa->stapi);
        
        if (amesa->st != cur_ctx)
        {
            /* Recalculate buffer dimensions */
            AROSMesaRecalculateBufferWidthHeight(amesa);

            /* Attach */
            amesa->stapi->make_current(amesa->stapi, amesa->st, 
                &amesa->framebuffer->base, &amesa->framebuffer->base);
        }            
    }
    else
    {
        /* Detach */
        amesa->stapi->make_current(amesa->stapi, NULL, NULL, NULL);
    }
        
    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

