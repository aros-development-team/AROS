/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for GDI hidd.
    Lang: English.
    
    Note: this implementation ignores GC_COLMASK. Windows GDI has no way to support it,
          however color masks seem to be not used anywhere in AROS.
*/

/****************************************************************************************/

#define __OOP_NOATTRBASES__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/oop.h>
#include <proto/utility.h>
#include <exec/alerts.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <aros/symbolsets.h>

#define SDEBUG 0
#define DEBUG 0
#define DEBUG_TEXT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "gdi.h"

#include "bitmap.h"

/****************************************************************************************/

#define AO(x)             (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

struct bitmapinfo_mono
{ 
    BITMAPINFOHEADER bmiHeader; 
    UWORD            bmiColors[2];
};

static OOP_AttrBase HiddBitMapAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGDIBitMapAB;

static struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_BitMap   , &HiddBitMapAttrBase   },
    { IID_Hidd_Sync     , &HiddSyncAttrBase     },
    { IID_Hidd_PixFmt   , &HiddPixFmtAttrBase   },
    /* Private bases */
    { IID_Hidd_GDIBitMap, &HiddGDIBitMapAB      },
    { NULL              , NULL                  }
};

/****************************************************************************************/

#ifdef DEBUG_PLANAR
#define PRINT_PLANE(bm, n, startx, xlim, ylim)                                          \
{                                                                                       \
    ULONG start = startx / 8;                                                           \
    ULONG xlimit = xlim / 8;                                                            \
    ULONG ylimit = (ylim < bm.Rows) ? ylim : bm.Rows;                                   \
    UBYTE *plane = bm.Planes[n] + start;                                                \
    UBYTE i;                                                                            \
    ULONG x, y;                                                                         \
                                                                                        \
    if (xlimit > (start - bm.BytesPerRow))                                              \
        xlimit = start - bm.BytesPerRow;                                                \
    bug("[GDIBitMap] Plane %u data (%u pixels from %u, address 0x%p):\n", n, xlimit * 8, start * 8, plane); \
    for (y = 0; y < ylimit; y++) {                                                      \
        for (x = 0; x < xlimit; x++) {                                                  \
            for (i = 0x80; i; i >>= 1)                                                  \
                bug((plane[x] & i) ? "#" : ".");                                        \
        }                                                                               \
        bug("\n");                                                                      \
        plane += bm.BytesPerRow;                                                        \
    }                                                                                   \
}

#define PRINT_MONO_DC(dc, startx, starty, width, height) \
{                                                        \
    ULONG x, y;                                          \
                                                         \
    bug("[GDIBitMap] Device context data:\n");           \
    for (y = starty; y < height; y++) {                  \
        for (x = startx; x < width; x++) {               \
            ULONG pix = GDICALL(GetPixel, dc, x, y);     \
                                                         \
            bug(pix ? "." : "#");                        \
        }                                                \
        bug("\n");                                       \
    }                                                    \
}

#else
#define PRINT_PLANE(bm, n, startx, xlim, ylim)
#define PRINT_MONO_DC(dc, startx, starty, width, height)
#endif

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    
    DB2(bug("[GDI] hidd.bitmap.gdibitmap::PutPixel(0x%p): (%lu, %lu) = 0x%08lX\n", o, msg->x, msg->y, msg->pixel));
    Forbid();
    GDICALL(SetROP2, data->dc, R2_COPYPEN);
    GDICALL(SetPixel, data->dc, msg->x, msg->y, msg->pixel);
    Permit();
    CHECK_STACK
}

/****************************************************************************************/

HIDDT_Pixel GDIBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_Pixel         pixel;

    Forbid();
    pixel = GDICALL(GetPixel, data->dc, msg->x, msg->y);
    Permit();
    CHECK_STACK

    return pixel;
}

/****************************************************************************************/

static void FillRect(struct bitmap_data *data, ULONG col, ULONG mode, ULONG minX, ULONG minY, ULONG width, ULONG height)
{
    APTR br, orig_br;
    DB2(bug("[GDI] Brush color 0x%08lX, mode 0x%08lX\n", col, mode));

    Forbid();
    br = GDICALL(CreateSolidBrush, col);
    if (br) {
        orig_br = GDICALL(SelectObject, data->dc, br);
        GDICALL(PatBlt, data->dc, minX, minY, width, height, mode);
        GDICALL(SelectObject, data->dc, orig_br);
        GDICALL(DeleteObject, br);
    }
    Permit();
}

