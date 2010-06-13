/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/


#include "arosmesa_internal.h"

#include "main/context.h"
#include "state_tracker/st_public.h"
#include "util/u_inlines.h"

#include <proto/exec.h>
#include <proto/utility.h>
#define DEBUG 0
#include <aros/debug.h>

#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <graphics/rpattr.h>
#include <proto/oop.h>
#include <hidd/gallium.h>
#include <aros/symbolsets.h>
#include <proto/intuition.h>

struct Library * AROSMesaCyberGfxBase = NULL;
static struct Library * AROSMesaOOPBase = NULL;
static struct Library * AROSMesaHIDDGalliumBase = NULL;
#define OOPBase AROSMesaOOPBase
static OOP_Object * driver = NULL;

/*****************************************************************************/
/*                             INIT FUNCTIONS                                */
/*****************************************************************************/

static BOOL AROSMesaHiddInit()
{
    /* Open required libraries and hidds */
    AROSMesaCyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0);
    AROSMesaOOPBase = OpenLibrary((STRPTR)"oop.library", 0);
    AROSMesaHIDDGalliumBase = OpenLibrary((STRPTR)"gallium.hidd", 4);

    if ((!AROSMesaCyberGfxBase) || (!AROSMesaOOPBase) || (!AROSMesaHIDDGalliumBase))
        return FALSE;

    return TRUE;
}

static BOOL AROSMesaHiddExit()
{
    /* Close libraries and hidds. Do not dispose the driver. It is handled in
       gallium.hidd */
    driver = NULL;

    if (AROSMesaHIDDGalliumBase)
        CloseLibrary(AROSMesaHIDDGalliumBase);

    if (AROSMesaOOPBase)
        CloseLibrary(AROSMesaOOPBase);

    if (AROSMesaCyberGfxBase)
        CloseLibrary(AROSMesaCyberGfxBase);

    return TRUE;
}

ADD2INIT(AROSMesaHiddInit, 0);
ADD2EXIT(AROSMesaHiddExit, 0);

/*****************************************************************************/
/*                             PRIVATE FUNCTIONS                             */
/*****************************************************************************/

static VOID AROSMesaHiddDestroyVisual(AROSMesaVisual aros_vis)
{
    if (aros_vis)
    {
        FreeVec(aros_vis);
    }
}

static VOID AROSMesaHiddSelectColorFormat(GLint bpp, struct pipe_screen * screen, 
    GLint * redBits, GLint * greenBits, GLint * blueBits, GLint * alphaBits,
    enum pipe_format * colorFormat)
{
    *redBits        = 0;
    *greenBits      = 0;
    *blueBits       = 0;
    *alphaBits      = 0;
    *colorFormat    = PIPE_FORMAT_NONE;

    if (bpp == 16)
    {
        /* Try PIPE_FORMAT_B5G6R5_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
        {
            *redBits        = 5;
            *greenBits      = 6;
            *blueBits       = 5;
            *alphaBits      = 0;
            *colorFormat    = PIPE_FORMAT_B5G6R5_UNORM;
        }
    }
    
    if (bpp == 32)
    {
        /* Try PIPE_FORMAT_B8G8R8A8_UNORM */
        if (screen->is_format_supported(screen, PIPE_FORMAT_B5G6R5_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_RENDER_TARGET, 0))
        {
            *redBits        = 8;
            *greenBits      = 8;
            *blueBits       = 8;
            *alphaBits      = 8;
            *colorFormat    = PIPE_FORMAT_B8G8R8A8_UNORM;
        }
    }    
}

