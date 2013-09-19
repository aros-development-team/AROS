/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>

#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <graphics/rpattr.h>

#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>


#ifdef __MORPHOS__
#ifndef NewRectRegion
#define NewRectRegion(_MinX,_MinY,_MaxX,_MaxY) \
({ struct Region *_region; \
        if ((_region = NewRegion())) \
        { struct Rectangle _rect; \
                _rect.MinX = _MinX; \
                _rect.MinY = _MinY; \
                _rect.MaxX = _MaxX; \
                _rect.MaxY = _MaxY; \
                if (!OrRectRegion(_region, &_rect)) { DisposeRegion(_region); _region = NULL; } \
        } \
        _region; \
})
#endif
#endif


#define CLASS_ASLBASE ((struct AslBase_intern *)cl->cl_UserData)
#define HOOK_ASLBASE  ((struct AslBase_intern *)hook->h_Data)

#define AslBase CLASS_ASLBASE

#define PREVIEW_TEXT "123 AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz!@#$%^&*()"
#define PREVIEW_BORDER_X 4
#define PREVIEW_BORDER_Y 4

/********************** ASL FONTPREVIEW CLASS **************************************************/

IPTR AslFontPreview__OM_NEW(Class * cl, Object * o, struct opSet * msg)
{
    struct AslFontPreviewData *data;
    struct TagItem fitags[] =
    {
        {IA_FrameType, FRAME_BUTTON},
        {IA_EdgesOnly, TRUE        },
        {IA_Recessed , TRUE        },
        {TAG_DONE, 0UL}
    };

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);

    if (o)
    {
        data = INST_DATA(cl, o);

        data->frame = NewObjectA(NULL, FRAMEICLASS, fitags);
        data->previewtext = (STRPTR)GetTagData(ASLFP_SampleText, 0, msg->ops_AttrList);
        if (!data->previewtext) data->previewtext = PREVIEW_TEXT;

        data->apen = 1;
        data->bpen = 0;
        data->drawstyle = FS_NORMAL;

    } /* if (o) */

    return (IPTR)o;
}

/***********************************************************************************/

IPTR AslFontPreview__OM_DISPOSE(Class * cl, Object * o, Msg msg)
{
    struct AslFontPreviewData *data;
    IPTR retval;

    data = INST_DATA(cl, o);
    if (data->frame) DisposeObject(data->frame);

    retval = DoSuperMethodA(cl, o, msg);

    return retval;
}

/***********************************************************************************/

IPTR AslFontPreview__OM_SET(Class * cl, Object * o, struct opSet * msg)
{
    struct AslFontPreviewData   *data;
    struct TagItem              *tag;
    struct TagItem              *tstate = msg->ops_AttrList;
    struct RastPort             *rp;
    BOOL                         redraw = FALSE;
    IPTR                         retval;

    data = INST_DATA(cl, o);

    retval = DoSuperMethodA(cl, o, (Msg)msg);

    while((tag = NextTagItem(&tstate)))
    {
        switch(tag->ti_Tag)
        {
            case ASLFP_APen:
                data->apen = tag->ti_Data;
                redraw = TRUE;
                break;

            case ASLFP_BPen:
                data->bpen = tag->ti_Data;
                redraw = TRUE;
                break;

            case ASLFP_Style:
                data->drawstyle = tag->ti_Data;
                redraw = TRUE;
                break;

            case ASLFP_Font:
                data->font = (struct TextFont *)tag->ti_Data;
                redraw = TRUE;
                break;
        }
    }

    if (redraw)
    {
        if ((rp = ObtainGIRPort(msg->ops_GInfo)))
        {
            struct gpRender gpr;

            gpr.MethodID = GM_RENDER;
            gpr.gpr_GInfo = msg->ops_GInfo;
            gpr.gpr_RPort = rp;
            gpr.gpr_Redraw = GREDRAW_UPDATE;

            DoMethodA(o, (Msg)&gpr);

            ReleaseGIRPort(rp);
        }
    }

    return retval;

}

/***********************************************************************************/

