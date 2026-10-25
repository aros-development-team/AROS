/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.
*/

#include "nouveau_intern.h"

unsigned long nouveau_compat_irq_count(void);
unsigned long nouveau_compat_usecs(void);

#define DEBUG 0
#include <aros/debug.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <libdrm/arosdrmmode.h>

#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddBitMapNouveauAttrBase

#define HiddBitMapAttrBase          (SD(cl)->bitMapAttrBase)
#define HiddPixFmtAttrBase          (SD(cl)->pixFmtAttrBase)
#define HiddBitMapNouveauAttrBase   (SD(cl)->bitMapNouveauAttrBase)

#define GART_TRANSFER_ALLOWED(width, height)    ((((width) * (height)) >= (32 * 32)) && (carddata->GART))

VOID HIDDNouveauSetOffsets(OOP_Object * bm, LONG newxoffset, LONG newyoffset)
{
    OOP_Class * cl = OOP_OCLASS(bm);
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, bm);
    bmdata->xoffset = newxoffset;
    bmdata->yoffset = newyoffset;
}

/* Video bitmaps (aHidd_BitMap_VideoFormat != 0) are scan-out only: no pixel method touches them */
#define VIDEO_BITMAP_REFUSED(bmdata, result)  if ((bmdata)->videoformat) return result

/* PUBLIC METHODS */
OOP_Object * METHOD(NouveauBitMap, Root, New)
{
    IPTR width, height, depth, displayable, bytesperpixel;
    IPTR scanout = FALSE, buffers = 1, videoformat = vHidd_VideoFmt_None;
    OOP_Object * pf;
    struct HIDDNouveauBitMapData * bmdata = NULL;
    HIDDT_StdPixFmt stdfmt = vHidd_StdPixFmt_Unknown;
    struct CardData * carddata = &(SD(cl)->carddata);
    ULONG i;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (!o)
        goto exit_fail;

    bmdata = OOP_INST_DATA(cl, o);

    /* Initialize default values */
    bmdata->fbid = 0;
    bmdata->xoffset = 0;
    bmdata->yoffset = 0;
    bmdata->bo = NULL;
    bmdata->nbufs = 0;
    
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (APTR)&pf);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &displayable);
    OOP_GetAttr(o, aHidd_BitMap_Scanout, &scanout);
    OOP_GetAttr(o, aHidd_BitMap_Buffers, &buffers);
    OOP_GetAttr(o, aHidd_BitMap_VideoFormat, &videoformat);
    OOP_GetAttr(pf, aHidd_PixFmt_StdPixFmt, &stdfmt);
    OOP_GetAttr(pf, aHidd_PixFmt_BytesPerPixel, &bytesperpixel);
    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
    
    D(bug("[Nouveau] BitMap New: %d x %d x %d\n", width, height, depth));

    /*
     * Formats: the screen formats as before; a scan-out bitmap may also be
     * BGRA32 (a GL colour buffer, registered as XRGB8888 - the plane blends
     * it opaque) or a packed video format (the base class gave it a private
     * 2 bytes per pixel pixfmt of colour model Video; only a plane reads it).
     */
    if (videoformat)
    {
        switch (videoformat)
        {
        case vHidd_VideoFmt_YUYV:
        case vHidd_VideoFmt_UYVY:
            if (bytesperpixel != 2)
                goto exit_fail;
            break;
        case vHidd_VideoFmt_NV12:
        case vHidd_VideoFmt_I420:
            if ((bytesperpixel != 1) || (width & 1) || (height & 1))
                goto exit_fail;
            break;
        default:
            goto exit_fail;
        }
        scanout = TRUE;
    }
    else
    {
        if ((stdfmt != vHidd_StdPixFmt_BGR032) && (stdfmt != vHidd_StdPixFmt_RGB16_LE) &&
            !(scanout && (stdfmt == vHidd_StdPixFmt_BGRA32)))
            goto exit_fail;

        /* Check if requested depth is a supported one */
        if (depth < 16)
            goto exit_fail;
    }
    
    /* Check if requested byted per pixel is a supported one */
    if ((bytesperpixel != 2) && (bytesperpixel != 4) && !videoformat)
        goto exit_fail;

    /* Initialize properties */
    bmdata->drawable.width = width;
    bmdata->drawable.height = height;
    bmdata->drawable.depth = bmdata->drawable.bitsPerPixel = depth;
    bmdata->drawable.pScreen = carddata;
    bmdata->bytesperpixel = bytesperpixel;
    bmdata->stdfmt = stdfmt;
    bmdata->videoformat = videoformat;

    if (displayable) bmdata->displayable = TRUE; else bmdata->displayable = FALSE;
    InitSemaphore(&bmdata->semaphore);

    if (scanout)
    {
        /*
         * Scan-out bitmap: N buffers of the same size and pitch, each a
         * contiguous linear VRAM surface a plane can show (the display
         * pitch alignment is HIDDNouveauAccelAllocSurface's). bo/fbid alias
         * the front buffer.
         */
        /*
         * Planar 4:2:0 (contract "Planar 4:2:0"): the Y plane is the bitmap's
         * width x height at the surface pitch (256-aligned on Tesla+, so the
         * chroma pitch pitch/2 is the multiple of 64 the window needs), the
         * chroma planes follow in the same buffer: I420 = U then V at
         * pitch/2 x height/2 each, NV12 = interleaved UV at pitch x
         * height/2; each 256-byte aligned. Allocated as extra rows.
         */
        ULONG rows = height;
        ULONG chromarows = (height + 1) / 2;

        if ((videoformat == vHidd_VideoFmt_NV12) || (videoformat == vHidd_VideoFmt_I420))
            rows += chromarows + 2;             /* + slack for the 256-byte rounding */
        if (buffers < 1)
            buffers = 1;
        if (buffers > NOUVEAU_MAX_BUFFERS)
            buffers = NOUVEAU_MAX_BUFFERS;
        LOCK_ENGINE
        for (i = 0; i < buffers; i++)
        {
            ULONG pitch = 0;

            bmdata->buf[i].bo = NULL;
            bmdata->buf[i].fbid = 0;
            HIDDNouveauAccelAllocSurface(carddata, width, rows, bytesperpixel * 8, &pitch, &bmdata->buf[i].bo);
            if (!bmdata->buf[i].bo)
                break;
            if (i == 0)
                bmdata->pitch = pitch;
            else if (pitch != bmdata->pitch)
            {
                nouveau_bo_ref(NULL, &bmdata->buf[i].bo);
                break;
            }
            bmdata->nbufs = i + 1;
        }
        UNLOCK_ENGINE
        if (bmdata->nbufs != buffers)
            goto exit_fail;
        bmdata->front = 0;
        bmdata->back = (bmdata->nbufs > 1) ? 1 : 0;
        bmdata->bo = bmdata->buf[0].bo;
        bmdata->planeoff[0] = bmdata->planeoff[1] = 0;
        if (videoformat == vHidd_VideoFmt_NV12)
            bmdata->planeoff[0] = (bmdata->pitch * height + 255) & ~255UL;
        else if (videoformat == vHidd_VideoFmt_I420)
        {
            bmdata->planeoff[0] = (bmdata->pitch * height + 255) & ~255UL;
            bmdata->planeoff[1] = (bmdata->planeoff[0] + (bmdata->pitch / 2) * chromarows + 255) & ~255UL;
            if ((bmdata->pitch & 127) ||
                (bmdata->planeoff[1] + (bmdata->pitch / 2) * chromarows > bmdata->bo->size))
                goto exit_fail;
        }
        nvlog("[Nouveau] scan-out bitmap %p: %ldx%ld %s%ld pitch %ld planes +%lu +%lu, %lu buffer(s)\n", o,
              (long)width, (long)height, videoformat ? "video format " : "depth ",
              videoformat ? (long)videoformat : (long)depth, (long)bmdata->pitch,
              (unsigned long)bmdata->planeoff[0], (unsigned long)bmdata->planeoff[1], (unsigned long)bmdata->nbufs);
        return o;
    }

    LOCK_ENGINE
    /* Creation of buffer object */
    HIDDNouveauAccelAllocSurface(&SD(cl)->carddata, bmdata->drawable.width, bmdata->drawable.height,
        bmdata->bytesperpixel * 8, &bmdata->pitch, &bmdata->bo);
    UNLOCK_ENGINE

    if (bmdata->bo == NULL)
        goto exit_fail;

    if (displayable)
        nvlog("[Nouveau] displayable bitmap %p: %ldx%ld depth %ld pitch %ld bo %p\n", o,
              (long)width, (long)height, (long)depth, (long)bmdata->pitch, bmdata->bo);

    return o;