/* Table of raster operations (ROPs) corresponding to AROS GC drawmodes */
static ULONG Fill_DrawModeTable[] = {
    BLACKNESS,
    0x00A000C9,
    0x00500325,
    PATCOPY,
    0x000A0329,
    0x00AA0029,
    PATINVERT,
    0x00FA0089,
    0x000500A9,
    0x00A50065, /* PDnx - not sure */
    DSTINVERT,
    0x00F50225,
    0x000F0001,
    0x00AF0229,
    0x005F00E9,
    WHITENESS
};

VOID GDIBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG col = GC_FG(msg->gc);
    ULONG mode = Fill_DrawModeTable[GC_DRMD(msg->gc)];

    D(bug("[GDI] hidd.bitmap.gdibitmap::FillRect(0x%p, %d,%d,%d,%d)\n", o, msg->minX, msg->minY, msg->maxX, msg->maxY));
    FillRect(data, col, mode, msg->minX, msg->minY, msg->maxX - msg->minX + 1, msg->maxY - msg->minY + 1);
    CHECK_STACK
}

/****************************************************************************************/

/* Raster operations for drawing primitives */
static ULONG R2_DrawModeTable[] = {
    R2_BLACK,
    R2_MASKPEN,     /* bitmap AND pen       */
    R2_MASKPENNOT,  /* NOT bitmap AND pen   */
    R2_COPYPEN,     /* pen                  */
    R2_MASKNOTPEN,  /* bitmap AND NOT pen   */
    R2_NOP,         /* bitmap               */
    R2_XORPEN,      /* bitmap XOR pen       */
    R2_MERGEPEN,    /* bitmap OR pen        */
    R2_NOTMERGEPEN, /* NOT (bitmap OR pen)  */
    R2_NOTXORPEN,   /* NOT (bitmap XOR pen) */
    R2_NOT,         /* NOT bitmap           */
    R2_MERGEPENNOT, /* NOT bitmap OR pen    */
    R2_NOTCOPYPEN,  /* NOT pen              */
    R2_MERGENOTPEN, /* NOT pen OR bitmap    */
    R2_NOTMASKPEN,  /* NOT (pen AND bitmap) */
    R2_WHITE
};

ULONG GDIBM__Hidd_BitMap__DrawPixel(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawPixel *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG col, mode;

    DB2(bug("[GDI] hidd.bitmap.gdibitmap::DrawPixel(0x%p): (%lu, %lu)\n", o, msg->x, msg->y));    
    col = GC_FG(msg->gc);
    mode = R2_DrawModeTable[GC_DRMD(msg->gc)];
    
    Forbid();
    GDICALL(SetROP2, data->dc, mode);
    GDICALL(SetPixel, data->dc, msg->x, msg->y, col);
    Permit();
    CHECK_STACK
    return 0;    
}

/****************************************************************************************/

/* Raster operations for copying a bitmap */
ULONG Copy_DrawModeTable[] = {
    BLACKNESS,
    SRCAND,      /* DSa  - src AND dest       */
    SRCERASE,    /* SDna - src AND NOT dest   */
    SRCCOPY,     /* S    - src                */
    0x00220326,  /* DSna - NOT src AND dest   */
    0x00AA0029,  /* D    - dest               */
    SRCINVERT,   /* DSx  - src XOR dest       */
    SRCPAINT,    /* DSo  - src OR dest        */
    NOTSRCERASE, /* DSon - NOT (src OR dest)  */
    0x00990066,  /* DSxn - NOT (src XOR dest) */
    DSTINVERT,   /* Dn   - NOT dest           */
    0x00DD0228,  /* SDno - src OR NOT dest    */
    NOTSRCCOPY,  /* Sn   - NOT src            */
    MERGEPAINT,  /* DSno - NOT src OR dest    */
    0x007700E6,  /* DSan - NOT (src AND dest) */
    WHITENESS
};
        
VOID GDIBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *src_pixfmt, *dst_pixfmt;
    OOP_Object *gfxhidd;
    HIDDT_StdPixFmt src_stdpf;
    APTR buf, src, dst;
    ULONG bufmod, bufsize;
    BITMAPINFOHEADER bitmapinfo =
    {
        sizeof(BITMAPINFOHEADER),
        0, 0,
        1,
        32,
        BI_RGB,
        0, 0, 0,  0, 0
    };

