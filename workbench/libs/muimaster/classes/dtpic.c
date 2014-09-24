/*
    Copyright © 2002-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Dtpic.mui. Source based on the one from MUIUndoc */

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>
#include <stdlib.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>

#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/datatypes.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "dtpic_private.h"

extern struct Library *MUIMasterBase;

#ifdef DataTypesBase
#undef DataTypesBase
#endif

#define DataTypesBase data->datatypesbase

static void killdto(struct Dtpic_DATA *data)
{
    if (data->bm_selected) FreeBitMap(data->bm_selected);
    if (data->bm_highlighted) FreeBitMap(data->bm_highlighted);
    data->bm = NULL;
    data->bmhd = NULL;
    data->bm_selected = NULL;
    data->bm_highlighted = NULL;

    if (data->dto)
    {
        DisposeDTObject(data->dto);
        data->dto = NULL;
    }

    if (data->datatypesbase)
    {
        CloseLibrary(data->datatypesbase);
    }
}

static void change_event_handler(Object *obj, struct Dtpic_DATA *data)
{
    // enable only events which we really need
    ULONG events = 0;

    if (data->darkenselstate)
    {
        events |= IDCMP_MOUSEBUTTONS;
    }
    else
    {
        // disable pending selected state
        data->selected = FALSE;
    }

    if (data->lightenonmouse)
    {
        // FIXME: change to IDCMP_MOUSEOBJECT if available
        events |= IDCMP_MOUSEMOVE;
    }
    else
    {
        // disable highlighting mode
        data->highlighted = FALSE;
    }

    if (data->deltaalpha)
    {
        events |= IDCMP_INTUITICKS;
    }

    if (events != data->ehn.ehn_Events)
    {
        // remove event handler if it was installed
        if (data->eh_active)
        {
            DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
        }
        // enable event handler for changed events
        data->ehn.ehn_Events = events;
        DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehn);
        data->eh_active = TRUE;
    }
}

static void update_alpha(struct Dtpic_DATA *data)
{
    if (data->fade < 0)
    {
        // immediately set alpha to end value
        data->currentalpha = data->alpha;
        data->deltaalpha = 0;
    }
    else
    {
        // calculate delta
        if (data->alpha > data->currentalpha)
        {
            // fading should happen every 1/20 sec.
            // Because we're using Intuiticks we must
            // convert the value.
            data->deltaalpha = data->fade * 50 / 20;
        }
        else if (data->alpha < data->currentalpha)
        {
            data->deltaalpha = -data->fade * 50 / 20;
        }
        else
        {
            data->deltaalpha = 0;
        }
    }
    D(bug("[Dtpic/update_alpha] alpha %d delta %d current %d\n", data->alpha, data->deltaalpha, data->currentalpha));
}

static struct BitMap *clone_bitmap(struct BitMap *from_bm, ULONG operation, ULONG value)
{
    if (from_bm == NULL)
        return NULL;

    struct BitMap *to_bm = NULL;
    struct RastPort rp;

    UWORD width = GetBitMapAttr(from_bm, BMA_WIDTH);
    UWORD height = GetBitMapAttr(from_bm, BMA_HEIGHT);
    UWORD depth = GetBitMapAttr(from_bm, BMA_DEPTH);

    InitRastPort(&rp);
    to_bm = AllocBitMap(width, height, depth, BMF_MINPLANES, from_bm);
    D(bug("[clone_bitmap] %p width %d height %d depth %d\n", to_bm, width, height, depth));
    if (to_bm)
    {
        rp.BitMap = to_bm;
        BltBitMapRastPort(from_bm, 0, 0, &rp, 0, 0, width, height, 0xC0);
        ProcessPixelArray(&rp, 0, 0, width, height, operation, value, NULL);
    }
    return to_bm;
}

