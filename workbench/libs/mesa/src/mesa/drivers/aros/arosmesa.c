/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id: arosmesa.c 33408 2010-05-31 08:28:26Z mazze $
*/


#include "arosmesa_internal.h"

#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "aros_driver_functions.h"
#include "aros_rb_functions.h"
#include "aros_fb_functions.h"
#include "aros_visual_functions.h"
#include "aros_context_functions.h"
#include "aros_swrast_functions.h"

#include <aros/debug.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

#include <graphics/rpattr.h>

#include <GL/arosmesa.h>

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include <GL/gl.h>
#include "context.h"
#include "extensions.h"
#include "framebuffer.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"
#include "vbo/vbo.h"

#include "renderbuffer.h"

/**********************************************************************/
/*****        Internal Data                     *****/
/**********************************************************************/

GLenum         LastError;                       /* The last error generated*/
struct Library * AROSMesaCyberGfxBase = NULL;    /* Base address for cybergfx */

/**********************************************************************/
/*****          AROSMesa API Functions                            *****/
/**********************************************************************/


/* Main AROSMesa API Functions */
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <cybergraphx/cybergraphics.h>

#if (AROS_BIG_ENDIAN == 1)
#define AROS_PIXFMT RECTFMT_ARGB32   /* Big Endian Archs. */
#else
#define AROS_PIXFMT RECTFMT_BGRA32   /* Little Endian Archs. */
#endif

static BOOL
aros_standard_init(AROSMesaContext amesa, struct TagItem *tagList)
{
    GLint requestedwidth = 0, requestedheight = 0;
    GLint requestedright = 0, requestedbottom = 0;
    
    D(bug("[AROSMESA] aros_standard_init(amesa @ %x, taglist @ %x)\n", amesa, tagList));
    D(bug("[AROSMESA] aros_standard_init: Using RastPort @ %x\n", amesa->visible_rp));

    amesa->visible_rp = CloneRastPort(amesa->visible_rp);

    D(bug("[AROSMESA] aros_standard_init: Cloned RastPort @ %x\n", amesa->visible_rp));

    /* We assume left and top are given or set to 0 */
    amesa->left = GetTagData(AMA_Left, 0, tagList);
    amesa->top = GetTagData(AMA_Top, 0, tagList);
    
    requestedright = GetTagData(AMA_Right, -1, tagList);
    requestedbottom = GetTagData(AMA_Bottom, -1, tagList);
    requestedwidth = GetTagData(AMA_Width, -1 , tagList);
    requestedheight = GetTagData(AMA_Height, -1 , tagList);

    /* Calculate rastport dimensions */
    amesa->visible_rp_width = 
        amesa->visible_rp->Layer->bounds.MaxX - amesa->visible_rp->Layer->bounds.MinX + 1;

    amesa->visible_rp_height = 
        amesa->visible_rp->Layer->bounds.MaxY - amesa->visible_rp->Layer->bounds.MinY + 1;
    
    /* right will be either passed or calculated from width or 0 */
    amesa->right = 0;
    if (requestedright < 0)
    {
        if (requestedwidth >= 0)
        {
            requestedright = amesa->visible_rp_width - amesa->left - requestedwidth;
            if (requestedright < 0) requestedright = 0;
        }
        else
            requestedright = 0;
    }
    amesa->right = requestedright;
    
    /* bottom will be either passed or calculated from height or 0 */
    amesa->bottom = 0;
    if (requestedbottom < 0)
    {
        if (requestedheight >= 0)
        {
            requestedbottom = amesa->visible_rp_height - amesa->top - requestedheight;
            if (requestedbottom < 0) requestedbottom = 0;
        }
        else
            requestedbottom = 0;
    }
    amesa->bottom = requestedbottom;

    _aros_recalculate_buffer_width_height(GET_GL_CTX_PTR(amesa));

    amesa->clearpixel = 0;   /* current drawing/clearing pens */
    
    if (amesa->screen)
    {
        if (amesa->depth == 0)
        {
            D(bug("[AROSMESA] aros_standard_init: WARNING - Illegal RastPort Depth, attempting to correct\n"));
            amesa->depth = GetCyberMapAttr(amesa->visible_rp->BitMap, CYBRMATTR_DEPTH);
        }
    }
    
    D(bug("[AROSMESA] aros_standard_init: Context Base dimensions set -:\n"));
    D(bug("[AROSMESA] aros_standard_init:    amesa->visible_rp_width        = %d\n", amesa->visible_rp_width));
    D(bug("[AROSMESA] aros_standard_init:    amesa->visible_rp_height       = %d\n", amesa->visible_rp_height));
    D(bug("[AROSMESA] aros_standard_init:    amesa->width                   = %d\n", amesa->width));
    D(bug("[AROSMESA] aros_standard_init:    amesa->height                  = %d\n", amesa->height));
    D(bug("[AROSMESA] aros_standard_init:    amesa->left                    = %d\n", amesa->left));
    D(bug("[AROSMESA] aros_standard_init:    amesa->right                   = %d\n", amesa->right));
    D(bug("[AROSMESA] aros_standard_init:    amesa->top                     = %d\n", amesa->top));
    D(bug("[AROSMESA] aros_standard_init:    amesa->bottom                  = %d\n", amesa->bottom));
    D(bug("[AROSMESA] aros_standard_init:    amesa->depth                   = %d\n", amesa->depth));

    return TRUE;
}