exit_fail:

    nvlog("[Nouveau]: Failed to create bitmap %dx%d %d %d\n", width, height, depth, stdfmt);

    if (o)
    {
        OOP_MethodID disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
    }

    return NULL;
}

VOID NouveauBitMap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    D(bug("[Nouveau] Dispose %x\n", o));

    LOCK_ENGINE
    /* Unregister from framebuffer if needed */
    if (bmdata->fbid != 0)
    {
        struct nouveau_device *nvdev = SD(cl)->carddata.dev;
        drmModeRmFB(NOUVEAU_DEV_FD(nvdev), bmdata->fbid);   
        bmdata->fbid = 0;
        if (bmdata->nbufs)
            bmdata->buf[bmdata->front].fbid = 0;
    }

    if (bmdata->nbufs)
    {
        /* Scan-out bitmap: bo aliases buf[front]; free every buffer */
        struct nouveau_device *nvdev = SD(cl)->carddata.dev;
        struct CardData *carddata = &(SD(cl)->carddata);
        ULONG i;

        if (bmdata->gpu_dirty)
        {
            if (carddata->pushbuf)
                nouveau_pushbuf_kick(carddata->pushbuf);
            if (carddata->ce_enabled && carddata->ce_pushbuf)
                nouveau_pushbuf_kick(carddata->ce_pushbuf);
            bmdata->gpu_dirty = FALSE;
        }
        for (i = 0; i < bmdata->nbufs; i++)
        {
            if (bmdata->buf[i].fbid)
                drmModeRmFB(NOUVEAU_DEV_FD(nvdev), bmdata->buf[i].fbid);
            bmdata->buf[i].fbid = 0;
            if (bmdata->buf[i].bo)
                nouveau_bo_ref(NULL, &bmdata->buf[i].bo);
        }
        bmdata->bo = NULL;
        bmdata->nbufs = 0;
    }

    if (bmdata->bo)
    {
        /* Engine work is submitted lazily: an operation still queued
           here has no fence yet, so the kernel would free the bo from
           under it. Submitting attaches the fence, after which the
           kernel keeps the bo and its mapping alive until the engine is
           done (nouveau_gem_object_unmap defers on the fence, ttm delays
           the delete). No need to wait here. */
        if (bmdata->gpu_dirty)
        {
            struct CardData *carddata = &(SD(cl)->carddata);

            if (carddata->pushbuf)
                nouveau_pushbuf_kick(carddata->pushbuf);
            if (carddata->ce_enabled && carddata->ce_pushbuf)
                nouveau_pushbuf_kick(carddata->ce_pushbuf);
            bmdata->gpu_dirty = FALSE;
        }
        nouveau_bo_ref(NULL, &bmdata->bo); /* Release reference */
    }
    UNLOCK_ENGINE

    OOP_DoSuperMethod(cl, o, msg);
}