static VOID AROSMesaHiddSelectDepthStencilFormat(struct pipe_screen * screen,
    GLint * depthBits, enum pipe_format * depthFormat, 
    GLint * stencilBits, enum pipe_format * stencilFormat)
{
    /* Defeaul values */
    *depthBits      = 0;
    *depthFormat    = PIPE_FORMAT_NONE;
    *stencilBits    = 0;
    *stencilFormat  = PIPE_FORMAT_NONE;
    
    /* Try PIPE_FORMAT_S8Z24_UNORM */
    if((*depthFormat == PIPE_FORMAT_NONE) && (*stencilFormat == PIPE_FORMAT_NONE) &&
        (screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0)))
    {
        *depthBits      = 24;
        *depthFormat    = PIPE_FORMAT_S8Z24_UNORM;
        *stencilBits    = 8;
        *stencilFormat  = PIPE_FORMAT_S8Z24_UNORM;
    }
    
    /* Try PIPE_FORMAT_Z16_UNORM */
    if((*depthFormat == PIPE_FORMAT_NONE) && 
        (screen->is_format_supported(screen, PIPE_FORMAT_Z16_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0)))
    {
        *depthBits      = 16;
        *depthFormat    = PIPE_FORMAT_Z16_UNORM;
    }
    
    /* Try PIPE_FORMAT_Z16_UNORM */
    if((*stencilFormat == PIPE_FORMAT_NONE) && 
        (screen->is_format_supported(screen, PIPE_FORMAT_S8_UNORM,
            PIPE_TEXTURE_2D, PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0)))
    {
        *stencilBits    = 8;
        *stencilFormat  = PIPE_FORMAT_S8_UNORM;
    }
}

static AROSMesaVisual AROSMesaHiddNewVisual(GLint bpp, struct pipe_screen * screen, struct TagItem *tagList)
{
    AROSMesaVisual aros_vis = NULL;
    GLvisual * vis = NULL;
    GLint  redBits, greenBits, blueBits, alphaBits, accumBits;
    GLint depthBits, stencilBits;
    BOOL noDepth, noStencil, noAccum;
    
    D(bug("[AROSMESA] AROSMesaHiddNewVisual\n"));
    
    noStencil   = GetTagData(AMA_NoStencil, GL_FALSE, tagList);
    noAccum     = GetTagData(AMA_NoAccum, GL_FALSE, tagList);
    noDepth     = GetTagData(AMA_NoDepth, GL_FALSE, tagList);

    /* Allocate memory for aros structure */
    aros_vis = AllocVec(sizeof(struct arosmesa_visual), MEMF_PUBLIC | MEMF_CLEAR);

    if (!aros_vis)
        return NULL;

    /* Color buffer */
    AROSMesaHiddSelectColorFormat(bpp, screen, &redBits, &greenBits, &blueBits,
        &alphaBits, &aros_vis->ColorFormat);
    if (aros_vis->ColorFormat == PIPE_FORMAT_NONE)
    {
        D(bug("[AROSMESA] AROSMesaHiddNewVisual - ERROR - No supported color format found\n"));        
        AROSMesaHiddDestroyVisual(aros_vis);
        return NULL;        
    } 
    
    /* Z-buffer / Stencil buffer */
    AROSMesaHiddSelectDepthStencilFormat(screen, &depthBits, &aros_vis->DepthFormat, 
        &stencilBits, &aros_vis->StencilFormat);
    if (noDepth)
    {
        depthBits = 0;
        aros_vis->DepthFormat = PIPE_FORMAT_NONE;
    }
    else
        if (aros_vis->DepthFormat == PIPE_FORMAT_NONE)
        {
            D(bug("[AROSMESA] AROSMesaHiddNewVisual - ERROR - No supported depth format found\n"));        
            AROSMesaHiddDestroyVisual(aros_vis);
            return NULL;        
        }

    if (noStencil)
    {
        stencilBits = 0;
        aros_vis->StencilFormat = PIPE_FORMAT_NONE;
    }
    else
        if (aros_vis->StencilFormat == PIPE_FORMAT_NONE)
        {
            D(bug("[AROSMESA] AROSMesaHiddNewVisual - ERROR - No supported stencil format found\n"));        
            AROSMesaHiddDestroyVisual(aros_vis);
            return NULL;        
        }

    /* Accum buffer */
    if (noAccum)
        accumBits = 0;
    else
        accumBits = 16;
    
    
    /* AMA_RGBMode, AMA_DoubleBuf and AMA_AlphaFlag are always GL_TRUE in this implementation */

    vis = GET_GL_VIS_PTR(aros_vis);

    /* Initialize mesa structure */
    if(!_mesa_initialize_visual(vis,
                                GL_FALSE,       /* Double buffer - AROSMesa uses front buffer as back buffer */
                                GL_FALSE,       /* stereo */
                                redBits,
                                greenBits,
                                blueBits,
                                alphaBits,
                                depthBits,
                                stencilBits,
                                accumBits,
                                accumBits,
                                accumBits,
                                alphaBits ? accumBits : 0,
                                0))
    {
        AROSMesaHiddDestroyVisual(aros_vis);
        return NULL;
    }

    return aros_vis;
}

