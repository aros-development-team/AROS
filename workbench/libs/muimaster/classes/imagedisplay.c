/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "imspec.h"
#include "imagedisplay_private.h"

/****** Imagedisplay.mui/MUIC_Imagedisplay ************
*
*   NAME
*       MUIC_Imagedisplay
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Preview a Zune image specification with optional centering at its
*       default size. Both FreeHoriz and FreeVert default to TRUE. Constructor
*       specification duplication failure or absence falls back to the string
*       "0:128"; construction fails only if that fallback allocation also fails.
*       Repeated Spec tags overwrite earlier copies without freeing them.
*       The object owns its specification string and setup-time image resources.
*       Complete Hide and Cleanup before disposal; Dispose frees the string and
*       returns zero without cleaning the image itself. Setup returns zero on
*       Area failure, otherwise one even if image preparation fails. Cleanup
*       calls image cleanup when a specification exists but does not clear the
*       image pointer; repeated Cleanup without a matching Setup is unsafe.
*
*   METHODS
*       AskMinMax adds three pixels to minimum width/height and sixteen to
*       default width/height, with both maxima MUI_MAXMAX. These object-size
*       requirements do not depend on the image or free-axis flags. Draw calls
*       Area and returns zero without MADF_DRAWOBJECT; otherwise it draws a
*       prepared image clipped to the content rectangle and returns one. It
*       always uses IDS_NORMAL.
*       If either axis is not free, Draw caps and centers each nonfree axis at
*       the backend default size when there is extra room; the other axis
*       remains at content size. If both axes are free, it instead caps and
*       centers each axis at the backend maximum. It does not enlarge smaller
*       content to the default size. Show calls Area but always returns one;
*       Hide returns the superclass result.
*       DragQuery refuses self and accepts another object when its
*       Imagedisplay_Spec or Pendisplay_Spec getter succeeds. DragDrop prefers
*       an image specification; otherwise it prefixes up to 64 pen-specification
*       bytes with "2:" and sets the copied result. It returns zero and does not
*       validate a successful getter's pointer or independently reject self. No
*       image-selection requester is implemented here.
*
*   NOTES
*       Spec is copied through the same numeric-MUII/string conversion helper
*       used by Image. Despite the header's struct MUI_ImageSpec pointer
*       annotation, this implementation stores and returns a specification
*       string. Set destroys the old string before copying the argument, then
*       replaces prepared resources as required and requests redraw without
*       relayout. Getter pointers must not be passed back to that setter because
*       they would be freed before use. Invalid specifications or allocation
*       failures do not have a separate setter error result.
*
*/
/****** Imagedisplay.mui/MUIA_Imagedisplay_Spec ************
*
*   NAME
*       MUIA_Imagedisplay_Spec
*
*   FUNCTION
*       Set or retrieve the preview specification.
*
*   INPUTS
*       Initialize, set and get. AROS accepts a specification string or
*       supported numeric MUII identifier and returns STRPTR, despite the header
*       structure-pointer annotation.
*
*   NOTES
*       The string is copied and owned; a getter pointer is borrowed until
*       replacement/disposal. Numeric configuration and pattern identifiers use
*       the conversion ranges described by Image. Set has no constructor-style
*       fallback on duplication failure, frees before copying, and rebuilds
*       active resources without requesting relayout. Avoid aliased input and
*       repeated constructor tags. No equality suppression occurs; tags reach
*       normal superclass notification processing.
*
*/
/****** Imagedisplay.mui/MUIA_Imagedisplay_UseDefSize ************
*
*   NAME
*       MUIA_Imagedisplay_UseDefSize
*
*   FUNCTION
*       Set both free-axis flags to the inverse of this value.
*
*   INPUTS
*       Initialize only, BOOL. With no sizing tags both free-axis flags are
*       TRUE. No local setter or getter.
*
*   NOTES
*       TRUE clears FreeHoriz and FreeVert, causing larger image regions to be
*       capped and centered at backend default dimensions during Draw. FALSE
*       enables both flags. Later per-axis tags can override either flag; tag
*       order matters. This does not change the object minimum/default/maximum
*       size requirements.
*
*/
/****** Imagedisplay.mui/MUIA_Imagedisplay_FreeHoriz ************
*
*   NAME
*       MUIA_Imagedisplay_FreeHoriz
*
*   FUNCTION
*       Control default-size centering on the horizontal axis.
*
*   INPUTS
*       Initialize only, BOOL, default TRUE. No local setter or getter.
*
*   NOTES
*       FALSE caps this drawing dimension at the backend default when content is
*       larger, centering the image on that axis. TRUE leaves it free; when both
*       flags are TRUE the separate backend-maximum caps apply. Smaller content
*       is not expanded. UseDefSize changes both flags, so constructor tag order
*       matters. Object size negotiation remains independent of these flags.
*
*/
/****** Imagedisplay.mui/MUIA_Imagedisplay_FreeVert ************
*
*   NAME
*       MUIA_Imagedisplay_FreeVert
*
*   FUNCTION
*       Control default-size centering on the vertical axis.
*
*   INPUTS
*       Initialize only, BOOL, default TRUE. No local setter or getter.
*
*   NOTES
*       FALSE caps this drawing dimension at the backend default when content is
*       larger, centering the image on that axis. TRUE leaves it free; when both
*       flags are TRUE the separate backend-maximum caps apply. Smaller content
*       is not expanded. UseDefSize changes both flags, so constructor tag order
*       matters. Object size negotiation remains independent of these flags.
*
*/