VOID METHOD(NouveauBitMap, Root, Get)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    ULONG idx;

    if (IS_BITMAP_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_BitMap_LeftEdge:
            *msg->storage = bmdata->xoffset;
            return;
        case aoHidd_BitMap_TopEdge:
            *msg->storage = bmdata->yoffset;
            return;
        case aoHidd_BitMap_FrontBuffer:
            *msg->storage = bmdata->nbufs ? bmdata->front : 0;
            return;
        case aoHidd_BitMap_BackBuffer:
            *msg->storage = bmdata->nbufs ? bmdata->back : 0;
            return;
        case aoHidd_BitMap_BufferHandle:
            *msg->storage = bmdata->nbufs ? bmdata->buf[bmdata->back].bo->handle : 0;
            return;
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NouveauBitMap, Root, Set)
{
    struct TagItem  *tag, *tstate;
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    ULONG idx;
    LONG newxoffset = bmdata->xoffset;
    LONG newyoffset = bmdata->yoffset;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_LeftEdge:
                newxoffset = tag->ti_Data;
                break;
            case aoHidd_BitMap_TopEdge:
                newyoffset = tag->ti_Data;
                break;
            }
        }
    }

    if ((newxoffset != bmdata->xoffset) || (newyoffset != bmdata->yoffset))
    {
        /* The display's compositor validated the position already */
        HIDDNouveauSetOffsets(o, newxoffset, newyoffset);
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData *carddata = &(SD(cl)->carddata);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);

    VIDEO_BITMAP_REFUSED(bmdata, );

    /* FIXME "Optimistics" synchronization (yes, I know it's wrong) */
    IPTR map = (IPTR)bmdata->bo->map;
    
    /* If the current map was NULL, wait until bitmap lock is released. 
       When it happens, map the buffer */
    if (map == (IPTR)NULL)
    {
        LOCK_BITMAP
        MAP_BUFFER
        addr += (IPTR)bmdata->bo->map;
    }
    else
        addr += map;

    if (bmdata->gpu_dirty)
    {
        nouveau_bo_wait(bmdata->bo, NOUVEAU_BO_RDWR, carddata->client);
        bmdata->gpu_dirty = FALSE;
    }

    switch(bmdata->bytesperpixel)
    {
    case(1):
        /* Not supported */
        break;
    case(2):
        hidd_writew(msg->pixel, (APTR)addr);
        break;
    case(4):
        hidd_writel(msg->pixel, (APTR)addr);
        break;
    }

    if (map == (IPTR)NULL)    
        UNLOCK_BITMAP
}

HIDDT_Pixel METHOD(NouveauBitMap, Hidd_BitMap, GetPixel)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData *carddata = &(SD(cl)->carddata);
    IPTR addr = (msg->x * bmdata->bytesperpixel) + (bmdata->pitch * msg->y);
    HIDDT_Pixel pixel = 0;

    VIDEO_BITMAP_REFUSED(bmdata, 0);

    /* FIXME "Optimistics" synchronization (yes, I know it's wrong) */
    IPTR map = (IPTR)bmdata->bo->map;

    /* If the current map was NULL, wait until bitmap lock is released. 
       When it happens, map the buffer */
    if (map == (IPTR)NULL)
    {
        LOCK_BITMAP
        MAP_BUFFER
        addr += (IPTR)bmdata->bo->map;
    }
    else
        addr += map;

    if (bmdata->gpu_dirty)
    {
        nouveau_bo_wait(bmdata->bo, NOUVEAU_BO_RDWR, carddata->client);
        bmdata->gpu_dirty = FALSE;
    }

    switch(bmdata->bytesperpixel)
    {
    case(1):
        /* Not supported */
        break;
    case(2):
        pixel = hidd_readw((APTR)addr);
        break;
    case(4):
        pixel = hidd_readl((APTR)addr);
        break;
    }
    
    if (map == (IPTR)NULL)    
        UNLOCK_BITMAP
    
    return pixel;
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, Clear)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    BOOL ret = FALSE;

    VIDEO_BITMAP_REFUSED(bmdata, );
    
    LOCK_ENGINE

    LOCK_BITMAP

    switch(carddata->Architecture)
    {
    case(NV_ARCH_03):
    case(NV_ARCH_04):
    case(NV_ARCH_10):
    case(NV_ARCH_20):
    case(NV_ARCH_30):
    case(NV_ARCH_40):
        ret = HIDDNouveauNV04FillSolidRect(carddata, bmdata, 
                    0, 0, bmdata->drawable.width - 1, bmdata->drawable.height - 1, GC_DRMD(msg->gc), GC_BG(msg->gc));
        break;
    case(NV_TESLA):
        ret = HIDDNouveauNV50FillSolidRect(carddata, bmdata, 
                    0, 0, bmdata->drawable.width - 1, bmdata->drawable.height - 1, GC_DRMD(msg->gc), GC_BG(msg->gc));
        break;
    case(NV_FERMI):
    case(NV_KEPLER):
    case(NV_MAXWELL):
    case(NV_PASCAL):
    case(NV_VOLTA):
    case(NV_TURING):
    case(NV_AMPERE):
    case(NV_HOPPER):
    case(NV_ADA):
    case(NV_BLACKWELL):
        ret = HIDDNouveauNVC0FillSolidRect(carddata, bmdata, 
                    0, 0, bmdata->drawable.width - 1, bmdata->drawable.height - 1, GC_DRMD(msg->gc), GC_BG(msg->gc));
        break;
    }    


    UNLOCK_BITMAP

    UNLOCK_ENGINE

    if (ret)
        return;    
    
    /* Fallback to default method */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/* There is no pHidd_BitMap_FillRect structure - it's the same as pHidd_BitMap_DrawRect */