static VOID AROSMesaHiddDestroyContext(AROSMesaContext amesa)
{
    if (amesa)
    {
        FreeVec(amesa);
    }
}

static AROSMesaFrameBuffer AROSMesaHiddNewFrameBuffer(AROSMesaContext amesa, AROSMesaVisual visual)
{
    AROSMesaFrameBuffer aros_fb = NULL;
    GLvisual * vis = GET_GL_VIS_PTR(visual);
    
    D(bug("[AROSMESA] AROSMesaHiddNewFrameBuffer\n"));

    /* Allocate memory for aros structure */
    aros_fb = AllocVec(sizeof(struct arosmesa_framebuffer), MEMF_PUBLIC | MEMF_CLEAR);

    if (!aros_fb)
        return NULL;
    
    /* Create framebuffer */
    aros_fb->stfb = st_create_framebuffer(vis,
                                    visual->ColorFormat, visual->DepthFormat, 
                                    visual->StencilFormat, amesa->width, 
                                    amesa->height, (void *) aros_fb);    
    
    return aros_fb;
}

static VOID AROSMesaHiddDestroyFrameBuffer(AROSMesaFrameBuffer aros_fb)
{
    if (aros_fb)
    {
        if (aros_fb->stfb)
        {
            /* So that reference count goes to 0 and buffer is freed */
            st_unreference_framebuffer(aros_fb->stfb);
            aros_fb->stfb = NULL;
        }
        
        FreeVec(aros_fb);
    }
}

static VOID AROSMesaHiddSelectRastPort(AROSMesaContext amesa, struct TagItem * tagList)
{
    amesa->ScreenInfo.Screen = (struct Screen *)GetTagData(AMA_Screen, 0, tagList);
    amesa->window = (struct Window *)GetTagData(AMA_Window, 0, tagList);
    amesa->visible_rp = (struct RastPort *)GetTagData(AMA_RastPort, 0, tagList);

    if (amesa->ScreenInfo.Screen)
    {
        D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Screen @ %x\n", amesa->ScreenInfo.Screen));
        if (amesa->window)
        {
            D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Window @ %x\n", amesa->window));
            if (!(amesa->visible_rp))
            {
                /* Use the windows rastport */
                amesa->visible_rp = amesa->window->RPort;
                D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Windows RastPort @ %x\n", amesa->visible_rp));
            }
        }
        else
        {
            if (!(amesa->visible_rp))
            {
                /* Use the screens rastport */
                amesa->visible_rp = &amesa->ScreenInfo.Screen->RastPort;
                D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Screens RastPort @ %x\n", amesa->visible_rp));
            }
        }
    }
    else
    {
        /* Not passed a screen */
        if (amesa->window)
        {
            D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Window @ %x\n", amesa->window));
            /* Use the windows Screen */
            amesa->ScreenInfo.Screen = amesa->window->WScreen;
            D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Windows Screen @ %x\n", amesa->ScreenInfo.Screen));

            if (!(amesa->visible_rp))
            {
                /* Use the windows rastport */
                amesa->visible_rp = amesa->window->RPort;
                D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Windows RastPort @ %x\n", amesa->visible_rp));
            }
        }
        else
        {
            /* Only Passed A Rastport */
            D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Using RastPort only!\n"));
        }
    }

    D(bug("[AROSMESA] AROSMesaHiddSelectRastPort: Using RastPort @ %x\n", amesa->visible_rp));
}