extern struct Library *MUIMasterBase;

#define MIF_FREEVERT         (1<<0)
#define MIF_FREEHORIZ        (1<<1)


/*
 * Construct Area, parse constructor tags and own copied specification.
 * Return object or zero on failure. Refer to class contract for fallback,
 * tag-order and repeated-string behavior; borrowed image references are not
 * retained.
 */
IPTR Imagedisplay__OM_NEW(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Imagedisplay_DATA *data;
    struct TagItem *tag, *tags;

    D(bug("Imagedisplay_New starts\n"));

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->flags = MIF_FREEHORIZ | MIF_FREEVERT;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Imagedisplay_Spec:
            data->spec = zune_image_spec_duplicate(tag->ti_Data);
            break;

        case MUIA_Imagedisplay_UseDefSize:
            _handle_bool_tag(data->flags, (!tag->ti_Data),
                (MIF_FREEHORIZ | MIF_FREEVERT));
            break;

        case MUIA_Imagedisplay_FreeHoriz:
            /* MUI implements some tag for optionnally prevent rescaling
             * of displayed image - without affecting imagedisplay resize -
             * see MUIPrefs/Buttons/Checkmarks/Look for nonrescaled image,
             * and try a popimage for yourself to see that by default they
             * get rescaled. It's not the same effect as MUI_Image_FreeHoriz.
             * -dlc 20030323
             */
            _handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEHORIZ);
            break;

        case MUIA_Imagedisplay_FreeVert:
            _handle_bool_tag(data->flags, tag->ti_Data, MIF_FREEVERT);
            break;
        }
    }

    if (!data->spec)
    {
        data->spec = StrDup("0:128");
    }

    if (!data->spec)
    {
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    D(bug("Imagedisplay_New(0x%p) spec=0x%p\n", obj, data->img));
    return (IPTR) obj;
}

/*
 * Free owned specification and invoke Area disposal, then return zero.
 * Requires prior Hide/Cleanup for prepared resources and no outstanding
 * users of getter pointers. Does not release borrowed external images.
 */
IPTR Imagedisplay__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    zune_image_spec_free(data->spec);
    DoSuperMethodA(cl, obj, (Msg) msg);
    return 0;
}

/*
 * Process local tags and replace prepared resources for Spec when set
 * up/shown, then return Area setter result. Spec input must not alias old
 * owned string, which is freed before duplication. Request redraw as
 * documented, not relayout; resource failures have no separate result.
 */
