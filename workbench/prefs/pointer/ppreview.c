/*
    Copyright  2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <datatypes/pictureclass.h>

// #define DEBUG 1
#include <zune/customclasses.h>
#include <zune/prefseditor.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>

#include <aros/debug.h>

#include "locale.h"
#include "ppreview.h"
#include "prefs.h"

/*** Instance Data **********************************************************/
struct PPreview_DATA
{
    Object                     *pprv_prevEditor;
    UWORD                       pprv_alpha;
    UWORD                       pprv_hspot_x;
    UWORD                       pprv_hspot_y;
    STRPTR                      pprv_filename;
    struct MUI_EventHandlerNode pprv_ehn;
    APTR                        pprv_dto;
    struct BitMapHeader        *pprv_bmhd;
    struct BitMap              *pprv_bm;
    LONG                        pprv_offset_x;
    LONG                        pprv_offset_y;
    LONG                        pprv_draw_width;
    LONG                        pprv_draw_height;
    BOOL                        pprv_set_hspot;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct PPreview_DATA *data = INST_DATA(cl, obj)

/*** Functions **************************************************************/
STATIC VOID killdto(struct PPreview_DATA *data)
{
    data->pprv_bm   = NULL;
    data->pprv_bmhd = NULL;

    if (data->pprv_dto)
    {
        DisposeDTObject(data->pprv_dto);
        data->pprv_dto = NULL;
    }
};

STATIC IPTR setup_datatype(Class *cl, Object *obj)
{
    SETUP_INST_DATA;

    if (data->pprv_dto) killdto(data); /* Object already existed */

    if (data->pprv_filename)
    {
        /* Prevent DOS Requesters from showing up */
        struct Process *me = (struct Process *)FindTask(0);
        APTR            oldwinptr = me->pr_WindowPtr;
        me->pr_WindowPtr = (APTR)-1;

        data->pprv_dto = NewDTObject(data->pprv_filename,
                            DTA_GroupID, GID_PICTURE,
                            OBP_Precision, PRECISION_IMAGE,
                            PDTA_Screen, _screen(obj),
                            PDTA_DestMode, PMODE_V43,
                            PDTA_UseFriendBitMap, TRUE,
                            TAG_DONE);

        me->pr_WindowPtr = oldwinptr;

        D(bug("[Pointer/setup] dto %p\n", data->pprv_dto));
        if (data->pprv_dto)
        {
            struct FrameInfo fri = {0};

            DoMethod(data->pprv_dto, DTM_FRAMEBOX, 0, &fri, &fri, sizeof(struct FrameInfo), 0);
            if (fri.fri_Dimensions.Depth > 0)
            {
                if (DoMethod(data->pprv_dto, DTM_PROCLAYOUT, 0, 1))
                {
                    GET(data->pprv_dto, PDTA_BitMapHeader, &data->pprv_bmhd);
                    if (data->pprv_bmhd)
                    {
                        if (data->pprv_bmhd->bmh_Masking != mskNone)
                            SET(obj, MUIA_FillArea, TRUE);
                        else
                            SET(obj, MUIA_FillArea, FALSE);

                        GetDTAttrs(data->pprv_dto, PDTA_DestBitMap, &data->pprv_bm, TAG_DONE);
                        if (!data->pprv_bm)
                        {
                            GetDTAttrs(data->pprv_dto, PDTA_BitMap, &data->pprv_bm, TAG_DONE);
                        }
                        D(bug("[Pointer/setup] BitMap %p\n", data->pprv_bm));
                        if (data->pprv_bm) return TRUE;
                    } /* if (data->bmhd) */
                } /* if (DoMethod(data->dto, DTM_PROCLAYOUT, 0, 1)) */
            } /* if (fri.fri_Dimensions.Depth > 0) */
        } /* if (data->dto) */
    } /* if (data->name) */
    killdto(data);

    return TRUE;
}

/*** Methods ****************************************************************/
Object *PPreview__OM_NEW(Class *cl, Object *obj, struct opSet *msg)
{
    struct TagItem  *tstate = msg->ops_AttrList;
    struct TagItem  *tag    = NULL;

    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj) return 0;

