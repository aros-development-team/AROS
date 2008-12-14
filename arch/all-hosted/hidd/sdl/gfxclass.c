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

static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSDLBitMapAttrBase;

static struct OOP_ABDescr attrbases[] = {
    { IID_Hidd_PixFmt,    &HiddPixFmtAttrBase    },
    { IID_Hidd_BitMap,    &HiddBitMapAttrBase    },
    { IID_Hidd_Sync,      &HiddSyncAttrBase      },
    { IID_Hidd_Gfx,       &HiddGfxAttrBase       },
    { IID_Hidd_SDLBitMap, &HiddSDLBitMapAttrBase },
    { NULL,               NULL                   }
};

static int sdl_gfxclass_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_gfxclass_init\n"));

    return OOP_ObtainAttrBases(attrbases);
}

static int sdl_gfxclass_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[sdl] sdl_gfxclass_expunge\n"));

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

ADD2INITLIB(sdl_gfxclass_init , 0)
ADD2EXPUNGELIB(sdl_gfxclass_expunge, 0)

#define SDLGfxBase ((LIBBASETYPEPTR) cl->UserData)

static const SDL_Rect mode_1600_1200 = { .w = 1600, .h = 1200 };
static const SDL_Rect mode_1280_1024 = { .w = 1280, .h = 1024 };
static const SDL_Rect mode_1280_960  = { .w = 1280, .h = 960  };
static const SDL_Rect mode_1152_864  = { .w = 1152, .h = 864  };
static const SDL_Rect mode_1024_768  = { .w = 1024, .h = 768  };
static const SDL_Rect mode_800_600   = { .w = 800,  .h = 600  };

static const SDL_Rect *default_modes[] = {
    &mode_1600_1200,
    &mode_1280_1024,
    &mode_1280_960,
    &mode_1152_864,
    &mode_1024_768,
    &mode_800_600,
    NULL
};