IPTR AslFontPreview__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    return 0;
}


/***********************************************************************************/

IPTR AslFontPreview__GM_RENDER(Class *cl, struct Gadget *g, struct gpRender *msg)
{
    struct AslFontPreviewData *data;
    struct RastPort           *rp;
    struct TagItem im_tags[] =
    {
        {IA_Width       , 0     },
        {IA_Height      , 0     },
        {TAG_DONE               }
    };
    struct Region *clip, *oldclip;

    WORD x, y, w, h, x2, y2;

    getgadgetcoords(g, msg->gpr_GInfo, &x, &y, &w, &h);

    data = INST_DATA(cl, g);
    rp = msg->gpr_RPort;

    if (msg->gpr_Redraw == GREDRAW_REDRAW)
    {
        if (data->frame)
        {
            im_tags[0].ti_Data = w;
            im_tags[1].ti_Data = h;

            SetAttrsA(data->frame, im_tags);

            DrawImageState(rp,
                           (struct Image *)data->frame,
                           x,
                           y,
                           IDS_NORMAL,
                           msg->gpr_GInfo->gi_DrInfo);

        #if AVOID_FLICKER
            {
                struct IBox ibox, fbox;

                fbox.Left = x;
                fbox.Top = y;
                fbox.Width = w;
                fbox.Height = h;

                ibox.Left = x + PREVIEW_BORDER_X;
                ibox.Top = y + PREVIEW_BORDER_Y;
                ibox.Width = w - PREVIEW_BORDER_X * 2;
                ibox.Height = h - PREVIEW_BORDER_Y * 2;

                PaintInnerFrame(msg->gpr_RPort,
                                msg->gpr_GInfo->gi_DrInfo,
                                data->frame,
                                &fbox,
                                &ibox,
                                msg->gpr_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN],
                                AslBase);
            }
        #endif
        }
    }

    x2 = x + w - 1 - PREVIEW_BORDER_X;
    y2 = y + h - 1 - PREVIEW_BORDER_Y;
    x += PREVIEW_BORDER_X;
    y += PREVIEW_BORDER_Y;

    if ((clip = NewRectRegion(x, y, x2, y2)))
    {
        struct TextFont *font;
        struct Layer *lay = msg->gpr_GInfo->gi_Layer;
        STRPTR text;
        IPTR   did_remap_colorfonts;
        WORD   textlen, pixellen;
        BOOL   updating;

        updating = (lay->Flags & LAYERUPDATING) != 0;
        if (updating) EndUpdate(lay, FALSE);

        font = data->font ? data->font : msg->gpr_GInfo->gi_DrInfo->dri_Font;
        text = data->font ? data->previewtext : (STRPTR)"???";

        oldclip = InstallClipRegion(lay, clip);

        if (updating) BeginUpdate(lay);

        SetAPen(rp, data->bpen);
        RectFill(rp, x, y, x2, y2);

        SetABPenDrMd(rp, data->apen, data->bpen, JAM2);
        SetFont(rp, font);
        SetSoftStyle(rp, data->drawstyle, AskSoftStyle(rp));

        textlen = strlen(text);
        pixellen = TextLength(rp, text, textlen);
        if (pixellen <= (x2 - x + 1))
        {
            x += (x2 - x + 1 - pixellen) / 2;
        }

        GetRPAttrs(rp, RPTAG_RemapColorFonts, &did_remap_colorfonts, TAG_DONE);
        SetRPAttrs(rp, RPTAG_RemapColorFonts, TRUE, TAG_DONE);

        Move(rp, x, (y + y2 + 1 - font->tf_YSize) / 2 + font->tf_Baseline);
        Text(rp, text, textlen);

        SetRPAttrs(rp, RPTAG_RemapColorFonts, did_remap_colorfonts, TAG_DONE);

        if (updating) EndUpdate(lay, FALSE);
        InstallClipRegion(lay, oldclip);
        if (updating) BeginUpdate(lay);

        DisposeRegion(clip);
    }
    return 0;
}

/***********************************************************************************/
