/*
    Copyright  1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for native Amiga chipset.
    Lang: English.
    
*/

/****************************************************************************************/

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
#include <hidd/gfx.h>
#include <aros/symbolsets.h>

#define CMDDEBUGUNIMP(x)
#define CMDDEBUGPIXEL(x)
#define DEBUG_TEXT(x)
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

#include "amigavideo_hidd.h"
#include "amigavideo_bitmap.h"

#include "chipset.h"
#include "blitter.h"

/****************************************************************************************/

#define AO(x) 	    	  (aoHidd_BitMap_ ## x)
#define GOT_BM_ATTR(code) GOT_ATTR(code, aoHidd_BitMap, bitmap)

/****************************************************************************************/

static void setrtg(struct amigavideo_staticdata *csd, BOOL showrtg)
{
}

OOP_Object *AmigaVideoBM__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    IPTR width, height, depth, disp;
    BOOL ok = TRUE;      
    struct amigabm_data *data;
    struct BitMap *pbm = NULL;
    struct pRoot_New mymsg = *msg;
    struct TagItem tags[] = {
        { aHidd_BitMap_Align, csd->aga ? 64 : 16 },
        { TAG_MORE, (IPTR) msg->attrList },
        { TAG_END, 0 }
    };

    DB2(bug("[AmigaVideo:Bitmap] %s()\n", __func__));

    mymsg.attrList = tags;
    o =(OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (NULL == o)
        return NULL;
        
    data = OOP_INST_DATA(cl, o);
    memset(data, 0, sizeof  (*data));

    data->align = csd->aga ? 64 : 16; // AGA 64-bit fetchmode needs 8-byte alignment

    /* Get some data about the dimensions of the bitmap */
    OOP_GetAttr(o, aHidd_BitMap_Width,	&width);
    OOP_GetAttr(o, aHidd_BitMap_Height,	&height);
    OOP_GetAttr(o, aHidd_BitMap_Depth, &depth);
    OOP_GetAttr(o, aHidd_BitMap_Displayable, &disp);
    OOP_GetAttr(o, aHidd_PlanarBM_BitMap, &pbm);

    DB2(bug("%dx%dx%d\n", width, height, depth));

    /* We cache some info */
    data->width = width;
    data->bytesperrow = ((width + data->align - 1) & ~(data->align - 1)) / 8;
    data->height = height;
    data->depth = depth;
    data->pixelcacheoffset = -1;
    data->pbm = pbm;
      
    if (!ok) {
        OOP_MethodID dispose_mid;

        dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);

        o = NULL;
    }
    
    DB2(bug("ret=%x bm=%x\n", o, data));

    return o;
}

VOID AmigaVideoBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct amigabm_data    *data;

    data = OOP_INST_DATA(cl, o);

    DB2(
      bug("[AmigaVideo:Bitmap] %s(0x%p)\n", __func__, o);
      bug("[AmigaVideo:Bitmap] %s: data @ 0x%p\n", __func__, data);
     )
    if (data->disp)
    {
        DB2(bug("[AmigaVideo:Bitmap] %s: removing displayed bitmap?!\n", __func__);)
    }

    OOP_DoSuperMethod(cl, o, msg);
    
    return;
}