#define pHidd_BitMap_FillRect pHidd_BitMap_DrawRect

VOID METHOD(NouveauBitMap, Hidd_BitMap, FillRect)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);
    BOOL ret = FALSE;

    VIDEO_BITMAP_REFUSED(bmdata, );
    
    LOCK_ENGINE

    LOCK_BITMAP

    switch(carddata->Architecture)
    {
    case(NV_ARCH_03):
    case(NV_ARCH_04):
    case(NV_ARCH_10):
    case(NV_ARCH_20):
    case(NV_ARCH_30):
    case(NV_ARCH_40):
        ret = HIDDNouveauNV04FillSolidRect(carddata, bmdata, 
                    msg->minX, msg->minY, msg->maxX, msg->maxY, GC_DRMD(msg->gc), GC_FG(msg->gc));
        break;
    case(NV_TESLA):
        ret = HIDDNouveauNV50FillSolidRect(carddata, bmdata, 
                    msg->minX, msg->minY, msg->maxX, msg->maxY, GC_DRMD(msg->gc), GC_FG(msg->gc));
        break;
    case(NV_FERMI):
    case(NV_KEPLER):
    case(NV_MAXWELL):
    case(NV_PASCAL):
    case(NV_VOLTA):
    case(NV_TURING):
    case(NV_AMPERE):
    case(NV_HOPPER):
    case(NV_ADA):
    case(NV_BLACKWELL):
        ret = HIDDNouveauNVC0FillSolidRect(carddata, bmdata, 
                    msg->minX, msg->minY, msg->maxX, msg->maxY, GC_DRMD(msg->gc), GC_FG(msg->gc));
        break;
    }

    UNLOCK_BITMAP

    UNLOCK_ENGINE

    if (ret)
        return;

    /* Fallback to default method */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutImage)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    LOCK_ENGINE

    LOCK_BITMAP

    /* For larger transfers use GART */
    if (GART_TRANSFER_ALLOWED(msg->width, msg->height))
    {
        BOOL result = FALSE;
        
        /* RAM->CPU->GART GART->GPU->VRAM */

        ObtainSemaphore(&carddata->gartsemaphore);
        
        result = HiddNouveauNVAccelUploadM2MF(
                    msg->pixels, msg->modulo, msg->pixFmt,
                    msg->x, msg->y, msg->width, msg->height, 
                    cl, o);

if (result) { bmdata->gpu_dirty = TRUE; HIDDNouveauFlushDisplayable(carddata, bmdata); }

        ReleaseSemaphore(&carddata->gartsemaphore);

        if (result)
        {
            UNLOCK_BITMAP;
            UNLOCK_ENGINE;
            return;
        }
    }

    /* Fallback */

    /* RAM->CPU->VRAM */
    {
    APTR dstBuff = NULL;
    
    MAP_BUFFER

    /* Calculate destination buffer pointer */
    dstBuff = (APTR)((IPTR)bmdata->bo->map + (msg->y * bmdata->pitch) + (msg->x * bmdata->bytesperpixel));
    
    HiddNouveauWriteFromRAM(
        msg->pixels, msg->modulo, msg->pixFmt,
        dstBuff, bmdata->pitch,
        msg->width, msg->height,
        cl, o);
    }


    UNLOCK_BITMAP

    UNLOCK_ENGINE
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, GetImage)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    LOCK_ENGINE

    LOCK_BITMAP

    /* For larger transfers use GART */
    if (GART_TRANSFER_ALLOWED(msg->width, msg->height))
    {
        BOOL result = FALSE;
        
        /* VRAM->GPU->GART GART->CPU->RAM */

        /* The engine download is not ordered against work queued on the
           other channel - settle any pending writes first */
        if (bmdata->gpu_dirty)
        {
            nouveau_bo_wait(bmdata->bo, NOUVEAU_BO_RDWR, carddata->client);
            bmdata->gpu_dirty = FALSE;
        }

        ObtainSemaphore(&carddata->gartsemaphore);
        
        result = HiddNouveauNVAccelDownloadM2MF(
                    msg->pixels, msg->modulo, msg->pixFmt,
                    msg->x, msg->y, msg->width, msg->height, 
                    cl, o);

        ReleaseSemaphore(&carddata->gartsemaphore);

        if (result)
        {
            UNLOCK_BITMAP;
            UNLOCK_ENGINE;
            return;
        }
    }

    /* Fallback */

    /* VRAM->CPU->RAM */
    {
    APTR srcBuff = NULL;
    
    MAP_BUFFER

    /* Calculate source buffer pointer */
    srcBuff = (APTR)((IPTR)bmdata->bo->map + (msg->y * bmdata->pitch) + (msg->x * bmdata->bytesperpixel));
    
    HiddNouveauReadIntoRAM(
        srcBuff, bmdata->pitch,
        msg->pixels, msg->modulo, msg->pixFmt,
        msg->width, msg->height,
        cl, o);
    }

    UNLOCK_BITMAP
    UNLOCK_ENGINE
}

