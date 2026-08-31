/*
    Copyright (C) 2002-2015, The AROS Development Team. All rights reserved.
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

    if (data->bg)
    {
        FreeVec(data->bg);
        data->bg = NULL;
    }
    if (data->comp)
    {
        FreeVec(data->comp);
        data->comp = NULL;
    }
    data->buf_width = 0;
    data->buf_height = 0;
    data->bg_valid = FALSE;

    if (data->dto)
    {
        DisposeDTObject(data->dto);
        data->dto = NULL;
    }

    if (data->datatypesbase)
    {
        CloseLibrary(data->datatypesbase);
        data->datatypesbase = NULL;
    }
}

/* Step size of the state transition per Intuitick. 255/60 gives roughly
 * an 85ms transition at 50 Intuiticks per second. */
#define DTPIC_STATE_FADE_STEP 60

static LONG dtpic_state_target(struct Dtpic_DATA *data)
{
    if (data->selected)
        return -127;
    if (data->highlighted)
        return 50;
    return 0;
}

static BOOL dtpic_state_animating(struct Dtpic_DATA *data)
{
    return data->state_offset != dtpic_state_target(data);
}

static void update_state(struct Dtpic_DATA *data)
{
    LONG target = dtpic_state_target(data);

    if (data->state_offset < target)
    {
        data->state_offset += DTPIC_STATE_FADE_STEP;
        if (data->state_offset > target)
            data->state_offset = target;
    }
    else if (data->state_offset > target)
    {
        data->state_offset -= DTPIC_STATE_FADE_STEP;
        if (data->state_offset < target)
            data->state_offset = target;
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

    if (data->deltaalpha || dtpic_state_animating(data))
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
    D(bug("[Dtpic/update_alpha] alpha %d delta %d current %d\n",
        data->alpha, data->deltaalpha, data->currentalpha));
}

static struct BitMap *clone_bitmap(struct BitMap *from_bm, ULONG operation,
    ULONG value)
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
    D(bug("[clone_bitmap] %p width %d height %d depth %d\n", to_bm, width,
        height, depth));
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
                                /* create BitMaps for selected and
                                 * highlighted state */
                                data->bm_selected =
                                    clone_bitmap(data->bm, POP_DARKEN, 127);
                                data->bm_highlighted =
                                    clone_bitmap(data->bm, POP_BRIGHTEN, 50);

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
        data->bg = NULL;
        data->comp = NULL;
        data->buf_width = 0;
        data->buf_height = 0;
        data->bg_valid = FALSE;
        data->state_offset = 0;

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

/*
 * Apply the selected/highlighted brightness offset to an ARGB pixel array
 * in place. Pixel format is RECTFMT_ARGB: low byte alpha, then red, green,
 * blue.
 */
static void dtpic_apply_state(ULONG *pixels, ULONG count, LONG offset)
{
    ULONG i;

    if (offset == 0)
        return;

    for (i = 0; i < count; i++)
    {
        ULONG px = pixels[i];
        LONG r = (px >> 8) & 0xff;
        LONG g = (px >> 16) & 0xff;
        LONG b = (px >> 24) & 0xff;

        r += offset;
        g += offset;
        b += offset;
        if (r > 255) r = 255;
        else if (r < 0) r = 0;
        if (g > 255) g = 255;
        else if (g < 0) g = 0;
        if (b > 255) b = 255;
        else if (b < 0) b = 0;

        pixels[i] = (px & 0xff) | ((ULONG) r << 8) | ((ULONG) g << 16)
            | ((ULONG) b << 24);
    }
}

/*
 * Render an image with an alpha channel by compositing it off-screen and
 * writing the result to the screen in one opaque blit. This avoids both the
 * accumulation caused by repeatedly alpha-blending on top of the previous
 * rendering and the flicker caused by erasing the background on screen.
 * Returns TRUE on success, FALSE if the caller has to fall back to direct
 * alpha blitting.
 */
static BOOL Dtpic_alpha_draw(Object *obj, struct Dtpic_DATA *data,
    ULONG flags)
{
    ULONG w = _mwidth(obj);
    ULONG h = _mheight(obj);
    ULONG *img;
    ULONG i;

    if (data->bg == NULL || data->comp == NULL ||
        data->buf_width != (LONG)w || data->buf_height != (LONG)h)
    {
        FreeVec(data->bg);
        FreeVec(data->comp);
        data->bg = AllocVec(w * h * 4, MEMF_ANY);
        data->comp = AllocVec(w * h * 4, MEMF_ANY);
        data->buf_width = w;
        data->buf_height = h;
        data->bg_valid = FALSE;
    }

    if (data->bg == NULL || data->comp == NULL)
        return FALSE;

    /* On a full redraw the superclass has already drawn the background onto
     * the rastport. Capture it so updates can reuse it without erasing the
     * screen. */
    if (flags & MADF_DRAWOBJECT)
    {
        if (!ReadPixelArray(data->bg, 0, 0, w * 4, _rp(obj), _mleft(obj),
            _mtop(obj), w, h, RECTFMT_ARGB))
        {
            /* Never reuse a possibly stale background */
            data->bg_valid = FALSE;
            return FALSE;
        }
        data->bg_valid = TRUE;
    }
    else if (!data->bg_valid)
    {
        return FALSE;
    }

    img = AllocVec(w * h * 4, MEMF_ANY);
    if (img == NULL)
        return FALSE;

    {
        struct pdtBlitPixelArray pa;
        pa.MethodID = PDTM_READPIXELARRAY;
        pa.pbpa_PixelData = (UBYTE *) img;
        pa.pbpa_PixelFormat = PBPAFMT_ARGB;
        pa.pbpa_PixelArrayMod = w * 4;
        pa.pbpa_Left = 0;
        pa.pbpa_Top = 0;
        pa.pbpa_Width = w;
        pa.pbpa_Height = h;
        if (!DoMethodA(data->dto, (Msg) & pa))
        {
            FreeVec(img);
            return FALSE;
        }
    }

    /* Apply the state, then blend the image over the cached background with
     * the global alpha value. Pixel format of all buffers is RECTFMT_ARGB:
     * low byte alpha, then red, green, blue. */
    dtpic_apply_state(img, w * h, data->state_offset);
    for (i = 0; i < w * h; i++)
    {
        ULONG s = img[i];
        ULONG d = data->bg[i];
        ULONG sa = (s & 0xff) * (ULONG) data->currentalpha >> 8;
        ULONG sr = (s >> 8) & 0xff;
        ULONG sg = (s >> 16) & 0xff;
        ULONG sb = (s >> 24) & 0xff;

        data->comp[i] = (((sb * sa + ((d >> 24) & 0xff) * (255 - sa)) >> 8) << 24)
            | (((sg * sa + ((d >> 16) & 0xff) * (255 - sa)) >> 8) << 16)
            | (((sr * sa + ((d >> 8) & 0xff) * (255 - sa)) >> 8) << 8)
            | 0xff;
    }

    WritePixelArray(data->comp, 0, 0, w * 4, _rp(obj), _mleft(obj),
        _mtop(obj), w, h, RECTFMT_ARGB);

    FreeVec(img);
    return TRUE;
}

IPTR Dtpic__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Dtpic_DATA *data = INST_DATA(cl, obj);

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
            if (!Dtpic_alpha_draw(obj, data, msg->flags))
            {
                /* Fallback: blend directly onto the rastport. On updates
                 * the superclass does not redraw the background, so the
                 * image would be alpha-blended on top of its own previous
                 * rendering. Restore the background first. */
                if ((msg->flags & MADF_DRAWUPDATE) &&
                    !(msg->flags & MADF_DRAWOBJECT))
                {
                    DoMethod(obj, MUIM_DrawBackground, _mleft(obj),
                        _mtop(obj), _mwidth(obj), _mheight(obj),
                        _mleft(obj), _mtop(obj), 0);
                }

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
                    {
                        dtpic_apply_state(img, _mwidth(obj) * _mheight(obj),
                            data->state_offset);
                        WritePixelArrayAlpha(img, 0, 0, _mwidth(obj) * 4,
                            _rp(obj), _mleft(obj), _mtop(obj),
                            _mwidth(obj), _mheight(obj), 0xffffffff);
                    }
                    FreeVec((APTR) img);
                }
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
            update_state(data);
            change_event_handler(obj, data);
            MUI_Redraw(obj, MADF_DRAWUPDATE);

            break;

        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code==SELECTDOWN)
            {
                if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
                {
                    data->selected = TRUE;
                    D(bug("selectdown %d %d\n", msg->imsg->MouseX,
                        msg->imsg->MouseY));
                    change_event_handler(obj, data);
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
            }
            else if (msg->imsg->Code==SELECTUP)
            {
                if (data->selected)
                {
                    data->selected = FALSE;
                    D(bug("selectup %d %d\n", msg->imsg->MouseX,
                        msg->imsg->MouseY));
                    change_event_handler(obj, data);
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
            }
            break;

        case IDCMP_MOUSEMOVE:
            if (_isinobject(obj, msg->imsg->MouseX, msg->imsg->MouseY))
            {
                if (!data->highlighted)
                {
                    data->highlighted = TRUE;
                    D(bug("mouse move %d %d\n", msg->imsg->MouseX,
                        msg->imsg->MouseY));
                    change_event_handler(obj, data);
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
            }
            else
            {
                if (data->highlighted)
                {
                    data->highlighted = FALSE;
                    change_event_handler(obj, data);
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                }
            }
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
        data->ehn.ehn_Events = 0;
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
        return Dtpic__MUIM_Hide(cl, obj, (APTR)msg);

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
