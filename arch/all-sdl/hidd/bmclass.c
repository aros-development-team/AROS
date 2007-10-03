/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>

#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "sdl_intern.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

#include "icon.h"

static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddSDLBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = {
    { IID_Hidd_PixFmt,    &HiddPixFmtAttrBase    },
    { IID_Hidd_BitMap,    &HiddBitMapAttrBase    },
    { IID_Hidd_SDLBitMap, &HiddSDLBitMapAttrBase },
    { NULL,               NULL                   }
};

static int sdl_bmclass_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_bmclass_init\n"));

    return OOP_ObtainAttrBases(attrbases);
}

static int sdl_bmclass_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_bmclass_expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(sdl_bmclass_init , 0)
ADD2EXPUNGELIB(sdl_bmclass_expunge, 0)

#define UPDATE(bmdata, x, y, w, h)                                                 \
    do {                                                                           \
        if (bmdata->is_onscreen)                                                   \
            SV(SDL_UpdateRect, bmdata->surface, x, y, w, h);                       \
    } while(0)

#define LOCK(s)                     \
    do {                            \
        if (SDL_MUSTLOCK(s))        \
            SV(SDL_LockSurface, s); \
    } while(0)

#define UNLOCK(s)                     \
    do {                              \
        if (SDL_MUSTLOCK(s))          \
            SV(SDL_UnlockSurface, s); \
    } while(0)

static SDL_Surface *icon;
static void load_icon(LIBBASETYPEPTR SDLGfxBase) {
    unsigned char *data, *pixel;
    int i;

    icon = S(SDL_CreateRGBSurface, SDL_SWSURFACE, icon_width, icon_height, 24, icon_red_mask, icon_green_mask, icon_blue_mask, 0);

    LOCK(icon);

    data = icon_header_data;
    pixel = icon->pixels;

    for (i = 0; i < icon_width * icon_height; i++) {
        ICON_HEADER_PIXEL(data, pixel);
        pixel += 3;
    }

    UNLOCK(icon);
}

#define SDLGfxBase ((LIBBASETYPEPTR) cl->UserData)

OOP_Object *SDLBitMap__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
    struct bmdata *bmdata;
    BOOL framebuffer;
    int width, height, depth;
    OOP_Object *pixfmt;
    SDL_Surface *s;
    ULONG red_mask, green_mask, blue_mask, alpha_mask;

    D(bug("[sdl] SDLBitMap::New\n"));

    o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o == NULL) {
        D(bug("[sdl] supermethod failed, bailing out\n"));
        return NULL;
    }

    bmdata = OOP_INST_DATA(cl, o);

    OOP_GetAttr(o, aHidd_BitMap_Width,  (IPTR) &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, (IPTR) &height);
    OOP_GetAttr(o, aHidd_BitMap_PixFmt, (IPTR) &pixfmt);

    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, (IPTR) &depth);

    D(bug("[sdl] width %d height %d depth %d\n", width, height, depth));

    framebuffer  = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    if (framebuffer) {
        D(bug("[sdl] creating new framebuffer\n"));

        /* XXX we should free any existing onscreen surface. the problem is
         * that we can't dispose the existing framebuffer object because the
         * caller may still have a handle on it. we could fiddle at its
         * innards well enough (store the current onscreen bitmap in class
         * static data, and now grab it and free its surface), but then we
         * have a bitmap with no associated surface, so we need checks for
         * that.
         *
         * I expect that if the caller wants to make a new framebuffer, it
         * should have to free the old one
         */

        if (!LIBBASE->use_hwsurface)
            D(bug("[sdl] hardware surface not available, using software surface instead\n"));

        if (icon == NULL) {
            D(bug("[sdl] loading window icon\n"));
            load_icon((LIBBASETYPEPTR) cl->UserData);
            SV(SDL_WM_SetIcon, icon, NULL);
        }

        s = S(SDL_SetVideoMode, width, height, depth,
                                (LIBBASE->use_hwsurface  ? SDL_HWSURFACE | SDL_HWPALETTE : SDL_SWSURFACE) |
                                (LIBBASE->use_fullscreen ? SDL_FULLSCREEN                : 0) |
                                SDL_ANYFORMAT);

        SV(SDL_WM_SetCaption, "AROS Research Operating System", "AROS");
    }

    else {
        OOP_GetAttr(pixfmt, aHidd_PixFmt_RedMask,   (IPTR) &red_mask);
        OOP_GetAttr(pixfmt, aHidd_PixFmt_GreenMask, (IPTR) &green_mask);
        OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueMask,  (IPTR) &blue_mask);
        OOP_GetAttr(pixfmt, aHidd_PixFmt_AlphaMask, (IPTR) &alpha_mask);

        D(bug("[sdl] creating new offscreen surface; masks: red 0x%08x green 0x%08x blue 0x%08x alpha 0x%08x\n", red_mask, green_mask, blue_mask, alpha_mask));

        s = S(SDL_CreateRGBSurface, SDL_SWSURFACE, width, height, depth, red_mask, green_mask, blue_mask, alpha_mask);
    }

    if (s == NULL) {
        OOP_MethodID dispose;

        D(bug("[sdl] failed to create surface: %s\n", S(SDL_GetError, )));

        dispose = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg) &dispose);

        return NULL;
    }

    bmdata->surface = s;

    if (framebuffer)
        bmdata->is_onscreen = TRUE;

    D(bug("[sdl] created surface: 0x%08x\n", s));

    return o;
}

