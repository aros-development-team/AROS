/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <stdio.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/gfxmacros.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "penspec.h"
#include "textengine.h"
#include "support.h"
#include "support_classes.h"
#include "pendisplay_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

/****** Pendisplay.mui/MUIC_Pendisplay ************
*
*   NAME
*       MUIC_Pendisplay
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display a color selected by a copied pen specification or by a borrowed
*       reference object. The default local specification selects MPEN_TEXT; the
*       local pen starts at -1 and Reference at NULL. Construction returns NULL
*       on superclass failure. RGBcolor and Spec initialization inputs are
*       dereferenced and must not be NULL. No local heap allocation holds these
*       values.
*       Setup first calls Area and then obtains a local pen only without
*       Reference. It returns one even if pen acquisition fails, zero on Area
*       setup failure. Cleanup releases an acquired pen, resets it to -1 and
*       returns the superclass result. Complete Cleanup before disposal. Dispose
*       delegates to Area but does not remove reference notifications;
*       explicitly detach Reference while both objects are alive before either
*       is disposed.
*
*   METHODS
*       AskMinMax adds 3 to both minimum dimensions and 16 to both default
*       dimensions, assigns both maxima MUI_MAXMAX and returns zero. Draw calls
*       Area, handles full/update flags, fills the content rectangle and returns
*       zero. A local -1 pen draws a shadow/background pattern. A referenced pen
*       is read as an index; Get status is ignored with index initially zero,
*       and a Pendisplay getter's 65535 failure value does not select the local
*       -1 pattern path. Use references with compatible screen/color-map
*       resources.
*       DragQuery refuses self and accepts a successful Pendisplay_Spec getter,
*       or an Imagedisplay_Spec string longer than two bytes starting with "2:".
*       DragDrop prefers Pendisplay_Spec, otherwise passes the image string
*       after its two-byte prefix to the Spec setter. It returns zero and does
*       not repeat the query checks. The setter copies a complete MUI_PenSpec,
*       so the source must provide 32 readable bytes at that address, not just a
*       short NUL-terminated pen string.
*
*   NOTES
*       Local Spec and RGB writes do not modify Reference. With a reference
*       present they update hidden local state while getters continue to use the
*       reference. Clearing Reference resumes use of the stored local
*       specification.
*       When a recognized color change occurs during setup, Set releases the
*       local pen, obtains another only without Reference, requests update
*       drawing and prepends affected-attribute notifications to the original
*       tags. Pen uses local pen & 0xffff, Spec points to the local structure,
*       and RGBcolor points to sampled local color only if a local pen was
*       obtained. Consequently these notifications need not equal redirected
*       getters. Before setup only the original tags reach the superclass. No
*       equality suppression, relayout, or separate setter error result is
*       provided.
*
*/
/****** Pendisplay.mui/MUIA_Pendisplay_Pen ************
*
*   NAME
*       MUIA_Pendisplay_Pen
*
*   FUNCTION
*       Retrieve the graphics pen index.
*
*   INPUTS
*       Get only. The result is an integer in IPTR storage, not an object
*       pointer as annotated in the header.
*
*   NOTES
*       Without Reference, Get returns the low 16 bits of the local pen and
*       reports success. An unavailable pen (-1), including before Setup or
*       after Cleanup, therefore reads as 65535. It is not a resource handle to
*       release. With Reference the query is forwarded unchanged. Use only a
*       valid acquired index with its matching color map; this getter does not
*       validate availability.
*
*/
/****** Pendisplay.mui/MUIA_Pendisplay_Reference ************
*
*   NAME
*       MUIA_Pendisplay_Reference
*
*   FUNCTION
*       Borrow an object as the source of getter values and displayed pen.
*
*   INPUTS
*       Initialize, set and get, Object pointer. Default NULL. Use a live
*       compatible object, without self-reference or reference cycles.
*
*   NOTES
*       All getters except Reference itself are forwarded, including inherited
*       Area getters. The pointer is borrowed and must stay valid. Registration
*       listens to the referenced Spec and sets this object's local Spec on
*       every notification; registration failure is ignored, and registration
*       does not copy the initial value. Constructor tags attempt registration
*       even for NULL and do not remove earlier registrations when repeated;
*       supply at most one non-NULL Reference tag at construction. Set removes
*       the old Spec notifications targeting this object and registers the new
*       non-NULL reference. Set to NULL while both objects remain alive to
*       detach. Disposal does not detach automatically. Get returns the
*       immediate reference, not the end of a chain.
*
*/
/****** Pendisplay.mui/MUIA_Pendisplay_RGBcolor ************
*
*   NAME
*       MUIA_Pendisplay_RGBcolor
*
*   FUNCTION
*       Copy RGB components into the local specification or retrieve a color
*       structure.
*
*   INPUTS
*       Initialize, set and get, pointer to struct MUI_RGBcolor. Supply a
*       non-NULL readable structure; each component is a full 32-bit ULONG.
*
*   NOTES
*       Input components are formatted as hexadecimal RGB in the local
*       specification; the input pointer is not retained. With no reference, Get
*       parses that specification. For an explicit RGB specification it returns
*       the requested components, even before Setup, not the approximated screen
*       color. For MUI, color-map or system pens it calls GetRGB32 using the
*       local pen and render context before checking availability. Do not query
*       those forms without valid setup and an acquired pen. The parser return
*       is ignored; malformed specifications have no reliable getter contract. A
*       successful Get returns a borrowed pointer to internal scratch RGB
*       storage, overwritten by later queries or color updates and invalid at
*       disposal. Reference redirects the query.
*
*/
/****** Pendisplay.mui/MUIA_Pendisplay_Spec ************
*
*   NAME
*       MUIA_Pendisplay_Spec
*
*   FUNCTION
*       Copy or retrieve the pen specification.
*
*   INPUTS
*       Initialize, set and get, pointer to struct MUI_PenSpec. The structure
*       contains a 32-byte buffer. Default selects MPEN_TEXT.
*
*   NOTES
*       Initialization copies the whole structure and requires non-NULL input.
*       Set copies a non-NULL structure; NULL leaves the local specification
*       unchanged and does not alone request a color update. Supply 32 readable
*       bytes containing a valid terminated AROS pen specification, such as m5,
*       p3, s3 or rffffffff,00000000,00000000. Set does not validate it. Without
*       Reference, Get returns a borrowed pointer to the embedded structure,
*       valid until disposal with contents mutable on later writes; it may be
*       passed back to Set. With Reference, Get is redirected. RGBcolor and Spec
*       tags overwrite the same local specification in tag order. No ownership
*       transfer occurs.
*
*/
/****** Pendisplay.mui/MUIM_Pendisplay_SetColormap ************
*
*   NAME
*       MUIM_Pendisplay_SetColormap
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Pendisplay_SetColormap, colormap);
*
*   FUNCTION
*       Select a color-map entry by setting a temporary pen specification.
*
*   INPUTS
*       colormap - LONG color-map index valid for the current screen.
*
*   RESULT
*       Zero in all cases. This is not an acquisition or setter-success
*       indicator.
*
*   NOTES
*       The method formats a stack-local specification and calls the normal Spec
*       setter, which copies it synchronously. Inputs are not range-checked.
*       Setup-time pen replacement, redraw and notifications follow Spec
*       setting. Reference remains attached; these methods change local state,
*       not the referenced object.
*
*/
/****** Pendisplay.mui/MUIM_Pendisplay_SetMUIPen ************
*
*   NAME
*       MUIM_Pendisplay_SetMUIPen
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Pendisplay_SetMUIPen, muipen);
*
*   FUNCTION
*       Select a MUI pen by setting a temporary pen specification.
*
*   INPUTS
*       muipen - LONG MUI pen selector, such as MPEN_TEXT.
*
*   RESULT
*       Zero in all cases. This is not an acquisition or setter-success
*       indicator.
*
*   NOTES
*       The method formats a stack-local specification and calls the normal Spec
*       setter, which copies it synchronously. Inputs are not range-checked.
*       Setup-time pen replacement, redraw and notifications follow Spec
*       setting. Reference remains attached; these methods change local state,
*       not the referenced object.
*
*/
/****** Pendisplay.mui/MUIM_Pendisplay_SetRGB ************
*
*   NAME
*       MUIM_Pendisplay_SetRGB
*
*   SYNOPSIS
*       IPTR result = DoMethod(obj, MUIM_Pendisplay_SetRGB, r, g, b);
*
*   FUNCTION
*       Select an explicit RGB color by setting a temporary pen specification.
*
*   INPUTS
*       r, g, b - ULONG red, green and blue components, each using the full
*       32-bit range.
*
*   RESULT
*       Zero in all cases. This is not an acquisition or setter-success
*       indicator.
*
*   NOTES
*       The method formats a stack-local specification and calls the normal Spec
*       setter, which copies it synchronously. Inputs are not range-checked.
*       Setup-time pen replacement, redraw and notifications follow Spec
*       setting. Reference remains attached; these methods change local state,
*       not the referenced object.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Construct Area, initialize default MPEN_TEXT specification and
 * unavailable pen, then copy constructor colors and register references in
 * tag order. Return object or zero on superclass failure. Spec/RGB inputs
 * must be valid non-NULL structures; reference registrations are borrowed
 * and unchecked. See class lifecycle contract.
 */