static void 
aros_select_rastport(AROSMesaContext amesa, struct TagItem * tagList)
{
    amesa->screen = (struct Screen *)GetTagData(AMA_Screen, 0, tagList);
    amesa->window = (struct Window *)GetTagData(AMA_Window, 0, tagList);
    amesa->visible_rp = (struct RastPort *)GetTagData(AMA_RastPort, 0, tagList);

    if (amesa->screen)
    {
        D(bug("[AROSMESA] aros_select_rastport: Screen @ %x\n", amesa->screen));
        if (amesa->window)
        {
            D(bug("[AROSMESA] aros_select_rastport: Window @ %x\n", amesa->window));
            if (!(amesa->visible_rp))
            {
                /* Use the windows rastport */
                amesa->visible_rp = amesa->window->RPort;
                D(bug("[AROSMESA] aros_select_rastport: Windows RastPort @ %x\n", amesa->visible_rp));
            }
        }
        else
        {
            if (!(amesa->visible_rp))
            {
                /* Use the screens rastport */
                amesa->visible_rp = &amesa->screen->RastPort;
                D(bug("[AROSMESA] aros_select_rastport: Screens RastPort @ %x\n", amesa->visible_rp));
            }
        }
    }
    else
    {
        /* Not passed a screen */
        if (amesa->window)
        {
            D(bug("[AROSMESA] aros_select_rastport: Window @ %x\n", amesa->window));
            /* Use the windows Screen */
            amesa->screen = amesa->window->WScreen;
            D(bug("[AROSMESA] aros_select_rastport: Windows Screen @ %x\n", amesa->screen));

            if (!(amesa->visible_rp))
            {
                /* Use the windows rastport */
                amesa->visible_rp = amesa->window->RPort;
                D(bug("[AROSMESA] aros_select_rastport: Windows RastPort @ %x\n", amesa->visible_rp));
            }
        }
        else
        {
            /* Only Passed A Rastport */
            D(bug("[AROSMESA] aros_select_rastport: Using RastPort only!\n"));
        }
    }

    D(bug("[AROSMESA] aros_select_rastport: Using RastPort @ %x\n", amesa->visible_rp));
}










AROSMesaContext AROSMesaCreateContextTags(long Tag1, ...)
{
  AROS_SLOWSTACKTAGS_PRE(Tag1)
  retval = AROSMesaCreateContext( AROS_SLOWSTACKTAGS_ARG(Tag1));
  AROS_SLOWSTACKTAGS_POST
}