IPTR Imagedisplay__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tag, *tags;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Imagedisplay_Spec:
            if (data->spec)
                zune_image_spec_free(data->spec);
            data->spec = zune_image_spec_duplicate(tag->ti_Data);

            if (_flags(obj) & MADF_CANDRAW)
                zune_imspec_hide(data->img);

            if (_flags(obj) & MADF_SETUP)
            {
                zune_imspec_cleanup(data->img);
                data->img =
                    zune_imspec_setup((IPTR) data->spec,
                    muiRenderInfo(obj));
            }

            if (_flags(obj) & MADF_CANDRAW)
                zune_imspec_show(data->img, obj);

            MUI_Redraw(obj, MADF_DRAWOBJECT);
            break;
        }
    }

    return (IPTR) DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Return borrowed specification string through valid caller-owned IPTR
 * storage with TRUE. Delegate all other getters to Area. No allocation or
 * ownership transfer; pointer expires on replacement/disposal.
 */
IPTR Imagedisplay__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);
    switch (msg->opg_AttrID)
    {
    case MUIA_Imagedisplay_Spec:
        *msg->opg_Storage = (IPTR) data->spec;
        return (TRUE);
    }

    return (IPTR) DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Call Area setup, return zero on failure, otherwise prepare image
 * resources and return one even if preparation fails. Requires live render
 * context and paired Cleanup. Image-specific inherited-attribute side
 * effects follow the public class contract.
 */
IPTR Imagedisplay__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return 0;

    if (data->spec)
        data->img =
            zune_imspec_setup((IPTR) data->spec, muiRenderInfo(obj));
    return 1;
}

/*
 * Clean prepared image resources according to the local pointer guard and
 * return Area result. Requires completed Hide and matching Setup with live
 * render context. Image clears img; Imagedisplay leaves the pointer
 * unchanged. Specification string remains owned.
 */
IPTR Imagedisplay__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    if (data->spec)
        zune_imspec_cleanup(data->img);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Call Area and adjust caller-owned MinMaxInfo using class size rules, then
 * return one. Requires valid setup state, font and image metadata when
 * applicable. No allocation or relayout request; see class and sizing
 * attributes for exact contributions.
 */
IPTR Imagedisplay__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 3;
    msg->MinMaxInfo->MinHeight += 3;

    msg->MinMaxInfo->DefWidth += 16;
    msg->MinMaxInfo->DefHeight += 16;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 1;
}

/*
 * Call Area Show, show non-NULL prepared image and return one regardless of
 * superclass status. Requires normal setup/show ordering and live render
 * context. No ownership transfer.
 */
IPTR Imagedisplay__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (data->img)
        zune_imspec_show(data->img, obj);
    return 1;
}

/*
 * Hide non-NULL prepared image before returning Area Hide result. Requires
 * live context and matched lifecycle; image resources remain until Cleanup.
 */
IPTR Imagedisplay__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    if (data->img)
        zune_imspec_hide(data->img);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Draw using the class-specific source precedence, flags, sizing and state
 * rules described above. Requires live render resources and valid borrowed
 * sources. No ownership transfer; return zero for local flag rejection,
 * otherwise one, ignoring backend drawing status.
 */