IPTR Pendisplay__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Pendisplay_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* Format identifiers need to be long enforced because of RawDoFmt()
     * limits */
    snprintf(data->penspec.buf, sizeof(data->penspec.buf), "%lc%d",
        (int)PST_MUI, (int)MPEN_TEXT);
    data->pen = -1;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Pendisplay_Reference:
            data->refobj = (Object *) tag->ti_Data;
            DoMethod(data->refobj, MUIM_Notify, MUIA_Pendisplay_Spec,
                MUIV_EveryTime, (IPTR) obj, 3, MUIM_Set,
                MUIA_Pendisplay_Spec, MUIV_TriggerValue);
            break;

        case MUIA_Pendisplay_RGBcolor:
            {
                struct MUI_RGBcolor *rgb =
                    (struct MUI_RGBcolor *)tag->ti_Data;
                snprintf(data->penspec.buf, sizeof(data->penspec.buf),
                    "%lc%08x,%08x,%08x", (int)PST_RGB,
                    (unsigned int)rgb->red, (unsigned int)rgb->green,
                    (unsigned int)rgb->blue);
            }
            break;

        case MUIA_Pendisplay_Spec:
            data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
            break;

        }
    }

    return (IPTR) obj;
}

/*
 * Delegate to Area and return its result. Requires prior Cleanup and
 * explicit reference detachment while both objects are alive. Does not
 * release a local pen or remove reference notifications here; external
 * objects remain caller-owned.
 */