AROSMesaContext AROSMesaCreateContext(struct TagItem *tagList)
{
    AROSMesaContext amesa = NULL;
    GLcontext * ctx = NULL;
    LastError = 0;
    struct dd_function_table functions;
    
    /* Try to open cybergraphics.library */
    if (CyberGfxBase == NULL)
    {
        if (!(CyberGfxBase = OpenLibrary((UBYTE*)"cybergraphics.library",0)))
            return NULL;
    }

    /* Allocate arosmesa_context struct initialized to zeros */
    if (!(amesa = (AROSMesaContext)AllocVec(sizeof(struct arosmesa_context), MEMF_PUBLIC|MEMF_CLEAR)))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: Failed to allocate AROSMesaContext\n"));
        LastError = AMESA_OUT_OF_MEM;
        return NULL;
    }

    ctx = GET_GL_CTX_PTR(amesa);

    D(bug("[AROSMESA] AROSMesaCreateContext: AROSMesaContext Allocated @ %x\n", amesa));
    D(bug("[AROSMESA] AROSMesaCreateContext:          gl_ctx Allocated @ %x\n", ctx));

    aros_select_rastport(amesa, tagList);

    if (!amesa->visible_rp)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext : No rastport provided. Exiting."));
        LastError = AMESA_RASTPORT_TAG_MISSING;
        FreeVec( amesa );
        return NULL;
    }

    D(bug("[AROSMESA] AROSMesaCreateContext: Creating new AROSMesaVisual\n"));

    if (!(amesa->visual = aros_new_visual(tagList)))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create AROSMesaVisual\n"));
        FreeVec( amesa );
        LastError = AMESA_OUT_OF_MEM;
        return NULL;
    }

    /* build table of device driver functions */
    _mesa_init_driver_functions(&functions);
    _aros_init_driver_functions(&functions);




    if (!_mesa_initialize_context(  ctx,
                                    GET_GL_VIS_PTR(amesa->visual),
                                    NULL,
                                    &functions,
                                    (void *) amesa))
    {
        _aros_destroy_visual(amesa->visual);
        FreeVec(amesa);
        return NULL;
    }

    _mesa_enable_sw_extensions(ctx);
    _mesa_enable_1_3_extensions(ctx);
    _mesa_enable_1_4_extensions(ctx);
    _mesa_enable_1_5_extensions(ctx);
    _mesa_enable_2_0_extensions(ctx);
    _mesa_enable_2_1_extensions(ctx);


    if ((aros_standard_init(amesa, tagList)) == FALSE)
    {
        goto amccontextclean;
    }


    if(!(amesa->framebuffer = aros_new_framebuffer(GET_GL_VIS_PTR(amesa->visual))))
    {
        LastError = AMESA_OUT_OF_MEM;
        goto amccontextclean;
    }

    amesa->renderbuffer = aros_new_renderbuffer();   

    _mesa_add_renderbuffer(GET_GL_FB_PTR(amesa->framebuffer), BUFFER_FRONT_LEFT,
        GET_GL_RB_PTR(amesa->renderbuffer));

    /* Set draw buffer as front */
    ctx->Color.DrawBuffer[0] = GL_FRONT;


    /* AROSMesa is always "double buffered" */
    /* if (amesa->visual->db_flag == GL_TRUE) */

    _mesa_add_soft_renderbuffers(GET_GL_FB_PTR(amesa->framebuffer),
                                GL_FALSE, /* color */
                                (GET_GL_VIS_PTR(amesa->visual))->haveDepthBuffer,
                                (GET_GL_VIS_PTR(amesa->visual))->haveStencilBuffer,
                                (GET_GL_VIS_PTR(amesa->visual))->haveAccumBuffer,
                                GL_FALSE, /* alpha */
                                GL_FALSE /* aux */ );


   
 
    ctx = GET_GL_CTX_PTR(amesa);

    _swrast_CreateContext(ctx);
    _vbo_CreateContext(ctx);
    _tnl_CreateContext(ctx);
    _swsetup_CreateContext(ctx);

    aros_swrast_initialize(ctx);

    _swsetup_Wakeup(ctx);


    /* use default TCL pipeline */
    TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

    return amesa;

amccontextclean:
    if (amesa->visual)
        _aros_destroy_visual(amesa->visual);

    _aros_destroy_context(amesa);

    return NULL;
}












