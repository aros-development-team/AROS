/*
 * sdl.hidd - SDL graphics/sound/keyboard for AROS hosted
 * Copyright (c) 2007 Robert Norris. All rights reserved.
 * Copyright (c) 2010 The AROS Development Team. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <utility/tagitem.h>
#include <oop/oop.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "sdl_intern.h"

#define DEBUG 0
#include <aros/debug.h>

#define LIBBASE (&xsd)

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
#if DEBUG
    char driver[128] = "";
#endif
    Uint32 surftype;
    const SDL_Rect **modes;
    struct TagItem *pftags = NULL;
    int nmodes, i;
    APTR tagpool;
    struct TagItem **synctags, *modetags, *msgtags;
    struct pRoot_New supermsg;

#if DEBUG
    S(SDL_VideoDriverName, driver, sizeof(driver));
    kprintf("sdlgfx: using %s driver\n", driver);
#endif
    info = SP(SDL_GetVideoInfo);
    pixfmt = info->vfmt;
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

    modes = SP(SDL_ListModes, NULL, surftype | LIBBASE->use_fullscreen ? SDL_FULLSCREEN : 0);
    D(bug("[sdl] available modes:"));
    if (modes == NULL) {
        D(bug(" none\n"));
        nmodes = 0;
    }
    else {
        if (modes == (const SDL_Rect **) -1) {
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

        synctags[i] = AllocPooled(tagpool, sizeof(struct TagItem) * 4);
        synctags[i][0].ti_Tag = aHidd_Sync_HDisp;       synctags[i][0].ti_Data = modes[i]->w;
        synctags[i][1].ti_Tag = aHidd_Sync_VDisp;       synctags[i][1].ti_Data = modes[i]->h;
        synctags[i][2].ti_Tag = aHidd_Sync_Description; synctags[i][2].ti_Data = (IPTR)"SDL:%hx%v";
        synctags[i][3].ti_Tag = TAG_DONE;
    }

    modetags = AllocPooled(tagpool, sizeof(struct TagItem) * (nmodes+2));

    modetags[0].ti_Tag = aHidd_Gfx_PixFmtTags;   modetags[0].ti_Data = (IPTR) pftags;

    for (i = 0; i < nmodes; i++) {
        if (synctags[i] == NULL) {
            modetags[1+i].ti_Tag = TAG_IGNORE;
            continue;
        }

        modetags[1+i].ti_Tag = aHidd_Gfx_SyncTags;
        modetags[1+i].ti_Data = (IPTR) synctags[i];
    }

    modetags[1+i].ti_Tag = TAG_DONE;

    msgtags = TAGLIST(
        aHidd_Gfx_ModeTags, (IPTR)modetags,
	aHidd_Name        , (IPTR)"sdl.monitor",
	aHidd_HardwareName, (IPTR)"Simple DirectMedia Layer",
	aHidd_ProducerName, (IPTR)"SDL development team (http://libsdl.org/credits.php)",
        TAG_MORE          , (IPTR)msg->attrList
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
    
    s = SP(SDL_GetVideoSurface);
    if (s != NULL) {
        D(bug("[sdl] freeing existing video surface\n"));
        SV(SDL_FreeSurface, s);
    }

    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}

VOID SDLGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG   	     idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Gfx_IsWindowed:
	    	*msg->storage = TRUE;
		return;

	    case aoHidd_Gfx_DriverName:
		*msg->storage = (IPTR)"SDL";
		return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID SDLGfx__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
	    switch(idx)
	    {
	    case aoHidd_Gfx_ActiveCallBack:
	        xsd.cb = (void *)tag->ti_Data;
		break;

	    case aoHidd_Gfx_ActiveCallBackData:
	        xsd.cbdata = (void *)tag->ti_Data;
		break;
	    }
	}
    }
    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

OOP_Object *SDLGfx__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg) {
    struct TagItem *msgtags;
    struct pHidd_Gfx_NewBitMap supermsg;
    struct gfxdata *data = OOP_INST_DATA(cl, o);

    D(bug("[sdl] SDLGfx::NewBitMap, UtilityBase is 0x%p\n", UtilityBase));

    if (GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList) != vHidd_ModeID_Invalid) {
        D(bug("[sdl] bitmap with valid mode, we can handle it\n"));

        msgtags = TAGLIST(
            aHidd_BitMap_ClassPtr, (IPTR) LIBBASE->bmclass,
            TAG_MORE,              (IPTR) msg->attrList
        );
	D(bug("[sdl] ClassPtr is 0x%p\n", LIBBASE->bmclass));
    } else
        msgtags = msg->attrList;

    supermsg.mID = msg->mID;
    supermsg.attrList = msgtags;

    D(bug("[sdl] Calling DoSuperMethod()\n"));
    o = (OOP_Object *) OOP_DoSuperMethod(cl, o, (OOP_Msg) &supermsg);

    if (GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList))
        data->framebuffer = o;

    D(bug("[sdl] Created bitmap 0x%p\n", o));
    return o;
}

VOID SDLGfx__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg) {
    struct SDL_Surface *src = NULL;
    struct SDL_Surface *dest = NULL;
    struct SDL_Rect srect, drect;

    D(bug("[sdl] SDLGfx::CopyBox\n"));

    OOP_GetAttr(msg->src,  aHidd_SDLBitMap_Surface, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_SDLBitMap_Surface, (IPTR *)&dest);

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

    return;
}

OOP_Object *SDLGfx__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct gfxdata *data = OOP_INST_DATA(cl, o);
    struct pHidd_Gfx_Show mymsg = {msg->mID, msg->bitMap, 0};
    IPTR width, height;

    /* Resetting SDL onscreen surface will destroy its old contents.
       Copy back old bitmap data if there's one and if asked to do so */
    if (data->shownbm && (msg->flags & fHidd_Gfx_Show_CopyBack)) {
	OOP_Object *colmap;
	IPTR numentries, i;

        OOP_GetAttr(data->framebuffer, aHidd_BitMap_Width, &width);
        OOP_GetAttr(data->framebuffer, aHidd_BitMap_Height, &height);
	
        OOP_GetAttr(data->framebuffer, aHidd_BitMap_ColorMap, (IPTR *)&colmap);
        OOP_GetAttr(colmap, aHidd_ColorMap_NumEntries, &numentries);
	
	/* We need also to copy colormap (palette) */
        for (i = 0; i < numentries; i ++) {
    	    HIDDT_Color col;

	    HIDD_CM_GetColor(colmap, i, &col);
	    HIDD_BM_SetColors(data->shownbm, &col, i, 1);
        }

	/* Our CopyBox() happily ignores the GC, so set it to NULL and don't bother */
        HIDD_Gfx_CopyBox(o, data->framebuffer, 0, 0, data->shownbm, 0, 0, width, height, NULL);
    }

    /* Set up new onscreen surface if there's a new bitmap to show.
       This will change resolution */
    if (msg->bitMap) {
        HIDDT_ModeID modeid = vHidd_ModeID_Invalid;
	IPTR depth;
	OOP_Object *sync, *pixfmt;
        SDL_Surface *s;
	struct TagItem bmtags[] = {
	    {aHidd_SDLBitMap_Surface, 0},
	    {aHidd_BitMap_Width	    , 0},
	    {aHidd_BitMap_Height    , 0},
	    {aHidd_BitMap_PixFmt    , 0},
	    {TAG_DONE               , 0}
	};

        /* Ask ModeID from our bitmap */
	OOP_GetAttr(msg->bitMap, aHidd_BitMap_ModeID, &modeid);
	if (modeid == vHidd_ModeID_Invalid)
	    return NULL;

	HIDD_Gfx_GetMode(o, modeid, &sync, &pixfmt);
        OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
	OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);

	/* Set up new onscreen surface */
        s = SP(SDL_SetVideoMode, width, height, depth,
              (LIBBASE->use_hwsurface  ? SDL_HWSURFACE | SDL_HWPALETTE : SDL_SWSURFACE) |
              (LIBBASE->use_fullscreen ? SDL_FULLSCREEN                : 0) |
              SDL_ANYFORMAT);
	if (!s)
	    return NULL;

	/* Tell new parameters to the framebuffer object */
	bmtags[0].ti_Data = (IPTR)s;
	bmtags[1].ti_Data = width;
	bmtags[2].ti_Data = height;
	bmtags[3].ti_Data = (IPTR)pixfmt;
	OOP_SetAttrs(data->framebuffer, bmtags);
    }
    
    data->shownbm = msg->bitMap;

    /* Framebuffer contents has been destroyed, so call superclass without fHidd_Gfx_Show_CopyBack */
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
}

