/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "scale_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

/****** Scale.mui/MUIC_Scale ************
*
*   NAME
*       MUIC_Scale
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Draw a horizontal percentage scale from 0% to 100%. This is a decorative
*       Area subclass with no value attribute, range configuration, numeric
*       input or automatic association with Gauge. The default orientation is
*       horizontal. Vertical orientation can be stored and retrieved but has no
*       scale rendering or class-specific size implementation.
*
*   METHODS
*       Setup calls Area and measures the literal string "0% 100%" using the
*       current font; it caches that width and the font height. It returns zero
*       if Area setup fails, otherwise one. Horizontal AskMinMax adds the cached
*       width to minimum/default widths, permits maximum width MUI_MAXMAX and
*       adds font height plus four pixels to all height fields. In vertical mode
*       it leaves superclass size fields unchanged. It returns zero.
*       Draw calls Area and, in horizontal mode, paints a bar, endpoint labels
*       and alternating long/short marks. It selects a power-of-two subdivision
*       count from available width and measured strings. The fit estimate
*       measures subdivision indices followed by percent signs, rather than the
*       actual percentage labels; it is not a guarantee that labels fit.
*       Endpoint labels are 0% and 100%; intermediate displayed values use
*       integer division. Temporary Zune text objects are freed after use;
*       allocation failure skips the affected label. Draw returns zero and has
*       no local draw-flag check.
*
*   NOTES
*       No persistent strings, bitmap, pen or native gadget are allocated by
*       this class. Use the normal Area lifecycle with a live font and render
*       context. Orientation changes request redraw but do not request relayout
*       or remeasure text. No vertical drawing is supplied after such a change.
*
*/
/****** Scale.mui/MUIA_Scale_Horiz ************
*
*   NAME
*       MUIA_Scale_Horiz
*
*   FUNCTION
*       Store or retrieve the requested scale orientation.
*
*   INPUTS
*       Initialize, set and get, BOOL. Default TRUE for horizontal. FALSE
*       selects the unimplemented vertical branch.
*
*   NOTES
*       A changed value requests full redraw; every supplied tag is still
*       forwarded to Area, including unchanged values. Get writes the stored
*       BOOL to caller-owned IPTR storage and returns TRUE. Set returns the
*       superclass result. There is no relayout request, so changing this
*       attribute does not itself update size negotiation. In vertical mode only
*       superclass drawing occurs and no class-specific sizes are added.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Construct Area and initialize Horiz TRUE unless overridden by tags.
 * Return object or zero on superclass failure. No owned resource or local
 * notification.
 */
IPTR Scale__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scale_DATA *data;
    struct TagItem *tag, *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->horiz = TRUE;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Scale_Horiz:
            data->horiz = tag->ti_Data;
            break;
        }
    }

    D(bug("muimaster.library/scale.c: Scale Object created at 0x%p\n",
            obj));

    return (IPTR) obj;
}

/*
 * Store each Horiz tag and request full redraw if any differs from the
 * preceding state. Forward all tags unchanged and return Area result.
 * Requires live object and valid tag list; no remeasurement or relayout
 * request.
 */
IPTR Scale__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scale_DATA *data;
    struct TagItem *tag, *tags;
    int need_redraw = 0;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Scale_Horiz:
            if (data->horiz != tag->ti_Data)
                need_redraw = 1;
            data->horiz = tag->ti_Data;
            break;
        }
    }

    if (need_redraw)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Return stored Horiz in valid caller-owned IPTR storage with TRUE;
 * delegate all other attributes to Area. No allocation or mutation.
 */
IPTR Scale__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Scale_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;
    IPTR tag = msg->opg_AttrID;

    switch (tag)
    {
    case MUIA_Scale_Horiz:
        *store = (IPTR) data->horiz;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Call Area setup; on success measure literal endpoint-label sample with
 * current font and cache width/height. Return zero on superclass failure,
 * otherwise one. Requires live font; owns no allocated measurement
 * resources.
 */
IPTR Scale__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Scale_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    {
        char *minlabel = "0% 100%";
        struct RastPort rp;

        InitRastPort(&rp);
        SetFont(&rp, _font(obj));

        data->label_minwidth = TextLength(&rp, minlabel, strlen(minlabel));
        data->label_height = _font(obj)->tf_YSize;
    }

    return 1;
}

/*
 * Call Area and add cached horizontal label sizes plus four-pixel bar
 * height; vertical branch adds nothing. Return zero ignoring superclass
 * result. Requires writable MinMaxInfo and prior Setup; no text
 * measurement.
 */
IPTR Scale__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct Scale_DATA *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->horiz)
    {
        msg->MinMaxInfo->MinWidth += data->label_minwidth;
        msg->MinMaxInfo->MinHeight += data->label_height + 4;
        msg->MinMaxInfo->DefWidth += data->label_minwidth;
        msg->MinMaxInfo->DefHeight += data->label_height + 4;
        msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
        msg->MinMaxInfo->MaxHeight += data->label_height + 4;
    }
    else
    {
    }
    return 0;
}