IPTR Pendisplay__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Pendisplay_DATA *data;

    data = INST_DATA(cl, obj);
    if (data->refobj)
    {
        /* kill notify here? But refobj might already have been killed :-\ */
    }

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Copy local colors and replace borrowed reference registrations. When set
 * up and color changes, release/reacquire the local pen, redraw and prepend
 * affected notification tags. Return superclass setter result, without
 * acquisition-status reporting. Valid structures and reference lifetimes
 * are required; see class contract for local versus redirected notification
 * values.
 */
IPTR Pendisplay__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Pendisplay_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    struct opSet supMsg;
    BOOL newcol = FALSE;
    IPTR retval;
    struct TagItem extra_tags[] = {{TAG_IGNORE, 0}, {TAG_IGNORE, 0},
        {TAG_IGNORE, 0}, {TAG_MORE, (IPTR)msg->ops_AttrList}};

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Pendisplay_Reference:
            if (data->refobj)
            {
                DoMethod(data->refobj, MUIM_KillNotifyObj,
                    MUIA_Pendisplay_Spec, (IPTR) obj);
            }

            data->refobj = (Object *) tag->ti_Data;

            if (data->refobj)
            {
                DoMethod(data->refobj, MUIM_Notify, MUIA_Pendisplay_Spec,
                    MUIV_EveryTime, (IPTR) obj, 3, MUIM_Set,
                    MUIA_Pendisplay_Spec, MUIV_TriggerValue);
            }
            newcol = TRUE;
            extra_tags[0].ti_Tag = MUIA_Pendisplay_Pen;
            extra_tags[1].ti_Tag = MUIA_Pendisplay_RGBcolor;
            extra_tags[2].ti_Tag = MUIA_Pendisplay_Spec;
            break;

        case MUIA_Pendisplay_RGBcolor:
            {
                struct MUI_RGBcolor *rgb =
                    (struct MUI_RGBcolor *)tag->ti_Data;

                snprintf(data->penspec.buf, sizeof(data->penspec.buf),
                    "%lc%08x,%08x,%08x", (int)PST_RGB,
                    (unsigned int)rgb->red, (unsigned int)rgb->green,
                    (unsigned int)rgb->blue);
            }
            newcol = TRUE;
            extra_tags[0].ti_Tag = MUIA_Pendisplay_Pen;
            extra_tags[2].ti_Tag = MUIA_Pendisplay_Spec;
            break;

        case MUIA_Pendisplay_Spec:
            if (tag->ti_Data)
            {
                data->penspec = *(struct MUI_PenSpec *)tag->ti_Data;
                newcol = TRUE;
            }
            extra_tags[0].ti_Tag = MUIA_Pendisplay_Pen;
            extra_tags[1].ti_Tag = MUIA_Pendisplay_RGBcolor;
            break;


        }
    }

    if (newcol && (_flags(obj) & MADF_SETUP))
    {
        if (data->pen != -1)
        {
            MUI_ReleasePen(muiRenderInfo(obj), data->pen);
            data->pen = -1;
        }

        if (!data->refobj)
        {
            data->pen =
                MUI_ObtainPen(muiRenderInfo(obj), &data->penspec, 0);
        }

        MUI_Redraw(obj, MADF_DRAWUPDATE);

        if (data->pen != -1)
            GetRGB32(muiRenderInfo(obj)->mri_Colormap, data->pen & 0xffff, 1,
                (ULONG *)&data->rgb);
        else
            extra_tags[1].ti_Tag = TAG_IGNORE;

        /* Prepare notification values for attributes affected by pen change */
        extra_tags[0].ti_Data = data->pen & 0xffff;
        extra_tags[1].ti_Data = (IPTR) &data->rgb;
        extra_tags[2].ti_Data = (IPTR) &data->penspec;
        
        supMsg.MethodID = msg->MethodID;
        supMsg.ops_AttrList = extra_tags;
        supMsg.ops_GInfo = msg->ops_GInfo;
        msg = &supMsg;
    }

    retval = DoSuperMethodA(cl, obj, (Msg) msg);

    return retval;
}

