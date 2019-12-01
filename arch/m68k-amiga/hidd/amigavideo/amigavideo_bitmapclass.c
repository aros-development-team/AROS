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
#include <graphics/modeid.h>
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
    struct Library *UtilityBase = csd->cs_UtilityBase;
    struct Library *OOPBase = csd->cs_OOPBase;
    IPTR width, height, depth, disp, modeid, coppersize;
    BOOL ok = TRUE;      
    struct amigabm_data *data;
    struct BitMap *pbm = NULL;
    struct pRoot_New mymsg = *msg;
    struct TagItem tags[] = {
        { aHidd_BitMap_Align, csd->aga ? 64 : 16 },
        { TAG_MORE, (IPTR) msg->attrList },
        { TAG_END, 0 }
    };

    D(bug("[AmigaVideo:Bitmap] %s()\n", __func__));

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
    OOP_GetAttr(o, aHidd_BitMap_ModeID , &modeid);

    D(bug("[AmigaVideo:Bitmap] %s: %dx%dx%d\n", __func__, width, height, depth));

    /* We cache some info */
    data->width = width;
    data->bytesperrow = ((width + data->align - 1) & ~(data->align - 1)) / 8;
    data->height = height;
    data->depth = depth;
    data->pixelcacheoffset = -1;
    data->pbm = pbm;

    if ((data->disp = disp))
    {
        D(bug("[AmigaVideo:Bitmap] %s: DISPLAYABLE bitmap\n", __func__));

        data->compositor = (OOP_Object *)GetTagData(aHidd_BitMap_AmigaVideo_Compositor, 0, msg->attrList);
        D(bug("[AmigaVideo:Bitmap] %s: compositor @ 0x%p\n", __func__, data->compositor));

        if (data->compositor)
        {
            data->palette = AllocVec(csd->max_colors * 3, MEMF_CLEAR);
            D(bug("[AmigaVideo:Bitmap] %s: palette data @ 0x%p\n", __func__, data->palette);)

            data->modeid = modeid;

            data->res = 0;
            if ((data->modeid & SUPER_KEY) == SUPER_KEY)
                data->res = 2;
            else if ((data->modeid & SUPER_KEY) == HIRES_KEY)
                data->res = 1;
            data->interlace = (data->modeid & LORESLACE_KEY) ? 1 : 0;

            coppersize = get_copper_list_length(csd, data->depth);

            data->copper2.copper2 = AllocVec(coppersize, MEMF_CLEAR | MEMF_CHIP);
            D(bug("[AmigaVideo:Bitmap] %s: allocated %d bytes for copperlist data @ 0x%p\n", __func__, coppersize, data->copper2.copper2);)

            if (data->interlace)
            {
                data->copper2i.copper2 = AllocVec(coppersize, MEMF_CLEAR | MEMF_CHIP);
                D(bug("[AmigaVideo:Bitmap] %s: interlaced copperlist data @ 0x%p\n", __func__, data->copper2i.copper2);)
            }
            setmode(csd, data);
        }
        else
            ok = FALSE;
    }
    if (!ok) {
        OOP_MethodID dispose_mid;

        dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
        OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);

        o = NULL;
    }

    D(bug("[AmigaVideo:Bitmap] %s: ret=%x bm=%x\n", __func__, o, data));

    return o;
}