OOP_Object *SDLGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg) {
    const SDL_VideoInfo *info;
    const SDL_PixelFormat *pixfmt;
    char driver[128] = "";
    Uint32 surftype;
    SDL_Rect **modes;
    struct TagItem *pftags;
    int nmodes, i;
    APTR tagpool;
    struct TagItem **synctags, *modetags, *msgtags;
    char *desc;
    struct pRoot_New supermsg;

    S(SDL_VideoDriverName, driver, sizeof(driver));
    info = S(SDL_GetVideoInfo);
    pixfmt = info->vfmt;

    kprintf("sdlgfx: using %s driver\n", driver);

    D(bug("[sdl] window manager: %savailable\n", info->wm_available ? "" : "not "));
    D(bug("[sdl] hardware surfaces: %savailable\n", info->hw_available ? "" : "not "));

    LIBBASE->use_hwsurface = info->hw_available ? TRUE : FALSE;
    surftype = LIBBASE->use_hwsurface ? SDL_HWSURFACE : SDL_SWSURFACE;

    D(bug("[sdl] colour model: %s\n", pixfmt->palette == NULL ? "truecolour" : "palette"));
    if (pixfmt->palette == NULL) {
        D(bug("[sdl] colour mask: alpha=0x%08x red=0x%08x green=0x%08x blue=0x%08x\n", pixfmt->Amask, pixfmt->Rmask, pixfmt->Gmask, pixfmt->Bmask, pixfmt->Amask));
        D(bug("[sdl] colour shift: alpha=%d red=%d green=%d blue=%d\n", pixfmt->Ashift, pixfmt->Rshift, pixfmt->Gshift, pixfmt->Bshift, pixfmt->Ashift));
    }

    if (pixfmt->palette != NULL) {
        /* XXX deal with palette (CLUT) modes */
    }

    else {
        /* select an appropriate AROS pixel format based on the SDL format */
        int stdpixfmt = vHidd_StdPixFmt_Unknown;
        int alpha_shift, red_shift, green_shift, blue_shift;

        if (pixfmt->BitsPerPixel == 32) {
            if (pixfmt->Amask == 0x000000ff && pixfmt->Rmask == 0x0000ff00 &&
                pixfmt->Gmask == 0x00ff0000 && pixfmt->Bmask == 0xff000000) {
                stdpixfmt = vHidd_StdPixFmt_ARGB32;
                alpha_shift = 24;
                red_shift = 16;
                green_shift = 8;
                blue_shift = 0;
            }

            else if (pixfmt->Amask == 0xff000000 && pixfmt->Rmask == 0x00ff0000 &&
                     pixfmt->Gmask == 0x0000ff00 && pixfmt->Bmask == 0x000000ff) {
                stdpixfmt = vHidd_StdPixFmt_BGRA32;
                alpha_shift = 0;
                red_shift = 8;
                green_shift = 16;
                blue_shift = 24;
            }

            else if (pixfmt->Amask == 0xff000000 && pixfmt->Rmask == 0x000000ff &&
                     pixfmt->Gmask == 0x0000ff00 && pixfmt->Bmask == 0x00ff0000) {
                stdpixfmt = vHidd_StdPixFmt_RGBA32;
                alpha_shift = 0;
                red_shift = 24;
                green_shift = 16;
                blue_shift = 8;
            }

            else if (pixfmt->Amask == 0x000000ff && pixfmt->Rmask == 0xff000000 &&
                     pixfmt->Gmask == 0x00ff0000 && pixfmt->Bmask == 0x0000ff00) {
                stdpixfmt = vHidd_StdPixFmt_ABGR32;
                alpha_shift = 24;
                red_shift = 0;
                green_shift = 8;
                blue_shift = 16;
            }

            else if (pixfmt->Amask == 0x00000000 && pixfmt->Rmask == 0x0000ff00 &&
                     pixfmt->Gmask == 0x00ff0000 && pixfmt->Bmask == 0xff000000) {
                stdpixfmt = vHidd_StdPixFmt_0RGB32;
                alpha_shift = 0;
                red_shift = 16;
                green_shift = 8;
                blue_shift = 0;
            }

            else if (pixfmt->Amask == 0x00000000 && pixfmt->Rmask == 0x00ff0000 &&
                     pixfmt->Gmask == 0x0000ff00 && pixfmt->Bmask == 0x000000ff) {
                stdpixfmt = vHidd_StdPixFmt_BGR032;
                alpha_shift = 0;
                red_shift = 8;
                green_shift = 16;
                blue_shift = 24;
            }

            else if (pixfmt->Amask == 0x00000000 && pixfmt->Rmask == 0x000000ff &&
                     pixfmt->Gmask == 0x0000ff00 && pixfmt->Bmask == 0x00ff0000) {
                stdpixfmt = vHidd_StdPixFmt_RGB032;
                alpha_shift = 0;
                red_shift = 24;
                green_shift = 16;
                blue_shift = 8;
            }

            else if (pixfmt->Amask == 0x00000000 && pixfmt->Rmask == 0xff000000 &&
                     pixfmt->Gmask == 0x00ff0000 && pixfmt->Bmask == 0x0000ff00) {
                stdpixfmt = vHidd_StdPixFmt_0BGR32;
                alpha_shift = 0;
                red_shift = 0;
                green_shift = 8;
                blue_shift = 16;
            }
        }

        else if (pixfmt->BitsPerPixel == 24) {
            /* XXX implement
            vHidd_StdPixFmt_RGB24
            vHidd_StdPixFmt_BGR24
            */
        }

        else if (pixfmt->BitsPerPixel == 16) {
            /* XXX implement
            vHidd_StdPixFmt_RGB16
            vHidd_StdPixFmt_RGB16_LE
            vHidd_StdPixFmt_BGR16
            vHidd_StdPixFmt_BGR16_LE
            */
        }

        else if (pixfmt->BitsPerPixel == 15) {
            /* XXX implement
            vHidd_StdPixFmt_RGB15,
            vHidd_StdPixFmt_RGB15_LE,
            vHidd_StdPixFmt_BGR15,
            vHidd_StdPixFmt_BGR15_LE,
            */
        }

        if (stdpixfmt == vHidd_StdPixFmt_Unknown) {
            stdpixfmt = vHidd_StdPixFmt_Native;
            alpha_shift = pixfmt->Ashift;
            red_shift = pixfmt->Rshift;
            green_shift = pixfmt->Gshift;
            blue_shift = pixfmt->Bshift;
        }

        D(bug("[sdl] selected pixel format %d\n", stdpixfmt));

        pftags = TAGLIST(
            aHidd_PixFmt_ColorModel,    vHidd_ColorModel_TrueColor,
            aHidd_PixFmt_RedShift,      red_shift,
            aHidd_PixFmt_GreenShift,    green_shift,
            aHidd_PixFmt_BlueShift,     blue_shift,
            aHidd_PixFmt_AlphaShift,    alpha_shift,
            aHidd_PixFmt_RedMask,       pixfmt->Rmask,
            aHidd_PixFmt_GreenMask,     pixfmt->Gmask,
            aHidd_PixFmt_BlueMask,      pixfmt->Bmask,
            aHidd_PixFmt_AlphaMask,     pixfmt->Amask,
            aHidd_PixFmt_Depth,         pixfmt->BitsPerPixel,
            aHidd_PixFmt_BitsPerPixel,  pixfmt->BitsPerPixel,
            aHidd_PixFmt_BytesPerPixel, pixfmt->BytesPerPixel,
            aHidd_PixFmt_StdPixFmt,     stdpixfmt,
            aHidd_PixFmt_BitMapType,    vHidd_BitMapType_Chunky
        );
    }

    LIBBASE->use_fullscreen = CFG_WANT_FULLSCREEN ? TRUE : FALSE;

    modes = S(SDL_ListModes, NULL, surftype | LIBBASE->use_fullscreen ? SDL_FULLSCREEN : 0);

    D(bug("[sdl] available modes:"));
    if (modes == NULL) {
        D(bug(" none\n"));
        nmodes = 0;
    }
    else {
        if (modes == (SDL_Rect **) -1) {
            D(bug(" (default)"));
            modes = default_modes;
        }
        for (nmodes = 0; modes[nmodes] != NULL && modes[nmodes]->w != 0; nmodes++)
            D(bug(" %dx%d", modes[nmodes]->w, modes[nmodes]->h));
        D(bug("\n"));
    }

    D(bug("[sdl] building %d mode sync items\n", nmodes));

    tagpool = CreatePool(MEMF_CLEAR, 1024, 128);

    synctags = AllocPooled(tagpool, sizeof(struct TagItem *) * (nmodes+1));

    for (i = 0; i < nmodes; i++) {
        /* remove modes larger than the current screen res */
        if (modes[i]->w > info->current_w || modes[i]->h > info->current_h) {
            synctags[i] = NULL;
            continue;
        }

        desc = AllocPooled(tagpool, 16);
        __sprintf(desc, "SDL:%dx%d", modes[i]->w, modes[i]->h);

        synctags[i] = AllocPooled(tagpool, sizeof(struct TagItem) * 4);
        synctags[i][0].ti_Tag = aHidd_Sync_HDisp;       synctags[i][0].ti_Data = modes[i]->w;
        synctags[i][1].ti_Tag = aHidd_Sync_VDisp;       synctags[i][1].ti_Data = modes[i]->h;
        synctags[i][2].ti_Tag = aHidd_Sync_Description; synctags[i][2].ti_Data = (IPTR) desc;
        synctags[i][3].ti_Tag = TAG_DONE;
    }

    modetags = AllocPooled(tagpool, sizeof(struct TagItem) * (nmodes+9));

    modetags[0].ti_Tag = aHidd_Gfx_PixFmtTags;   modetags[0].ti_Data = (IPTR) pftags;
    modetags[1].ti_Tag = aHidd_Sync_PixelClock;  modetags[1].ti_Data = 100000000;
    modetags[2].ti_Tag = aHidd_Sync_LeftMargin;  modetags[2].ti_Data = 0;
    modetags[3].ti_Tag = aHidd_Sync_RightMargin; modetags[3].ti_Data = 0;
    modetags[4].ti_Tag = aHidd_Sync_UpperMargin; modetags[4].ti_Data = 0;
    modetags[5].ti_Tag = aHidd_Sync_LowerMargin; modetags[5].ti_Data = 0;
    modetags[6].ti_Tag = aHidd_Sync_HSyncLength; modetags[6].ti_Data = 0;
    modetags[7].ti_Tag = aHidd_Sync_VSyncLength; modetags[7].ti_Data = 0;

    for (i = 0; i < nmodes; i++) {
        if (synctags[i] == NULL) {
            modetags[8+i].ti_Tag = TAG_IGNORE;
            continue;
        }

        modetags[8+i].ti_Tag = aHidd_Gfx_SyncTags;
        modetags[8+i].ti_Data = (IPTR) synctags[i];
    }

    modetags[8+i].ti_Tag = TAG_DONE;

    msgtags = TAGLIST(
        aHidd_Gfx_ModeTags, (IPTR) modetags,
        TAG_MORE,           (IPTR) msg->attrList
    );

    supermsg.mID = msg->mID;
    supermsg.attrList = msgtags;

    D(bug("[sdl] hidd tags built, calling supermethod\n"));

    o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) &supermsg);

    DeletePool(tagpool);

    if (o == NULL) {
        D(bug("[sdl] supermethod failed, bailing out\n"));
        return NULL;
    }

    return (OOP_Object *) o;
}