/*
 * Forward every getter except Reference to a live reference when present.
 * Otherwise write local values to caller-owned IPTR storage and return
 * TRUE, or FALSE for unavailable RGB; delegate unknown attributes to Area.
 * Returned structures are borrowed. Non-RGB sampling needs valid setup and
 * pen before the availability check; parser failure is unchecked.
 */
IPTR Pendisplay__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct Pendisplay_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    /* Redirect queries to the object we reference, if any */
    if (data->refobj != NULL && msg->opg_AttrID != MUIA_Pendisplay_Reference)
        return DoMethodA(data->refobj, (Msg) msg);

    switch (msg->opg_AttrID)
    {
    case MUIA_Pendisplay_Pen:
        *store = (IPTR) data->pen & 0xffff;
        break;

    case MUIA_Pendisplay_Reference:
        *store = (IPTR) data->refobj;
        break;

    case MUIA_Pendisplay_RGBcolor:
        {
            struct MUI_PenSpec_intern intpenspec;
            zune_pen_spec_to_intern(&data->penspec, &intpenspec);

            switch (intpenspec.p_type)
            {
            case PST_MUI:
            case PST_CMAP:
            case PST_SYS:
                GetRGB32(muiRenderInfo(obj)->mri_Colormap,
                    data->pen & 0xffff, 1, (ULONG *)&data->rgb);
                break;
            case PST_RGB:
                data->rgb = intpenspec.p_rgb;
                break;
            }

            if (data->pen != -1 || intpenspec.p_type == PST_RGB)
                *store = (IPTR) &data->rgb;
            else
                return FALSE;
        }
        break;

    case MUIA_Pendisplay_Spec:
        *store = (IPTR) &data->penspec;
        break;

    default:
        return DoSuperMethodA(cl, obj, (Msg) msg);
    }

    return TRUE;
}

