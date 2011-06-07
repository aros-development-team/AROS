/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <aros/debug.h>

/*****************************************************************************

    NAME */

      AROS_LH2(void, AROSMesaSetRast,

/*  SYNOPSIS */ 
      AROS_LHA(AROSMesaContext, amesa, A0),
      AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
      struct Library *, MesaBase, 12, Mesa)

/*  FUNCTION

        Sets a new rendering target for an existing context
 
    INPUTS

        tagList - a pointer to tags to be used during creation.
 
    TAGS

        AMA_Window - pointer to Window onto which scene is to be rendered. Must
                     be provided.

    RESULT

        None
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (amesa)
    {
        /* Check if at least one of window, rastport or screen have been passed */
        if ((GetTagData(AMA_Screen, 0, tagList) != 0) || 
            (GetTagData(AMA_Window, 0, tagList) != 0) ||
            (GetTagData(AMA_RastPort, 0, tagList) != 0))
        {
            /* If there already is visible_rp, free it */
            if (amesa->visible_rp)
                FreeRastPort(amesa->visible_rp);
            /* Do standard rast port selection */
            AROSMesaSelectRastPort(amesa, tagList);

            /* TODO: what about left/right/top/bottom/width/height tags - keep those values intact? */
            /* Do standard initialization */
            AROSMesaStandardInit(amesa, tagList); 

            /* TODO: what to do with visual and framebuffer, if BPP changes, we are in trouble */

            /* After the new render target has been attached, invoke framebuffer recalculation */
            AROSMesaCheckAndUpdateBufferSize(amesa);
        }
    }

    AROS_LIBFUNC_EXIT
}