#if defined (AROS_MESA_SHARED)
AROS_LH1(void, AROSMesaMakeCurrent,
    AROS_LHA(AROSMesaContext, amesa, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT
#else
void AROSMesaMakeCurrent(AROSMesaContext amesa)
{
#endif
    SAVE_REG
    
    PUT_MESABASE_IN_REG
    
    /* Make the specified context the current one */
    /* the order of operations here is very important! */
    D(bug("[AROSMESA] AROSMesaMakeCurrent(amesa @ %x)\n", amesa));


    if (amesa)
    {
        struct gl_framebuffer * fb = GET_GL_FB_PTR(amesa->framebuffer);
        GLcontext * ctx = GET_GL_CTX_PTR(amesa);
        _glapi_check_multithread();

        /* Set the framebuffer's size.  This causes the
        * osmesa_renderbuffer_storage() function to get called.
        */
        _aros_recalculate_buffer_width_height(ctx);
        _mesa_resize_framebuffer(ctx, fb, amesa->width, amesa->height);
        fb->Initialized = GL_TRUE; /* XXX TEMPORARY? */

        _mesa_make_current(ctx, fb, fb);

        /* Remove renderbuffer attachment, then re-add.  This installs the
        * renderbuffer adaptor/wrapper if needed (for bpp conversion).
        */

        /* this updates the visual's red/green/blue/alphaBits fields */
        _mesa_update_framebuffer_visual(fb);

        /* update the framebuffer size */
        _mesa_resize_framebuffer(ctx, fb, amesa->width, amesa->height);


        D(bug("[AROSMESA] AROSMesaMakeCurrent: set current mesa context/buffer\n"));

        D(bug("[AROSMESA] AROSMesaMakeCurrent: initialised rasterizer driver functions\n"));
    }

    RESTORE_REG
#if defined (AROS_MESA_SHARED)
    AROS_LIBFUNC_EXIT
#endif
}

#if defined (AROS_MESA_SHARED)
AROS_LH1(void, AROSMesaSwapBuffers,
    AROS_LHA(AROSMesaContext, amesa, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT
#else
void AROSMesaSwapBuffers(AROSMesaContext amesa)
{        
#endif
    SAVE_REG
    
    PUT_MESABASE_IN_REG
    /* copy/swap back buffer to front if applicable */

    D(bug("[AROSMESA] AROSMesaSwapBuffers(amesa @ %x)\n", amesa));

    WritePixelArray(
        amesa->renderbuffer->buffer, 
        0,
        0,
        4 * GET_GL_RB_PTR(amesa->renderbuffer)->Width, 
        amesa->visible_rp, 
        amesa->left, 
        amesa->top, 
        amesa->width, 
        amesa->height, 
        AROS_PIXFMT);
        
    RESTORE_REG
#if defined (AROS_MESA_SHARED)
    AROS_LIBFUNC_EXIT
#endif
}


#if defined (AROS_MESA_SHARED)
AROS_LH1(void, AROSMesaDestroyContext,
    AROS_LHA(AROSMesaContext, amesa, A0),
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT
#else
void AROSMesaDestroyContext(AROSMesaContext amesa)
{
#endif
    SAVE_REG
    
    PUT_MESABASE_IN_REG

    /* destroy a AROS/Mesa context */
    D(bug("[AROSMESA] AROSMesaDestroyContext(amesa @ %x)\n", amesa));

    if (!amesa)
    {
        RESTORE_REG
        return;
    }

    GLcontext * ctx = GET_GL_CTX_PTR(amesa);

    if (ctx)
    {
        GET_CURRENT_CONTEXT(cur_ctx);

        if (cur_ctx == ctx)
        {
            /* Unbind if current */
            _mesa_make_current(NULL, NULL, NULL);
        }


        _swsetup_DestroyContext(ctx);
        _swrast_DestroyContext(ctx);
        _tnl_DestroyContext(ctx);
        _vbo_DestroyContext(ctx);

        _aros_destroy_visual(amesa->visual);
        _mesa_reference_framebuffer(&amesa->framebuffer, NULL); /* So that reference count goes to 0 */
        _aros_destroy_context(amesa);
    }
    
    RESTORE_REG
#if defined (AROS_MESA_SHARED)
    AROS_LIBFUNC_EXIT
#endif
}


#if defined (AROS_MESA_SHARED)
AROS_LH0(AROSMesaContext, AROSMesaGetCurrentContext,
    struct Library *, MesaBase, 0, Mesa)
{
    AROS_LIBFUNC_INIT
#else
AROSMesaContext AROSMesaGetCurrentContext()
{
#endif
    SAVE_REG
    
    PUT_MESABASE_IN_REG

    GET_CURRENT_CONTEXT(ctx);

    RESTORE_REG;
    
    return (AROSMesaContext)ctx;

#if defined (AROS_MESA_SHARED)
    AROS_LIBFUNC_EXIT
#endif
}