VOID SDLBitMap__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);

    D(bug("[sdl] SDLBitMap::Dispose\n"));

    if (bmdata->surface != NULL) {
        D(bug("[sdl] destroying surface 0x%08x\n", bmdata->surface));

        SV(SDL_FreeSurface, bmdata->surface);
        bmdata->surface = NULL;
    }
    
    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

VOID SDLBitMap__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);

//    D(bug("[sdl] SDLBitMap::Get\n"));

    switch (SDLBM_ATTR(msg->attrID)) {
        case aoHidd_SDLBitMap_Surface:
            *msg->storage = (IPTR) bmdata->surface;
            break;

        case aoHidd_SDLBitMap_IsOnScreen:
            *msg->storage = (IPTR) bmdata->is_onscreen;
            break;

        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
            break;
    }
}

BOOL SDLBitMap__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *pixfmt;
    SDL_Color *colors;
    int i;

    //D(bug("[sdl] SDLBitMap::SetColors\n"));

    pixfmt = BM_PIXFMT(o);
    if (HIDD_PF_COLMODEL(pixfmt) == vHidd_ColorModel_StaticPalette ||
        HIDD_PF_COLMODEL(pixfmt) == vHidd_ColorModel_TrueColor) {

        return OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    }

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg) msg))
        return FALSE;

    colors = AllocVec(sizeof(SDL_Color) * msg->numColors, MEMF_CLEAR);

    for (i = 0; i < msg->numColors; i++) {
        colors[i].r = msg->colors[i].red;
        colors[i].g = msg->colors[i].green;
        colors[i].b = msg->colors[i].blue;
    }

    S(SDL_SetColors, bmdata->surface, colors, msg->firstColor, msg->numColors);

    D(bug("[sdl] set %d colours for surface 0x%08x\n", msg->numColors, bmdata->surface));

    return TRUE;
}

#define PUTPIXEL8(p,c)  (* (Uint8 *) (p) = (c))
#define GETPIXEL8(p)    (* (Uint8 *) (p))

#define PUTPIXEL16(p,c) (* (Uint16 *) (p)) = (c)
#define GETPIXEL16(p)   (* (Uint16 *) (p))

#define PUTPIXEL32(p,c) (* (Uint32 *) (p)) = (c)
#define GETPIXEL32(p)   (* (Uint32 *) (p))

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define PUTPIXEL24(p,c)                         \
    do {                                        \
        ((Uint8 *) p)[0] = ((c) >> 16) & 0xff;  \
        ((Uint8 *) p)[1] = ((c) >> 8) & 0xff;   \
        ((Uint8 *) p)[2] = (c) & 0xff;          \
    } while(0)