static GLboolean AROSMesaHiddRecalculateBufferWidthHeight(AROSMesaContext amesa)
{
    GLsizei newwidth = 0;
    GLsizei newheight = 0;
    
    D(bug("[AROSMESA] AROSMesaHiddRecalculateBufferWidthHeight\n"));
    
    
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
        D(bug("[AROSMESA] AROSMesaHiddRecalculateBufferWidthHeight: current height    =   %d\n", amesa->height));
        D(bug("[AROSMESA] AROSMesaHiddRecalculateBufferWidthHeight: current width     =   %d\n", amesa->width));
        D(bug("[AROSMESA] AROSMesaHiddRecalculateBufferWidthHeight: new height        =   %d\n", newheight));
        D(bug("[AROSMESA] AROSMesaHiddRecalculateBufferWidthHeight: new width         =   %d\n", newwidth));
        
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
        
            D(bug("[AROSMESA] AROSMesaHiddRecalculateBufferWidthHeight: Clipping Rastport to Window's dimensions\n"));

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

static VOID AROSMesaHiddCheckaAndUpdateBufferSize(AROSMesaContext amesa)
{
    if (AROSMesaHiddRecalculateBufferWidthHeight(amesa))
        st_resize_framebuffer(amesa->framebuffer->stfb, amesa->width, amesa->height);
}

static BOOL AROSMesaHiddStandardInit(AROSMesaContext amesa, struct TagItem *tagList)
{
    GLint requestedwidth = 0, requestedheight = 0;
    GLint requestedright = 0, requestedbottom = 0;

    D(bug("[AROSMESA] AROSMesaHiddStandardInit(amesa @ %x, taglist @ %x)\n", amesa, tagList));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit: Using RastPort @ %x\n", amesa->visible_rp));

    amesa->visible_rp = CloneRastPort(amesa->visible_rp);

    D(bug("[AROSMESA] AROSMesaHiddStandardInit: Cloned RastPort @ %x\n", amesa->visible_rp));

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
    
    /* Init screen information */
    if (amesa->ScreenInfo.Screen)
    {
        amesa->ScreenInfo.Depth         = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_DEPTH);
        amesa->ScreenInfo.BitsPerPixel  = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_BPPIX) * 8;
        amesa->ScreenInfo.Width         = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_WIDTH);
        amesa->ScreenInfo.Height        = GetCyberMapAttr(amesa->ScreenInfo.Screen->RastPort.BitMap, CYBRMATTR_HEIGHT);
    }
    
    D(bug("[AROSMESA] AROSMesaHiddStandardInit: Context Base dimensions set -:\n"));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->visible_rp_width        = %d\n", amesa->visible_rp_width));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->visible_rp_height       = %d\n", amesa->visible_rp_height));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->width                   = %d\n", amesa->width));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->height                  = %d\n", amesa->height));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->left                    = %d\n", amesa->left));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->right                   = %d\n", amesa->right));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->top                     = %d\n", amesa->top));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->bottom                  = %d\n", amesa->bottom));
    D(bug("[AROSMESA] AROSMesaHiddStandardInit:    amesa->depth                   = %d\n", amesa->ScreenInfo.Depth));

    return TRUE;
}


/* FIXME HACK FIXME */
#include <proto/layers.h>