    SETUP_INST_DATA;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_PPreview_Alpha:
                data->pprv_alpha = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotX:
                data->pprv_hspot_x = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotY:
                data->pprv_hspot_y = tag->ti_Data;
                break;

            case MUIA_PPreview_SetHSpot:
                data->pprv_set_hspot= tag->ti_Data;
                break;

            case MUIA_PPreview_FileName:
                FreeVec(data->pprv_filename);
                data->pprv_filename = StrDup((STRPTR)tag->ti_Data);
                break;

        }
    }

    data->pprv_offset_x = -1;
    data->pprv_offset_y = -1;

    data->pprv_ehn.ehn_Events   = IDCMP_MOUSEBUTTONS;
    data->pprv_ehn.ehn_Priority = 0;
    data->pprv_ehn.ehn_Flags    = 0;
    data->pprv_ehn.ehn_Object   = obj;
    data->pprv_ehn.ehn_Class    = cl;

    return obj;
}

IPTR PPreview__OM_DISPOSE(Class *cl, Object *obj, Msg msg)
{
    SETUP_INST_DATA;

    FreeVec(data->pprv_filename);
    return DoSuperMethodA(cl, obj, msg);
}

IPTR PPreview__OM_SET(Class *cl, Object *obj, struct opSet *msg)
{
    SETUP_INST_DATA;

    struct TagItem       *tags  = msg->ops_AttrList;
    struct TagItem       *tag;
    BOOL needs_redraw           = FALSE;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch(tag->ti_Tag)
        {
            case MUIA_PPreview_Alpha:
                needs_redraw = TRUE;
                data->pprv_alpha = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotX:
                needs_redraw = TRUE;
                data->pprv_hspot_x = tag->ti_Data;
                break;

            case MUIA_PPreview_HSpotY:
                needs_redraw = TRUE;
                data->pprv_hspot_y = tag->ti_Data;
                break;

            case MUIA_PPreview_SetHSpot:
                needs_redraw = TRUE;
                data->pprv_set_hspot = tag->ti_Data;
                break;

            case MUIA_PPreview_FileName:
                needs_redraw = TRUE;
                FreeVec(data->pprv_filename);
                data->pprv_filename = StrDup((STRPTR)tag->ti_Data);
                //if (_flags(obj) & MADF_SETUP) setup_datatype(cl, obj);
                setup_datatype(cl, obj);
                break;

        } /* switch(tag->ti_Tag) */
    } /* while ((tag = NextTagItem(&tags)) != NULL) */

    if (needs_redraw)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__OM_GET(Class *cl, Object *obj, struct opGet *msg)
{
    SETUP_INST_DATA;

    switch (msg->opg_AttrID)
    {
        case MUIA_PPreview_Alpha:
            *msg->opg_Storage = data->pprv_alpha;
            return TRUE;

        case MUIA_PPreview_HSpotX:
            *msg->opg_Storage = data->pprv_hspot_x;
            return TRUE;

        case MUIA_PPreview_HSpotY:
            *msg->opg_Storage = data->pprv_hspot_y;
            return TRUE;

        case MUIA_PPreview_SetHSpot:
            *msg->opg_Storage = data->pprv_set_hspot;
            return TRUE;

        case MUIA_PPreview_FileName:
            *msg->opg_Storage = (IPTR)data->pprv_filename;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__MUIM_Setup(Class *cl, Object *obj, struct MUIP_Setup *msg)
{
    SETUP_INST_DATA;

    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->pprv_ehn);

    data->pprv_prevEditor = (Object *)XGET((Object *)XGET(obj, MUIA_Parent), MUIA_Parent);

    return setup_datatype(cl, obj);
}

IPTR PPreview__MUIM_Cleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg)
{
    SETUP_INST_DATA;

    killdto(data);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->pprv_ehn);

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR PPreview__MUIM_Draw(Class *cl, Object *obj, struct MUIP_Draw *msg)
{
    SETUP_INST_DATA;

    DoSuperMethodA(cl, obj, (Msg)msg);

    data->pprv_offset_x = -1;
    data->pprv_offset_y = -1;

    if ((msg->flags & MADF_DRAWOBJECT) && data->pprv_bm)
    {
        ULONG drawwidth, drawheight, drawoffsetx, drawoffsety;

        ULONG depth = (ULONG) GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH);
        LONG bmwidth  = data->pprv_bmhd->bmh_Width;
        LONG bmheight = data->pprv_bmhd->bmh_Height;

        // calculate for centered rendering
        if (_width(obj) > bmwidth + 2) // two pixels for bounding box
        {
            drawwidth = bmwidth;
            drawoffsetx = (_width(obj) - drawwidth) / 2 + _left(obj);
        }
        else
        {
            drawwidth = _width(obj);
            drawoffsetx = _left(obj);
        }

        if (_height(obj) > bmheight + 2)
        {
            drawheight = bmheight;
            drawoffsety = (_height(obj) - drawheight) / 2 + _top(obj);
        }
        else
        {
            drawheight = _height(obj);
            drawoffsety = _top(obj);
        }

        // remember offset for event handler
        data->pprv_offset_x = drawoffsetx;
        data->pprv_offset_y = drawoffsety;
        data->pprv_draw_width = drawwidth;
        data->pprv_draw_height = drawheight;

        D(bug("[Pointer/Draw] bitmap %p depth %u\n", data->pprv_bm, depth));
        if ((depth >= 15) && (data->pprv_bmhd->bmh_Masking == mskHasAlpha))
        {
            /* Transparency on high color rast port with alpha channel in picture*/
            ULONG * img = AllocVec(bmwidth * bmheight * 4, MEMF_ANY);
            if (img)
            {
                struct pdtBlitPixelArray pa;
                pa.MethodID = PDTM_READPIXELARRAY;
                pa.pbpa_PixelData = (UBYTE *) img;
                pa.pbpa_PixelFormat = PBPAFMT_ARGB;
                pa.pbpa_PixelArrayMod = bmwidth * 4;
                pa.pbpa_Left = 0;
                pa.pbpa_Top = 0;
                pa.pbpa_Width = bmwidth;
                pa.pbpa_Height = bmheight;
                if (DoMethodA(data->pprv_dto, (Msg) &pa))
                {
                    D(bug("[Pointer/Draw] ReadPixelarray for d>=15 OK\n"));
                    WritePixelArrayAlpha
                    (
                        img, 0, 0, bmwidth * 4, _rp(obj),
                        drawoffsetx, drawoffsety, drawwidth, drawheight, 0xffffffff
                    );
                }
                FreeVec((APTR) img);
            }
        }
        else
        {
            if (data->pprv_bmhd->bmh_Masking == mskHasMask)
            {
                /* Transparency with mask */
                APTR mask = NULL;

                GetDTAttrs(data->pprv_dto, PDTA_MaskPlane, (IPTR)&mask, TAG_DONE);

                if (mask)
                    BltMaskBitMapRastPort(data->pprv_bm, 0, 0, _rp(obj), drawoffsetx,
                        drawoffsety, drawwidth, drawheight, 0xE0, (PLANEPTR)mask);
            }
            else
            {
                /* All other cases */
                BltBitMapRastPort(data->pprv_bm, 0, 0, _rp(obj), drawoffsetx, drawoffsety,
                    drawwidth, drawheight, 0xC0);
            }
        }

        // draw bounding box
        SetAPen(_rp(obj), 1);
        Move(_rp(obj), drawoffsetx - 1, drawoffsety - 1);
        Draw(_rp(obj), drawoffsetx + drawwidth + 1, drawoffsety - 1);
        Draw(_rp(obj), drawoffsetx + drawwidth + 1, drawoffsety + drawheight + 1);
        Draw(_rp(obj), drawoffsetx - 1, drawoffsety + drawheight + 1);
        Draw(_rp(obj), drawoffsetx - 1, drawoffsety - 1);

        // draw hotspot
        if (data->pprv_set_hspot)
        {
            D(bug("[Pointer/Draw] Draw hotspot at %d %d\n", drawoffsetx + data->pprv_hspot_x, drawoffsety + data->pprv_hspot_y));
	    /* I experimented with inversion, it looks better IMHO - sonic
            SetAPen(_rp(obj), 1); */
	    SetDrMd(_rp(obj), COMPLEMENT);
            if (data->pprv_hspot_x < drawwidth)
            {
                Move(_rp(obj), drawoffsetx + data->pprv_hspot_x, drawoffsety);
                Draw(_rp(obj), drawoffsetx + data->pprv_hspot_x, drawoffsety + drawheight);
            }
            if (data->pprv_hspot_y < drawheight)
            {
                Move(_rp(obj), drawoffsetx, drawoffsety + data->pprv_hspot_y);
                Draw(_rp(obj), drawoffsetx + drawwidth, drawoffsety + data->pprv_hspot_y);
            }
	    /* Reset drawmode back to JAM2, otherwise shit happens */
	    SetDrMd(_rp(obj), JAM2);
        }
    }

    return 0;
}