VOID SDLGfx__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg) {
    SDL_Surface *s;

    D(bug("[sdl] SDLGfx::Dispose\n"));
    
    s = S(SDL_GetVideoSurface);
    if (s != NULL) {
        D(bug("[sdl] freeing existing video surface\n"));
        SV(SDL_FreeSurface, s);
    }

    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

OOP_Object *SDLGfx__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg) {
    OOP_Object *bmclass = NULL, *friend, *gfxhidd;
    BOOL displayable;
    HIDDT_ModeID modeid;
    HIDDT_StdPixFmt stdpixfmt;
    struct TagItem *msgtags;
    struct pHidd_Gfx_NewBitMap supermsg;

    D(bug("[sdl] SDLGfx::NewBitMap\n"));

    if (GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList)) {
        D(bug("[sdl] framebuffer bitmap, we can handle it\n"));
        bmclass = LIBBASE->bmclass;
    }

    else if (GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList)) {
        D(bug("[sdl] displayable bitmap, we can handle it\n"));
        bmclass = LIBBASE->bmclass;
    }

    else if ((HIDDT_ModeID) GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList) != vHidd_ModeID_Invalid) {
        D(bug("[sdl] bitmap with valid mode, we can handle it\n"));
        bmclass = LIBBASE->bmclass;
    }

    else if ((HIDDT_StdPixFmt) GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList) == vHidd_StdPixFmt_Unknown) {
        friend = (OOP_Object *) GetTagData(aHidd_BitMap_Friend, NULL, msg->attrList);
        if (friend != NULL) {
            OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (APTR) &gfxhidd);
            if (gfxhidd == o) {
                D(bug("[sdl] bitmap with unknown pixel format and sdl friend bitmap, we can handle it\n"));
                bmclass = LIBBASE->bmclass;
            }
        }
    }

    if (bmclass != NULL)
        msgtags = TAGLIST(
            aHidd_BitMap_ClassPtr, (IPTR) LIBBASE->bmclass,
            TAG_MORE,              (IPTR) msg->attrList
        );

    else
        msgtags = msg->attrList;

    supermsg.mID = msg->mID;
    supermsg.attrList = msgtags;

    o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) &supermsg);

    return o;
}