#define GETPIXEL24(p)   (((Uint8 *) p)[0] << 16 | ((Uint8 *) p)[1] << 8 | ((Uint8 *)p)[2])
#else
#define PUTPIXEL24(p,c)                         \
    do {                                        \
        ((Uint8 *) p)[0] = (c) & 0xff;          \
        ((Uint8 *) p)[1] = ((c) >> 8) & 0xff;   \
        ((Uint8 *) p)[2] = ((c) >> 16) & 0xff;  \
    } while(0)
#define GETPIXEL24(p)   (((Uint8 *) p)[0] | ((Uint8 *) p)[1] << 8 | ((Uint8 *) p)[2] << 16)
#endif

VOID SDLBitMap__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    int bytesperpixel = bmdata->surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *) bmdata->surface->pixels + msg->y * bmdata->surface->pitch + msg->x * bytesperpixel;
    Uint32 c = msg->pixel;

    //D(bug("[sdl] SDLBitMap::PutPixel\n"));
    //D(bug("[sdl] x %d y %d colour 0x%08x bytesperpixel %d\n", msg->x, msg->y, c, bytesperpixel));
    
    LOCK(bmdata->surface);

    switch (bytesperpixel) {
        case 1:
            PUTPIXEL8(p, c);
            break;

        case 2:
            PUTPIXEL16(p, c);
            break;

        case 3:
            PUTPIXEL24(p, c);
            break;

        case 4:
            PUTPIXEL32(p, c);
            break;
    }

    UNLOCK(bmdata->surface);

    UPDATE(bmdata, msg->x, msg->y, 1, 1);
}

HIDDT_Pixel SDLBitMap__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    int bytesperpixel = bmdata->surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *) bmdata->surface->pixels + msg->y * bmdata->surface->pitch + msg->x * bytesperpixel;
    Uint32 c;

    //D(bug("[sdl] SDLBitMap::GetPixel\n"));
    //D(bug("[sdl] x %d y %d bytesperpixel %d\n", msg->x, msg->y, bytesperpixel));

    LOCK(bmdata->surface);

    switch(bytesperpixel) {

        case 1:
            c = GETPIXEL8(p);
            break;

        case 2:
            c = GETPIXEL16(p);
            break;

        case 3:
            c = GETPIXEL24(p);
            break;

        case 4:
            c = GETPIXEL32(p);
            break;
    }

    UNLOCK(bmdata->surface);

    //D(bug("[sdl] returning pixel 0x%08x\n", c));
    
    return (HIDDT_Pixel) c;
}

VOID SDLBitMap__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    int depth;
    Uint32 red_mask, green_mask, blue_mask, alpha_mask;
    BOOL native32 = FALSE;
    SDL_Surface *s;
    SDL_Rect srect, drect;

    D(bug("[sdl] SDLBitMap::PutImage\n"));

    switch (msg->pixFmt) {
        case vHidd_StdPixFmt_Native32:
            D(bug("[sdl] native32 format, making a note to ensure 4-byte pixels later\n"));
            native32 = TRUE;

        case vHidd_StdPixFmt_Native:
            D(bug("[sdl] native format, using our attributes\n"));

            depth      = bmdata->surface->format->BitsPerPixel;
            red_mask   = bmdata->surface->format->Rmask;
            green_mask = bmdata->surface->format->Gmask;
            blue_mask  = bmdata->surface->format->Bmask;
            alpha_mask = bmdata->surface->format->Amask;

            break;

        default:
            D(bug("[sdl] pixel format %d, asking the gfxhidd for attributes\n", msg->pixFmt));

            OOP_Object *gfxhidd = OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR) &gfxhidd);
            OOP_Object *pixfmt = HIDD_Gfx_GetPixFmt(gfxhidd, msg->pixFmt);

            OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth,     (IPTR) &depth);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_RedMask,   (IPTR) &red_mask);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_GreenMask, (IPTR) &green_mask);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueMask,  (IPTR) &blue_mask);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_AlphaMask, (IPTR) &alpha_mask);

            break;
    }

    D(bug("[sdl] source format: depth %d red 0x%08x green 0x%08x blue 0x%08x alpha 0x%08x\n", depth, red_mask, green_mask, blue_mask, alpha_mask));

    s = S(SDL_CreateRGBSurfaceFrom, msg->pixels, msg->width, msg->height, depth, msg->modulo, red_mask, green_mask, blue_mask, alpha_mask);
    if (native32) {
        D(bug("[sdl] native32 format, setting pixel width to 4 bytes\n"));
        s->format->BytesPerPixel = 4;
    }

    srect.x = 0;
    srect.y = 0;
    srect.w = msg->width;
    srect.h = msg->height;

    drect.x = msg->x;
    drect.y = msg->y;

    D(bug("[sdl] blitting %dx%d image to surface 0x%08x at [%d,%d]\n", srect.w, srect.h, bmdata->surface, drect.x, drect.y));

    S(SDL_BlitSurface, s, &srect, bmdata->surface, &drect);

    SV(SDL_FreeSurface, s);

    UPDATE(bmdata, msg->x, msg->y, msg->width, msg->height);
}