static struct OOP_MethodDescr SDLGfx_Root_descr[] = {
    {(OOP_MethodFunc)SDLGfx__Root__New    , moRoot_New    },
    {(OOP_MethodFunc)SDLGfx__Root__Dispose, moRoot_Dispose},
    {(OOP_MethodFunc)SDLGfx__Root__Get    , moRoot_Get    },
    {(OOP_MethodFunc)SDLGfx__Root__Set    , moRoot_Set    },
    {NULL				  , 0             }
};
#define NUM_SDLGfx_Root_METHODS 4

static struct OOP_MethodDescr SDLGfx_Hidd_Gfx_descr[] = {
    {(OOP_MethodFunc)SDLGfx__Hidd_Gfx__NewBitMap, moHidd_Gfx_NewBitMap},
    {(OOP_MethodFunc)SDLGfx__Hidd_Gfx__Show, moHidd_Gfx_Show},
    {(OOP_MethodFunc)SDLGfx__Hidd_Gfx__CopyBox, moHidd_Gfx_CopyBox},
    {NULL, 0}
};
#define NUM_SDLGfx_Hidd_Gfx_METHODS 3

struct OOP_InterfaceDescr SDLGfx_ifdescr[] = {
    {SDLGfx_Root_descr    , IID_Root    , NUM_SDLGfx_Root_METHODS    },
    {SDLGfx_Hidd_Gfx_descr, IID_Hidd_Gfx, NUM_SDLGfx_Hidd_Gfx_METHODS},
    {NULL                 , NULL                                     }
};