IPTR setup_datatype(struct IClass *cl, Object *obj)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    if (data->dto)
        killdto(data);          /* Object already existed */

    if (data->name)
    {
        if ((data->datatypesbase = OpenLibrary("datatypes.library", 39)))
        {
            /* Prevent DOS Requesters from showing up */

            struct Process *me = (struct Process *)FindTask(0);
            APTR oldwinptr = me->pr_WindowPtr;

            me->pr_WindowPtr = (APTR) - 1;

            data->dto = NewDTObject(data->name, DTA_GroupID, GID_PICTURE,
                OBP_Precision, PRECISION_IMAGE,
                PDTA_Screen, _screen(obj),
                PDTA_DestMode, PMODE_V43,
                PDTA_UseFriendBitMap, TRUE, TAG_DONE);
            me->pr_WindowPtr = oldwinptr;

            if (data->dto)
            {
                struct FrameInfo fri = { 0 };

                DoMethod(data->dto, DTM_FRAMEBOX, 0, &fri, &fri,
                    sizeof(struct FrameInfo), 0);

                if (fri.fri_Dimensions.Depth > 0)
                {
                    if (DoMethod(data->dto, DTM_PROCLAYOUT, 0, 1))
                    {
                        get(data->dto, PDTA_BitMapHeader, &data->bmhd);

                        if (data->bmhd)
                        {
                            if (data->bmhd->bmh_Masking != mskNone)
                                set(obj, MUIA_FillArea, TRUE);
                            else
                                set(obj, MUIA_FillArea, FALSE);

                            GetDTAttrs(data->dto, PDTA_DestBitMap,
                                &data->bm, TAG_DONE);

                            if (!data->bm)
                            {
                                GetDTAttrs(data->dto, PDTA_BitMap,
                                    &data->bm, TAG_DONE);
                            }

                            if (data->bm)
                            {
                                // create BitMaps for selected and highlighted state
                                data->bm_selected = clone_bitmap(data->bm, POP_DARKEN, 127);
                                data->bm_highlighted = clone_bitmap(data->bm, POP_BRIGHTEN, 50);

                                return TRUE;
                            }
                        }
                    }
                }
            }
        }
    }
    killdto(data);

    return TRUE;
}

IPTR Dtpic__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);

    if (obj)
    {
        struct Dtpic_DATA *data = INST_DATA(cl, obj);
        struct TagItem *tags = msg->ops_AttrList;
        struct TagItem *tag;

        // initial values
        data->currentalpha = data->alpha = 0xff;

        while ((tag = NextTagItem(&tags)) != NULL)
        {
            switch (tag->ti_Tag)
            {
            case MUIA_Dtpic_Name:
                // acc. to AOS4-MUI4 autodoc the string isn't copied
                data->name = (STRPTR)tag->ti_Data;
                break;
            case MUIA_Dtpic_Alpha:
                data->alpha = tag->ti_Data;
                break;
            case MUIA_Dtpic_DarkenSelState:
                data->darkenselstate = tag->ti_Data ? TRUE : FALSE;
                break;
            case MUIA_Dtpic_Fade:
                data->fade = tag->ti_Data;
                break;
            case MUIA_Dtpic_LightenOnMouse:
                data->lightenonmouse = tag->ti_Data ? TRUE : FALSE;
                break;
            }
        }

        data->ehn.ehn_Events = 0;
        data->ehn.ehn_Priority = 0;
        data->ehn.ehn_Flags = 0;
        data->ehn.ehn_Object = obj;
        data->ehn.ehn_Class = cl;

        update_alpha(data);
    }

    return (IPTR) obj;
}

IPTR Dtpic__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    return setup_datatype(cl, obj);
}

IPTR Dtpic__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    if (data->eh_active)
    {
        DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
    }

    killdto(data);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Dtpic__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->bm)
    {
        msg->MinMaxInfo->MinWidth += data->bmhd->bmh_Width;
        msg->MinMaxInfo->MinHeight += data->bmhd->bmh_Height;
        msg->MinMaxInfo->DefWidth += data->bmhd->bmh_Width;
        msg->MinMaxInfo->DefHeight += data->bmhd->bmh_Height;
        msg->MinMaxInfo->MaxWidth += data->bmhd->bmh_Width;
        msg->MinMaxInfo->MaxHeight += data->bmhd->bmh_Height;
    }

    return retval;
}