/*  EnterFunc(bug("GDIGfx.BitMap::PutImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
                  msg->pixels, msg->x, msg->y, msg->width, msg->height));*/
    
    switch(msg->pixFmt) {
    case vHidd_StdPixFmt_Native:
    case vHidd_StdPixFmt_Native32:
        src_stdpf = vHidd_StdPixFmt_0BGR32_Native;
        break;
    default:
        src_stdpf = msg->pixFmt;
    }

    bufmod = msg->width * sizeof(HIDDT_Pixel);
    bufsize = bufmod * msg->height;
    buf = AllocMem(bufsize, MEMF_ANY);
    if (buf)
    {
        ULONG drmd = GC_DRMD(msg->gc);

        OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
        src_pixfmt = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(gfxhidd, src_stdpf);
        /* DIB pixels are expected to be 0x00RRGGBB */
        dst_pixfmt = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(gfxhidd, vHidd_StdPixFmt_0RGB32_Native);
        src = msg->pixels;
        dst = buf;
        HIDD_BM_ConvertPixels(o, &src, src_pixfmt, msg->modulo, &dst, dst_pixfmt, bufmod,
                              msg->width, msg->height, NULL);
        bitmapinfo.biWidth = msg->width;
        bitmapinfo.biHeight = -msg->height; /* Minus here means top-down bitmap */
        Forbid();
        GDICALL(StretchDIBits, data->dc, msg->x, msg->y, msg->width, msg->height, 0, 0, msg->width, msg->height, buf, (BITMAPINFO *)&bitmapinfo, DIB_RGB_COLORS, Copy_DrawModeTable[drmd]);
        Permit();
        FreeMem(buf, bufsize);
    }
    CHECK_STACK
}

/****************************************************************************************/

/* TODO: These little stubs help to detect methods calls */

VOID GDIBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutImageLUT *msg)
{
    EnterFunc(bug("GDIGfx.BitMap::PutImageLUT(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
                  msg->pixels, msg->x, msg->y, msg->width, msg->height));
        
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    CHECK_STACK
}

VOID GDIBM__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImageLUT *msg)
{
    EnterFunc(bug("GDIGfx.BitMap::GetImageLUT(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
                  msg->pixels, msg->x, msg->y, msg->width, msg->height));
        
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    CHECK_STACK
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__GetImage(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_GetImage *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    HIDDT_PixelFormat *src_pixfmt, *dst_pixfmt;
    OOP_Object *gfxhidd;
    HIDDT_StdPixFmt dst_stdpf;
    APTR tmp_dc, tmp_bitmap, dc_bitmap;
    APTR buf, src, dst;
    ULONG bufmod, bufsize;
    BITMAPINFOHEADER bitmapinfo = {
        sizeof(BITMAPINFOHEADER),
        0, 0,
        1,
        32,
        BI_RGB,
        0, 0, 0,  0, 0
    };

/*  EnterFunc(bug("GDIGfx.BitMap::GetImage(pa=%p, x=%d, y=%d, w=%d, h=%d)\n",
                  msg->pixels, msg->x, msg->y, msg->width, msg->height));*/

    switch(msg->pixFmt) {
    case vHidd_StdPixFmt_Native:
    case vHidd_StdPixFmt_Native32:
        dst_stdpf = vHidd_StdPixFmt_0BGR32_Native;
        break;
    default:
        dst_stdpf = msg->pixFmt;
    }

    bufmod = msg->width * sizeof(HIDDT_Pixel);
    bufsize = bufmod * msg->height;
    buf = AllocMem(bufsize, MEMF_ANY);
    if (buf) {
        /* First we have to extract requested rectangle into temporary bitmap because GetDIBits() can limit only scan lines number */
        Forbid();
        tmp_dc = GDICALL(CreateCompatibleDC, data->display);
        if (tmp_dc) {
            tmp_bitmap = GDICALL(CreateCompatibleBitmap, data->display, msg->width, msg->height);
            if (tmp_bitmap) {
                dc_bitmap = GDICALL(SelectObject, tmp_dc, tmp_bitmap);
                if (dc_bitmap) {
                    GDICALL(BitBlt, tmp_dc, 0, 0, msg->width, msg->height, data->dc, msg->x, msg->y, SRCCOPY);
                    bitmapinfo.biWidth = msg->width;
                    bitmapinfo.biHeight = -msg->height; /* Minus here means top-down bitmap */
                    GDICALL(GetDIBits, tmp_dc, tmp_bitmap, 0, msg->height, buf, (BITMAPINFO *)&bitmapinfo, DIB_RGB_COLORS);
                    GDICALL(SelectObject, tmp_dc, dc_bitmap);
                }
                GDICALL(DeleteObject, tmp_bitmap);
            }
            GDICALL(DeleteDC, tmp_dc);
        }
        Permit();
        OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *)&gfxhidd);
        /* DIB pixels will be 0x00RRGGBB) */
        src_pixfmt = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(gfxhidd, vHidd_StdPixFmt_0RGB32_Native);
        dst_pixfmt = (HIDDT_PixelFormat *)HIDD_Gfx_GetPixFmt(gfxhidd, dst_stdpf);
        dst = msg->pixels;
        src = buf;
        HIDD_BM_ConvertPixels(o, &src, src_pixfmt, bufmod, &dst, dst_pixfmt, msg->modulo,
                              msg->width, msg->height, NULL);
        FreeMem(buf, bufsize);
    }
    CHECK_STACK
}