VOID SDLBitMap__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    int depth;
    Uint32 red_mask, green_mask, blue_mask, alpha_mask;
    BOOL native32 = FALSE;
    SDL_Surface *s;
    SDL_Rect srect;

    D(bug("[sdl] SDLBitMap::GetImage\n"));

    switch (msg->pixFmt) {
        case vHidd_StdPixFmt_Native32:
            D(bug("[sdl] native32 format, making a note to ensure 4-byte pixels later\n"));
            native32 = TRUE;

        case vHidd_StdPixFmt_Native:
            D(bug("[sdl] native format, using our attributes\n"));

            depth      = bmdata->surface->format->BitsPerPixel;
            red_mask   = bmdata->surface->format->Rmask;
            green_mask = bmdata->surface->format->Gmask;
            blue_mask  = bmdata->surface->format->Bmask;
            alpha_mask = bmdata->surface->format->Amask;

            break;

        default:
            D(bug("[sdl] pixel format %d, asking the gfxhidd for attributes\n", msg->pixFmt));

            OOP_Object *gfxhidd = OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR) &gfxhidd);
            OOP_Object *pixfmt = HIDD_Gfx_GetPixFmt(gfxhidd, msg->pixFmt);

            OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth,     (IPTR) &depth);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_RedMask,   (IPTR) &red_mask);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_GreenMask, (IPTR) &green_mask);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueMask,  (IPTR) &blue_mask);
            OOP_GetAttr(pixfmt, aHidd_PixFmt_AlphaMask, (IPTR) &alpha_mask);

            break;
    }

    D(bug("[sdl] target format: depth %d red 0x%08x green 0x%08x blue 0x%08x alpha 0x%08x\n", depth, red_mask, green_mask, blue_mask, alpha_mask));

    s = S(SDL_CreateRGBSurfaceFrom, msg->pixels, msg->width, msg->height, depth, msg->modulo, red_mask, green_mask, blue_mask, alpha_mask);
    if (native32) {
        D(bug("[sdl] native32 format, setting pixel width to 4 bytes\n"));
        s->format->BytesPerPixel = 4;
    }

    srect.x = msg->x;
    srect.y = msg->y;
    srect.w = msg->width;
    srect.h = msg->height;

    D(bug("[sdl] blitting %dx%d image at [%d,%d] to surface 0x%08x\n", srect.w, srect.h, srect.x, srect.y, bmdata->surface));

    S(SDL_BlitSurface, bmdata->surface, &srect, s, NULL);

    SV(SDL_FreeSurface, s);
}