static VOID HACK_BlitSurcaceOnRastport(AROSMesaContext amesa, struct pipe_surface * surf)
{
    struct Layer *L = amesa->visible_rp->Layer;
    struct ClipRect *CR;
    struct Rectangle renderableLayerRect;
    
    /* Render only if screen is visible */
    if (amesa->window->WScreen != IntuitionBase->FirstScreen)
        return;

    if (!IsLayerVisible(L))
        return;

    LockLayerRom(L);
    
    renderableLayerRect.MinX = L->bounds.MinX + amesa->left;
    renderableLayerRect.MaxX = L->bounds.MaxX - amesa->right;
    renderableLayerRect.MinY = L->bounds.MinY + amesa->top;
    renderableLayerRect.MaxY = L->bounds.MaxY - amesa->bottom;
    
    /*  Do not clip renderableLayerRect to screen rast port. CRs are already clipped and unclipped 
        layer coords are needed: see surface_copy */
    
    CR = L->ClipRect;
    
    for (;NULL != CR; CR = CR->Next)
    {
        D(bug("Cliprect (%d, %d, %d, %d), lobs=%p\n",
            CR->bounds.MinX, CR->bounds.MinY, CR->bounds.MaxX, CR->bounds.MaxY,
            CR->lobs));

        /* I assume this means the cliprect is visible */
        if (NULL == CR->lobs)
        {
            struct Rectangle result;
            
            if (AndRectRect(&renderableLayerRect, &CR->bounds, &result))
            {
                /* This clip rect intersects renderable layer rect */
                struct pHidd_Gallium_DisplaySurface dsmsg = {
                mID : OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplaySurface),
                context : amesa->st->pipe,
                rastport : amesa->visible_rp,
                left : result.MinX - L->bounds.MinX - amesa->left, /* x in the source buffer */
                top : result.MinY - L->bounds.MinY - amesa->top, /* y in the source buffer */
                width : result.MaxX - result.MinX + 1, /* width of the rect in source buffer */
                height : result.MaxY - result.MinY + 1, /* height of the rect in source buffer */
                surface : surf,
                absx : result.MinX, /* Absolute (on screen) X of dest blit */
                absy : result.MinY, /* Absolute (on screen) Y of the dest blit */
                relx : result.MinX - L->bounds.MinX, /* Relative (on rastport) X of the desc blit */
                rely : result.MinY - L->bounds.MinY /* Relative (on rastport) Y of the desc blit */
                };
                OOP_DoMethod(driver, (OOP_Msg)&dsmsg);
                            
                /* FIXME: clip last 4 parameters to actuall surface deminsions */
/*                pipe->surface_copy(pipe, visiblescreen, 
                            result.MinX, 
                            result.MinY, 
                            surface, 
                            result.MinX - L->bounds.MinX - msg->left, 
                            result.MinY - L->bounds.MinY - msg->top, 
                            result.MaxX - result.MinX + 1, 
                            result.MaxY - result.MinY + 1);*/
            }
        }
    }

    /* Flush all copy operations done */
//    pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);


    UnlockLayerRom(L);
}

/* FIXME HACK FIXME */




/*****************************************************************************/
/*                             PUBLIC FUNCTIONS                              */
/*****************************************************************************/

AROSMesaContext AROSMesaCreateContextTags(long Tag1, ...)
{
  AROS_SLOWSTACKTAGS_PRE(Tag1)
  retval = AROSMesaCreateContext(AROS_SLOWSTACKTAGS_ARG(Tag1));
  AROS_SLOWSTACKTAGS_POST
}