VOID SDLGfx__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg) {
    struct SDL_Surface *src, *dest;
    struct SDL_Rect srect, drect;
    BOOL is_onscreen;

    D(bug("[sdl] SDLGfx::CopyBox\n"));

    OOP_GetAttr(msg->src,  aHidd_SDLBitMap_Surface, &src);
    OOP_GetAttr(msg->dest, aHidd_SDLBitMap_Surface, &dest);

    if (src == NULL || dest == NULL) {
        D(bug("[sdl] missing a surface: src is 0x%08x, dest is 0x%08x. letting the superclass deal with it\n", src, dest));

        OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

        return;
    }

    srect.x = msg->srcX;
    srect.y = msg->srcY;
    srect.w = msg->width;
    srect.h = msg->height;

    drect.x = msg->destX;
    drect.y = msg->destY;

    D(bug("[sdl] blitting %dx%d rect from src 0x%08x [%d,%d] to dest 0x%08x [%d,%d]\n", msg->width, msg->height, src, msg->srcX, msg->srcY, dest, msg->destX, msg->destY));

    S(SDL_BlitSurface, src, &srect, dest, &drect);

    OOP_GetAttr(msg->dest, aHidd_SDLBitMap_IsOnScreen, &is_onscreen);
    if (is_onscreen) {
        D(bug("[sdl] refreshing onscreen surface 0x%08x\n", dest));

        SV(SDL_UpdateRect, dest, msg->destX, msg->destY, msg->width, msg->height);
    }

    return;
}