IPTR Dtpic__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    // TODO: rendering of different states

    D(bug("[Dtpic/MUIM_Draw] selected %d highlighted %d alpha %d\n",
        data->selected, data->highlighted, data->currentalpha));

    DoSuperMethodA(cl, obj, (Msg) msg);

    if ((msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)) && data->bm)
    {
        /* Note: codes taken from picture.datatype GM_RENDER routine */
        ULONG depth = (ULONG) GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH);

        if ((depth >= 15) && (data->bmhd->bmh_Masking == mskHasAlpha))
        {
            /* Transparency on high color rast port with alpha channel in 
             * picture */
            ULONG *img =
                AllocVec(_mwidth(obj) * _mheight(obj) * 4, MEMF_ANY);
            if (img)
            {
                struct pdtBlitPixelArray pa;
                pa.MethodID = PDTM_READPIXELARRAY;
                pa.pbpa_PixelData = (UBYTE *) img;
                pa.pbpa_PixelFormat = PBPAFMT_ARGB;
                pa.pbpa_PixelArrayMod = _mwidth(obj) * 4;
                pa.pbpa_Left = 0;
                pa.pbpa_Top = 0;
                pa.pbpa_Width = _mwidth(obj);
                pa.pbpa_Height = _mheight(obj);
                if (DoMethodA(data->dto, (Msg) & pa))
                    WritePixelArrayAlpha(img, 0, 0, _mwidth(obj) * 4,
                        _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj),
                        _mheight(obj), 0xffffffff);
                FreeVec((APTR) img);
            }
        }
        else
        {
            if (data->bmhd->bmh_Masking == mskHasMask)
            {
                /* Transparency with mask */
                APTR mask = NULL;

                GetDTAttrs(data->dto, PDTA_MaskPlane, (IPTR) & mask,
                    TAG_DONE);

                if (mask)
                    BltMaskBitMapRastPort(data->bm, 0, 0, _rp(obj),
                        _mleft(obj), _mtop(obj), _mwidth(obj),
                        _mheight(obj), 0xE0, (PLANEPTR) mask);
            }
            else
            {
                /* All other cases */

                struct BitMap *bm = data->bm;
                if (data->selected)
                {
                    bm = data->bm_selected;
                    D(bug("render selected\n"));
                }
                else if (data->highlighted)
                {
                    D(bug("render highlighted\n"));
                    bm = data->bm_highlighted;
                }
                else
                {
                    D(bug("render normal\n"));
                }
                
                BltBitMapRastPort(bm, 0, 0, _rp(obj), _mleft(obj),
                    _mtop(obj), _mwidth(obj), _mheight(obj), 0xC0);
            }
        }
    }

    return 0;
}

IPTR Dtpic__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    //struct Dtpic_DATA *data = INST_DATA(cl, obj);

    return DoSuperMethodA(cl, obj, msg);
}