/* The per-pixel routines below, run against a staging copy of the
   destination rectangle instead of the aperture (see
   HIDDNouveauNVC0StagingRect): the rows arrive with the engine and go
   back with it, and the CPU never reads VRAM. */
struct nouveau_staging_ctx
{
    OOP_Class *cl;
    OOP_Object *o;
    struct pHidd_BitMap_PutAlphaImage *ai;
    struct pHidd_BitMap_PutAlphaTemplate *at;
    struct pHidd_BitMap_PutTemplate *t;
    struct pHidd_BitMap_PutPattern *pat;
    UBYTE cpp;
};

static VOID nouveau_staging_alphaimage(APTR rows, ULONG pitch, LONG line0, LONG lines, APTR ctx)
{
    struct nouveau_staging_ctx *c = ctx;
    struct pHidd_BitMap_PutAlphaImage *msg = c->ai;
    APTR src = (UBYTE *)msg->pixels + (IPTR)line0 * msg->modulo;

    if (c->cpp == 4)
        HIDDNouveauBitMapPutAlphaImage32(rows, pitch, src, msg->modulo, 0, 0, msg->width, lines);
    else
        HIDDNouveauBitMapPutAlphaImage16(rows, pitch, src, msg->modulo, 0, 0, msg->width, lines);
}

static VOID nouveau_staging_alphatemplate(APTR rows, ULONG pitch, LONG line0, LONG lines, APTR ctx)
{
    struct nouveau_staging_ctx *c = ctx;
    struct pHidd_BitMap_PutAlphaTemplate *msg = c->at;
    UBYTE *src = msg->alpha + (IPTR)line0 * msg->modulo;

    if (c->cpp == 4)
        HIDDNouveauBitMapPutAlphaTemplate32(rows, pitch, msg->gc, c->o, msg->invertalpha,
            src, msg->modulo, 0, 0, msg->width, lines);
    else
        HIDDNouveauBitMapPutAlphaTemplate16(rows, pitch, msg->gc, c->o, msg->invertalpha,
            src, msg->modulo, 0, 0, msg->width, lines);
}

static VOID nouveau_staging_template(APTR rows, ULONG pitch, LONG line0, LONG lines, APTR ctx)
{
    struct nouveau_staging_ctx *c = ctx;
    struct pHidd_BitMap_PutTemplate *msg = c->t;
    UBYTE *src = msg->masktemplate + (IPTR)line0 * msg->modulo;

    if (c->cpp == 4)
    {
        struct pHidd_BitMap_PutMemTemplate32 __m =
        {
            SD(c->cl)->mid_PutMemTemplate32, msg->gc, src, msg->modulo,
            msg->srcx, rows, pitch, 0, 0, msg->width, lines, msg->inverttemplate
        }, *m = &__m;
        OOP_DoMethod(c->o, (OOP_Msg)m);
    }
    else
    {
        struct pHidd_BitMap_PutMemTemplate16 __m =
        {
            SD(c->cl)->mid_PutMemTemplate16, msg->gc, src, msg->modulo,
            msg->srcx, rows, pitch, 0, 0, msg->width, lines, msg->inverttemplate
        }, *m = &__m;
        OOP_DoMethod(c->o, (OOP_Msg)m);
    }
}

static VOID nouveau_staging_pattern(APTR rows, ULONG pitch, LONG line0, LONG lines, APTR ctx)
{
    struct nouveau_staging_ctx *c = ctx;
    struct pHidd_BitMap_PutPattern *msg = c->pat;
    UBYTE *mask = msg->mask ? msg->mask + (IPTR)line0 * msg->maskmodulo : NULL;

    if (c->cpp == 4)
    {
        struct pHidd_BitMap_PutMemPattern32 __m =
        {
            SD(c->cl)->mid_PutMemPattern32, msg->gc, msg->pattern, msg->patternsrcx,
            msg->patternsrcy + line0, msg->patternheight, msg->patterndepth, msg->patternlut,
            msg->invertpattern, mask, msg->maskmodulo, msg->masksrcx,
            rows, pitch, 0, 0, msg->width, lines
        }, *m = &__m;
        OOP_DoMethod(c->o, (OOP_Msg)m);
    }
    else
    {
        struct pHidd_BitMap_PutMemPattern16 __m =
        {
            SD(c->cl)->mid_PutMemPattern16, msg->gc, msg->pattern, msg->patternsrcx,
            msg->patternsrcy + line0, msg->patternheight, msg->patterndepth, msg->patternlut,
            msg->invertpattern, mask, msg->maskmodulo, msg->masksrcx,
            rows, pitch, 0, 0, msg->width, lines
        }, *m = &__m;
        OOP_DoMethod(c->o, (OOP_Msg)m);
    }
}

