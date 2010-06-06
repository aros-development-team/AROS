/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: aros_driver_functions.c 31750 2009-08-29 14:09:49Z deadwood $
*/

#include "aros_driver_functions.h"

#include <aros/debug.h>
#include <proto/cybergraphics.h>
#include <graphics/rpattr.h>
#include <proto/graphics.h>


#include "arosmesa_internal.h"

#include <GL/arosmesa.h>
#include "swrast/swrast.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"
#include "main/framebuffer.h"

static const GLubyte * _aros_get_string(GLcontext *ctx, GLenum name)
{
    D(bug("[AROSMESA] aros_renderer_string()\n"));

    (void) ctx;
    switch (name)
    {
        case GL_RENDERER:
            return "AROS TrueColor Rasterizer";
        default:
            return NULL;
    }
}

/* implements glClearColor - Set the color used to clear the color buffer. */
static void _aros_clear_color( GLcontext *ctx, const GLfloat color[4] )
{
    AROSMesaContext amesa = (AROSMesaContext)ctx->DriverCtx;
    amesa->clearpixel = TC_ARGB((GLubyte)(color[RCOMP] * 255), 
                                (GLubyte)(color[GCOMP] * 255), 
                                (GLubyte)(color[BCOMP] * 255), 
                                (GLubyte)(color[ACOMP] * 255));
    
    D(bug("[AROSMESA] aros_clear_color(c=%x,r=%d,g=%d,b=%d,a=%d) = %x\n", 
        ctx, (GLubyte)(color[RCOMP] * 255), (GLubyte)(color[GCOMP] * 255), (GLubyte)(color[BCOMP] * 255),
        (GLubyte)(color[ACOMP] * 255), amesa->clearpixel));
}


/*
* Clear the specified region of the color buffer using the clear color
* or index as specified by one of the two functions above.
*/
static void _aros_clear(GLcontext* ctx, GLbitfield mask)
{
    AROSMesaContext amesa = (AROSMesaContext)ctx->DriverCtx;

    D(bug("[AROSMESA] _aros_clear(x:%d,y:%d,w:%d,h:%d)\n", ctx->Viewport.X, 
        ctx->Viewport.Y, ctx->Viewport.Width, ctx->Viewport.Height));

    if ((mask & (BUFFER_BIT_FRONT_LEFT)))
    {  
        if(amesa->renderbuffer != NULL)
        {
            GLint minx, miny, maxx, maxy, x, y;
            GLint width = GET_GL_RB_PTR(amesa->renderbuffer)->Width;
            GLint height = GET_GL_RB_PTR(amesa->renderbuffer)->Height;
            
            minx = ctx->Viewport.X; if (minx < 0) minx = 0;
            
            maxx = ctx->Viewport.X + ctx->Viewport.Width - 1; if (maxx > width - 1) maxx = width - 1;
            
            miny = height - ctx->Viewport.Height; if (miny < 0) miny = 0;
            
            maxy = height - ctx->Viewport.Y; if (maxy > height - 1) maxy = height - 1;
            
            D(bug("[AROSMESA] _aros_clear(minx:%d,miny:%d,maxx:%d,maxy:%d)\n", minx, miny, maxx, maxy));
            
            ULONG * dp = (ULONG*)amesa->renderbuffer->buffer;
            dp += miny * width; /* Jump to first line */
            for (y = miny; y <= maxy; y++)
            {
                dp += minx;
                for (x = minx; x <= maxx; x++, dp++)
                {
                    *dp = amesa->clearpixel;
                }
                dp += width - 1 - maxx;
            }

            mask &= ~BUFFER_BIT_FRONT_LEFT;
        }
        else
        {
            D(bug("[AROSMESA] _aros_clear: Serious ERROR amesa->front_rp = NULL detected\n"));
        }
    }

    if ((mask & (BUFFER_BIT_BACK_LEFT)))
    {  
        /* Noop */
        mask &= ~BUFFER_BIT_BACK_LEFT;
    }
  
    if (mask)
        _swrast_Clear( ctx, mask ); /* Remaining buffers to be cleared .. */
}

static void _aros_update_state(GLcontext *ctx, GLbitfield new_state)
{
    _swrast_InvalidateState(ctx, new_state);
    _swsetup_InvalidateState(ctx, new_state);
    _vbo_InvalidateState(ctx, new_state);
    _tnl_InvalidateState(ctx, new_state);
}

static void _aros_viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    AROSMesaContext amesa = (AROSMesaContext)ctx->DriverCtx;

    D(bug("[AROSMESA] _aros_viewport\n"));
    
    if (_aros_recalculate_buffer_width_height(ctx))
        _mesa_resize_framebuffer(ctx, GET_GL_FB_PTR(amesa->framebuffer), amesa->width, amesa->height);
}


GLboolean
_aros_recalculate_buffer_width_height(GLcontext *ctx)
{
    AROSMesaContext amesa = (AROSMesaContext)ctx->DriverCtx;
    
    GLsizei newwidth = 0;
    GLsizei newheight = 0;
    
    D(bug("[AROSMESA] _aros_recalculate_width_height\n"));
    
    
    amesa->visible_rp_width = 
        amesa->visible_rp->Layer->bounds.MaxX - amesa->visible_rp->Layer->bounds.MinX + 1;

    amesa->visible_rp_height = 
        amesa->visible_rp->Layer->bounds.MaxY - amesa->visible_rp->Layer->bounds.MinY + 1;


    newwidth = amesa->visible_rp_width - amesa->left - amesa->right;
    newheight = amesa->visible_rp_height - amesa->top - amesa->bottom;
    
    if (newwidth < 0) newwidth = 0;
    if (newheight < 0) newheight = 0;
    
    
    if ((newwidth != amesa->width) || (newheight != amesa->height))
    {
        /* The drawing area size has changed. Buffer must change */
        D(bug("[AROSMESA] _aros_recalculate_width_height: current height    =   %d\n", amesa->height));
        D(bug("[AROSMESA] _aros_recalculate_width_height: current width     =   %d\n", amesa->width));
        D(bug("[AROSMESA] _aros_recalculate_width_height: new height        =   %d\n", newheight));
        D(bug("[AROSMESA] _aros_recalculate_width_height: new width         =   %d\n", newwidth));
        
        amesa->width = newwidth;
        amesa->height = newheight;
        
        if (amesa->window)
        {
            struct Rectangle rastcliprect;
            struct TagItem crptags[] =
            {
                { RPTAG_ClipRectangle      , (IPTR)&rastcliprect },
                { RPTAG_ClipRectangleFlags , (RPCRF_RELRIGHT | RPCRF_RELBOTTOM) },
                { TAG_DONE }
            };
        
            D(bug("[AROSMESA] _aros_recalculate_width_height: Clipping Rastport to Window's dimensions\n"));

            /* Clip the rastport to the visible area */
            rastcliprect.MinX = amesa->left;
            rastcliprect.MinY = amesa->top;
            rastcliprect.MaxX = amesa->left + amesa->width;
            rastcliprect.MaxY = amesa->top + amesa->height;
            SetRPAttrsA(amesa->visible_rp, crptags);
        }
        
        return GL_TRUE;
    }
    
    return GL_FALSE;
}



void _aros_init_driver_functions(struct dd_function_table * functions)
{
    functions->GetString = _aros_get_string;
    functions->UpdateState = _aros_update_state;
    functions->GetBufferSize = NULL;
    functions->Clear = _aros_clear;
    functions->ClearColor = _aros_clear_color;
    functions->Viewport = _aros_viewport;
}