AROSMesaContext AROSMesaCreateContext(struct TagItem *tagList)
{
    AROSMesaContext amesa = NULL;
    struct pipe_screen * screen = NULL;
    struct pipe_context * pipe = NULL;
    struct pHidd_Gallium_CreatePipeScreen cpsmsg;

    if (driver == NULL)
    {
        /* Create gallium driver factory */
        OOP_Object * galliumdriverfactory = 
            OOP_NewObject(NULL, CLID_Hidd_GalliumDriverFactory, NULL);
        
        if (galliumdriverfactory)
        {
            /* Ask for gallium driver */
            driver = HIDD_GalliumDriverFactory_GetDriver(galliumdriverfactory);

            /* Dispose factory */
            OOP_DisposeObject(galliumdriverfactory);
        }
    }

    /* Check if driver is acquired */
    if (!driver)
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR - failed to retrieve Gallium3D driver\n"));
        return NULL;
    }
    
    /* Allocate arosmesa_context struct initialized to zeros */
    if (!(amesa = (AROSMesaContext)AllocVec(sizeof(struct arosmesa_context), MEMF_PUBLIC|MEMF_CLEAR)))
    {
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR - failed to allocate AROSMesaContext\n"));
        return NULL;
    }
    
    /* FIXME; shouldn't RastPort be part of framebuffer? */
    AROSMesaHiddSelectRastPort(amesa, tagList);
    
    /* FIXME: check if any rastport is available */
    
    /* FIXME: later this might be placed in initialization of framebuffer */
    AROSMesaHiddStandardInit(amesa, tagList);   


    cpsmsg.mID = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_CreatePipeScreen);
    screen = (struct pipe_screen *)OOP_DoMethod(driver, (OOP_Msg)&cpsmsg);
    
    if (!screen)
    {
        /* TODO: Route error handling to one place */
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create gallium driver screen\n"));
        FreeVec(amesa);
        return NULL;
    }

    
    D(bug("[AROSMESA] AROSMesaCreateContext: Creating new AROSMesaVisual\n"));

    if (!(amesa->visual = AROSMesaHiddNewVisual(amesa->ScreenInfo.BitsPerPixel, screen, tagList)))
    {
        /* TODO: Route error handling to one place */
        /* TODO: Destroy screen */
        D(bug("[AROSMESA] AROSMesaCreateContext: ERROR -  failed to create AROSMesaVisual\n"));
        FreeVec( amesa );
        return NULL;
    }

    pipe = screen->context_create(screen, NULL);
    
    /* FIXME: If pipe == NULL */
    
    amesa->st = st_create_context(pipe, GET_GL_VIS_PTR(amesa->visual), NULL);
    
    /* Set up some needed pointers */
    amesa->st->ctx->DriverCtx = amesa;
    pipe->priv = amesa;
    
    /* Initial update of buffer dimensions (amesa->width/amesa->height) */
    AROSMesaHiddRecalculateBufferWidthHeight(amesa);
    
    
    /* FIXME: Provide rastport to framebuffer ? */
    amesa->framebuffer = AROSMesaHiddNewFrameBuffer(amesa, amesa->visual);
    
    return amesa;
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

    /* FIXME: if passed context is the same as current context, check buffer sizes */
    /* FIXME: if there was old context active, flush it and NULL the pointers to buffers */
    
    if (amesa)
    {
        st_make_current(amesa->st, amesa->framebuffer->stfb, amesa->framebuffer->stfb);
        
        /* Resize must be done here */
        AROSMesaHiddRecalculateBufferWidthHeight(amesa);
        st_resize_framebuffer(amesa->framebuffer->stfb, amesa->width, amesa->height);
    }
    else
    {
        /* Detach */
        st_make_current( NULL, NULL, NULL );
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

    struct pipe_surface *surf;

    /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
    st_notify_swapbuffers(amesa->framebuffer->stfb);

    /* FIXME: should be ST_SURFACE_BACK_LEFT */
    st_get_framebuffer_surface(amesa->framebuffer->stfb, ST_SURFACE_FRONT_LEFT, &surf);

    if (surf) 
    {
        HACK_BlitSurcaceOnRastport(amesa, surf);
    }

    /* Flush. Executes all code posted in DisplaySurface */
    st_flush(amesa->st, PIPE_FLUSH_RENDER_CACHE, NULL);

    AROSMesaHiddCheckaAndUpdateBufferSize(amesa);

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
        return;

    GLcontext * ctx = GET_GL_CTX_PTR(amesa);

    if (ctx)
    {
        struct pHidd_Gallium_DestroyPipeScreen dpsmsg = {
        mID : OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DestroyPipeScreen),
        screen : ctx->st->pipe->screen
        };
        
        GET_CURRENT_CONTEXT(cur_ctx);

        if (cur_ctx == ctx)
        {
            /* Unbind if current */
            st_make_current( NULL, NULL, NULL );
        }

        st_finish(ctx->st);

        st_destroy_context(ctx->st);
        
        AROSMesaHiddDestroyFrameBuffer(amesa->framebuffer);
        AROSMesaHiddDestroyVisual(amesa->visual);
        AROSMesaHiddDestroyContext(amesa);

        OOP_DoMethod(driver, (OOP_Msg)&dpsmsg);
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

