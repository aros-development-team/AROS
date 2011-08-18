/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for Vesa.
    Lang: English.
*/

#define __OOP_NOATTRBASES__

#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

#include "vesagfxclass.h"
#include "bitmap.h"
#include "hardware.h"
#include "compositing.h"

#include LC_LIBDEFS_FILE

static void RefreshBox(OOP_Object *gfx, OOP_Object * bm, LONG x1, LONG y1,
    LONG x2, LONG y2);

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

static AROS_UFH3(void, ResetHandler,
		 AROS_UFHA(struct HWData *, hwdata, A1),
		 AROS_UFHA(APTR, unused, A5),
		 AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    ClearBuffer(hwdata);

    AROS_USERFUNC_EXIT
}

OOP_Object *PCVesa__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
	{aHidd_PixFmt_RedShift,     0}, /*  0 */
	{aHidd_PixFmt_GreenShift,   0}, /*  1 */
	{aHidd_PixFmt_BlueShift,    0}, /*  2 */
	{aHidd_PixFmt_AlphaShift,   0}, /*  3 */
	{aHidd_PixFmt_RedMask,      0}, /*  4 */
	{aHidd_PixFmt_GreenMask,    0}, /*  5 */
	{aHidd_PixFmt_BlueMask,     0}, /*  6 */
	{aHidd_PixFmt_AlphaMask,    0}, /*  7 */
	{aHidd_PixFmt_ColorModel,   0}, /*  8 */
	{aHidd_PixFmt_Depth,        0}, /*  9 */
	{aHidd_PixFmt_BytesPerPixel,0}, /* 10 */
	{aHidd_PixFmt_BitsPerPixel, 0}, /* 11 */
	{aHidd_PixFmt_StdPixFmt,    vHidd_StdPixFmt_Native}, /* 12 */
	{aHidd_PixFmt_CLUTShift,    0}, /* 13 */
	{aHidd_PixFmt_CLUTMask,     0}, /* 14 */
	{aHidd_PixFmt_BitMapType,   vHidd_BitMapType_Chunky}, /* 15 */
	{TAG_DONE, 0UL }
    };
    struct TagItem sync_mode[] =
    {
	{aHidd_Sync_HDisp,      0},
	{aHidd_Sync_VDisp,      0},
	{aHidd_Sync_HMax,	16384},
	{aHidd_Sync_VMax,	16384},
	{aHidd_Sync_Description, (IPTR)"VESA:%hx%v"},
	{TAG_DONE, 0UL}
    };
    struct TagItem modetags[] =
    {
	{aHidd_Gfx_PixFmtTags, (IPTR)pftags},
	{aHidd_Gfx_SyncTags,   (IPTR)sync_mode},
	{TAG_DONE, 0UL}
    };
    struct TagItem yourtags[] =
    {
	{aHidd_Gfx_ModeTags, (IPTR)modetags},
	{TAG_MORE, 0UL}
    };
    struct pRoot_New yourmsg;

    /* Protect against some stupid programmer wishing to
       create one more VESA driver */
    if (XSD(cl)->vesagfxhidd)
	return NULL;

    pftags[0].ti_Data = XSD(cl)->data.redshift;
    pftags[1].ti_Data = XSD(cl)->data.greenshift;
    pftags[2].ti_Data = XSD(cl)->data.blueshift;
    pftags[4].ti_Data = XSD(cl)->data.redmask;
    pftags[5].ti_Data = XSD(cl)->data.greenmask;
    pftags[6].ti_Data = XSD(cl)->data.bluemask;
    pftags[8].ti_Data = (XSD(cl)->data.depth > 8) ? vHidd_ColorModel_TrueColor : vHidd_ColorModel_Palette;
    pftags[9].ti_Data = (XSD(cl)->data.depth > 24) ? 24 : XSD(cl)->data.depth;
    pftags[10].ti_Data = XSD(cl)->data.bytesperpixel;
    pftags[11].ti_Data = (XSD(cl)->data.bitsperpixel > 24) ? 24 : XSD(cl)->data.bitsperpixel;
    pftags[14].ti_Data = (1 << XSD(cl)->data.depth) - 1;

    sync_mode[0].ti_Data = XSD(cl)->data.width;
    sync_mode[1].ti_Data = XSD(cl)->data.height;

    yourtags[1].ti_Data = (IPTR)msg->attrList;
    yourmsg.mID = msg->mID;
    yourmsg.attrList = yourtags;
    msg = &yourmsg;
    EnterFunc(bug("VesaGfx::New()\n"));
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
	struct VesaGfx_data *data = OOP_INST_DATA(cl, o);
        struct TagItem comptags[] =
        {
            {aHidd_Compositing_GfxHidd, (IPTR)o},
            {aHidd_Compositing_RefreshCallBack, (IPTR)RefreshBox},
            {TAG_DONE, 0}
        };

	D(bug("Got object from super\n"));
	XSD(cl)->vesagfxhidd = o;

	data->ResetInterrupt.is_Code = ResetHandler;
	data->ResetInterrupt.is_Data = &XSD(cl)->data;
	AddResetCallback(&data->ResetInterrupt);

        /* Create compositing and GC objects */
        XSD(cl)->compositing =
            OOP_NewObject(XSD(cl)->compositingclass, NULL, comptags);
        XSD(cl)->gc = HIDD_Gfx_NewGC(o, NULL);
    }
    ReturnPtr("VesaGfx::New", OOP_Object *, o);
}