/*
 * Call Area; horizontally draw line, marks and temporary text objects,
 * skipping labels on allocation failure and freeing successful objects.
 * Vertical branch does nothing locally. Return zero; no local draw-flag
 * filtering. Requires valid font/render geometry; index-based fit estimate
 * does not guarantee label fit.
 */
IPTR Scale__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Scale_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);
    D(bug("muimaster.library/scale.c: Draw Scale Object "
        "at 0x%p %ldx%ldx%ldx%ld\n",
        obj, _left(obj), _top(obj), _right(obj), _bottom(obj)));

    if (data->horiz)
    {
        int i;
        //int  subdiv;
        char buf[255];
        int k;
        BOOL drawpct;

        /* main horizontal bar */
        SetAPen(_rp(obj), _pens(obj)[MPEN_TEXT]);
        Move(_rp(obj), _mleft(obj), _mtop(obj) + 3);
        Draw(_rp(obj), _mright(obj), _mtop(obj) + 3);

        /* testing possible layouts: 1 = 0...100 ; 2 = 0...50..100 ; etc ... */
        for (i = 2; i < _mwidth(obj); i *= 2)
        {
            int j;
            int total_width = 0;
            int too_big = 0;

            for (j = 0; j <= i; j++)
            {
                //int pct = j * 100 / i;

                snprintf(buf, 255, "%d%%", (int)j);
                total_width += TextLength(_rp(obj), buf, strlen(buf));
                if (total_width > (3 * _mwidth(obj) / 8))
                {
                    too_big = 1;
                    break;
                }
            }
            if (too_big)
                break;
        }

        for (k = 0, drawpct = TRUE; k <= i; k++)
        {
            /* draw 0% */
            if (k == 0)
            {
                ZText *ztext = zune_text_new(NULL, "0%", ZTEXT_ARG_NONE, 0);
                if (ztext)
                {
                    zune_text_get_bounds(ztext, obj);
                    zune_text_draw(ztext, obj, _mleft(obj), _mright(obj),
                        _mtop(obj) + 4);
                    zune_text_destroy(ztext);
                }
                Move(_rp(obj), _mleft(obj), _mtop(obj));
                Draw(_rp(obj), _mleft(obj), _mtop(obj) + 3);
                drawpct = FALSE;
            }
            else if (k == i)    /* draw 100% */
            {
                ZText *ztext =
                    zune_text_new("\33r", "100%", ZTEXT_ARG_NONE, 0);
                if (ztext)
                {
                    zune_text_get_bounds(ztext, obj);
                    zune_text_draw(ztext, obj, _mleft(obj), _mright(obj),
                        _mtop(obj) + 4);
                    zune_text_destroy(ztext);
                }
                Move(_rp(obj), _mright(obj), _mtop(obj));
                Draw(_rp(obj), _mright(obj), _mtop(obj) + 3);
                drawpct = FALSE;
            }
            else if (drawpct)   /* draw intermediate values and lines */
            {
                ZText *ztext;
                int val = k * 100 / i;

                snprintf(buf, 255, "%d%%", (int)val);
                ztext = zune_text_new(NULL, buf, ZTEXT_ARG_NONE, 0);
                if (ztext)
                {
                    int width;
                    zune_text_get_bounds(ztext, obj);
                    width = TextLength(_rp(obj), buf, strlen(buf));
                    zune_text_draw(ztext, obj,
                        _mleft(obj) + _mwidth(obj) * k / i - width / 2,
                        _mright(obj), _mtop(obj) + 4);
                    zune_text_destroy(ztext);
                }
                Move(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i,
                    _mtop(obj));
                Draw(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i,
                    _mtop(obj) + 3);
                drawpct = FALSE;
            }
            else                /* draw intermediate lines */
            {
                Move(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i,
                    _mtop(obj) + 1);
                Draw(_rp(obj), _mleft(obj) + _mwidth(obj) * k / i,
                    _mtop(obj) + 3);
                drawpct = TRUE;
            }
        }
    }
    else
    {
    }
    return 0;
}

#if ZUNE_BUILTIN_SCALE
/*
 * When built in, dispatch local methods and return their results; delegate
 * other messages to Area. Requires ABI-correct message and live object
 * except construction. Ownership and lifecycle follow the class contract.
 */
BOOPSI_DISPATCHER(IPTR, Scale_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Scale__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Scale__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Scale__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Scale__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_AskMinMax:
        return Scale__MUIM_AskMinMax(cl, obj, (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Scale__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Scale_desc =
{
    MUIC_Scale,
    MUIC_Area,
    sizeof(struct Scale_DATA),
    (void *) Scale_Dispatcher
};
#endif /* ZUNE_BUILTIN_SCALE */