VOID SDLBitMap__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    struct SDL_Rect rect;
    int bytesperpixel = bmdata->surface->format->BytesPerPixel;
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);

    D(bug("[sdl] SDLBitMap::FillRect\n"));

    rect.x = msg->minX;
    rect.y = msg->minY;
    rect.w = msg->maxX - msg->minX + 1;
    rect.h = msg->maxY - msg->minY + 1;

    D(bug("[sdl] target surface 0x%08x, width %d, height %d, depth %d\n", bmdata->surface, bmdata->surface->w, bmdata->surface->h, bmdata->surface->format->BitsPerPixel));
    D(bug("[sdl] target rect x %d y %d w %d h %d\n", rect.x, rect.y, rect.h, rect.y));
    D(bug("[sdl] colour 0x%08x, mode %d\n", fg, mode));

    switch(mode) {
        case vHidd_GC_DrawMode_Copy:
            SV(SDL_FillRect, bmdata->surface, &rect, fg);

            break;
        
        case vHidd_GC_DrawMode_Invert:
            LOCK(bmdata->surface);

            HIDD_BM_InvertMemRect(o,
                                  bmdata->surface->pixels,
                                  msg->minX * bytesperpixel,
                                  msg->minY,
                                  msg->maxX * bytesperpixel + bytesperpixel - 1,
                                  msg->maxY,
                                  bmdata->surface->pitch);

            UNLOCK(bmdata->surface);

            break;
        
        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            break;
        
    }

    UPDATE(bmdata, rect.x, rect.y, rect.w, rect.h);
}

VOID SDLBitMap__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    Uint32 c;

    D(bug("[sdl] SDLBitMap::Clear\n"));

    c = GC_BG(msg->gc);

    D(bug("[sdl] filling surface 0x%08x with colour 0x%08x\n", bmdata->surface, c));

    S(SDL_FillRect, bmdata->surface, NULL, c);

    UPDATE(bmdata, 0, 0, 0, 0);
}

VOID SDLBitMap__Hidd_BitMap__BlitColorExpansion(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_BlitColorExpansion *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    int bytesperpixel = bmdata->surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *) bmdata->surface->pixels + msg->destY * bmdata->surface->pitch + msg->destX * bytesperpixel;
    int x, y;
    HIDDT_Pixel fg, bg;
    ULONG ce;
    Uint32 pixel;
    ULONG *srcline;
    
    D(bug("[sdl] SDLBitMap::BlitColorExpansion\n"));

    D(bug("[sdl] target surface 0x%08x rect x %d y %d w %d h %d\n", bmdata->surface, msg->destX, msg->destY, msg->width, msg->height));

    fg = GC_FG(msg->gc);
    bg = GC_BG(msg->gc);
    ce = GC_COLEXP(msg->gc);

    srcline = AllocMem(msg->width * sizeof(ULONG), 0);

    LOCK(bmdata->surface);

    switch (ce) {

        case vHidd_GC_ColExp_Transparent:
            D(bug("[sdl] transparent colour expansion, fg 0x%08x\n", fg));

            switch (bytesperpixel) {

                case 1:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++) {
                            if (srcline[x] != 0)
                                PUTPIXEL8(&p[x], fg);
                        }
                        p += bmdata->surface->pitch;
                    }
                    break;

                case 2:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++) {
                            if (srcline[x] != 0)
                                PUTPIXEL16(&(((Uint16 *) p)[x]), fg);
                        }
                        p += bmdata->surface->pitch;
                    }
                    break;

                case 3:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++) {
                            if (srcline[x] != 0)
                                PUTPIXEL24(&(((Uint32 *) p)[x]), fg);
                        }
                        p += bmdata->surface->pitch;
                    }
                    break;

                case 4:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++) {
                            if (srcline[x] != 0)
                                PUTPIXEL32(&(((Uint32 *) p)[x]), fg);
                        }
                        p += bmdata->surface->pitch;
                    }
                    break;
            }

            break;


        case vHidd_GC_ColExp_Opaque:
            D(bug("[sdl] opaque colour expansion, fg 0x%08x bg %08x\n", fg, bg));

            switch (bytesperpixel) {

                case 1:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++)
                            PUTPIXEL8(&p[x], srcline[x] != 0 ? fg : bg);
                        p += bmdata->surface->pitch;
                    }
                    break;

                case 2:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcY, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++)
                            PUTPIXEL16(&(((Uint16 *) p)[x]), srcline[x] != 0 ? fg : bg);
                        p += bmdata->surface->pitch;
                    }
                    break;

                case 3:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++)
                            PUTPIXEL24(&(((Uint32 *) p)[x]), srcline[x] != 0 ? fg : bg);
                        p += bmdata->surface->pitch;
                    }
                    break;

                case 4:
                    for (y = 0; y < msg->height; y++) {
                        HIDD_BM_GetImage(msg->srcBitMap, srcline, msg->width * sizeof(ULONG), msg->srcX, msg->srcY + y, msg->width, 1, vHidd_StdPixFmt_Native32);
                        for (x = 0; x < msg->width; x++)
                            PUTPIXEL32(&(((Uint32 *) p)[x]), srcline[x] != 0 ? fg : bg);
                        p += bmdata->surface->pitch;
                    }
                    break;
            }

            break;
    }

    UNLOCK(bmdata->surface);

    FreeMem(srcline, msg->width * sizeof(ULONG));

    UPDATE(bmdata, msg->destX, msg->destY, msg->width, msg->height);
}