VOID PCVesa__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct VesaGfx_data *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->ResetInterrupt);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    XSD(cl)->vesagfxhidd = NULL;
}

VOID PCVesa__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Gfx_NoFrameBuffer:
                *msg->storage = TRUE;
		return;
            case aoHidd_Gfx_SupportsHWCursor:
                *msg->storage = TRUE;
		return;
            case aoHidd_Gfx_HWSpriteTypes:
                *msg->storage = vHidd_SpriteType_DirectColor;
		return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *PCVesa__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    HIDDT_ModeID modeid;
    struct TagItem tags[3];
    struct pHidd_Gfx_NewBitMap yourmsg;

    EnterFunc(bug("VesaGfx::NewBitMap()\n"));
    modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    if (modeid != vHidd_ModeID_Invalid)
    {
	tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)XSD(cl)->bmclass;
	tags[1].ti_Tag = aHidd_VesaGfxBitMap_CompositingHidd;
	tags[1].ti_Data = (IPTR)XSD(cl)->compositing;
	tags[2].ti_Tag = TAG_MORE;
	tags[2].ti_Data = (IPTR)msg->attrList;
	yourmsg.mID = msg->mID;
	yourmsg.attrList = tags;
	msg = &yourmsg;
    }
    ReturnPtr("VesaGfx::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

ULONG PCVesa__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gfx_ShowViewPorts *msg)
{
    struct pHidd_Compositing_BitMapStackChanged bscmsg =
    {
        mID : XSD(cl)->mid_BitMapStackChanged,
        data : msg->Data
    };
    D(bug("[VesaGfx] ShowViewPorts enter TopLevelBM %x\n", msg->Data->Bitmap));
    OOP_DoMethod(XSD(cl)->compositing, (OOP_Msg)&bscmsg);
    return TRUE; /* Indicate driver supports this method */
}

static struct HIDD_ModeProperties modeprops =
{
    DIPF_IS_SPRITES,
    1,
    COMPF_ABOVE
};

ULONG PCVesa__Hidd_Gfx__ModeProperties(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gfx_ModeProperties *msg)
{
    ULONG len = msg->propsLen;
    if (len > sizeof(modeprops))
        len = sizeof(modeprops);
    CopyMem(&modeprops, msg->props, len);

    return len;
}

