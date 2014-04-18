/*
    Copyright 2011-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "arosmesa_funcs.h"
#include <proto/exec.h>

void AROSMesaGetConfig(AROSMesaContext amesa, GLenum pname, GLint * params);

/*****************************************************************************

    NAME */

      AROS_LH3(void, AROSMesaGetConfig,

/*  SYNOPSIS */ 
      AROS_LHA(AROSMesaContext, amesa, A0),
      AROS_LHA(GLenum, pname, D0),
      AROS_LHA(GLint *, params, A1),

/*  LOCATION */
      struct Library *, MesaBase, 13, Mesa)

/*  FUNCTION

        Gets value of selected parameter
 
    INPUTS

        pname - enum value of parameter

        params - pointer to integer where the value is to be put

    RESULT

        None
 
    BUGS

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROSMesaGetConfig(amesa, pname, params);

    AROS_LIBFUNC_EXIT
}

void AROSMesaGetConfig(AROSMesaContext amesa, GLenum pname, GLint * params)
{
    LONG depthbits, stencilbits, accumbits, rbbits, gbits, abits;
    
    switch(amesa->stvis.depth_stencil_format)
    {
        case(PIPE_FORMAT_S8_USCALED_Z24_UNORM): depthbits = 24; stencilbits = 8; break;
        case(PIPE_FORMAT_X8Z24_UNORM): depthbits = 24; stencilbits = 0; break;
        case(PIPE_FORMAT_Z24X8_UNORM): depthbits = 24; stencilbits = 0; break;
        case(PIPE_FORMAT_Z16_UNORM): depthbits = 16; stencilbits = 0; break;
        default: depthbits = -1; stencilbits = -1;
    }

    switch(amesa->stvis.accum_format)
    {
        case(PIPE_FORMAT_R16G16B16A16_SNORM): accumbits = 16; break;
        default: accumbits = -1;
    }

    switch(amesa->stvis.color_format)
    {
        case(PIPE_FORMAT_B5G6R5_UNORM): rbbits = 5; gbits = 6; abits = 0; break;
        case(PIPE_FORMAT_B8G8R8A8_UNORM): rbbits = 8; gbits = 8; abits = 8; break;
        default: rbbits = -1; gbits = -1; abits = 1;
    }

    if (amesa)
    {
        switch(pname)
        {
            case GL_RED_BITS:
                *params = rbbits;
                break;
            case GL_GREEN_BITS:
                *params = gbits;
                break;
            case GL_BLUE_BITS:
                *params = rbbits;
                break;
            case GL_ALPHA_BITS:
                *params = abits;
                break;
            case GL_DOUBLEBUFFER:
                *params = 1;
                break;
            case GL_DEPTH_BITS:
                *params = depthbits;
                break;
            case GL_STENCIL_BITS:
                *params = stencilbits;
                break;
            case GL_ACCUM_RED_BITS:
                *params = accumbits;
                break;
            case GL_ACCUM_GREEN_BITS:
                *params = accumbits;
                break;
            case GL_ACCUM_BLUE_BITS:
                *params = accumbits;
                break;
            case GL_ACCUM_ALPHA_BITS:
                *params = accumbits;
                break;
            case GL_STEREO:
                *params = 0;
                break;
            default:
                *params = -1;
        }
    }
}