/* No 2D engine and no cheap CPU path to VRAM on this generation */
#define STAGING_BOUNCE_WANTED \
    ((carddata->Architecture >= NV_BLACKWELL) && (bmdata->bytesperpixel > 1))

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutAlphaImage)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    LOCK_ENGINE
    LOCK_BITMAP

    if (STAGING_BOUNCE_WANTED)
    {
        struct nouveau_staging_ctx ctx = { cl, o, msg, NULL, NULL, NULL, bmdata->bytesperpixel };

        if (HIDDNouveauNVC0StagingRect(carddata, bmdata, msg->x, msg->y,
                msg->width, msg->height, nouveau_staging_alphaimage, &ctx))
        {
            UNLOCK_BITMAP
            UNLOCK_ENGINE
            return;
        }
    }

    /* Try hardware method NV10-NV40*/
    if (
        ((carddata->Architecture >= NV_ARCH_10) && (carddata->Architecture <= NV_ARCH_40))
        
        && (bmdata->bytesperpixel > 1)
        && (GART_TRANSFER_ALLOWED(msg->width, msg->height)))
    {
        BOOL result = FALSE;
        
        /* RAM->CPU->GART GART->GPU->VRAM */

        ObtainSemaphore(&carddata->gartsemaphore);
        
        result = HiddNouveauAccelARGBUpload3D(
                    msg->pixels, msg->modulo,
                    msg->x, msg->y, msg->width, msg->height, 
                    cl, o);
        
        ReleaseSemaphore(&carddata->gartsemaphore);

        if (result)
        {
            MAP_BUFFER; /* FIXME: This is needed to flush execution buffer, atrifact otherwise */
            UNLOCK_BITMAP;
            UNLOCK_ENGINE;
            return;
        }
    }
    
    /* Try optimization for NV50 */
    if (
        (carddata->Architecture >= NV_TESLA)
        
        && (bmdata->bytesperpixel > 1)
        && (GART_TRANSFER_ALLOWED(msg->width, msg->height)))
    {
        /* Hardware method is not currently possible for NV50 as the implementation
           relies on tiled bitmaps. AROS uses linear bitmaps for all card families.
           The optimization in this case is to use base class implementation,
           which does GetImage->Process->PutImage. Since all NV50 cards are
           PCI-E based, the greatest limiting factor - VRAM->RAM download
           speed - is not a problem (1.1 Gbps on my GF8300). This approach is
           actually faster than "per-pixel" functions below by order of 10. */
           
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        UNLOCK_BITMAP;
        UNLOCK_ENGINE;
        return;
    }

    /* Fallback to software method */
    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            MAP_BUFFER

            HIDDNouveauBitMapPutAlphaImage16(bmdata->bo->map, bmdata->pitch, msg->pixels, msg->modulo, msg->x, 
                msg->y, msg->width, msg->height);
        }
        break;

    case 4:
        {
            MAP_BUFFER

            HIDDNouveauBitMapPutAlphaImage32(bmdata->bo->map, bmdata->pitch, msg->pixels, msg->modulo, msg->x, 
                msg->y, msg->width, msg->height);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
    UNLOCK_ENGINE
}

ULONG METHOD(NouveauBitMap, Hidd_BitMap, BytesPerLine)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    
    return (bmdata->pitch);
}

BOOL METHOD(NouveauBitMap, Hidd_BitMap, ObtainDirectAccess)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData *carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, FALSE);    /* LockBuffer is the way in */
    
    LOCK_ENGINE
    LOCK_BITMAP
    MAP_BUFFER

    *msg->addressReturn = (UBYTE*)bmdata->bo->map;
    *msg->widthReturn = bmdata->pitch / bmdata->bytesperpixel;
    *msg->heightReturn = bmdata->drawable.height;
    *msg->bankSizeReturn = *msg->memSizeReturn = bmdata->pitch * bmdata->drawable.height;

    return TRUE;
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, ReleaseDirectAccess)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);

    UNLOCK_BITMAP
    UNLOCK_ENGINE
}

#define COMPLEMENT_JAM2_DECISION_BLOCK                                              \
    else if (GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Invert)                          \
    {                                                                               \
        /* COMPLEMENT - read & write. Base method uses GetImage/PutImage.           \
           It is better to use it, if GetImage is fast(==PCIE) */                   \
        if (GART_TRANSFER_ALLOWED(msg->width, msg->height) && (carddata->IsPCIE))   \
        {                                                                           \
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);                                 \
            return;                                                                 \
        }                                                                           \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        /* JAM2 - only write. Base method uses PutImage. It is                      \
           better to use it, if it is accelerated */                                \
        if (GART_TRANSFER_ALLOWED(msg->width, msg->height))                         \
        {                                                                           \
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);                                 \
            return;                                                                 \
        }                                                                           \
    }                                                                               \


