/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Bitmap class for FB Gfx hidd.
*/

#include <proto/oop.h>
#include <proto/utility.h>
#include <assert.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <aros/symbolsets.h>
#define DEBUG 0
#include <aros/debug.h>

#include <string.h>

#include "fbgfx_hidd.h"
#include "fbgfx_refresh.h"

#include LC_LIBDEFS_FILE

#define MNAME_ROOT(x) FBGfxBM__Root__ ## x
#define MNAME_BM(x) FBGfxBM__Hidd_BitMap__ ## x

/*********** BitMap::New() *************************************/
OOP_Object *MNAME_ROOT(New)(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    EnterFunc(bug("FBGfx.BitMap::New()\n"));
    
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    if (o)
    {
        OOP_MethodID       disp_mid;
        struct FBGfxBitMapData *data;
        HIDDT_ModeID       modeid;
        OOP_Object        *sync = NULL, *pf = NULL, *dmenum;
        uint32_t bytes;

        data = OOP_INST_DATA(cl, o);

        /* Get attr values */
        OOP_GetAttr(o, aHidd_BitMap_Display , (APTR)&data->displayhidd);
        OOP_GetAttr(o, aHidd_BitMap_PixFmt  , (APTR)&data->pixfmtobj);
        OOP_GetAttr(o, aHidd_BitMap_ModeID  , &modeid);
        OOP_GetAttr(o, aHidd_ChunkyBM_Buffer, (IPTR *)&data->VideoData);

        dmenum = (OOP_Object *)OOP_GET(data->displayhidd, aHidd_Display_DMEnumerator);
        if (!dmenum)
            goto dispose;
        HIDD_DMEnum_GetMode(dmenum, modeid, &sync, &pf);
        if (!sync || !pf)
            goto dispose;

        data->width        = OOP_GET(o, aHidd_BitMap_Width);
        data->height       = OOP_GET(o, aHidd_BitMap_Height);
        data->bytesperline = OOP_GET(o, aHidd_BitMap_BytesPerRow);
        data->bytesperpix  = OOP_GET(data->pixfmtobj, aHidd_PixFmt_BytesPerPixel);
        data->bpp          = OOP_GET(data->pixfmtobj, aHidd_PixFmt_BitsPerPixel);
        data->disp_width   = OOP_GET(sync, aHidd_Sync_HDisp);
        data->disp_height  = OOP_GET(sync, aHidd_Sync_VDisp);

        if (data->width <= 0 || data->height <= 0 || !data->VideoData ||
            !fbgfx_buffer_size(data->width, data->height, data->bytesperline,
                               data->bytesperpix, &bytes))
            goto dispose;

        D(bug("[FBGfx:BitMap] Bitmap %ld x %ld, %u bytes per pixel, %u bytes per line\n",
              data->width, data->height, data->bytesperpix, data->bytesperline));
        D(bug("[FBGfx:BitMap] Video data at 0x%p (%u bytes)\n", data->VideoData, data->bytesperline * data->height));

        if (OOP_GET(data->pixfmtobj, aHidd_PixFmt_ColorModel) != vHidd_ColorModel_Palette)
            ReturnPtr("FBGfx.BitMap::New()", OOP_Object *, o);

        /* Software palette: 256 R,G,B entries, black until set */
        data->DAC = AllocMem(768, MEMF_ANY | MEMF_CLEAR);
        D(bug("[FBGfx:BitMap] Palette data at 0x%p\n", data->DAC));
        if (data->DAC)
            ReturnPtr("FBGfx.BitMap::New()", OOP_Object *, o);

dispose:
        disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

        OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);
        o = NULL;
    } /* if created object */

    ReturnPtr("FBGfx.BitMap::New()", OOP_Object *, o);
}

/**********  Bitmap::Dispose()  ***********************************/
VOID MNAME_ROOT(Dispose)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct FBGfxBitMapData *data = OOP_INST_DATA(cl, o);

    D(bug("[FBGfx:BitMap] Dispose(0x%p)\n", o));

    if (data->DAC)
        FreeMem(data->DAC, 768);

    OOP_DoSuperMethod(cl, o, msg);

    ReturnVoid("FBGfx.BitMap::Dispose");
}

/*** BitMap::Get() *******************************************/

