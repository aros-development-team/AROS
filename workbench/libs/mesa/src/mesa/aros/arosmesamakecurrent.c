/*
    Copyright 2009-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include "state_tracker/st_public.h"
#include <proto/exec.h>

AROS_LH1(void, AROSMesaMakeCurrent,
    AROS_LHA(AROSMesaContext, amesa, A0),
    struct Library *, MesaBase, 8, Mesa)
{
    AROS_LIBFUNC_INIT

    SAVE_REG
    
    PUT_MESABASE_IN_REG

    /* FIXME: if passed context is the same as current context, check buffer sizes */
    /* FIXME: if there was old context active, flush it and NULL the pointers to buffers */
    
    if (amesa)
    {
        st_make_current(amesa->st, amesa->framebuffer->stfb, amesa->framebuffer->stfb);
        
        /* Resize must be done here */
        AROSMesaRecalculateBufferWidthHeight(amesa);
        st_resize_framebuffer(amesa->framebuffer->stfb, amesa->width, amesa->height);
    }
    else
    {
        /* Detach */
        st_make_current( NULL, NULL, NULL );
    }
        
    RESTORE_REG

    AROS_LIBFUNC_EXIT
}