static void DrawCursor(struct VesaGfx_staticdata *data)
{
    struct pHidd_Compositing_DisplayRectChanged cmsg =
    {
        mID : OOP_GetMethodID(IID_Hidd_Compositing,
            moHidd_Compositing_DisplayRectChanged),
        x : data->cursor_x,
        y : data->cursor_y,
        width : data->cursor_width,
        height : data->cursor_height
    };

    OOP_DoMethod(data->compositing, (OOP_Msg)&cmsg);
}

static void RedrawCursor(OOP_Class *cl, OOP_Object *o, OOP_Object *bm,
    WORD x1, WORD y1, WORD x2, WORD y2)
{
    struct VesaGfx_staticdata *data = XSD(cl);
    OOP_Object *curbm = data->cursor_scratch_bm;
    OOP_Class *bmcl = OOP_OCLASS(bm);
    struct BitmapData *bmdata = OOP_INST_DATA(bmcl, bm),
        *curbmdata = OOP_INST_DATA(bmcl, curbm);
    UWORD pixel_offset;

    pixel_offset = (y1 - data->cursor_y) * data->cursor_width
        + x1 - data->cursor_x;
    HIDD_Gfx_CopyBox(o, bm, x1 - bmdata->xoffset, y1 - bmdata->yoffset,
        curbm, 0, 0, x2 - x1 + 1, y2 - y1 + 1, data->gc);

    HIDD_BM_PutAlphaImage(curbm, data->gc,
        (UBYTE *)(data->cursor_pixels + pixel_offset),
        data->cursor_width * sizeof(ULONG), 0, 0,
        x2 - x1 + 1, y2 - y1 + 1);

    curbmdata->xoffset = x1;
    curbmdata->yoffset = y1;
}

static void BlankCursor(struct VesaGfx_staticdata *data)
{
    struct pHidd_Compositing_DisplayRectChanged cmsg =
    {
        mID : OOP_GetMethodID(IID_Hidd_Compositing,
            moHidd_Compositing_DisplayRectChanged),
        x : data->cursor_x,
        y : data->cursor_y,
        width : data->cursor_width,
        height : data->cursor_height
    };

    OOP_DoMethod(data->compositing, (OOP_Msg)&cmsg);
}

BOOL PCVesa__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gfx_SetCursorShape *msg)
{
    struct VesaGfx_staticdata *data = XSD(cl);
    BOOL success = TRUE;
    IPTR width, height;
    ULONG *buffer;
    OOP_Object *scratch_bm;

    /* Fill in a new cursor buffer, free old one and draw new cursor */
    OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
    OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
    buffer = AllocVec(width * height * sizeof(ULONG), MEMF_CLEAR);
    if (buffer == NULL)
        success = FALSE;

    if (success)
    {
        struct TagItem bmtags[] =
        {
            {aHidd_BitMap_Width, width},
            {aHidd_BitMap_Height, height},
            {aHidd_BitMap_Displayable, TRUE},
            {aHidd_BitMap_ModeID, 0},
            {TAG_DONE, 0}
        };

        /* Create scratch bitmap for drawing cursor */
        scratch_bm = HIDD_Gfx_NewBitMap(o, bmtags);
        if (scratch_bm == NULL)
            success = FALSE;
    }

    if (success)
    {
        /* Clear and deallocate old cursor */
        if (data->cursor_visible)
            BlankCursor(data);
        FreeVec(data->cursor_pixels);
        HIDD_Gfx_DisposeBitMap(o, data->cursor_scratch_bm);

        data->cursor_pixels = buffer;
        data->cursor_scratch_bm = scratch_bm;

        data->cursor_width = width;
        data->cursor_height = height;
        HIDD_BM_GetImage(msg->shape, (UBYTE *)data->cursor_pixels,
            data->cursor_width * 4, 0, 0, data->cursor_width,
            data->cursor_height, vHidd_StdPixFmt_ARGB32);

        if (data->cursor_visible)
            DrawCursor(data);
    }

    if (!success)
        FreeVec(buffer);

    return success;
}