VOID AmigaVideoBM__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *UtilityBase = csd->cs_UtilityBase;
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG   	    idx;
    BOOL moved = FALSE;

    DB2(bug("[AmigaVideo:Bitmap] %s()\n", __func__));
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        DB2(bug("%d/%d\n", tag->ti_Tag, tag->ti_Data));
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            DB2(bug("->%d\n", idx));
            switch(idx)
            {
            case aoHidd_BitMap_Focus:
                    {
                        volatile struct Custom *custom = (struct Custom*)0xdff000;
                        struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
                        D(bug("[AmigaVideo:Bitmap] %s: aoHidd_BitMap_Focus\n", __func__);)
                        custom->bplcon0 = GfxBase->system_bplcon0;
                    }
                    break;
            case aoHidd_BitMap_Visible:
                    data->disp = tag->ti_Data;
                    if (data->disp) {
                        setrtg(csd, FALSE);
                        setbitmap(csd, data);
                    } else {
                        resetmode(csd);
                        setrtg(csd, TRUE);
                    }
                    break;
            case aoHidd_BitMap_LeftEdge:
                    if (data->leftedge != tag->ti_Data) {
                        data->leftedge = tag->ti_Data;
                        moved = TRUE;
                    }
                    break;
            case aoHidd_BitMap_TopEdge:
                    if (data->topedge != tag->ti_Data) {
                        data->topedge = tag->ti_Data;
                        if (data->topedge < 0)
                            data->topedge = 0;
                        if (data->topedge >= data->height)
                            data->topedge = data->height - 1;
                        moved = TRUE;
                    }
                    break;
            }
        }
    }
    DB2(bug("AmigaVideoBM__Root__Set Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (moved && csd->disp == data)
        setscroll(csd, data);
}

VOID AmigaVideoBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    DB2(bug("AmigaVideoBM__Root__Get %d, Attr=%d AmigaVideoBitmap=%d\n", msg->attrID, __IHidd_Attr, __IHidd_BitMap_AmigaVideo));
    if (IS_AmigaVideoBM_ATTR(msg->attrID, idx)) {
        DB2(bug("AVBM=%d\n", idx));
        switch (idx)
        {
        case aoHidd_BitMap_AmigaVideo_Drawable:
            *msg->storage = TRUE;
            return;
        }
    } else if (IS_BITMAP_ATTR(msg->attrID, idx)) {
        DB2(bug("BM=%d\n", idx));
        switch (idx)
        {
        case aoHidd_BitMap_LeftEdge:
            *msg->storage = data->leftedge;
            return;
        case aoHidd_BitMap_TopEdge:
            *msg->storage = data->topedge;
            return;
        case aoHidd_BitMap_Visible:
            *msg->storage = data->disp;
            return;
        case aoHidd_BitMap_Align:
            *msg->storage = csd->aga ? 64 : 16;
            return;
        case aoHidd_BitMap_BytesPerRow:
            if (data->bytesperrow == 0) {
                IPTR width = 0;
                IPTR align = csd->aga ? 64 : 16;
                OOP_GetAttr(o, aHidd_BitMap_Width, &width);
                *msg->storage = ((width + align - 1) & ~(align - 1)) / 8;
            } else {
                *msg->storage = data->bytesperrow;
            }
            return;
        }
    }
    DB2(bug("AmigaVideoBM__Root__Get Exit\n"));
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

static int AmigaVideoBM_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[AmigaVideo:Bitmap] %s()\n", __func__));
    return TRUE; //return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int AmigaVideoBM_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[AmigaVideo:Bitmap] %s()\n", __func__));
    //OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(AmigaVideoBM_Init, 0);
ADD2EXPUNGELIB(AmigaVideoBM_Expunge, 0);

/****************************************************************************************/

BOOL AmigaVideoBM__Hidd_BitMap__SetColors(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    struct amigavideo_staticdata *csd = CSD(cl);

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
        return FALSE;
    return setcolors(csd, msg, data->disp);
}

/****************************************************************************************/

#define CLEARCACHE flushpixelcache(data)
/* Better than nothing but m68k assembly C2P still needed for best performance */
static void flushpixelcache(struct amigabm_data *data)
{
    UBYTE i, x;
    ULONG offset = data->pixelcacheoffset;
    struct BitMap *bm = data->pbm;
    UBYTE **plane = bm->Planes;

    if (data->writemask) {
        ULONG tmpplanes[8];
        ULONG pixel, notpixel, wmask;
        if (~data->writemask) {
            for (i = 0; i < bm->Depth; i++) {
                if (plane[i] == (UBYTE*)-1)
                    tmpplanes[i] = 0xffffffff;
                else if (plane[i] == NULL)
                    tmpplanes[i] = 0x00000000;
                else
                    tmpplanes[i] = *((ULONG*)(plane[i] + offset));
            }
        }
        pixel = 0x80000000;
        wmask = 1;
        for (x = 0; pixel; x++, pixel >>= 1, wmask <<= 1) {
            if (data->writemask & wmask) {
                UBYTE c = data->pixelcache[x];
                UBYTE mask = 1;
                notpixel = ~pixel;
                for (i = 0; i < data->depth; i++, mask <<= 1) {
                    if (plane[i] != NULL && plane[i] != (UBYTE *)-1) {
                        if (c & mask)
                            tmpplanes[i] |= pixel;
                        else
                            tmpplanes[i] &= notpixel;
                    }
                }
            }
        }
        for (i = 0; i < data->depth; i++) {
            if (plane[i] != NULL && plane[i] != (UBYTE *)-1)
                *((ULONG*)(plane[i] + offset)) = tmpplanes[i];
        }
    }
    data->pixelcacheoffset = -1;
    data->writemask = 0;
}

VOID AmigaVideoBM__Hidd_BitMap__PutPixel(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutPixel *msg)
{
    struct amigabm_data    *data;
    ULONG   	    	    offset;
    UBYTE   	    	    bit;
    
    data = OOP_INST_DATA(cl, o);
    
    offset = msg->x / 8 + msg->y * data->bytesperrow;  
    if ((offset & ~3) != data->pixelcacheoffset) {
        CLEARCACHE;
        data->pixelcacheoffset = offset & ~3;
    }
    bit = (offset - data->pixelcacheoffset) * 8 + (msg->x & 7);
    data->pixelcache[bit] = msg->pixel;
    data->writemask |= 1 << bit;

    CMDDEBUGPIXEL(bug("PutPixel: %dx%d %x\n", msg->x, msg->y, msg->pixel);)
}

/****************************************************************************************/

ULONG AmigaVideoBM__Hidd_BitMap__GetPixel(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_GetPixel *msg)
{
    struct amigabm_data    *data;
    ULONG   	    	    offset;
    UBYTE   	    	    i, c, bit;

    data = OOP_INST_DATA(cl, o);
    offset = msg->x / 8 + msg->y * data->bytesperrow;

    if ((offset & ~3) != data->pixelcacheoffset) {
        ULONG tmpplanes[8], mask;
        UBYTE x;
        UBYTE **plane = data->pbm->Planes;

        CLEARCACHE;
        data->pixelcacheoffset = offset & ~3;
        for (i = 0; i < data->depth; i++) {
            if (plane[i] == (UBYTE*)-1)
                tmpplanes[i] = 0xffffffff;
            else if (plane[i] == NULL)
                tmpplanes[i] = 0x00000000;
            else
                tmpplanes[i] = *((ULONG*)(plane[i] + data->pixelcacheoffset));
        }
        mask = 0x80000000;
        for (x = 0; mask; x++, mask >>= 1) {
            UBYTE c = 0, pixel = 1;
            for(i = 0; i < data->depth; i++, pixel <<= 1) {
                if (tmpplanes[i] & mask)
                    c |= pixel;
            }
            data->pixelcache[x] = c;
        }
    }
    bit = (offset - data->pixelcacheoffset) * 8 + (msg->x & 7);
    c = data->pixelcache[bit];
    CMDDEBUGPIXEL(bug("GetPixel: %dx%d %x\n", msg->x, msg->y, c);)
    return c;
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__DrawLine(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_DrawLine *msg)
{
    OOP_Object  *gc = msg->gc;
    HIDDT_Pixel fg = GC_FG(gc);
    HIDDT_DrawMode mode = GC_DRMD(gc);
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    APTR doclip = GC_DOCLIP(gc);
    WORD linepatmask = (1 << GC_LINEPATCNT(gc)) - 1;

    CLEARCACHE;
    if ((linepatmask & GC_LINEPAT(gc)) != linepatmask) {
        // TODO: blitter pattern support
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }    
    if (msg->x1 == msg->x2 || msg->y1 == msg->y2) {
        WORD x1 = msg->x1, x2 = msg->x2;
        WORD y1 = msg->y1, y2 = msg->y2;
        if (x1 > x2) {
            WORD tmp = x1;
            x1 = x2;
            x2 = tmp;
        }
        if (y1 > y2) {
            WORD tmp = y1;
            y1 = y2;
            y2 = tmp;
        }
        if (doclip && (x1 > GC_CLIPX2(gc)
            || x2 < GC_CLIPX1(gc)
            || y1 > GC_CLIPY2(gc)
            || y2 < GC_CLIPY1(gc)))
            return;

        if (x2 < 0)
            return;
        if (y2 < 0)
           return;
        if (x1 >= data->width)
           return;
        if (y1 >= data->height)
           return;
        if (x1 < 0)
           x1 = 0;
        if (y1 < 0)
           y1 = 0;
        if (x2 >= data->width)
            x2 = data->width - 1;
        if (y2 >= data->height)
           y2 = data->height - 1;

        if (doclip) {
            if (x1 == x2) {
                if (y1 < GC_CLIPY1(gc))
                    y1 = GC_CLIPY1(gc);
                if (y2 > GC_CLIPY2(gc))
                    y2 = GC_CLIPY2(gc);
            } else {
                if (x1 < GC_CLIPX1(gc))
                    x1 = GC_CLIPX1(gc);
                else if (x2 > GC_CLIPX2(gc))
                    x2 = GC_CLIPX2(gc);
            }
        }

        if (!blit_fillrect(csd, data->pbm, x1, y1, x2, y2, fg, mode))
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    } else {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)
    }
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o,
                                 struct pHidd_BitMap_PutPattern *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    CLEARCACHE;
    D(bug("PutPattern(%dx%d,%dx%d,mask=%x,mod=%d,masksrcx=%d)\n(%x,%dx%d,h=%d,d=%d,lut=%x,inv=%d)(fg=%d,bg=%d,colexp=%d,drmd=%d)\n",
        msg->x, msg->y, msg->width, msg->height,
        msg->mask, msg->maskmodulo, msg->masksrcx,
        msg->pattern, msg->patternsrcx, msg->patternsrcy, msg->patternheight, msg->patterndepth, msg->patternlut, msg->invertpattern,
        GC_FG(msg->gc), GC_BG(msg->gc), GC_COLEXP(msg->gc), GC_DRMD(msg->gc)));

    if (!blit_putpattern(csd, data->pbm, msg))
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}


/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutImageLUT(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_PutImageLUT *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct amigabm_data   *data;  
    
    CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)

    data = OOP_INST_DATA(cl, o);
    CLEARCACHE;
    
    planeoffset = msg->y * data->bytesperrow + msg->x / 8;
    
    for(y = 0; y < msg->height; y++)
    {
        UBYTE *src = pixarray;
        
        plane = data->pbm->Planes;
        
        for(d = 0; d < data->depth; d++)
        {
            ULONG dmask = 1L << d;
            ULONG pmask = 0x80 >> (msg->x & 7);
            UBYTE *pl = *plane;
            
            if (pl == (UBYTE *)-1) continue;
            if (pl == NULL) continue;
            
            pl += planeoffset;

            for(x = 0; x < msg->width; x++)
            {
                if (src[x] & dmask)
                {
                    *pl |= pmask;
                }
                else
                {
                    *pl &= ~pmask;
                }
                
                if (pmask == 0x1)
                {
                    pmask = 0x80;
                    pl++;
                }
                else
                {
                    pmask >>= 1;
                }
                
            } /* for(x = 0; x < msg->width; x++) */
            
            plane++;
            
        } /* for(d = 0; d < data->depth; d++) */
        
        pixarray += msg->modulo;
        planeoffset += data->bytesperrow;
        
    } /* for(y = 0; y < msg->height; y++) */
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__GetImageLUT(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_BitMap_GetImageLUT *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct amigabm_data    *data;  
    UBYTE   	    	    prefill;
    
    data = OOP_INST_DATA(cl, o);

    D(bug("[AmigaVideo:Bitmap] %s: Get %dx%d to %dx%d from %d planes to buffer at %p\n",
                        __func__, msg->x, msg->y, msg->x + msg->width - 1, msg->y + msg->height - 1, data->depth, msg->pixels));

    planeoffset = msg->y * data->bytesperrow + msg->x / 8;

    prefill = 0;
    for (d = 0; d < data->depth; d++)
    {
        if (data->pbm->Planes[d] == (UBYTE *)-1)
        {
            prefill |= (1L << d);
        }
    }

    for (y = 0; y < msg->height; y++)
    {
        UBYTE *dest = pixarray;

        plane = data->pbm->Planes;
        for(x = 0; x < msg->width; x++)
        {
            dest[x] = prefill;
        }
        
        for (d = 0; d < data->depth; d++)
        {
            ULONG dmask = 1L << d;
            ULONG pmask = 0x80 >> (msg->x & 7);
            UBYTE *pl = *plane;

            if (pl == (UBYTE *)-1) continue;
            if (pl == NULL) continue;

            pl += planeoffset;

            for (x = 0; x < msg->width; x++)
            {
                if (*pl & pmask)
                {
                    dest[x] |= dmask;
                }
                else
                {
                    dest[x] &= ~dmask;
                }
                
                if (pmask == 0x1)
                {
                    pmask = 0x80;
                    pl++;
                }
                else
                {
                    pmask >>= 1;
                }
                
            } /* for(x = 0; x < msg->width; x++) */
            
            plane++;
            
        } /* for(d = 0; d < data->depth; d++) */
        
        pixarray    += msg->modulo;
        planeoffset += data->bytesperrow;
        
    } /* for(y = 0; y < msg->height; y++) */

    D(bug("[AmigaVideo:Bitmap] %s: Got %d\n", __func__, *(UBYTE *)msg->pixels));
}


/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutImage(OOP_Class *cl, OOP_Object *o,
                                struct pHidd_BitMap_PutImage *msg)
{
    WORD    	    	    x, y, d;
    UBYTE   	    	    *pixarray = (UBYTE *)msg->pixels;
    UBYTE   	    	    **plane;
    ULONG   	    	    planeoffset;
    struct amigabm_data    *data = OOP_INST_DATA(cl, o);

    CLEARCACHE;

    if ((msg->pixFmt != vHidd_StdPixFmt_Native) &&
        (msg->pixFmt != vHidd_StdPixFmt_Native32))
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        return;
    }
    CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)
    
    planeoffset = msg->y * data->bytesperrow + msg->x / 8;
    
    for(y = 0; y < msg->height; y++)
    {
        switch(msg->pixFmt)
        {
            case vHidd_StdPixFmt_Native:
            {
                UBYTE *src = pixarray;
        
                plane = data->pbm->Planes;

                for(d = 0; d < data->depth; d++)
                {
                    ULONG dmask = 1L << d;
                    ULONG pmask = 0x80 >> (msg->x & 7);
                    UBYTE *pl = *plane;

                    if (pl == (UBYTE *)-1) continue;
                    if (pl == NULL) continue;

                    pl += planeoffset;

                    for(x = 0; x < msg->width; x++)
                    {
                        if (src[x] & dmask)
                        {
                            *pl |= pmask;
                        }
                        else
                        {
                            *pl &= ~pmask;
                        }

                        if (pmask == 0x1)
                        {
                            pmask = 0x80;
                            pl++;
                        }
                        else
                        {
                            pmask >>= 1;
                        }

                    } /* for(x = 0; x < msg->width; x++) */

                    plane++;

                } /* for(d = 0; d < data->depth; d++) */

                pixarray += msg->modulo;
                planeoffset += data->bytesperrow;
            }
            break;

            case vHidd_StdPixFmt_Native32:
            {
                HIDDT_Pixel *src = (HIDDT_Pixel *)pixarray;
        
                plane = data->pbm->Planes;

                for(d = 0; d < data->depth; d++)
                {
                    ULONG dmask = 1L << d;
                    ULONG pmask = 0x80 >> (msg->x & 7);
                    UBYTE *pl = *plane;

                    if (pl == (UBYTE *)-1) continue;
                    if (pl == NULL) continue;

                    pl += planeoffset;

                    for(x = 0; x < msg->width; x++)
                    {
                        if (src[x] & dmask)
                        {
                            *pl |= pmask;
                        }
                        else
                        {
                            *pl &= ~pmask;
                        }

                        if (pmask == 0x1)
                        {
                            pmask = 0x80;
                            pl++;
                        }
                        else
                        {
                            pmask >>= 1;
                        }

                    } /* for(x = 0; x < msg->width; x++) */

                    plane++;

                } /* for(d = 0; d < data->depth; d++) */

                pixarray += msg->modulo;
                planeoffset += data->bytesperrow;
            }
            
            break;
            
        } /* switch(msg->pixFmt) */    
        
    } /* for(y = 0; y < msg->height; y++) */
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__FillRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_DrawRect *msg)
{
    HIDDT_Pixel fg = GC_FG(msg->gc);
    HIDDT_DrawMode mode = GC_DRMD(msg->gc);
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    CLEARCACHE;
    if (!blit_fillrect(csd, data->pbm, msg->minX, msg->minY, msg->maxX, msg->maxY, fg, mode)) {
        CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutTemplate(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_PutTemplate *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    CLEARCACHE;
    if (!blit_puttemplate(csd, data->pbm, msg)) {
        CMDDEBUGUNIMP(bug("PutTemplate: %x x=%d y=%d w=%d h=%d srcx=%d modulo=%d invert=%d\n",
            msg->masktemplate, msg->x, msg->y, msg->width, msg->height, msg->srcx, msg->inverttemplate);)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}


/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__UpdateRect(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
}

/****************************************************************************************/

BOOL AmigaVideoBM__Hidd_PlanarBM__SetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_SetBitMap *msg)
{
    CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

BOOL AmigaVideoBM__Hidd_PlanarBM__GetBitMap(OOP_Class *cl, OOP_Object *o,
                                   struct pHidd_PlanarBM_GetBitMap *msg)
{
    CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)
    return OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