IPTR PPreview__MUIM_AskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    IPTR retval = DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth  += 64;
    msg->MinMaxInfo->MinHeight += 64;
    msg->MinMaxInfo->DefWidth  += 64;
    msg->MinMaxInfo->DefHeight += 64;
    msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight  = MUI_MAXMAX;

    return retval;
}

IPTR PPreview__MUIM_HandleEvent(Class *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
    SETUP_INST_DATA;

    #define _between(a,x,b) ((x)>=(a) && (x)<=(b))
    #define _isinobject(x,y) (_between(data->pprv_offset_x,(x),data->pprv_offset_x+data->pprv_draw_width) && \
        _between(data->pprv_offset_y,(y),data->pprv_offset_y+data->pprv_draw_height))

    if (data->pprv_set_hspot && (data->pprv_offset_x != -1) && msg->imsg)
    {
        switch (msg->imsg->Class)
        {
            case IDCMP_MOUSEBUTTONS:
            {
                if (msg->imsg->Code == SELECTUP)
                {
                    D(bug("[PPreview/HandleEvent] offx %d offy %d w %d h %d mx %d my %d\n",
                        data->pprv_offset_x, data->pprv_offset_y,
                        data->pprv_draw_width, data->pprv_draw_height,
                        msg->imsg->MouseX, msg->imsg->MouseY));
                    if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
                    {
                        data->pprv_hspot_x = msg->imsg->MouseX - data->pprv_offset_x;
                        data->pprv_hspot_y = msg->imsg->MouseY - data->pprv_offset_y;
                        D(bug("[PPreview/HandleEvent] X %d Y %d\n", data->pprv_hspot_x, data->pprv_hspot_y));
                        MUI_Redraw(obj, MADF_DRAWOBJECT);

                        SET(data->pprv_prevEditor, MUIA_PrefsEditor_Changed, TRUE);
                    }
                }
            }
            break;

        }
    }

    #undef _between
    #undef _isinobject

    return 0;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_9
(
    PPreview, NULL, MUIC_Area, NULL,
    OM_NEW,             struct opSet *,
    OM_DISPOSE,         Msg,
    OM_SET,             struct opSet *,
    OM_GET,             struct opGet *,
    MUIM_Setup,         struct MUIP_Setup *,
    MUIM_Cleanup,       struct MUIP_Cleanup *,
    MUIM_Draw,          struct MUIP_Draw *,
    MUIM_AskMinMax,     struct MUIP_AskMinMax *,
    MUIM_HandleEvent,   struct MUIP_HandleEvent *
);