BOOL PCVesa__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gfx_SetCursorPos *msg)
{
    struct VesaGfx_staticdata *data = XSD(cl);
    WORD old_x = data->cursor_x, old_y = data->cursor_y;

    data->cursor_x = msg->x;
    data->cursor_y = msg->y;

    if (data->cursor_visible)
    {
        struct pHidd_Compositing_DisplayRectChanged cmsg =
        {
            mID : OOP_GetMethodID(IID_Hidd_Compositing,
                moHidd_Compositing_DisplayRectChanged),
            x : old_x,
            y : old_y,
            width : data->cursor_width,
            height : data->cursor_height
        };

        OOP_DoMethod(data->compositing, (OOP_Msg)&cmsg);

        cmsg.x = data->cursor_x;
        cmsg.y = data->cursor_y;

        OOP_DoMethod(data->compositing, (OOP_Msg)&cmsg);
    }

    return TRUE;
}

BOOL PCVesa__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o,
    struct pHidd_Gfx_SetCursorVisible *msg)
{
    struct VesaGfx_staticdata *data = XSD(cl);

    data->cursor_visible = msg->visible != 0;

    if (data->cursor_visible)
        DrawCursor(data);
    else
        BlankCursor(data);

    return TRUE;
}

static void RefreshBox(OOP_Object *gfx, OOP_Object * bm, LONG x1, LONG y1,
    LONG x2, LONG y2)
{
    OOP_Class *cl = OOP_OCLASS(gfx);
    struct VesaGfx_staticdata *data = XSD(cl);
    WORD min_x, min_y, max_x, max_y, bm_x, bm_y;
    BOOL draw_cursor = FALSE;

    LOCK_FRAMEBUFFER(XSD(cl));

    if (bm != NULL)
    {
        OOP_Class *bmcl = OOP_OCLASS(bm);
        struct BitmapData *bmdata = OOP_INST_DATA(bmcl, bm),
            *curbmdata = OOP_INST_DATA(bmcl, data->cursor_scratch_bm);

        bm_x = bmdata->xoffset;
        bm_y = bmdata->yoffset;

        if (data->cursor_visible)
        {
            /* Get intersection of cursor box and refreshed box */
            min_x = MAX(bmdata->xoffset + x1, data->cursor_x);
            min_y = MAX(bmdata->yoffset + y1, data->cursor_y);
            max_x = MIN(bmdata->xoffset + x2,
                data->cursor_x + data->cursor_width - 1);
            max_y = MIN(bmdata->yoffset + y2,
                data->cursor_y + data->cursor_height - 1);

            /* Redraw part of cursor that was overwritten */
            if (min_x <= max_x && min_y <= max_y)
            {
                RedrawCursor(cl, gfx, bm, min_x, min_y, max_x, max_y);
                draw_cursor = TRUE;
            }
        }

        if (draw_cursor)
        {

            /* Draw strip above cursor area */
            if (bm_y + y1 < min_y)
                vesaDoRefreshArea(&data->data, bmdata, x1, y1,
                    x2, min_y - 1 - bm_y);

            /* Draw strip to left of cursor area */
            if (bm_x + x1 < min_x)
                vesaDoRefreshArea(&data->data, bmdata, x1, min_y - bm_y,
                    min_x - 1 - bm_x, max_y - bm_y);

            /* Draw cursor area */
            vesaDoRefreshArea(&data->data, curbmdata, 0, 0,
                max_x - min_x, max_y - min_y);

            /* Draw strip to right of cursor area */
            if (bm_x + x2 > max_x)
                vesaDoRefreshArea(&data->data, bmdata,
                    max_x + 1 - bm_x, min_y - bm_y, x2, max_y - bm_y);

            /* Draw strip below cursor area */
            if (bm_y + y2 > max_y)
                vesaDoRefreshArea(&data->data, bmdata,
                    x1, max_y + 1 - bm_y, x2, y2);
        }
        else
            vesaDoRefreshArea(&data->data, bmdata, x1, y1, x2, y2);
    }
    else
        ClearRect(&data->data, x1, y1, x2 - x1 + 1, y2 - y1 + 1);

    UNLOCK_FRAMEBUFFER(XSD(cl));
}