/*
 * Return zero if Area setup fails, otherwise obtain a local pen only
 * without Reference and return one even on acquisition failure. Requires
 * live render information and valid specification. Pair with Cleanup before
 * disposal.
 */
IPTR Pendisplay__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Pendisplay_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return 0;

    if (!data->refobj)
    {
        data->pen = MUI_ObtainPen(muiRenderInfo(obj), &data->penspec, 0);
    }

    return 1;
}

/*
 * Release the locally acquired pen if present, reset it to -1 and return
 * Area cleanup result. Requires live setup resources. Does not release
 * reference objects or unregister their notifications.
 */
IPTR Pendisplay__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Pendisplay_DATA *data = INST_DATA(cl, obj);

    if (data->pen != -1)
    {
        MUI_ReleasePen(muiRenderInfo(obj), data->pen);
        data->pen = -1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Extend caller-owned Area bounds by minimum 3 and default 16 on both axes;
 * set both maxima MUI_MAXMAX and return zero. Requires valid MinMaxInfo
 * storage. No allocation or ownership transfer.
 */
IPTR Pendisplay__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 3;
    msg->MinMaxInfo->MinHeight += 3;
    msg->MinMaxInfo->DefWidth += 16;
    msg->MinMaxInfo->DefHeight += 16;
    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return 0;
}

/*
 * Call Area, then fill content for full/update flags using local or
 * referenced pen. Local -1 draws a pattern; referenced getter status is
 * ignored with index initialized to zero. Reset area-fill pattern and
 * return zero. Requires valid drawing context and compatible referenced
 * color map; no resource ownership transfer.
 */
IPTR Pendisplay__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Pendisplay_DATA *data = INST_DATA(cl, obj);
    LONG color;

    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return FALSE;

    if (data->refobj)
    {
        IPTR val = 0;

        get(data->refobj, MUIA_Pendisplay_Pen, &val);
        color = (LONG) val;
    }
    else
    {
        color = data->pen;
    }

    if (color == -1)
    {
        static UWORD pat[] = { 0x1111, 0x2222, 0x4444, 0x8888 };

        SetAfPt(_rp(obj), pat, 2);
        SetABPenDrMd(_rp(obj), _pens(obj)[MPEN_SHADOW],
            _pens(obj)[MPEN_BACKGROUND], JAM2);

    }
    else
    {
        SetABPenDrMd(_rp(obj), MUIPEN(color), 0, JAM1);
    }

    RectFill(_rp(obj), _mleft(obj), _mtop(obj), _mright(obj),
        _mbottom(obj));

    SetAfPt(_rp(obj), NULL, 0);

    return 0;
}

/*
 * Format a stack-local pen specification and set the copied Spec attribute
 * synchronously; return zero regardless of setter or acquisition status.
 * Requires valid object and inputs described by the corresponding public
 * method. May deliver notifications; does not change the referenced object.
 */
IPTR Pendisplay__MUIM_Pendisplay_SetColormap(struct IClass *cl,
    Object *obj, struct MUIP_Pendisplay_SetColormap *msg)
{
    struct MUI_PenSpec penspec;

    snprintf(penspec.buf, sizeof(penspec.buf), "%lc%d", (int)PST_CMAP,
        (int)msg->colormap);
    set(obj, MUIA_Pendisplay_Spec, (IPTR) &penspec);

    return 0;
}

/*
 * Format a stack-local pen specification and set the copied Spec attribute
 * synchronously; return zero regardless of setter or acquisition status.
 * Requires valid object and inputs described by the corresponding public
 * method. May deliver notifications; does not change the referenced object.
 */
IPTR Pendisplay__MUIM_Pendisplay_SetRGB(struct IClass *cl, Object *obj,
    struct MUIP_Pendisplay_SetRGB *msg)
{
    struct MUI_PenSpec penspec;

    snprintf(penspec.buf, sizeof(penspec.buf), "%lc%08x,%08x,%08x",
        (int)PST_RGB, (unsigned int)msg->r, (unsigned int)msg->g,
        (unsigned int)msg->b);
    set(obj, MUIA_Pendisplay_Spec, (IPTR) &penspec);

    return 0;
}