/****************************************************************************************/

VOID GDIBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    ULONG               idx;
    
    if (IS_GDIBM_ATTR(msg->attrID, idx)) {
        switch (idx)
        {
        case aoHidd_GDIBitMap_DeviceContext:
            *msg->storage = (IPTR)data->dc;
            return;
        }
    } else if (IS_BM_ATTR(msg->attrID, idx)) {
        switch (idx)
        {
        case aoHidd_BitMap_LeftEdge:
            *msg->storage = data->bm_left;
            return;
        case aoHidd_BitMap_TopEdge:
            *msg->storage = data->bm_top;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

VOID GDIBM__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, obj);
    struct TagItem  *tag, *tstate;
    ULONG           idx;
    BOOL            change_position = FALSE;

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if (IS_BM_ATTR(tag->ti_Tag, idx)) {
            switch(idx)
            {
            case aoHidd_BitMap_LeftEdge:
                data->bm_left = tag->ti_Data;
                change_position = TRUE;
                break;
            case aoHidd_BitMap_TopEdge:
                data->bm_top = tag->ti_Data;
                change_position = TRUE;
                break;
            }
        }
    }

    if (change_position) {
        /* Fix up position. We can completely scroll out
           of our window into all 4 sides, but not more */
        if (data->bm_left > data->win_width)
            data->bm_left = data->win_width;
        else if (data->bm_left < -data->bm_width)
            data->bm_left = -data->bm_width;
        if (data->bm_top > data->win_height)
            data->bm_top = data->win_height;
        else if (data->bm_top < -data->bm_height)
            data->bm_top = -data->bm_height;

        Forbid();
        if (data->window)
            USERCALL(SetWindowPos, data->window, NULL, data->bm_left - 1, data->bm_top - 1, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
        Permit();
    }

    OOP_DoSuperMethod(cl, obj, (OOP_Msg)msg);
}

/****************************************************************************************/

OOP_Object *GDIBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    OOP_Object  *friend = NULL, *pixfmt;
    APTR         display, my_dc, my_bitmap;
    APTR         orig_bitmap = NULL;
    ULONG        width, height;
    HIDDT_ModeID modeid;
    IPTR         win_width  = 0;
    IPTR         win_height = 0;
    IPTR         depth;
    IPTR         attrs[num_Hidd_BitMap_Attrs];
    struct bitmap_data *data;
    
    DECLARE_ATTRCHECK(bitmap);

    EnterFunc(bug("GDIBM::New()\n"));    
    /* Parse the attributes */
    if (0 != OOP_ParseAttrs(msg->attrList, attrs, num_Hidd_BitMap_Attrs,
                            &ATTRCHECK(bitmap), HiddBitMapAttrBase))
    {
        D(kprintf("!!! GDIGfx::BitMap() FAILED TO PARSE ATTRS !!!\n"));
        
        return NULL;
    }
    
    if (GOT_BM_ATTR(Friend))
        friend = (OOP_Object *)attrs[AO(Friend)];
    else 
        friend = NULL;
        
    width  = attrs[AO(Width)];
    height = attrs[AO(Height)];
    pixfmt = (OOP_Object *)attrs[AO(PixFmt)];

    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);

    /* Get the device context from the friend bitmap */