VOID METHOD(NouveauBitMap, Hidd_BitMap, PutAlphaTemplate)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    if (STAGING_BOUNCE_WANTED)
    {
        struct nouveau_staging_ctx ctx = { cl, o, NULL, msg, NULL, NULL, bmdata->bytesperpixel };
        BOOL done;

        LOCK_ENGINE
        LOCK_BITMAP
        done = HIDDNouveauNVC0StagingRect(carddata, bmdata, msg->x, msg->y,
                msg->width, msg->height, nouveau_staging_alphatemplate, &ctx);
        UNLOCK_BITMAP
        UNLOCK_ENGINE
        if (done)
            return;
    }

    /* Select acceleration method based on hardware and buffer size */
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    {
        /* JAM1 - read & write. Base method uses GetImage/PutImage.
           Use 3D alpha blending where possible */
        if (GART_TRANSFER_ALLOWED(msg->width, msg->height))
        {
            /* These cards support 3D alpha blending */
            if ((carddata->Architecture >= NV_ARCH_10) && (carddata->Architecture <= NV_ARCH_40))
            {
                BOOL result = FALSE;
                HIDDT_Color color;
                LONG fg_red, fg_green, fg_blue;

                HIDD_BM_UnmapPixel(o, GC_FG(msg->gc), &color);

                fg_red   = color.red >> 8;
                fg_green = color.green >> 8;
                fg_blue  = color.blue >> 8;
                
                LOCK_ENGINE
                LOCK_BITMAP
                
                ObtainSemaphore(&carddata->gartsemaphore);
                
                result = HiddNouveauAccelAPENUpload3D(msg->alpha, msg->invertalpha,
                    msg->modulo, (fg_red << 16) | (fg_green << 8) | fg_blue, 
                    msg->x, msg->y, msg->width, msg->height, cl, o);

                ReleaseSemaphore(&carddata->gartsemaphore);
                MAP_BUFFER; /* FIXME: This is needed to flush execution buffer, atrifact otherwise */
                UNLOCK_BITMAP
                UNLOCK_ENGINE

                if (result) return;
            }
            
            /* These cards don't support 3D alpha blending (yet), but they are all
               PCIE so GetImage is fast */
            if (carddata->Architecture >= NV_TESLA)
            {
                OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
                return;
            }
        }
    }
    COMPLEMENT_JAM2_DECISION_BLOCK 

    /* This is software fallback */
    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            HIDDNouveauBitMapPutAlphaTemplate16(bmdata->bo->map, bmdata->pitch, msg->gc, o, msg->invertalpha,
                msg->alpha, msg->modulo, msg->x, msg->y, msg->width, msg->height);
        }
        break;

    case 4:
        {
            HIDDNouveauBitMapPutAlphaTemplate32(bmdata->bo->map, bmdata->pitch, msg->gc, o, msg->invertalpha,
                msg->alpha, msg->modulo, msg->x, msg->y, msg->width, msg->height);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutTemplate)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    if (STAGING_BOUNCE_WANTED)
    {
        struct nouveau_staging_ctx ctx = { cl, o, NULL, NULL, msg, NULL, bmdata->bytesperpixel };
        BOOL done;

        LOCK_ENGINE
        LOCK_BITMAP
        done = HIDDNouveauNVC0StagingRect(carddata, bmdata, msg->x, msg->y,
                msg->width, msg->height, nouveau_staging_template, &ctx);
        UNLOCK_BITMAP
        UNLOCK_ENGINE
        if (done)
            return;
    }

    /* Select execution method based on hardware and buffer size */
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    {
        /* JAM1 - read & write. Base method uses GetImage/PutImage.
           Use software fallback. It performs only limited writes and thus it
           is faster than base method. */
    }
    COMPLEMENT_JAM2_DECISION_BLOCK

    /* This is software fallback */
    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            struct pHidd_BitMap_PutMemTemplate16 __m = 
            {
                SD(cl)->mid_PutMemTemplate16, msg->gc, msg->masktemplate, msg->modulo,
                msg->srcx, bmdata->bo->map, bmdata->pitch, msg->x, msg->y,
                msg->width, msg->height, msg->inverttemplate
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;

    case 4:
        {
            struct pHidd_BitMap_PutMemTemplate32 __m = 
            {
                SD(cl)->mid_PutMemTemplate32, msg->gc, msg->masktemplate, msg->modulo,
                msg->srcx, bmdata->bo->map, bmdata->pitch, msg->x, msg->y,
                msg->width, msg->height, msg->inverttemplate
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, PutPattern)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData * carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    if (STAGING_BOUNCE_WANTED)
    {
        struct nouveau_staging_ctx ctx = { cl, o, NULL, NULL, NULL, msg, bmdata->bytesperpixel };
        BOOL done;

        LOCK_ENGINE
        LOCK_BITMAP
        done = HIDDNouveauNVC0StagingRect(carddata, bmdata, msg->x, msg->y,
                msg->width, msg->height, nouveau_staging_pattern, &ctx);
        UNLOCK_BITMAP
        UNLOCK_ENGINE
        if (done)
            return;
    }

    /* Select execution method based on hardware and buffer size */
    if (GC_COLEXP(msg->gc) == vHidd_GC_ColExp_Transparent)
    {
        /* JAM1 - read & write. Base method uses GetImage/PutImage.
           Use software fallback. It performs only limited writes and thus it
           is faster than base method. */
    }
    COMPLEMENT_JAM2_DECISION_BLOCK

    /* This is software fallback */
    LOCK_BITMAP
    MAP_BUFFER

    switch(bmdata->bytesperpixel)
    {
    case 1:
        /* Not supported */
        break;

    case 2:
        {
            struct pHidd_BitMap_PutMemPattern16 __m = 
            {
                SD(cl)->mid_PutMemPattern16, msg->gc, msg->pattern, msg->patternsrcx,
                msg->patternsrcy, msg->patternheight, msg->patterndepth, msg->patternlut,
                msg->invertpattern, msg->mask, msg->maskmodulo, msg->masksrcx,
                bmdata->bo->map, bmdata->pitch, msg->x, msg->y, msg->width, msg->height
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;

    case 4:
        {
            struct pHidd_BitMap_PutMemPattern32 __m = 
            {
                SD(cl)->mid_PutMemPattern32, msg->gc, msg->pattern, msg->patternsrcx,
                msg->patternsrcy, msg->patternheight, msg->patterndepth, msg->patternlut,
                msg->invertpattern, msg->mask, msg->maskmodulo, msg->masksrcx,
                bmdata->bo->map, bmdata->pitch, msg->x,msg->y, msg->width,
                msg->height
            }, *m = &__m;
            OOP_DoMethod(o, (OOP_Msg)m);
        }
        break;
    } /* switch(bmdata->bytesperpixel) */

    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, DrawLine)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData *carddata = &(SD(cl)->carddata);

    VIDEO_BITMAP_REFUSED(bmdata, );

    LOCK_BITMAP

    if ((GC_DRMD(msg->gc) == vHidd_GC_DrawMode_Copy) && (GC_COLMASK(msg->gc) == ~0))
    {
        MAP_BUFFER

        HIDDNouveauBitMapDrawSolidLine(bmdata, msg->gc, msg->x1, msg->y1, msg->x2, msg->y2);

        UNLOCK_BITMAP

        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    
    UNLOCK_BITMAP
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, UpdateRect)
{
    /* Scan-out is direct: nothing to notify, the display's compositor
       drives what is shown through Hidd_Display::Show() */
}

/*
 * Scan-out bitmap buffers (aHidd_BitMap_Scanout).
 *
 * LockBuffer maps the buffer's VRAM through BAR1 and hands that mapping
 * out: the simplest thing that works on every card. BAR1 writes are
 * uncached and slow on the Titan (the PutImage path stages through GART
 * and the copy engine for that reason); a staged lock - a cached system
 * memory copy per buffer, UnlockBuffer's dirty rectangle copied into VRAM
 * by the engine - is the planned follow-up (design 4.4) and keeps the same
 * interface. UnlockBuffer therefore has nothing to flush here.
 */
BOOL METHOD(NouveauBitMap, Hidd_BitMap, LockBuffer)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    struct CardData *carddata = &(SD(cl)->carddata);
    struct nouveau_bo *bo;

    if (!msg->addressReturn || !msg->pitchReturn)
        return FALSE;
    if (bmdata->nbufs)
    {
        if (msg->buffer >= bmdata->nbufs)
            return FALSE;
        bo = bmdata->buf[msg->buffer].bo;
    }
    else
    {
        if (msg->buffer != 0)
            return FALSE;
        bo = bmdata->bo;
    }
    if (!bo)
        return FALSE;

    LOCK_ENGINE
    LOCK_BITMAP
    if (!bo->map)
        nouveau_bo_map(bo, NOUVEAU_BO_RDWR, carddata->client);
    if (bo->map && (bo == bmdata->bo) && bmdata->gpu_dirty)
    {
        /* engine work still queued for the front buffer: settle it first */
        nouveau_bo_wait(bo, NOUVEAU_BO_RDWR, carddata->client);
        bmdata->gpu_dirty = FALSE;
    }
    UNLOCK_BITMAP
    UNLOCK_ENGINE

    if (!bo->map)
        return FALSE;
    *msg->addressReturn = (UBYTE *)bo->map;
    *msg->pitchReturn = bmdata->pitch;
    if (msg->planeOffsets)
    {
        /* I420: U, V planes (chroma pitch = pitch/2); NV12: UV plane, 0; packed: 0, 0 */
        msg->planeOffsets[0] = bmdata->planeoff[0];
        msg->planeOffsets[1] = bmdata->planeoff[1];
    }
    return TRUE;
}

VOID METHOD(NouveauBitMap, Hidd_BitMap, UnlockBuffer)
{
    /* Direct VRAM mapping: the writes are already where the plane reads them */
}

/*
 * Make another buffer the front one. The display re-programs the plane
 * that shows this bitmap (same geometry, new framebuffer) and waits for
 * the vblank unless fHidd_Swap_NoWait; a bitmap no plane shows is reported
 * as changed to the display, whose compositor re-blits an RGB surface (a
 * video bitmap off a plane shows nothing). The displaced buffer joins the
 * ring: BackBuffer is the next one after the new front.
 */
BOOL METHOD(NouveauBitMap, Hidd_BitMap, SwapBuffers)
{
    struct HIDDNouveauBitMapData * bmdata = OOP_INST_DATA(cl, o);
    OOP_Object *display = NULL;
    ULONG oldfbid, newfbid;

    if (!bmdata->nbufs || (msg->newFront >= bmdata->nbufs))
        return FALSE;
    OOP_GetAttr(o, aHidd_BitMap_Display, (IPTR *)&display);
    if (!display)
        return FALSE;

    if (msg->newFront == bmdata->front)
        return TRUE;

    /* Both framebuffers must exist before the plane can be switched */
    if (!HIDDNouveauEnsureBufferFB(o, msg->newFront))
        return FALSE;
    (void)HIDDNouveauEnsureBufferFB(o, bmdata->front);

    LOCK_BITMAP
    bmdata->buf[bmdata->front].fbid = bmdata->fbid;
    oldfbid = bmdata->fbid;
    newfbid = bmdata->buf[msg->newFront].fbid;
    bmdata->front = msg->newFront;
    bmdata->back = (bmdata->front + 1) % bmdata->nbufs;
    bmdata->bo = bmdata->buf[bmdata->front].bo;
    bmdata->fbid = newfbid;
    UNLOCK_BITMAP

    if (!HIDDNouveauPlaneSwap(display, oldfbid, newfbid, (msg->flags & fHidd_Swap_NoWait) ? FALSE : TRUE))
    {
        /* Not on a plane: the compositor shows it from the front buffer */
        HIDD_Display_UpdateRect(display, o, 0, 0, bmdata->drawable.width, bmdata->drawable.height);
    }
    return TRUE;
}