/*
 * Format a stack-local pen specification and set the copied Spec attribute
 * synchronously; return zero regardless of setter or acquisition status.
 * Requires valid object and inputs described by the corresponding public
 * method. May deliver notifications; does not change the referenced object.
 */
IPTR Pendisplay__MUIM_Pendisplay_SetMUIPen(struct IClass *cl, Object *obj,
    struct MUIP_Pendisplay_SetMUIPen *msg)
{
    struct MUI_PenSpec penspec;

    snprintf(penspec.buf, sizeof(penspec.buf), "%lc%d", (int)PST_MUI,
        (int)msg->muipen);
    set(obj, MUIA_Pendisplay_Spec, (IPTR) &penspec);

    return 0;
}

/*
 * Refuse self, accept successful Pendisplay Spec Get or a non-NULL image
 * string longer than two bytes with 2: prefix, otherwise refuse. Requires
 * live source; no copy or ownership transfer. Does not establish full
 * structure readability for a later drop.
 */
IPTR Pendisplay__MUIM_DragQuery(struct IClass *cl, Object *obj,
    struct MUIP_DragQuery *msg)
{
    STRPTR spec = NULL;

    if (msg->obj == obj)
        return MUIV_DragQuery_Refuse;

    if (get(msg->obj, MUIA_Pendisplay_Spec, &spec))
        return MUIV_DragQuery_Accept;

    if (get(msg->obj, MUIA_Imagedisplay_Spec, &spec))
    {
        if ((NULL != spec) && (strlen(spec) > 2) && (spec[0] == '2')
            && (spec[1] == ':'))
            return MUIV_DragQuery_Accept;
    }

    return MUIV_DragQuery_Refuse;
}

/*
 * Set a copied pen specification from source, preferring Pendisplay then
 * image spec after two bytes. Return zero, ignoring setter status. Requires
 * live source and 32 readable valid specification bytes. Does not repeat
 * query, prefix, length or self checks.
 */
IPTR Pendisplay__MUIM_DragDrop(struct IClass *cl, Object *obj,
    struct MUIP_DragDrop *msg)
{
    STRPTR spec = NULL;

    if (get(msg->obj, MUIA_Pendisplay_Spec, &spec))
        set(obj, MUIA_Pendisplay_Spec, spec);
    else if (get(msg->obj, MUIA_Imagedisplay_Spec, &spec))
        set(obj, MUIA_Pendisplay_Spec, spec + 2);
    return 0;
}

#if ZUNE_BUILTIN_PENDISPLAY
/*
 * Dispatch local methods and return their results; delegate unknown
 * messages to Area. Requires ABI-correct message and live object except
 * construction. Resources and borrowed references follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Pendisplay_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Pendisplay__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Pendisplay__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Pendisplay__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Pendisplay__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Pendisplay__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Pendisplay__MUIM_Cleanup(cl, obj,
            (struct MUIP_Cleanup *)msg);
    case MUIM_AskMinMax:
        return Pendisplay__MUIM_AskMinMax(cl, obj,
            (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Pendisplay__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    case MUIM_Pendisplay_SetColormap:
        return Pendisplay__MUIM_Pendisplay_SetColormap(cl, obj,
            (struct MUIP_Pendisplay_SetColormap *)msg);
    case MUIM_Pendisplay_SetMUIPen:
        return Pendisplay__MUIM_Pendisplay_SetMUIPen(cl, obj,
            (struct MUIP_Pendisplay_SetMUIPen *)msg);
    case MUIM_Pendisplay_SetRGB:
        return Pendisplay__MUIM_Pendisplay_SetRGB(cl, obj,
            (struct MUIP_Pendisplay_SetRGB *)msg);
    case MUIM_DragQuery:
        return Pendisplay__MUIM_DragQuery(cl, obj,
            (struct MUIP_DragQuery *)msg);
    case MUIM_DragDrop:
        return Pendisplay__MUIM_DragDrop(cl, obj,
            (struct MUIP_DragDrop *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Pendisplay_desc =
{
    MUIC_Pendisplay,
    MUIC_Area,
    sizeof(struct Pendisplay_DATA),
    (void *) Pendisplay_Dispatcher
};
#endif /* ZUNE_BUILTIN_PENDISPLAY */