IPTR Imagedisplay__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Imagedisplay_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & MADF_DRAWOBJECT))
        return 0;

    if (data->img)
    {
        WORD left, top, width, height;
        APTR clip;

        left = _mleft(obj);
        top = _mtop(obj);
        width = _mwidth(obj);
        height = _mheight(obj);

        /* if either horiz or vert is not rescalable, center it at def size */
        if (!(data->flags & MIF_FREEVERT) || !(data->flags & MIF_FREEHORIZ))
        {
            struct MUI_MinMax minmax;

            zune_imspec_askminmax(data->img, &minmax);
            data->defwidth = minmax.DefWidth;
            data->defheight = minmax.DefHeight;

            if (!(data->flags & MIF_FREEVERT) && (height > data->defheight))
            {
                WORD freespace = height - data->defheight;

                top += freespace / 2;
                height = data->defheight;
            }

            if (!(data->flags & MIF_FREEHORIZ) && (width > data->defwidth))
            {
                WORD freespace = width - data->defwidth;

                left += freespace / 2;
                width = data->defwidth;
            }
        }
        else
        {
            struct MUI_MinMax minmax;

            zune_imspec_askminmax(data->img, &minmax);

            if (width > minmax.MaxWidth)
            {
                width = minmax.MaxWidth;
                left = _mleft(obj) + (_mwidth(obj) - width) / 2;
            }

            if (height > minmax.MaxHeight)
            {
                height = minmax.MaxHeight;
                top = _mtop(obj) + (_mheight(obj) - height) / 2;
            }
        }

        clip = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj),
            _mwidth(obj), _mheight(obj));
        zune_imspec_draw(data->img, muiRenderInfo(obj),
            left, top, width, height, 0, 0, IDS_NORMAL);
        MUI_RemoveClipping(muiRenderInfo(obj), clip);
    }

    return 1;
}

/*
 * Refuse self; accept a live source if either supported specification
 * getter succeeds, otherwise refuse. Does not validate returned data or
 * change ownership. Requires valid source object; no superclass delegation.
 */
IPTR Imagedisplay__MUIM_DragQuery(struct IClass *cl, Object *obj,
    struct MUIP_DragQuery *msg)
{
    IPTR dummy = 0;

    if (msg->obj == obj)
        return MUIV_DragQuery_Refuse;
    if (get(msg->obj, MUIA_Imagedisplay_Spec, &dummy)
        || get(msg->obj, MUIA_Pendisplay_Spec, &dummy))
        return MUIV_DragQuery_Accept;
    return MUIV_DragQuery_Refuse;
}

/*
 * Copy source image spec through setter or convert up to 64 pen-spec bytes
 * with 2: prefix. Return zero, ignoring setter result. Requires live source
 * and valid getter data; no local self-drop rejection, pointer validation
 * or ownership transfer.
 */
IPTR Imagedisplay__MUIM_DragDrop(struct IClass *cl, Object *obj,
    struct MUIP_DragDrop *msg)
{
    IPTR spec = 0;

    if (get(msg->obj, MUIA_Imagedisplay_Spec, &spec))
    {
        set(obj, MUIA_Imagedisplay_Spec, spec);
    }
    else if (get(msg->obj, MUIA_Pendisplay_Spec, &spec))
    {
        char buf[67];

        strcpy(buf, "2:");
        strncpy(buf + 2, (const char *)spec, 64);
        buf[66] = 0;
        set(obj, MUIA_Imagedisplay_Spec, (IPTR) buf);
    }
    return 0;
}


#if ZUNE_BUILTIN_IMAGEDISPLAY
/*
 * Dispatch local overrides and return their results; delegate other
 * messages to Area. Requires ABI-correct message and live object except
 * construction. Resources follow class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Imagedisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Imagedisplay__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Imagedisplay__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Imagedisplay__OM_SET(cl, obj, (APTR) msg);
    case OM_GET:
        return Imagedisplay__OM_GET(cl, obj, (APTR) msg);
    case MUIM_AskMinMax:
        return Imagedisplay__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Setup:
        return Imagedisplay__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Imagedisplay__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Imagedisplay__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Imagedisplay__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Imagedisplay__MUIM_Draw(cl, obj, (APTR) msg);
    case MUIM_DragQuery:
        return Imagedisplay__MUIM_DragQuery(cl, obj, (APTR) msg);
    case MUIM_DragDrop:
        return Imagedisplay__MUIM_DragDrop(cl, obj, (APTR) msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Imagedisplay_desc =
{
    MUIC_Imagedisplay,
    MUIC_Area,
    sizeof(struct Imagedisplay_DATA),
    (void *) Imagedisplay_Dispatcher
};
#endif /* ZUNE_BUILTIN_IMAGEDISPLAY */