IPTR Dtpic__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;
    ULONG needs_redraw = 0;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Dtpic_Name:
            /* If no filename or different filenames */
            if (!data->name || strcmp(data->name, (char *)tag->ti_Data))
            {
                data->name = (STRPTR)tag->ti_Data;

                /* Run immediate setup only if base class is setup up */
                if (_flags(obj) & MADF_SETUP)
                    setup_datatype(cl, obj);
                needs_redraw = 1;
            }
            break;
        case MUIA_Dtpic_Alpha:
            data->alpha = tag->ti_Data;
            break;
        case MUIA_Dtpic_Fade:
            data->fade = tag->ti_Data;
            break;
        }
    }

    update_alpha(data);
    if (_flags(obj) & MADF_SETUP)
        change_event_handler(obj, data);

    if (needs_redraw)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Dtpic__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Dtpic_Name:
        *(msg->opg_Storage) = (IPTR) data->name;
        return TRUE;
    case MUIA_Dtpic_Alpha:
        *(msg->opg_Storage) = data->alpha;
        return TRUE;
    case MUIA_Dtpic_DarkenSelState:
        *(msg->opg_Storage) = data->darkenselstate;
        return TRUE;
    case MUIA_Dtpic_Fade:
        *(msg->opg_Storage) = data->fade;
        return TRUE;
    case MUIA_Dtpic_LightenOnMouse:
        *(msg->opg_Storage) = data->lightenonmouse;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Dtpic__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
        switch (msg->imsg->Class)
        {
        case IDCMP_INTUITICKS:
            data->currentalpha += data->deltaalpha;
            if (data->deltaalpha > 0)
            {
                if (data->currentalpha > data->alpha)
                {
                    // reached target alpha, no more incrementing
                    data->currentalpha = data->alpha;
                    data->deltaalpha = 0;
                }
            }
            else if (data->deltaalpha < 0)
            {
                if (data->currentalpha < data->alpha)
                {
                    // reached target alpha, no more decrementing
                    data->currentalpha = data->alpha;
                    data->deltaalpha = 0;
                }
            }
            D(bug("intuitick %d %d\n", msg->imsg->MouseX, msg->imsg->MouseY));
            update_alpha(data);
            change_event_handler(obj, data);
            MUI_Redraw(obj, MADF_DRAWUPDATE);

            break;

        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code==SELECTDOWN)
            {
                if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
                {
                    data->selected = TRUE;
                    D(bug("selectdown %d %d\n", msg->imsg->MouseX, msg->imsg->MouseY));
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
            }
            else if (msg->imsg->Code==SELECTUP)
            {
                if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
                {
                    data->selected = FALSE;
                    D(bug("selectup %d %d\n", msg->imsg->MouseX, msg->imsg->MouseY));
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
            }
            break;

        case IDCMP_MOUSEMOVE:
            if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
            {
                data->highlighted = TRUE;
                D(bug("mouse move %d %d\n", msg->imsg->MouseX, msg->imsg->MouseY));
            }
            else
            {
                data->highlighted = FALSE;
            }
            MUI_Redraw(obj, MADF_DRAWUPDATE);
            break;
        }
    }

    return 0;
}

IPTR Dtpic__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    change_event_handler(obj, data);

    return retval;
}

IPTR Dtpic__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

    // remove event handler if it was installed
    if (data->eh_active)
    {
        DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehn);
        data->eh_active = FALSE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);

}

#if ZUNE_BUILTIN_DTPIC
BOOPSI_DISPATCHER(IPTR, Dtpic_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Dtpic__OM_NEW(cl, obj, (APTR)msg);
    case OM_DISPOSE:
        return Dtpic__OM_DISPOSE(cl, obj, (APTR)msg);
    case OM_SET:
        return Dtpic__OM_SET(cl, obj, (APTR)msg);
    case OM_GET:
        return Dtpic__OM_GET(cl, obj, (APTR)msg);

    case MUIM_Setup:
        return Dtpic__MUIM_Setup(cl, obj, (APTR)msg);
    case MUIM_Cleanup:
        return Dtpic__MUIM_Cleanup(cl, obj, (APTR)msg);

    case MUIM_Show:
        return Dtpic__MUIM_Show(cl, obj, (APTR)msg);
    case MUIM_Hide:
        return Dtpic__MUIM_Show(cl, obj, (APTR)msg);

    case MUIM_AskMinMax:
        return Dtpic__MUIM_AskMinMax(cl, obj, (APTR)msg);
    case MUIM_Draw:
        return Dtpic__MUIM_Draw(cl, obj, (APTR)msg);
    case MUIM_HandleEvent:
        return Dtpic__MUIM_HandleEvent(cl, obj, (APTR)msg);

    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Dtpic_desc =
{
    MUIC_Dtpic,
    MUIC_Area,
    sizeof(struct Dtpic_DATA),
    (void *) Dtpic_Dispatcher
};
#endif /* ZUNE_BUILTIN_DTPIC */