VOID MNAME_ROOT(Get)(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct FBGfxBitMapData *data = OOP_INST_DATA(cl, o);
    ULONG              idx;

    if (IS_BM_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_BitMap_Visible:
            *msg->storage = data->disp;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*** BitMap::Set() *******************************************/

VOID MNAME_ROOT(Set)(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct FBGfxBitMapData *data = OOP_INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;
    ULONG           idx;
    IPTR xoffset, yoffset;
    BOOL was_visible;
    BOOL moved;

    LOCK_FRAMEBUFFER(XSD(cl));
    xoffset = data->xoffset;
    yoffset = data->yoffset;
    was_visible = (data->disp != 0);

    tstate = msg->attrList;
    while((tag = NextTagItem(&tstate)))
    {
        if(IS_BM_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
            case aoHidd_BitMap_Visible:
                D(bug("[FBGfx:BitMap] Setting Visible to %d\n", tag->ti_Data));
                data->disp = tag->ti_Data ? 1 : 0;
                break;
            }
        }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    /* Bring local X/Y offsets back into sync with validated values */
    OOP_GetAttr(o, aHidd_BitMap_LeftEdge, &xoffset);
    OOP_GetAttr(o, aHidd_BitMap_TopEdge, &yoffset);

    moved = ((LONG)xoffset != data->xoffset) || ((LONG)yoffset != data->yoffset);
    if (moved)
    {
        D(bug("[FBGfx:BitMap] Scroll to (%d, %d)\n", (LONG)xoffset, (LONG)yoffset));
        data->xoffset = (LONG)xoffset;
        data->yoffset = (LONG)yoffset;
    }

    /*
     * A bitmap that has just become visible may already hold rendered
     * content, so it is copied out once with the final offsets, exactly as
     * after a scroll.
     */
    if (data->disp && (moved || !was_visible))
    {
        /* A single visible bitmap leaves uncovered bands black. */
        ClearBuffer(&XSD(cl)->data);
        if (!was_visible && data->DAC)
            DACLoad(XSD(cl), data->DAC, 0, 256);
        fbDoRefreshArea(&XSD(cl)->data, data, 0, 0, data->width, data->height);
    }
    UNLOCK_FRAMEBUFFER(XSD(cl));
}

/*** BitMap::SetColors() *************************************/

BOOL MNAME_BM(SetColors)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_SetColors *msg)
{
    struct FBGfxBitMapData *data = OOP_INST_DATA(cl, o);
    ULONG first = msg->firstColor;
    ULONG count = msg->numColors;
    ULONG limit;
    ULONG xc_i, col_i;

    D(bug("[FBGfx:BitMap] SetColors(%u, %u)\n", first, count));

    /* The software palette has 256 entries; truecolor is bounded by depth */
    if (data->DAC)
        limit = 256;
    else if ((data->bpp > 0) && (data->bpp < 32))
        limit = 1UL << data->bpp;
    else
        limit = 0xFFFFFFFF;

    /* Reject out of range requests before the superclass sees them */
    if ((first > limit) || (count > limit - first))
        return FALSE;
    if (count == 0)
        return TRUE;
    if (!msg->colors)
        return FALSE;

    LOCK_FRAMEBUFFER(XSD(cl));
    if (!OOP_DoSuperMethod(cl, o, (OOP_Msg)msg)) {
        D(bug("[FBGfx:BitMap] DoSuperMethod() failed\n"));
        UNLOCK_FRAMEBUFFER(XSD(cl));
        return FALSE;
    }

    if (data->DAC) {
        /* The palette is applied in software and needs full 8 bit components */
        for ( xc_i = first, col_i = 0;
              col_i < count;
              xc_i ++, col_i ++) {
            data->DAC[xc_i*3]   = msg->colors[col_i].red   >> 8;
            data->DAC[xc_i*3+1] = msg->colors[col_i].green >> 8;
            data->DAC[xc_i*3+2] = msg->colors[col_i].blue  >> 8;
        }

        /* The display shows converted pixels, so redraw them in the new colours */
        if (data->disp) {
            DACLoad(XSD(cl), data->DAC, first, count);
            fbDoRefreshArea(&XSD(cl)->data, data, 0, 0, data->width, data->height);
        }
    }
    UNLOCK_FRAMEBUFFER(XSD(cl));
    return TRUE;
}

VOID MNAME_BM(UpdateRect)(OOP_Class *cl, OOP_Object *o, struct pHidd_BitMap_UpdateRect *msg)
{
    struct FBGfxBitMapData *data = OOP_INST_DATA(cl, o);

    D(bug("[FBGfx:BitMap] UpdateRect(%d, %d, %d, %d), bitmap 0x%p\n", msg->x, msg->y, msg->width, msg->height, o));
    LOCK_FRAMEBUFFER(XSD(cl));
    if (data->disp) {
        /* WORD coordinates and extents promote to LONG without overflow. */
        fbDoRefreshArea(&XSD(cl)->data, data, msg->x, msg->y,
                        (LONG)msg->x + msg->width, (LONG)msg->y + msg->height);
    }
    UNLOCK_FRAMEBUFFER(XSD(cl));
}