VOID AmigaVideoBM__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct amigabm_data    *data;

    data = OOP_INST_DATA(cl, o);

    D(
      bug("[AmigaVideo:Bitmap] %s(0x%p)\n", __func__, o);
      bug("[AmigaVideo:Bitmap] %s: data @ 0x%p\n", __func__, data);
     )

    if (data->vis)
    {
        D(bug("[AmigaVideo:Bitmap] %s: WARNING! destroying visible bitmap!\n", __func__);)
    }

    if (data->disp)
    {
        D(bug("[AmigaVideo:Bitmap] %s: destroying displayable bitmap\n", __func__);)
        FreeVec(data->palette);
        FreeVec(data->copper2.copper2);
        FreeVec(data->copper2i.copper2);
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
    LONG newxoffset = data->leftedge;
    LONG newyoffset = data->topedge;

    DB2(bug("[AmigaVideo:Bitmap] %s()\n", __func__));
    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        DB2(bug("[AmigaVideo:Bitmap] %s: %d/%d\n", __func__, tag->ti_Tag, tag->ti_Data));
        if(IS_BITMAP_ATTR(tag->ti_Tag, idx))
        {
            DB2(bug("[AmigaVideo:Bitmap] %s: ->%d\n", __func__, idx));
            switch(idx)
            {
            case aoHidd_BitMap_Focus:
                    {
                        volatile struct Custom *custom = (struct Custom*)0xdff000;
                        struct GfxBase *GfxBase = (struct GfxBase *)csd->cs_GfxBase;
                        D(bug("[AmigaVideo:Bitmap] %s: aoHidd_BitMap_Focus\n", __func__);)
                    }
                    break;
            case aoHidd_BitMap_Visible:
                    data->vis = tag->ti_Data;
                    if (data->vis) {
                        setrtg(csd, FALSE);
                        setbitmap(csd, data);
                    } else {
                        resetmode(csd);
                        setrtg(csd, TRUE);
                    }
                    break;
            case aoHidd_BitMap_LeftEdge:
                    newxoffset = tag->ti_Data;
                    break;
            case aoHidd_BitMap_TopEdge:
                    newyoffset = tag->ti_Data;
                    break;
            }
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    if (newyoffset < 0)
        newyoffset = 0;
    if (newyoffset >= data->height)
        newyoffset = data->height - 1;

    if ((newxoffset != data->leftedge) || (newyoffset != data->topedge))
    {
        struct pHidd_Compositor_BitMapPositionChanged bpcmsg =
        {
            mID : CSD(cl)->mid_BitMapPositionChanged,
            bm : o
        };
        data->leftedge = newxoffset;
        data->topedge = newyoffset;
        OOP_DoMethod(data->compositor, (OOP_Msg)&bpcmsg);
    }

    DB2(bug("[AmigaVideo:Bitmap] %s: Exit\n", __func__));
}

VOID AmigaVideoBM__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct Library *OOPBase = csd->cs_OOPBase;
    struct amigabm_data *data = OOP_INST_DATA(cl, o);
    ULONG idx;
    BOOL handled = FALSE;

    DB2(bug("[AmigaVideo:Bitmap] %s: %d\n", __func__, msg->attrID));
    if (IS_AmigaVideoBM_ATTR(msg->attrID, idx)) {
        DB2(bug("[AmigaVideo:Bitmap] %s: AVBM=%d\n", __func__, idx));
        switch (idx)
        {
        case aoHidd_BitMap_AmigaVideo_Drawable:
            *msg->storage = TRUE;
            handled = TRUE;
            break;
        }
    } else if (IS_BITMAP_ATTR(msg->attrID, idx)) {
        DB2(bug("[AmigaVideo:Bitmap] %s: BM=%d\n", __func__, idx));
        switch (idx)
        {
        case aoHidd_BitMap_LeftEdge:
            *msg->storage = data->leftedge;
            handled = TRUE;
            break;

        case aoHidd_BitMap_TopEdge:
            *msg->storage = data->topedge;
            handled = TRUE;
            break;

        case aoHidd_BitMap_Visible:
            *msg->storage = data->vis;
            handled = TRUE;
            break;

        case aoHidd_BitMap_Align:
            *msg->storage = csd->aga ? 64 : 16;
            handled = TRUE;
            break;

        case aoHidd_BitMap_BytesPerRow:
            if (data->bytesperrow == 0) {
                IPTR width = 0;
                IPTR align = csd->aga ? 64 : 16;
                OOP_GetAttr(o, aHidd_BitMap_Width, &width);
                *msg->storage = ((width + align - 1) & ~(align - 1)) / 8;
            } else {
                *msg->storage = data->bytesperrow;
            }
            handled = TRUE;
            break;
        }
    }
    if (!handled)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    DB2(bug("[AmigaVideo:Bitmap] %s: Exit\n", __func__));
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

    D(bug("[AmigaVideo:Bitmap] %s()\n", __func__));

    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg))
        return FALSE;
    return setcolors(csd, data, msg);
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

    CMDDEBUGPIXEL(bug("[AmigaVideo:Bitmap] %s: %dx%d %x\n", __func__, msg->x, msg->y, msg->pixel);)
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
    CMDDEBUGPIXEL(bug("[AmigaVideo:Bitmap] %s: %dx%d %x\n", __func__, msg->x, msg->y, c);)
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
    BOOL doSuper = TRUE;

    DB2(bug("[AmigaVideo:Bitmap] %s()\n", __func__);)

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

        if (blit_fillrect(csd, data->pbm, x1, y1, x2, y2, fg, mode))
            doSuper = FALSE;
    }
    if (doSuper)
    {
        DB2(bug("[AmigaVideo:Bitmap] %s: asking super to handle ..\n", __func__);)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
}

/****************************************************************************************/

VOID AmigaVideoBM__Hidd_BitMap__PutPattern(OOP_Class *cl, OOP_Object *o,
                                 struct pHidd_BitMap_PutPattern *msg)
{
    struct amigavideo_staticdata *csd = CSD(cl);
    struct amigabm_data *data = OOP_INST_DATA(cl, o);

    CLEARCACHE;
    D(bug("[AmigaVideo:Bitmap] %s(%dx%d,%dx%d,mask=%x,mod=%d,masksrcx=%d)\n(%x,%dx%d,h=%d,d=%d,lut=%x,inv=%d)(fg=%d,bg=%d,colexp=%d,drmd=%d)\n", 
        __func__,
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
        CMDDEBUGUNIMP(bug("[AmigaVideo:Bitmap] %s: %x x=%d y=%d w=%d h=%d srcx=%d modulo=%d invert=%d\n", __func__,
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