/*  if (NULL != friend)
    {
        OOP_GetAttr(friend, aHidd_GDIBitMap_Drawable, (IPTR *)&friend_drawable);
    }*/

    D(bug("Creating GDI bitmap: %ldx%ldx%ld\n", width, height, depth));
    display = (APTR)GetTagData(aHidd_GDIBitMap_SysDisplay, 0, msg->attrList);
    modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    /* This relies on the fact that bitmaps with aHidd_BitMap_Displayable set to TRUE always
       also get aHidd_BitMap_ModeID with valid value. Currently this seems to be true and i
       beleive it should stay so */
    if (modeid != vHidd_ModeID_Invalid) {
        OOP_Object *gfx = (OOP_Object *)attrs[AO(GfxHidd)];
        OOP_Object *sync, *pixfmt;

        D(bug("[GDI] Display driver object: 0x%p\n", gfx));
        HIDD_Gfx_GetMode(gfx, modeid, &sync, &pixfmt);
        OOP_GetAttr(sync, aHidd_Sync_HDisp, &win_width);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &win_height);
        D(bug("[GDI] Display window size: %dx%d\n", win_width, win_height));
    }

    Forbid();
    my_dc = GDICALL(CreateCompatibleDC, display);
    D(bug("[GDI] Memory device context: 0x%p\n", my_dc));
    if (my_dc) {
        my_bitmap = GDICALL(CreateCompatibleBitmap, display, (width + 15) & ~15, height);
        D(bug("[GDI] Memory bitmap: 0x%p\n", my_bitmap));
        if (my_bitmap)
            orig_bitmap = GDICALL(SelectObject, my_dc, my_bitmap);
        D(bug("[GDI] Olriginal DC bitmap: 0x%p\n", orig_bitmap));
    }
    Permit();

    if (!my_dc)
        return NULL;
    if (!orig_bitmap)
        goto dispose_bitmap;
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    D(bug("[GDI] Object created by superclass: 0x%p\n", o));
    if (o) {
        data = OOP_INST_DATA(cl, o);
        /* Get some info passed to us by the gdigfxhidd class */
        data->dc         = my_dc;
        data->bitmap     = my_bitmap;
        data->dc_bitmap  = orig_bitmap;
        data->display    = display;
        data->window     = NULL;
        data->win_width  = win_width;
        data->win_height = win_height;
        data->bm_width   = width;
        data->bm_height  = height;
        data->bm_left    = 0;
        data->bm_top     = 0;
        CHECK_STACK
        ReturnPtr("GDIGfx.BitMap::New()", OOP_Object *, o);
    } /* if (object allocated by superclass) */
dispose_bitmap:    
    Forbid();
    if (orig_bitmap)
        GDICALL(SelectObject, my_dc, orig_bitmap);
    if (my_bitmap)
        GDICALL(DeleteObject, my_bitmap);
    GDICALL(DeleteDC, my_dc);
    Permit();
    
    ReturnPtr("GDIGfx.BitMap::New()", OOP_Object *, NULL);
    
}

/****************************************************************************************/

VOID GDIBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    EnterFunc(bug("GDIGfx.BitMap::Dispose()\n"));
    
    Forbid();
    if (data->node.mln_Pred)
        Remove((struct Node *)data);
    if (data->window)
        NATIVECALL(GDI_PutMsg, data->window, WM_CLOSE, 0, 0);
    if (data->dc_bitmap)
        GDICALL(SelectObject, data->dc, data->dc_bitmap);
    if (data->bitmap)
        GDICALL(DeleteObject, data->bitmap);
    if (data->dc)
        GDICALL(DeleteDC, data->dc);
    Permit();
    
    OOP_DoSuperMethod(cl, o, msg);

    CHECK_STACK    
    ReturnVoid("GDIGfx.BitMap::Dispose");
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__Clear(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_Clear *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    IPTR width, height;
    
    EnterFunc(bug("[GDI] hidd.bitmap.gdibitmap::Clear()\n"));
    
    /* Get width & height from bitmap superclass */
  
    OOP_GetAttr(o, aHidd_BitMap_Width,  &width);
    OOP_GetAttr(o, aHidd_BitMap_Height, &height);
    
    FillRect(data, GC_BG(msg->gc), PATCOPY, 0, 0, width, height);
}

/****************************************************************************************/

VOID GDIBM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct bitmap_data *data = OOP_INST_DATA(cl, o);

    Forbid();
    if (data->window) {
        RECT r = {
            msg->x,
            msg->y,
            msg->x + msg->width,
            msg->y + msg->height
        };

        USERCALL(RedrawWindow, data->window, &r, NULL, RDW_INVALIDATE|RDW_UPDATENOW);
    }
    Permit();
}

/****************************************************************************************/

static int GDIBM_Init(LIBBASETYPEPTR LIBBASE)
{
    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int GDIBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(GDIBM_Init, 0);
ADD2EXPUNGELIB(GDIBM_Expunge, 0);