VOID SDLBitMap__Hidd_BitMap__PutAlphaImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutAlphaImage *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    SDL_Surface *s;
    SDL_Rect srect, drect;

    D(bug("[sdl] SDLBitMap::PutAlphaImage\n"));

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    s = S(SDL_CreateRGBSurfaceFrom, msg->pixels, msg->width, msg->height, 32, msg->modulo, 0xff0000, 0xff00, 0xff, 0xff000000);
#else
    s = S(SDL_CreateRGBSurfaceFrom, msg->pixels, msg->width, msg->height, 32, msg->modulo, 0xff00, 0xff0000, 0xff000000, 0xff);
#endif

    srect.x = 0;
    srect.y = 0;
    srect.w = msg->width;
    srect.h = msg->height;

    drect.x = msg->x;
    drect.y = msg->y;

    D(bug("[sdl] blitting %dx%d alpha image to surface 0x%08x at [%d,%d]\n", srect.w, srect.h, bmdata->surface, drect.x, drect.y));

    S(SDL_BlitSurface, s, &srect, bmdata->surface, &drect);

    SV(SDL_FreeSurface, s);

    UPDATE(bmdata, drect.x, drect.y, srect.w, srect.h);
}

VOID SDLBitMap__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg) {
    struct bmdata *bmdata = OOP_INST_DATA(cl, o);
    SDL_Rect rect;

    D(bug("[sdl] SDLBitMap::PutTemplate\n"));

    LOCK(bmdata->surface);

    switch (bmdata->surface->format->BytesPerPixel) {
        case 1:
            HIDD_BM_PutMemTemplate8(o, msg->gc, msg->template, msg->modulo, msg->srcx, bmdata->surface->pixels, bmdata->surface->pitch, msg->x, msg->y, msg->width, msg->height, msg->inverttemplate);
            break;

        case 2:
            HIDD_BM_PutMemTemplate16(o, msg->gc, msg->template, msg->modulo, msg->srcx, bmdata->surface->pixels, bmdata->surface->pitch, msg->x, msg->y, msg->width, msg->height, msg->inverttemplate);
            break;

        case 3:
            HIDD_BM_PutMemTemplate24(o, msg->gc, msg->template, msg->modulo, msg->srcx, bmdata->surface->pixels, bmdata->surface->pitch, msg->x, msg->y, msg->width, msg->height, msg->inverttemplate);
            break;

        case 4:
            HIDD_BM_PutMemTemplate32(o, msg->gc, msg->template, msg->modulo, msg->srcx, bmdata->surface->pixels, bmdata->surface->pitch, msg->x, msg->y, msg->width, msg->height, msg->inverttemplate);
            break;
    }

    UNLOCK(bmdata->surface);
        SV(SDL_UnlockSurface, bmdata->surface);

    UPDATE(bmdata, msg->x, msg->y, msg->width, msg->height);
}
